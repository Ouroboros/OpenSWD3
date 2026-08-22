# 剧情 VM 输入菜单重置 `0x0042C3B0`

状态：`assembly_exact`、`unit_tested`、`asset_absence_verified`、`platform_adapted`、`sdl_runtime_integrated`；`sub_406D30`的B9/B11 owner仍为明确外部依赖，SDL不伪造成功。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042C3B0..0x0042C3F6`

opcode：135

## 1. 记录与固定写入

记录只含u16 opcode，固定长度2，无operand。handler严格按顺序写四个完整dword：

```text
dword_4FBAB0 = 4
dword_4CAEBC = 1
dword_4CAEC0 = 0
dword_4CAEB0 = 3
```

`dword_4CAEB0`是既有high-priority state；新游戏提交路径把它清零。`dword_4CAEBC/C0`是同一模式族的submode与auxiliary，新游戏分别写1/1。现代复用frame coordinator和SDL现有high-priority owner。`dword_4FBAB0`由special-mode/player-control路径共同读取，现代建立中性special-input owner，等待B9消费者接入。

四个原版全局不可能缺失。现代runtime按四个原始写点逐项借用；任一binding缺失只在对应写点返回`runtime_unavailable`，不回滚此前已经完成的写入，也不调用后续helper、推进IP、service audio或发布previous。

## 2. `sub_406D30`边界

四个dword全部写完后，无参数调用`sub_406D30`。该helper严格执行：

1. 从`dword_4B7CB0`开始清零`0x80`个dword，即完整`0x200`字节输入/菜单工作区。
2. 写`byte_4CC160=0`、`byte_4CC161=1`、`dword_4CC298=0`和`dword_4CB240=0`。
3. 对`0x004CC200`公共action record调用`sub_40DC00`重置。
4. 对`0x004CB968/0x004CBC10/0x004CBEB8`三条存档预览action调用`sub_4099C0`。
5. 对signed `dword_4CC2A0`执行向零除3；以`group*3 + 0/1/2`分别调用`sub_409600`重建三条预览。
6. 调用`sub_409B80`完成预览状态，返回1；调用者不读取返回值。

该helper已归属special-modes，内部又借用persistence和asset-runtime。现代new-game transition已有同名跨模块合同，但SDL实现仍未建立菜单工作区与存档预览owner。本handler通过可失败VM port转交B9/B11：测试替身成功时验证完整后续时序；生产SDL当前明确返回false，使handler停在四个状态写之后，不假装helper已完成。

## 3. IP、audio与yield

只有helper成功返回后才执行：

```text
IP += 2
AIL_serve()      // handler内部
previous = 135   // common join发布
AIL_serve()      // common join最终service
yield
```

机器在调用helper后从保存槽重取物理脚本指针，再更新u16 IP；第一次audio发生在IP提交后、common join发布previous之前。`var_28|ESI==0`，common join发布previous后必经`0x0042D4D7`第二次audio再返回。handler没有same-call continuation、分支、operand或自修改。

完整两字节记录可位于`IP=0x7FFE`并精确结束于窗口尾：四写、helper、IP=`0x8000`、第一次audio、previous、第二次audio和yield均完成，不读取后继字节。

## 4. 资产与验证

完整线性TALK目录对opcode135为0条物理记录/0 probes，因此使用`asset_absence_verified`而不伪造真实回放。四文件基础raw `0x0087`字样为`14/45/14/5`；`0x4087`仅在TALK1出现1处，`0x8087/0xC087`为零。上述字样均不位于已证明的线性记录入口。

synthetic覆盖四raw alias、四项完整dword覆盖、reset callback观察四写、两个audio callback分别观察已提交IP/未发布previous与common已发布previous、event顺序、四个runtime binding逐项缺失、B9/B11 reset port失败保留四写，以及`IP=0x7FFE`精确尾。成功路径固定一次reset、两次audio、previous135与yield；失败路径不伪造后续副作用。

Story VM synthetic、real及initial-session三项通过。Linux core 186/186与app 192/192完整门以exit0通过。未启动原版或OpenSWD3游戏EXE。
