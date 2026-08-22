# 剧情 VM 按角色 GUID 删除对话 `0x0042B8E6`

状态：`platform_adapted`、有效运行域`assembly_exact`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042B8E6..0x0042B9C1`

opcode：`118` / `OP_118_REMOVE_DIALOGS_FOR_ROLE_GUID`

## 1. selector与空链

记录固定四字节：

```text
+0  u16 opcode
+2  u16 role GUID selector
```

机器先读取selector；值为`0xFFF0`时只把局部AX替换为context source GUID，不写回脚本。随后才读取`dword_4ACF48`消息链head。空链直接进入成功尾，不访问角色表、消息字段或计数器。

公共fetch已把raw word按`0x3FFF`归一，因此`0076/4076/8076/C076`共享语义。

## 2. 遍历、匹配与解链

非空链使用`unk_4ACF00`的`+0x48`作为predecessor sentinel。每轮严格执行：

1. 从当前消息`+0x16`零扩展读取raw role index；
2. 调用`sub_40C060(index)`，按角色表`+0x24`取得GUID；
3. 与selector比较；
4. miss时current与predecessor都推进到各自`+0x48`；
5. match时把`predecessor->next`改为`current->next`，predecessor保持不动；
6. 重新读取消息raw role index；值不为`0xFFFD`时，把该角色`+0x26 interaction_gate`低word清零；
7. 依次释放消息`+0x38`文字分配、`+0x44`caption分配和`0x4C`消息节点；
8. 从predecessor重新读取下一节点；
9. 最后更新共享对话计数。

因此handler按“消息所属角色index映射出的GUID”删除全部匹配项，不只删除首项，也不直接把selector当index。重复GUID角色的消息都会删除；非匹配项保持原相对顺序。连续匹配项仍使用同一predecessor。

现代实现以`std::list::splice`先摘出节点，再清`interaction_gate`，随后显式释放text、caption并销毁节点，保留原解链与释放顺序。额外的choice/vector元数据随节点RAII销毁，是64位owner适配，不改变物理消息语义。

## 3. 角色索引危险点

`sub_40C060`先把raw index与角色数量比较；越界时原版弹出诊断，但随后仍按该index读取角色GUID，属于裸数组危险点。现代在同一GUID读取阶段返回`role_not_found`：当前无效消息仍留在链中，IP和previous不发布；此前已完成的匹配删除、角色gate清零及计数递减不回滚。

原handler在match后另检查raw index是否为`0xFFFD`，仅该值跳过角色gate清零。但正常256项角色表中，`0xFFFD`在更早的`sub_40C060` GUID读取已经越界；不能把该后置检查伪装成安全的detached-message路径。synthetic以一个先行合法match再遇`0xFFFD`锁定此分阶段停止合同。

受检role span与owned消息链替代固定角色表、裸predecessor和手工free，故分类为`platform_adapted`；全部合法index路径保持机器行为。

## 4. counter、IP与same-call

每删除一项，机器在三次free后重新读取完整`dword_4A9920`：

```text
lock = value & 0x00008000
count = value & 0x00007FFF
count = max(count - 1, 0)
value = lock | count
```

高16位及bit15之外的位全部清除。count已为0时继续删除仍保持0；bit15保持。计数按每个删除项更新，不按角色或单次handler只减一。

成功后u16 IP加4，发布normalized previous118，不service audio、不yield，并在同一次解释器调用继续取后继。完整记录精确结束在`0x8000`时，先完成删除、counter、IP和previous，再由下一fetch返回窗口越界。只有opcode而缺selector时，在接触消息链前返回operand越界。

## 5. 资产锁与验证

线性TALK目录锁定1669条物理记录/1669 probes，全部raw `0x0076`、长度4：

```text
TALK1    24
TALK2   278
TALK3     9
TALK4  1358
```

selector只有六种：

```text
0x0000      1
0x0001     11
0x000E      1
0x0142      3
0x2710    873
0x2711    780
```

当前线性记录没有`0xFFF0/0xFFFD/0xFFFE/0xFFFF`。高位raw alias未作为线性入口观察；全文件唯一`0xC076`字样不是证明入口。

真实回放使用`TALK1.DAT@0x0000462C`起连续两条记录`{118,0},{118,1}`，验证同一次step依次删除GUID 0和1的消息、清两个角色gate、把`0x8002`计数降为`0x8000`，再进入后继。synthetic另覆盖四raw alias空链、重复GUID与连续/分隔match、非匹配顺序、高位清除、低15位零夹、先行删除后无效index停止、selector截断、`0xFFF0`不写回及完整精确尾。

Story VM synthetic、real及initial-session三项通过；Linux core 186/186及app 192/192完整门通过。未启动原版或OpenSWD3游戏EXE。
