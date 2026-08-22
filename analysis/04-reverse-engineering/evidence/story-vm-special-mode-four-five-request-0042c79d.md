# 剧情 VM 特殊模式4/5请求 `0x0042C79D`

状态：`assembly_exact`（有效owner/helper域）、`unit_tested`、`real_asset_tested`、`platform_adapted`、`sdl_runtime_integrated`；`sub_406D30`的B9/B11 owner仍为明确外部依赖，SDL不伪造成功。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042C79D..0x0042C7FA`

opcode：144 / `OP_144_REQUEST_SPECIAL_MODE_4_OR_5`

## 1. 记录与特殊模式选择

名义记录固定4字节：

```text
+0  u16 opcode
+2  u8  selector
+3  u8  unread padding
```

机器在读取selector之前先写：

```text
special_mode_state = 0x80000004
```

随后只读取`+2`低字节：selector为0时再次写相同mode4；selector为1时改写mode5；其他u8值保持第一次mode4。`+3`从不读取，高字节不参与选择。

现代按原访问点先借用并写实际special-mode owner，再检查selector byte。selector缺失时保留已提交mode4，并以`operand_out_of_range`停止；不清后续状态、不调用helper、不推进IP、不发布previous，也不service audio。

## 2. reset前后分阶段写入

selector处理后严格执行：

1. `high_priority_state = 0`。
2. 调用`sub_406D30`重置输入/菜单工作区与三条存档预览。
3. `high_priority_submode = 0`。
4. `high_priority_auxiliary = 0`。
5. `special_input_mode = 0`。

五项状态复用opcode133/135以来已接入SDL的实际runtime owner。每个现代binding只在对应原始写点检查；失败不回滚已提交mode或此前清零项。

原`sub_406D30`无失败返回，但完整owner跨special-modes、persistence与asset-runtime。现代复用opcode135建立的可失败窄port：测试替身成功时继续完整尾；SDL当前返回false，使handler停在mode和high-priority state已提交之后，不伪造B9/B11 helper成功，也不执行后三写。

## 3. IP、previous、audio与yield

成功尾为：

```text
IP += 4
previous = 144
AIL_serve()
yield
```

opcode144自身不写ESI，normal continuation carry为0。common join先发布previous144，再进入audio tail；这与opcode135的局部audio/previous顺序不同，不能继承其结论。

完整4字节记录可位于`IP=0x7FFC`并精确结束于窗口尾。因`+3`未读，opcode还可位于`IP=0x7FFD`，只要`+2`selector位于`0x7FFF`：所有状态、IP=`0x8001`、previous、audio和yield仍完成。

## 4. 资产与组合验证

完整线性TALK目录锁定1条物理记录/1 probe：

```text
TALK1.DAT@0x00025F51  raw0090, selector0, padding0
```

四库基础raw `0x0090`字样总数为`12/0/9/1`；三个高位alias字样均为零。真实记录回放请求mode4，按顺序完成reset与四项清零，随后previous/audio/yield。

synthetic覆盖四raw alias、selector `0/1/2/255`、unread padding、reset/audio callback时序、special mode首写后的selector截断、special-mode/high-state/submode/auxiliary/input五项binding失败、reset helper失败、完整精确尾及缺padding尾。

opcode134耗尽first资源并自修改下一word为144的synthetic和真实损伤链也已更新为完整组合回放：same-call进入144，使用遗留后继低字节选择mode4，最终audio/yield；此组合验证不替代本handler独立验收，也不重新计算opcode134。

Story VM synthetic、real及initial-session三项通过。Linux core `186/186`与app `192/192`完整门通过。未启动原版或OpenSWD3游戏EXE。

关联helper证据：`story-vm-input-menu-reset-0042c3b0.md`。
