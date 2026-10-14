# 剧情 VM 镜头移动组 `0x00429066`

## 结论

`sub_427920` 的opcodes50、70、73共享唯一入口`0x00429066`，但使用两种物理长度：

```text
opcode50 / opcode70 (10 bytes)
+0  u16 raw opcode
+2  s16 X tile displacement / absolute target X tile
+4  s16 Y tile displacement / absolute target Y tile
+6  u16 requested X step
+8  u16 requested Y step

opcode73 (8 bytes)
+0  u16 raw opcode
+2  u16 role selector
+4  u16 requested X step
+6  u16 requested Y step
```

三条都会覆盖已有镜头移动。50直接使用相对tile位移；70用absolute target tile减去当前viewport left/top的算术右移tile值；73按角色world坐标构造居中且受地图限制的640×480目标viewport，再减当前viewport得到tile位移。

共同路径按机器顺序写raw step、把tile位移左移4位成pixel、用signed `IDIV`检查每轴是否整除、把非整除step改为4、最后按位移符号设置step方向。随后按viewport四边和地图pixel边界夹取remaining displacement；夹取发生在step确定之后，不重新清零、改方向或重算step。

完整成功路径推进50/70的10字节或73的8字节，发布previous并same-call continuation；没有audio、yield或MAPS fallback。

## 共享入口与分支

入口先只检查两个remaining displacement；非零时调用`nullsub_1`诊断，但不检查step状态且仍覆盖旧移动：

```text
00429066  mov ecx,[remaining_y]
0042906C  mov eax,[remaining_x]
00429071  mov esi,ecx
00429073  or esi,eax
00429075  jz 0042909C
...       nullsub_1(existing-move diagnostic)
```

随后按effective opcode分支：

```text
0042909C  cmp edx,32h      ; opcode50
004290A5  cmp edx,46h      ; opcode70
004290AE  cmp edx,49h      ; opcode73
```

### opcode50：relative tile displacement

```text
00429175  movsx eax,word ptr [current+2]
00429179  mov [remaining_x],eax
0042917E  movsx ecx,word ptr [current+4]
00429182  mov [remaining_y],ecx
00429188  operand_index = 3
```

X写先于Y读，两个remaining此时仍是tile单位。

### opcode70：absolute target tile

```text
0042914C  movsx eax,word ptr [current+2]
00429150  mov edx,[viewport_left]
00429156  mov [target_x_local],eax
0042915A  movsx ecx,word ptr [current+4]
0042915E  sar edx,4
00429161  sub eax,edx
00429163  mov edx,[viewport_top]
00429169  sar edx,4
0042916C  mov [remaining_x],eax
00429171  sub ecx,edx
00429182  mov [remaining_y],ecx
00429188  operand_index = 3
```

两个target word都在第一次remaining写之前读取。absolute target不在分支内提前夹取；它先形成target-current tile displacement，后面才进入共享pixel endpoint clamp。

### opcode73：move to role

selector原样交给`sub_40C0D0`，本handler不把`0xFFF0`换成context GUID；因此`0xFFF0`是ordinary字面GUID，`0xFFFE`仍由helper选择controlled index。ordinary查找跳过bit28角色并取首个clear match。

lookup返回值被机器忽略。handler先复制当前viewport四个dword，再按输出index读取role world Y/X并调用`sub_40D160`：

```text
004290B7  mov cx,[current+2]
004290C1  call sub_40C0D0
004290C6..004290F0  copy viewport to local rect
004290FF  mov edx,[role+8]  ; world Y, unchecked index
00429105  mov eax,[role+4]  ; world X
0042910D  call sub_40D160(world_x,world_y,&target_rect)
00429126  target.left -= viewport.left
0042912C  target.top  -= viewport.top
00429131  sar target.left_delta,4
00429134  sar target.top_delta,4
00429137  mov [remaining_x],eax
0042913C  mov [remaining_y],ecx
00429142  operand_index = 2
```

