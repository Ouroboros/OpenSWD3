# 剧情 VM 次要角色 bit30 条件重载 `0x0042B6A5`

状态：`platform_adapted`、有效运行域`assembly_exact`、`unit_tested`、`real_asset_tested`、`asset_absence_verified`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042B6A5..0x0042B707`

opcodes：

- `110`：`OP_110_RELOAD_IF_NO_SECONDARY_ROLE_BIT30`
- `111`：`OP_111_RELOAD_IF_ANY_SECONDARY_ROLE_BIT30`

## 1. 角色扫描

handler先读取完整u32角色数量，把扫描index初始化为1，并把谓词初始化为false。角色数量`<=1`时直接跳过扫描；否则从角色1开始，按固定`0xD8`步长只测试`role+0x10`完整flags的bit30，扫描到第一个置位角色时立即停止。

角色0永远不参与。扫描不修改角色、目标或VM状态；机器保存的最终扫描index只供局部账本，不进入后续业务写入。

opcode111在扫描后把谓词精确反相。因此分支合同为：

```text
                没有次要角色bit30    至少一个次要角色bit30
opcode110       重载                  顺序消费
opcode111       顺序消费              重载
```

## 2. target访问与控制流

物理记录固定为6字节：

```text
+0  u16 opcode
+2  u32 同文件TALK data target
```

`+2 u32`只在重载分支读取，而且读取发生在扫描流程结束（首命中可提前结束）之后。顺序分支完全不读target，即使窗口只剩opcode两个字节，也仍把u16 IP和物理指针各推进6字节，发布previous110/111并同调用取下一条。

重载分支通过`loc_428310 → loc_42CCD5 → sub_42E430`执行：

1. 读取`+2 u32 target`；
2. service audio一次；
3. 写入当前TALK data offset；
4. 把u16 IP清零；
5. seek到物理`target + 0x200`并读取新的`0x8000`窗口；
6. 把物理指针换成新窗口首地址；
7. 发布previous110/111并同调用取新窗口首指令。

现代实现复用已闭环的`load_same_file_story_window`。文件、seek、短读和窗口owner以typed失败隔离；失败保留audio、target、IP零、window invalidation和previous发布等原危险点之前的副作用，因此整体分类为`platform_adapted`。

## 3. 资产锁

线性TALK目录结果：

```text
opcode110  0条 / 0 probes
opcode111 24条 / 24 probes
```

opcode110使用`asset_absence_verified`，不把零散raw字样冒充入口。opcode111全部为raw `0x006F`、长度6，文件分布为：

```text
TALK1.DAT 16
TALK2.DAT  8
TALK3.DAT  0
TALK4.DAT  0
```

24个target全部位于对应TALK文件有效范围内且向后跳转；23个目标首指令是opcode109，1个是opcode1026。真实回放使用`TALK2.DAT@0x0000F981`：无次要角色bit30时顺序进入opcode67；角色1置bit30时重载`0x0000F72D`并执行其count18 opcode109记录。

既有`TALK1.DAT@0x00054136`的opcode18→111链也已更新，证明无bit30时opcode111跳过未使用target并同调用进入后继等待。

## 4. 验证

synthetic覆盖两个opcode的四raw alias和两种谓词结果、角色0排除、角色数量1、checked load失败、重载分支target截断、顺序分支未读target、重载/顺序精确窗口尾、previous、audio与same-call后继。

Story VM synthetic、real及initial-session三项通过。Linux core 186/186、app 192/192通过。未启动原版或OpenSWD3游戏EXE。
