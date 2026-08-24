# 护驾属性对比面板 `0x00442130`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00442130..0x004425BB`，527行，无FUNCTION CHUNK；唯一caller为41680。

## 三组固定属性

当前记录基址为attribute cache `0x50*party_index`，参考记录固定基址`0x140`。入口先生成颜色`(0x19,0x17,0x11)`，三组按原顺序处理：

1. `dword[0x18]+dword[0x3C]`，行Y=`0x152`，独立值X=`0x17C`。
2. `dword[0x1C]+dword[0x40]`，行Y=`0x166`，独立值X=`0x1E0`。
3. `dword[0x24]`，行Y=`0x17A`，独立值X=`panel_offset+0x244`。

加减全部以u32回绕后按i32格式化。每组先发布`"%-6s%5d"`标签值，再以`"%d"`发布独立值；坐标保持`panel_offset+0x1C8`及`0x3E-panel_offset`。

当前减参考非零时，正差选资源`0x2465`，负差选`0x2463`；解析后把source word、u16宽高用于`panel_offset+0x22E,row`图标，再以`"%-5d"`在`panel_offset+0x23E,row`显示参考值。资源不可用只在原resource pointer解引用点typed-stop，保留此前文本。

## slot尾行

- slot0读取参考`+0x44` signed dword；`-1`返回cache owner token，否则格式`"%-6s%5d%%"`。
- slot7/8读取参考`+0x48` packed dword；`FFFFFFFF`直接返回-1，否则low16/high16按`"%-6s%5d/%-5d"`显示。
- slot9/10读取参考`+0x4C`；`FFFFFFFF`返回-1，否则先做u32乘5回绕，再按signed `"%-6s%5d%%"`显示。
- 其他slot保留cache owner token返回适配；原裸指针在modern state中以allocation token表达。

六类静态标签、icon解析和文本/icon draw由既有GuardianRenderPorts承载；41680原`refresh_attribute_cache` opaque request已回收为直接helper，聚合状态、颜色及operation计数并传播typed-stop。

UT覆盖三组offset与回绕、正/负/零差、2465/2463、图标坐标尺寸、slot0/7/9格式、sentinel返回、资源缺失和party cache越界，以及41680嵌套序列。定向测试通过。

workpack双生成稳定为`86/227`，SHA256均为`32e96a68100419940e3d32ad8359a857208ffffc25bc3cbbfa9a50dadb625ebe`；下一单元`0x004425C0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
