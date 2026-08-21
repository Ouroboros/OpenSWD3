# 剧情 VM 世界场景显示与清屏 `0x00429693`

## 结论

`sub_427920` 的`0x00429693`由主表opcode60、61共享，两条物理长度均为2字节且无operand：

```text
60  恢复世界场景合成
61  清零完整framebuffer并暂停世界场景合成
```

两条首先都把`dword_4C9A18`的bit0清零。opcode60随后直接推进2字节；opcode61继续清零`dword_4CD76C`指向的`0x25800`个dword，再把场景flag低字节bit0置回1。两条最终均发布normalized previous并跨帧让出。

## 共享入口与分支

入口严格按normalized opcode减60分流：

```text
00429693  mov ecx,dword_4C9A18
00429699  add edx,-60
0042969C  and ecx,0FFFFFFFEh
0042969F  cmp edx,1
004296A2  mov dword_4C9A18,ecx
004296A8  jnz 0042D1C4
```

因此bit0清除是两条共同且最先发生的副作用：

- opcode60的delta为0，`jnz`跳到共享+2尾；
- opcode61的delta为1，落入framebuffer清零路径。

原版对整个dword执行`AND 0xFFFFFFFE`，只改变bit0并保留其余31位。modern世界帧owner把已知运行flag建模为低`u8`，本组同样只改变bit0并保留bits1–7；没有复制不存在于现代帧模型中的高24位。这是全局状态收窄的平台适配，不改变现代有效域内的场景合成行为。

## opcode61清屏与重新置位顺序

opcode61落入：

```text
004296AE  mov edi,dword_4CD76C
004296B4  mov ecx,25800h
004296B9  xor eax,eax
004296BB  rep stosd
004296BD  mov eax,dword_4C9A18
004296C2  mov ebx,[current_local]
004296C6  or al,dl
004296C8  add ebx,2
004296CB  mov dword_4C9A18,eax
004296D0  add word ptr [context_ip],2
004296D5  mov [current_local],ebx
004296D9  jmp 0042B0AE
```

此时`DL==1`，所以`or al,dl`只把低字节bit0置回1；之前清位后保存的其他位不变。顺序不可交换：

1. bit0先清零；
2. 从全局取framebuffer裸指针；
3. 清零完整固定画布；
4. bit0重新置1；
5. IP推进与previous发布。

原版不检查framebuffer指针。如果裸指针无效，故障发生在bit0已经清零之后，重新置位、IP推进和previous发布之前。modern SDL端口持有已构造的`LegacyFramebuffer`，以`std::ranges::fill(framebuffer.physical_pixels(), 0)`清零；平台有效域移除了原版悬空裸指针，但测试回调固定了清屏发生时bit0已经清零。

## framebuffer尺寸与场景含义

渲染常量直接锁定：

```text
width    = 640
height   = 480
pixels   = 0x4B000
bytes    = 0x96000
DWORDs   = 0x25800
```

因此原版`rep stosd`与SDL端口清的是同一块`640 × 480 × 16-bit`软件framebuffer，不是窗口展示surface。

现代`kLegacyWorldFrameClearOnly == 0x01`映射这一bit：

- bit0为0时执行世界背景、空间角色、主图片动作和其余世界层；
- bit0为1时走`clear_only`路径，再清固定画布并只保留次图片动作等受允许层。

所以opcode60恢复世界场景合成；opcode61立即清空当前画布并让后续帧暂停世界场景合成。

## IP、previous、yield与边界

opcode60远尾为：

```text
0042D1C4  add ebx,2
0042D1C7  add word ptr [context_ip],2
0042D1CC  mov [current_local],ebx
0042D1D0  jmp 0042B0AE
```

opcode61在本地执行同样的+2更新后跳共同join。两路入口均未设置`ESI=1`，故共同join发布normalized opcode并返回：

- IP按u16加2；
- previous分别为60或61；
- 状态为yielded，同调用不获取下一条；
- 从`0x7FFE`开始的完整2字节记录可以先完成flag/清屏副作用和previous发布，再以IP `0x8000`正常让出。

modern的`scene_render_flags == nullptr`是原版固定全局owner不存在域的typed stop；它发生在任何flag或framebuffer副作用之前，且不推进IP、不发布previous。

## 真实资产

锁定目录共有41条物理记录、41个entry probe：

| 文件 | opcode60 | opcode61 | 合计 |
| --- | ---: | ---: | ---: |
| `TALK1.DAT` | 5 | 6 | 11 |
| `TALK2.DAT` | 5 | 4 | 9 |
| `TALK3.DAT` | 6 | 6 | 12 |
| `TALK4.DAT` | 5 | 4 | 9 |
| 合计 | 21 | 20 | 41 |

全部decoded记录长度2；60均为raw `0x003C`，61均为raw `0x003D`。四文件逐字节原始字样计数为：

| raw word | 原始出现 | decoded入口 |
| --- | ---: | ---: |
| `0x003C` | 447 | 21 |
| `0x403C/0x803C/0xC03C` | 0 | 0 |
| `0x003D` | 151 | 20 |
| `0x403D` | 1 | 0 |
| `0x803D/0xC03D` | 0 | 0 |

唯一`0x403D`字样位于`TALK1.DAT@0x000002E0`的文件头连续dword目录（相邻值`0x4035/0x4039/0x403D/0x4041/0x4045`），不是指令入口。原始低位字样也不能替代锁定目录记录。

代表性真实回放：

| 文件偏移 | 字节 | 行为 |
| --- | --- | --- |
| `TALK1.DAT@0x000044E3` | `3D 00` | 清屏并暂停世界场景 |
| `TALK1.DAT@0x000046B6` | `3C 00` | 恢复世界场景 |

## 测试覆盖

- opcode60、61各四种raw alias，全部归一化到对应语义opcode；
- 初始flag `0xA5`固定bits1–7保留：60结束为`0xA4`，61清屏时观测`0xA4`、结束恢复`0xA5`；
- 60不调用清屏端口，61恰调用一次；
- 两条均推进2、发布对应previous、`executed_instruction_count=1`并yield；
- 两条owner缺失均在所有副作用前返回`runtime_unavailable`；
- 两条`0x7FFE`精确尾均完成副作用、previous发布并把IP推进到`0x8000`；
- TALK1各一条真实60/61记录顺序回放；
- 既有剧情100真实长链覆盖三次opcode61清屏与一次opcode60恢复，并继续到首场对话/后续战斗请求；
- SDL端口使用固定`LegacyFramebuffer::physical_pixels()`，其常量尺寸与原版`0x96000`字节一致。

## 双向收敛与分类

LST→C++：共享入口、共同先清bit0、60直接尾、61完整framebuffer清零后重新置bit0、+2、normalized previous与yield均有映射。

C++→LST：没有额外展示调用、同调用继续、清屏后再清bit、漏发previous或改变bits1–7。u8 flag owner、typed owner检查和持有型SDL framebuffer只隔离原版固定全局/裸指针无效域。

```text
assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated
```
