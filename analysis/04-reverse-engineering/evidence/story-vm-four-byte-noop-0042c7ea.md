# 剧情 VM 四字节空操作 `0x0042C7EA`

状态：`assembly_exact`、`unit_tested`、`real_asset_tested`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042C7EA..0x0042C7F6`

opcode：`98`

## 1. handler 行为

机器在分派入口已经取得当前两字节 opcode；本 handler 随后只执行以下步骤：

1. 物理脚本指针 `EBX += 4`；
2. 16 位 instruction offset `word ptr [EBP] += 4`；
3. 保存新的物理脚本指针到 `var_50`；
4. 进入 `0x0042B0AE` common join。

fetch loop 在每次分派前把 `ESI`清零，本 handler 不修改它。common join 因而发布归一化 previous opcode 98，并按 `ESI=0` yield。它不是 same-call no-op。

名义上的 `+2 u16` payload 从未被机器读取，也没有 helper、全局状态或平台依赖。modern handler同样不检查、不解码、不使用payload，只在成功取得当前opcode后推进4、发布previous98并yield。

## 2. 窗口边界

完整四字节记录起于`0x7FFC`时，handler先完成IP=`0x8000`、previous98和yield；不会尝试执行后继指令。

因为payload未读，仅两字节opcode起于`0x7FFE`也会完成：IP推进至`0x8002`并发布previous98/yield。下一次VM调用的typed fetch才返回`instruction_out_of_range`，此前副作用不回滚。synthetic以四个raw alias分别锁定正常记录、完整精确尾和缺payload尾。

## 3. 资产锁与验证

线性TALK目录锁定3条物理记录/3 probes，全部raw `0x0062`、长度4，分布：

```text
TALK1/2/3/4 = 0/1/2/0
```

真实记录及机器未读payload为：

```text
TALK2.DAT@0x0001708D  payload 0x0190
TALK3.DAT@0x0000B039  payload 0x006C
TALK3.DAT@0x0000CFD2  payload 0x0001
```

三条记录均在`0x7FFC`精确尾回放，验证IP=`0x8000`、previous98和yield。Story VM synthetic、real及initial-session三项共同通过；未启动原版或OpenSWD3游戏EXE。

分类：`assembly_exact`。该handler没有需要替换的Win32、裸owner或外部依赖；现代实现逐项保持推进宽度、未读payload、previous publication和yield协议。
