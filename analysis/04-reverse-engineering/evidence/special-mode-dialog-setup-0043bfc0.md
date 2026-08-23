# 标准模式对话/场景准备 `0x0043BFC0`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与调用图

唯一行为真值为`swd3.exe.lst`。函数物理范围是`0x0043BFC0..0x0043C08F`，下一入口为`0x0043C090`。直接caller为`0x00443BD0`与`0x00446700`，各调用两次。直接callee为`0x00437DF0`接口解析和`0x0042E790`绘制准备；modern通过窄port转发，不复制其owner。

函数参数为三个i32绘制值和一个只读取低16位的word。共享输入还有当前记录索引`dword_4AB378`、接口source值`dword_4ACBA0`和216字节stride记录表。

## 2. 前置副作用顺序

函数严格先执行：

```text
clear_surface(0x96000)
interface = resolve(0x2711)
interface.vtable_14(interface, 0, source_value, 0, 0, 0)
```

modern port将后两步合并为带精确service ID和source值的typed接口请求。当前记录索引的首次表读取发生在这两个副作用之后；索引越界typed-stop仍保留clear与interface调用，但不执行后续draw或状态写。

## 3. 记录索引与绘制调用

LST把当前索引依次乘3、乘9、乘8，得到`index * 216`的32位offset。所选记录向`0x0042E790`提供一个dword字段：

```text
draw(arg0, arg1, arg2, record.draw_value, 0, 1, 1)
```

modern以typed record span表达216字节stride中的实际读取字段，并在原表读取点检查索引。三个参数保持full i32，不截断。

## 4. 固定填充与状态发布

绘制后，函数按顺序：

- 以`0xCF`填满128字节marker buffer。
- 把第四参数低16位写入input word。
- 把一个dword与一个word清0。
- 只把packed dword低16位写1，高16位保持。
- 从同一216字节记录读取`+0xB8/+0xAC/+0xB0`三个dword，并依次发布到three/first/return state owner。

原函数最终EAX为`+0xB0`字段。modern结果以bit-preserving i32返回同一值。

## 5. typed边界与验证

`LegacyStandardModeDialogSetupState`拥有marker与七个写入owner；`LegacyStandardModeDialogSetupRecord`只表达实际读取的四字段；port记录clear、interface和draw。

`special_modes.legacy_initial_menu`覆盖：

- clear→interface→draw严格顺序。
- `0x96000`与`0x2711`固定常量及interface source。
- draw的三个调用参数、record dword、固定0/1/1尾参数。
- 128字节全部写`CF`。
- input word、dword/word清零与packed dword仅低16位置1。
- `B8/AC/B0`字段到三个state owner的映射及B0返回。
- full i32绘制参数与高位返回bit保持。
- 越界索引时clear/interface已发生，draw和state写保持未发生。

定向测试通过。workpack连续生成两轮均为`27/227`，SHA256均为`1889fde199030e1d4b75d152cfd8a3628b9d388beedf57f292fa61b30cd9b3a9`；只新增关闭`0x0043BFC0`，`0x0043C090`仍为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
