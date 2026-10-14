# 剧情 VM 画面三通道渐变启动 `0x004293AC`

## 结论

`sub_427920` 的 opcode52 独占入口 `0x004293AC`，物理长度固定为 16 字节：

```text
+0   u16 raw opcode
+2   s16 current red
+4   s16 current green
+6   s16 current blue
+8   s16 target red
+10  s16 target green
+12  s16 target blue
+14  u16 duration
```

handler 把三个 current 与三个 target 从有符号 16 位整数精确转换为单精度浮点，把 duration 零扩展为 dword countdown，再依次计算三个 `(target-current)/duration` 单精度 step。完整路径推进 16 字节、发布归一化 previous并在同一次调用继续；没有音频或 yield。

原实现的数学式在非零时长资产域一致，但它先验证完整 16 字节，丢失原版逐阶段写入；成功路径还漏掉共同出口的 previous 发布。修正后恢复机器 unsafe 顺序，并显式固定 x87 零时长的无穷与 indefinite NaN 位型。

## operand 与写入顺序

### 三个 current 逐读逐写

```text
004293AC  movsx ecx,word ptr [current+2]
004293B0  save signed current red
004293B4  fild signed current red
004293B8  fstp current_red

004293BE  movsx edx,word ptr [current+4]
004293C2  save signed current green
004293C8  fild signed current green
004293CC  fstp current_green

004293D2  movsx eax,word ptr [current+6]
004293D6  save signed current blue
004293DA  fild signed current blue
004293DE  fstp current_blue
```

因此 `+4`、`+6` 或 `+8` 读取失败时，已经完成的前序 current 写入必须保留。

### 三个 target 全读后才开始写

```text
004293E4  movsx edi,word ptr [current+8]
004293E8  movsx esi,word ptr [current+10]
004293F4  fild target red local
004293F8  movsx ecx,word ptr [current+12]
004293FC  fstp target_red
00429402  fild target green local
0042940A  fstp target_green
00429410  fild target blue local
00429414  fstp target_blue
```

`+8/+10/+12` 三个读取都发生在第一次 target 全局写之前。`+10` 或 `+12` 读取失败时，三个 target 必须全部保持旧值；只有三项均可读后才按 red、green、blue 顺序写入。

### duration 与 step

`0x004293C6 xor edx,edx` 先清高位，`0x0042941A mov dx,[current+14]` 再读取 duration，故它是完整零扩展 u16，而不是 signed 值。随后写 dword countdown，并把 duration 以 `fild dword` 留在 x87 栈上供三次除法共用。

每一轴先把刚写入的 current float 交给 `sub_489654`。该 helper 临时把 x87 rounding mode 改为 toward zero、执行 `fistp qword` 再恢复；这里 current 刚由 s16 整数生成，转换结果精确等于原 current 整数。CPU 再做 target integer 减 current integer，`fild` 差值、除以栈上的 duration，并 `fstp dword` 到对应 step。顺序为 red、green、blue，最后 `fstp st` 弹出 duration。

## x87 除法与零时长

非零 duration 的有效输入域为：

```text
delta    = target_s16 - current_s16 = -65535..65535
duration = u16 = 1..65535
```

兼容 helper 用至少 binary64 精度完成整数比值后写回 binary32。该域的分子、分母均不超过 16 位；非精确值到 binary32 midpoint 的最小距离大于 binary64 半 ulp，因此不会产生相对原 x87 extended→binary32 的双重舍入差异。

原 x87 控制字默认屏蔽浮点异常。duration 为零时，`fdiv` 后 `fstp dword` 的位型为：

```text
positive delta / +0 -> 0x7F800000 (+Inf)
negative delta / +0 -> 0xFF800000 (-Inf)
zero delta     / +0 -> 0xFFC00000 (x87 indefinite quiet NaN)
```

modern 显式生成这三种位型，不依赖宿主编译器对浮点零除和 NaN payload 的选择，也不把零时长改成默认时长、错误状态或提前停止。

在宿主 x87 最小探针中，对全部资产 54 种唯一 `(delta,duration)` 组合，加零时长三种符号与正负极值，共 59 组逐位比较；x87 `fild/fdiv/fstp dword` 与兼容 helper 为 0 差异。

## 成功、共同出口与平台边界

三项 step 全部写完后：

```text
0042948A  add context_ip,16
0042948F  add current,16
00429492  fstp st
00429494  save current
00429498  jmp 0042B0AE
```

共同出口发布有效 opcode52 后同调用 next fetch。modern 在 IP 推进后补齐 `previous_opcode` 发布。

`frame_color` 是 modern typed owner。原版第一次 owner 写发生在 `+2` 读取和整数转浮点之后；modern 因此先验证并读取 `+2`，再在首次 current-red 写入点检查 owner。owner 缺失返回 `runtime_unavailable`，不写颜色状态、IP或previous。

## 窗口边界

以可用 operand 数量 0..6 覆盖 0x8000 窗口尾：

- current red/green/blue 各自在对应 word 可读后立即写入；
- target red/green/blue 只有三项全部可读后才整体进入三次写入；
- duration 不可读时保留六项 current/target 写入，但 countdown 与三项 step 不变；
- 完整 16 字节记录从 `0x7FF0` 精确结束于 `0x8000` 时，先完成 countdown、step、IP和previous，再由下一 fetch 返回 `instruction_out_of_range`。

## 真实资产

`story-vm-talk-linear-records.tsv` 锁定：

| 文件 | 物理记录 | entry probes |
| --- | ---: | ---: |
| `TALK1.DAT` | 733 | 733 |
| `TALK2.DAT` | 25 | 25 |
| `TALK3.DAT` | 174 | 174 |
| `TALK4.DAT` | 429 | 429 |
| 合计 | 1361 | 1361 |

全部记录 raw 为 `0x0034`、长度 16。资产参数范围：六个 signed component 均在 `-30..30`，duration 在 `1..46`，没有零时长记录。

四个 TALK 文件中原始低位字样 `0x0034` 共 2228 次，但只有 1361 个已解码入口可作为指令证据。`0x4034/0x8034` 字样为 0；`0xC034` 仅有 1 个偶然字节候选且不是入口，不能伪造高位 alias 资产。

代表性真实回放：

```text
TALK1.DAT@0x000043B8
34 00 E2 FF E2 FF E2 FF 00 00 00 00 00 00 06 00
```

即 current `(-30,-30,-30)`、target `(0,0,0)`、duration `6`，三项 step 均为 `5.0F`；验证完整物理字节、状态、16 字节推进、previous与same-call continuation。

## 测试覆盖

- 四种 raw 高位 alias与归一化previous；
- signed component 全范围、duration `0xFFFF` 零扩展、正常正负/分数step；
- duration 0 的 `+Inf/-Inf/0xFFC00000` 位型；
- `frame_color` owner 首次访问时点；
- operand 0..6 个可用时的current逐写、target全读后全写、countdown/step保留；
- `0x7FF0`完整精确尾；
- 无audio、无yield、同调用续取；
- `TALK1.DAT@0x000043B8`真实回放；
- 全资产54种唯一差值/时长组合的x87位级对照。

## 双向收敛与分类

LST→C++：七个operand的符号/零扩展、current逐写、target三读后三写、countdown、`sub_489654`等价整数差、x87三次除法与零时长、IP、previous和same-call均一一映射。

C++→LST：没有完整记录预验、提前target写、signed duration、宿主NaN payload漂移、零时长正规化、yield、audio或漏发previous。新增owner/window检查只隔离原版不可安全表达的地址越界域并保留此前效果。

```text
assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated
```
