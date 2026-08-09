# 边框、缩略图与数字绘制 helper

状态：`assembly_exact`、`asset_metadata_verified`

完整 LST 是唯一行为真值。本单元逐指令覆盖三个互不共享状态、但都直接服务于旧
`640×480` 软件画布的小函数：

| 地址 | 行为 | OpenSWD3 映射 |
|---|---|---|
| `0x0040DE50` | 带全局动画相位的四边灰阶框 | `draw_legacy_animated_border` |
| `0x0040E080` | 原地生成 `160×120` 存档缩略图 | `downsample_legacy_thumbnail_in_place` |
| `0x004117F0` | 从右向左绘制十进制数字及固定装饰 | `draw_legacy_decorated_number` |

IDA 伪码只用于定位调用点和临时变量名，不改变以下指令级结论。

## 1. `0x0040DE50` 动画边框

六个参数依次是目标首地址、`x/y/width/height` 和以像素为单位的 pitch。入口只做四个
旧边界判断：

```text
x < 0                 -> return
y < 0                 -> return
x + width  >= 640     -> return
y + height >= 480     -> return
```

加法是 32 位回绕，右、下边界的等号也会拒绝。通过入口后，从全局
`0x004CC2D4` 读取局部 phase，严格按以下四段写入：

```text
top:    (x, y)               向右 width 次
right:  (x+width, y)         向下 height 次
bottom: (x+width, y+height)  向左 width 次
left:   (x, y+height)        向上 height 次
```

因此每边包含起点、不包含终点，四个角各写一次；不能把它实现成四条都包含两端的
普通矩形边框。每次写入前计算：

```text
level = (0x3f - local_phase) & 0x1f
rgb555 = level | (level << 5) | (level << 10)
local_phase = (local_phase + 1) & 0x1f
```

原函数把同一个 `rgb555` 复制到两个 16 位 lane，调用 `sub_4238B0(...,2)`，再只取
低 lane 写入，所以现代实现复用 `legacy_pack_color_pair`，保留当前输出 mask 的 forward
转换。四边完成后，保存的不是局部 perimeter phase，而是：

```text
global_phase = (old_global_phase + 1) & 0x1f
```

宽或高为零时，对应循环不执行，但只要入口边界通过，全局 phase 仍前进一次。现代
span 边界只隔离旧函数在异常指针、pitch 或回绕尺寸下的越界写；正常寻址与相位不变。

## 2. `0x0040E080` 原地缩略图

函数只有一个 `u16*` 参数，源和目标都从同一地址开始。外循环固定 120 次，内循环
固定 160 次：每次复制一个 `u16` 后，源前进 8 字节；每个输出行结束后再前进
`0xF00` 字节。合计每行前进：

```text
160 * 8 + 0xF00 = 0x1400 bytes = 4 * 640 * sizeof(u16)
```

所以输出像素恰好是每个 `4×4` 源块左上角的点采样：

```text
dst[y * 160 + x] = src[(y * 4) * 640 + x * 4]
```

结果覆盖原缓冲开头 `0x4B00` 个 word，后续数据保持不变。源读位置始终先于可能覆盖
它的目标写，因此原地执行不需要临时缓冲。实现要求至少有旧固定画布的
`0x4B000` 个 word；不足时返回安全状态，不复制旧越界读取。

## 3. `0x004117F0` 十进制数字与装饰

原 ABI 有四个参数：`x`、`y`、一个完全未读取的第三参数和无符号 `value`。数字循环
是 do-while，所以零也绘制一个 `0`。每轮严格执行：

1. 用无符号除法取得 `digit = value % 10`；
2. 查询 `sub_4315D0(0x2354, digit)`；
3. `x -= width + 2`；
4. 在 `(x, y+1)` 调用 `sub_4170E0`；
5. `value /= 10`，非零时继续。

这使查询和绘制顺序固定为个位、十位、百位……，但视觉结果从右向左排列。所有数字
完成后查询 `sub_4315D0(0x245E,0)`，在 `(x-40,y-8)` 绘制固定装饰。两类 blit 都把
旧全局 `0x004C9A24` 复制到 opacity step，传入 `flags=0x14` 和末参数零；现代接口把
opacity step 显式化，并复用已经逐槽闭环的 `blit_legacy_copy_paths`。

TSW 目录独立确认两个十六进制资源号均落在 `all_sys.tsw`：

| 资源号 | 名称 | 帧 | 尺寸 | 存储 |
|---|---|---:|---:|---:|
| `0x2354`（9044） | 戰鬥平常數字 | 10 | 每帧 `10×15` | 8-bit |
| `0x245E`（9310） | 錢幣符號 | 1 | `28×28` | 8-bit |

rendering 继续通过 `LegacyFramePieceProvider` 借用资源视图，不在 B4 内复制 B6 的
TSW/ACT 资源所有权。provider 缺失、零尺寸或 blitter 异常是现代安全返回；正常资源
请求顺序、坐标回绕、两像素字距、装饰偏移和 `0x14` 分派保持原行为。

## 4. 验证

独立 UT 固定：

- 四边的精确写入顺序、灰阶 phase、RGB555→RGB565 forward 转换、等号边界、零高
  循环和全局 phase 更新；
- 全部 19,200 个缩略图输出点与原 `640×480` 输入的 `4×4` 左上角样本逐项相等，
  并确认后续 word 未被覆盖；
- `407` 的资源查询序列为 `7,0,4` 后接装饰，坐标按各帧宽度从右向左变化，第三参数
  不参与行为；零值仍查询一个零帧；资源和几何异常显式隔离。

Linux Clang `core` 为 57/57 CTest，Windows LLVM `app` 为 59/59 CTest。
