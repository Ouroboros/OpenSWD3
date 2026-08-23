# 标准模式数据库forward排序 `0x0043F160`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。范围`0x0043F160..0x0043F1DE`，71行；唯一caller F880，无callee。

函数先清零0xB0字节本地块，再逐项从`forward_head`摘除节点、把节点next写0，并以u16 `text_index`插入本地链。比较条件与F0D0相同，但首项previous key来自已清零的本地`+4`，因此正常形成升序链；不会读取或清`FCAE4`陈旧sentinel。结束时写回head，并只清`FCAE8/FCAEA`两个word。

新增无port typed helper和`forward_build_tail_word` owner。UT以3→1→5输入锁定1→3→5输出、每节点next重写、`FCAE4`保持77以及`FCAE8/FCAEA`清0。F880现直接调用本helper并传播后续BB40返回合同。

定向测试通过。workpack双生成稳定为`61/227`，SHA256均为`aafa7a79eceafd91431f24d3968462f30e96a2f1efd5f87f9bd7e038dc568253`；下一单元`0x0043F1E0`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