`sub_40D160`的目标viewport顺序为：

1. `left = world_x - 320`，signed negative则置0；
2. `top = world_y - 240`，signed negative则置0；
3. `right = left + 640`；若`right >= map_width*16`，写`left=(map_width-40)*16`、`right=map_width*16`；
4. `bottom = top + 480`；若`bottom >= map_height*16`，写`top=(map_height-30)*16`、`bottom=map_height*16`。

全部加减与左移都是32-bit wrapping，比较为signed。地图尺寸0或小于40×30也不会提前失败，会保留underflow/wrap结果。

## 共享step、IDIV与方向顺序

`operand_index=3`时读取`+6/+8`，`operand_index=2`时读取`+4/+6`：

```text
00429198  mov si,[current+index*2]
0042919C  mov [step_x],esi          ; u16 zero-extended
004291A2  mov di,[current+index*2+2]
004291A9  mov [step_y],edi
004291A7  if tile_x == 0: step_x=0
004291B9  if tile_y == 0: step_y=0
004291C5  tile_x <<= 4
004291C8  tile_y <<= 4
004291CD  mov [remaining_x],pixel_x
004291D2  mov [remaining_y],pixel_y
```

每个非零pixel axis按X后Y执行signed `IDIV step`：

- step为0会在机器`IDIV`处产生整数除零；
- remainder为0保留requested step；
- remainder非0只诊断，并把该轴step改为正4；
- 两轴除法均完成后，才按negative displacement把step取负。

因此Y轴除零时，X轴已经可能改为正4，但尚未应用X负号。modern用`camera_step_divide_by_zero`隔离CPU fault，并保留故障前已经写入的pixel remaining、raw/回退step及原顺序；不把它偷换成`operand_out_of_range`、默认4或成功移动。

## endpoint clamp与原版诊断误读

方向确定后按当前viewport边缘夹取：

```text
if signed_wrap(viewport.left + remaining_x) < 0:
    remaining_x = -viewport.left
if signed_wrap(viewport.right + remaining_x) > signed(map_width << 4):
    remaining_x = signed(map_width << 4) - viewport.right
if signed_wrap(viewport.top + remaining_y) < 0:
    remaining_y = -viewport.top
if signed_wrap(viewport.bottom + remaining_y) > signed(map_height << 4):
    remaining_y = signed(map_height << 4) - viewport.bottom
```

夹取只改remaining，不再改step。即使夹取把remaining变成0，先前非零step仍保留，后续opcode51会同时检查remaining与step。

任一轴夹取后，原版会准备diagnostic参数。共享代码固定读取`current+2/+4/+6/+8`，没有适配opcode73的8-byte布局；因此opcode73发生endpoint clamp时还会把`current+8`（下一条指令word）误读为diagnostic参数。该值不影响业务，但它是条件unsafe read。modern仅在opcode73确实发生共同endpoint clamp时要求`+8`可读：若8-byte记录恰好结束在0x8000窗口尾，先保留全部movement/clamp效果，再以`operand_out_of_range`阻止IP/previous；未夹取的同位置记录可完成效果、IP/previous后在下一fetch返回`instruction_out_of_range`。

## 分阶段unsafe顺序与平台适配

50的读取和写入顺序是：

```text
read +2 -> write X tile displacement
read +4 -> write Y tile displacement
```

70则先读取`+2`、viewport left和`+4`，之后才依次写X/Y tile displacement；因此`+4`截断不会保留新的X值。两条随后共同执行：

```text
read first step -> write raw X step
read second step -> write raw Y step
zero-axis step reset -> both remaining shift to pixels
X IDIV -> Y IDIV -> sign -> viewport/map clamp -> IP/previous
```

73的顺序是：

```text
read selector -> lookup -> copy viewport -> unchecked role Y/X
-> derive/write X/Y tile displacement
-> read/write X step -> read/write Y step
-> shared IDIV/sign/clamp -> optional diagnostic +8 read -> IP/previous
```

