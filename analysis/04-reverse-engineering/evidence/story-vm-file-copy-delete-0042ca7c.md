# 剧情 VM 文件复制/删除 `0x0042CA7C`

状态：`platform_adapted`、`unit_tested`、`asset_absence_verified`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042CA7C..0x0042CBAF`

opcode：158 / `OP_158_COPY_STORY_FILE`；159 / `OP_159_DELETE_STORY_FILE`

## 1. 共享变长记录

两条opcode共享同一handler。机器把物理指针直接增加4，因此自身`+2` word完全不读取；从`+4`开始逐字节扫描首个未对齐`25 51`（`%Q`），同时把marker前字节复制到全局C字符串scratch。

物理消费长度为：

```text
6 + marker前字节数
```

IP只推进到`%Q`之后。扫描不把NUL当终止符；如果payload中先出现NUL，机器仍继续扫描到`%Q`，但后续`lstrcatA`只能看到首个NUL前的文件名。现代保持这两个不同边界。

找不到完整marker时，原版继续越过窗口裸读；现代在该首次窗口外读取点返回typed stop，不推进IP、不触发宿主激活门、不调用文件port，也不发布previous。

## 2. 原始lookahead类别

定位marker后，机器严格按以下顺序执行：

1. 写`dword_4BABA4 = 1`；
2. 把IP与物理指针更新到`%Q`之后；
3. 从下一条指令的`+2`读取i16类别；
4. 分配512-byte目标路径buffer并构造双路径；
5. 执行文件API，释放buffer。

类别不是当前记录自身的`+2` word：

```text
next_instruction[+2] == 0  -> Video\
next_instruction[+2] == 1  -> Music\
其他signed i16值           -> 根目录
```

lookahead缺失发生在激活门写入和IP推进之后，但在分配、路径构造、文件API与previous之前。现代以独立host-frame port锁定该副作用时点。

## 3. copy/delete方向与返回值

原版启动时把`Buffer`初始化为启动工作目录加尾`\`，把`byte_4B82F8`初始化为第一张CD-ROM根。handler对source与destination追加相同的可选`Video\`/`Music\`和文件名：

```text
source      = CD/data root + category + filename
destination = launch root  + category + filename
```

opcode158调用`CopyFileA(source, destination, FALSE)`，允许覆盖目标。opcode159只调用`DeleteFileA(source)`，不删除destination。两个Win32返回值都完全忽略，随后释放临时buffer。

SDL以配置data root替代CD root，以启动前launch directory替代`Buffer`，把脚本反斜杠映射为宿主分隔符，使用覆盖copy和source-only remove。空文件名与目录删除保持Win32失败语义；文件API失败仍返回VM并继续。固定512-byte heap、共享无界scratch和宿主文件路径由typed RAII/path承接，不复制越界内存破坏。

`dword_4BABA4`是Win32消息泵frame execution gate；原激活事件负责恢复。SDL已明确采用焦点变化时继续后台运行的宿主策略，因此host-frame port在SDL为有记录的no-op，不把该Win32门复制成会永久停帧的业务状态。

## 4. previous、same-call与资产

两条opcode在文件API后都设置`ESI=1`并进入`loc_42B0AA`：恢复`%Q`后的物理指针，发布normalized previous158/159，并在同一次VM调用中fetch下一条；无audio、无yield。文件API失败不改变该合同。

完整线性TALK目录中opcode158和159均为0条物理记录/0 probes，使用`asset_absence_verified`。四种raw word全文件双字节候选计数为：

```text
opcode158   009E 409E 809E C09E
TALK1.DAT      3    0    0    0
TALK2.DAT     15    0    0    0
TALK3.DAT     98    0    0    0
TALK4.DAT     12    0    0    0

opcode159   009F 409F 809F C09F
TALK1.DAT     55    0    0    1
TALK2.DAT    105    0    0    0
TALK3.DAT      9    0    0    0
TALK4.DAT      4    0    0    0
```

这些字样均非线性指令入口。synthetic覆盖两opcode各四raw alias、自身prefix未读、三类lookahead、内嵌NUL、文件API失败、marker缺失、marker后lookahead截断、host-frame时序、最后window word类别和same-call successor。

Story VM synthetic、real及initial-session三项通过。Linux core `186/186`、app `192/192`完整门通过。未启动原版或OpenSWD3游戏EXE。
