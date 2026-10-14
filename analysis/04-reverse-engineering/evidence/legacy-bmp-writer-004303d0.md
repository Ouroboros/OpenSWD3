# `sub_4303D0` 16 位 framebuffer → 24 位 BMP 写入合同

## 结论

完整 LST `0x004303D0..0x004306B0` 证明该函数是原版共用的 BMP
写入器，不只是截图文件名逻辑。ABI 为：

```text
int __cdecl sub_4303D0(
    const uint16_t* pixels,
    int width,
    int height,
    const char* filename
)
```

它先复制连续的 `width*height*2` 字节，通过当前 reverse pixel
converter 把物理 surface 像素还原成 RGB555，再生成 bottom-up、24-bit、
未压缩 BMP。`src/rendering/legacy_bmp_writer.cpp` 按这些指令实现；伪码仅作
辅助阅读，不参与行为裁决。

## LST 行为分段

| 地址 | 可观察行为 |
|---|---|
| `0x00430406..0x00430468` | 计算 `width*height` 和两字节源大小；建立 14-byte `BITMAPFILEHEADER` 与 40-byte `BITMAPINFOHEADER`。初始 `bfSize=0`、`bfOffBits=0`，`biSize=40`、planes=1、bitcount=24，其余字段为零。 |
| `0x0043046C..0x00430490` | 分配并逐字节复制输入；调用 `sub_4238D0(copy,pixel_count)`，即当前 reverse converter。原 framebuffer 不被修改。 |
| `0x004304A3..0x004304D7` | 以 `GENERIC_WRITE + OPEN_ALWAYS` 打开目标；打开失败释放副本并返回 0。`OPEN_ALWAYS` 不截断既有文件。 |
| `0x004304DC..0x00430521` | seek 到 0，依次写出仍带零占位的 14-byte file header 和 40-byte information header。 |
| `0x00430526..0x00430553` | 行长取 `(width*3+3)&~3`，只分配一次并清零，因此每行 padding 固定为零。 |
| `0x00430560..0x004305E2` | 源从最后一行开始；每个 RGB555 word 展开为 `B=(pixel<<3)`、`G=(pixel>>2)&0xF8`、`R=(pixel>>7)&0xF8`，按 BGR 三字节写入。 |
| `0x004305E7..0x0043060F` | 每写完第 `0,15,30,...` 行调用一次 `AIL_serve`。 |
| `0x00430615..0x00430655` | 读取当前文件位置，seek 到 2 回填 `bfSize`；seek 到 10 回填字面值 54 作为 `bfOffBits`。 |
| `0x0043065A..0x00430693` | 关闭、释放两个临时缓冲，再调用一次 `AIL_serve`，返回 1。 |

函数不会把 5-bit 通道复制低位来扩展到 8-bit；只左移三位，所以每个输出
通道只能是 `0,8,...,248`。这属于原版图像字节合同，不能改成常见的
`(v<<3)|(v>>2)`。

## 两个调用者

- `0x0040A2D7..0x0040A325`：内建 P 键截图。调用者先按
  `%sScrnShot\\%05d.bmp` 从 0 扫到 `<0x1869E`，每次探测前执行
  `AIL_serve`，然后固定传入全局 framebuffer、640、480 和首个未打开的
  文件名。
- `0x00453514..0x00453563`：战斗内部受状态门控的快照，文件名为
  `c:\\snap\\%d.bmp`。这是同一写入器的第二个调用者，不改变编码格式。

第二条路径属于 B10 战斗状态机；B4 只拥有共用 BMP 写入合同。

## 当前实现与验证

`LegacyBmpWriterPorts` 明确保留 seek/write/current-position/close/audio-service
边界，而不是只生成一张“看起来相似”的图片。UT 固定验证：

- 2×2 RGB555 的完整 70 字节 BMP，包括 header、bottom-up BGR 顺序和 padding；
- RGB565 surface 经当前 reverse converter 后输出正确 RGB555 颜色；
- 16 行时只在行 0、15 和最终关闭后维护音频；
- `OPEN_ALWAYS` 覆盖已有文件但保留逻辑末尾之后的旧尾部字节；
- 输入尺寸、源长度和打开失败边界。

SDL3 运行壳已把 P 键路径接到该实现：磁盘空间仍使用原门槛，文件仍位于
游戏数据目录下的 `ScrnShot/`，编号扫描仍由既有 `app::screenshot` 合同
完成。当前 SDL texture 是 RGB565，因此后端显式选择
`rgb565_to_rgb555` reverse transform。

原汇编对整数溢出、分配失败和打开后的 I/O 失败没有可靠保护。现代实现只在
这些异常输入/宿主失败边界返回显式状态并安全关闭文件；正常正尺寸、成功 I/O
路径的字节与调用节奏不变。这是宿主兼容性隔离，不修正游戏逻辑。

验证结果：Linux `core` 52/52、Linux `app` 54/54、Windows LLVM `app`
54/54 CTest 通过。未启动原版 EXE；后续 framebuffer oracle 可按
`p4-dynamic-oracle-capture-protocol.md` 对最终 BMP 和转换前后缓冲做差分。