ordinary lookup miss原版形成role index`0xFFFFFFFF`，第一次unsafe role read在viewport copy之后。modern先保留lookup与camera-owner顺序，再在该role coordinate访问点返回`role_not_found`；不patch MAPS。public VM session仍在opcode fetch前隔离无效controlled owner。

`camera_pan`是入口第一组global owner；camera owner对70在`+2`读取后首次访问，对73在selector lookup后、unchecked role坐标前首次访问，对50则在step/IDIV完成后的clamp阶段首次访问。typed `runtime_unavailable`按这些首次访问点返回并保留此前效果。

## 真实资产

锁定`story-vm-talk-linear-records.tsv`：

| opcode | 物理记录 | entry probes | TALK1/TALK2/TALK3/TALK4 | raw | 长度 |
| ---: | ---: | ---: | --- | --- | ---: |
| 50 | 17 | 17 | 6/10/0/1 | 全部`0x0032` | 10 |
| 70 | 62 | 62 | 20/11/6/25 | 全部`0x0046` | 10 |
| 73 | 34 | 34 | 10/7/4/13 | 全部`0x0049` | 8 |

合计113条物理记录、113个entry probe。opcode50真实relative范围X=`-25..24`、Y=`-32..419`；出现step 0时对应axis displacement均为0，没有资产内CPU divide fault。opcode70 target X=`0..313`、Y=`0..118`，step组合仅`2/2、4/4、4/16、8/8、16/16`。opcode73 selector为`1`共32条、`4617`和`39`各1条，step为`2/2、4/4、8/8`。

原始byte-word候选远多于证明记录，高位alias仅有少量偶然候选（`0xC032` 4次、`0xC046` 5次、`0xC049` 2次），均没有entry证明；不把候选字节冒充asset opcode。

代表性真实回放：

- `TALK1.DAT@0x0000A18C`: `32 00 00 00 E0 FF 08 00 04 00`，relative `(0,-32)`、step `(8,4)`；
- `TALK1.DAT@0x000046B8`: `46 00 05 00 08 00 04 00 10 00`，absolute target `(5,8)`、step `(4,16)`；
- `TALK1.DAT@0x000096A3`: `49 00 01 00 08 00 08 00`，role selector 1、step `(8,8)`。

三条均验证物理字节、movement结果、长度推进、previous与same-call continuation。

## 测试覆盖

- 三opcode各自四种raw alias、已有movement覆盖、normalized previous与same-call continuation；
- relative signed位移、absolute target减viewport tile origin、role居中/地图目标viewport；
- FFF0字面GUID、FFFE controlled index、bit28 skip与首个clear match、ordinary miss和invalid controlled owner；
- requested step exact division、non-divisible fallback 4、zero axis/zero step、X/Y divide-zero的分阶段效果；
- map zero尺寸wrap、角色目标远端clamp、共同left/right/top/bottom endpoint clamp，以及clamp后不重算step；
- 50/70在`+2/+4/+6/+8`每级截断的partial state和`0x7FF6`完整尾；
- 73 selector/X-step/Y-step截断、`0x7FF8`无clamp完整尾、clamp diagnostic `+8`可用/不可用两路；
- camera-pan/camera owner首次访问顺序、无MAPS、无audio；
- 三条真实TALK回放。

## 双向收敛与分类

LST→C++：三分支参数、signed/u16扩展、role target helper、step读取、zero-axis、pixel conversion、X/Y IDIV顺序、fallback4、方向、signed wrapping四边clamp、条件diagnostic overread、长度、previous与same-call均一一映射。

C++→LST：没有提前pre-clamp absolute target、拒绝zero map、把zero step静默改值、clamp后重算step、FFF0替换、MAPS fallback、audio或yield。新增status及owner/window检查只隔离原版CPU/pointer/window unsafe域，并保留此前effects。

```text
assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated
```
