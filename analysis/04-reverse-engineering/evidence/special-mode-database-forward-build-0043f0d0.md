# 标准模式数据库forward构建 `0x0043F0D0`

状态：`platform_adapted`、`unit_tested`

## LST范围与算法

唯一行为真值为`swd3.exe.lst`。范围`0x0043F0D0..0x0043F159`，99行；唯一caller F000，唯一callee F7C0。

arg0为adjustment head owner，arg4为以forward head开头的输出owner block，arg8为page selection。函数先清forward head及输出block `+8` word，但不清`+4` word。随后逐项调用F7C0：

- 未命中：保留在adjustment链并推进source link。
- 命中：从adjustment链摘除，按u16 `text_index`插入forward链。
- 插入比较严格保留原怪异条件：`current_key >= new_key`且`previous_key < new_key`才在当前位置插入；首项previous key读取未清零的输出block `+4`陈旧word。因此陈旧值较大时，小key可能被追加到大key之后。

新增`forward_build_sentinel`映射`FCAE4`并保持不清，`forward_build_word`映射`FCAE8`且每次写0。F7C0保持单节点/page typed筛选边界，不提前关闭。

## F000回接与测试

F000删除原`build_database_forward_list`整块port，直接调用F0D0；随后仅在输出head为空时调用D5D0。

UT覆盖：

- 5、3两个命中节点在sentinel0时构成3→5升序链，adjustment清空。
- sentinel10时保留5→3反常顺序，证明陈旧`FCAE4`未被“修复”；`FCAE8`仍写0。
- F000先执行F080、再逐节点F7C0，count/window/BC90 owner保持正确。
- E080/E170无adjustment source时F7C0零调用，旧伪造generic事件删除；后续D5D0/BCC0顺序保持。

## 验证

定向测试通过。workpack双生成稳定为`60/227`，SHA256均为`5d77602f24d93396086abdf6653387e67d22e60a7a41996b102588148ea90920`；下一单元`0x0043F160`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
