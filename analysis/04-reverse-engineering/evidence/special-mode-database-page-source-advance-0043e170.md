# 标准模式数据库页来源正向循环 `0x0043E170`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与边界

唯一行为真值为`swd3.exe.lst`。函数范围`0x0043E170..0x0043E243`，105行；DA30有两个direct call点，B480另以callback地址绑定。直接callee F000/BCC0/F880/F1E0均已关闭；sample保留平台边界。

E170直接复用E080已建立的database cycle ports：F000现直接typed调用并在内部保留F080/F0D0/D5D0边界，BCC0继续保留missing-node边界、MAPS payload和state-owned shared-text buffer。DA30命中E170时直接调用typed helper，不再走通用地址port。

## 2. phase 1

交互phase1对页选择执行32-bit加1，signed比较大于2时写0；负值加1后若仍小于等于2则原样保留。随后严格执行：

1. 直接调用已关闭F000重建forward head、count及window owner。
2. window/local清0，visible预置16。
3. 直接调用BCC0，发布完整count、current head、visible count、selected node及shared text。
4. BCC0 typed-stop时保留此前副作用，不调用后续边界。
5. 成功后依次F880、F1E0，最后sample `0x2E`返回EAX；display flags保持不变。

UT以三节点FFDC链锁定页2→0、count3、内建CP950 `B5 4C 00`、五步调用及flags保持；F000返回空且missing insertion不发布时，在BCC0原不安全点typed-stop，helper count2且不sample。

## 3. phase 2/3与EAX

phase2先读取toggle到EAX。toggle等于1时跳过sample并保留EAX1；其他值调用sample `0x107`，sample返回覆盖EAX。runtime input bit1置位时保持toggle，否则写toggle=1；写全局不改变EAX。UT覆盖非1 sample后写1、bit1 gate保持原值，以及toggle1不sample。

phase3写countdown `0xC8`并返回EAX0；其他phase保留DEC链EAX。

## 4. DA30耦合修正

DA30 lower-panel进入E170前已要求runtime bit1清。因此E170成功后必写toggle1，第二个button真实进入E3D0；旧的double-E170高层假设不可达。UT锁定toggle1时无sample，toggle2时sample107一次，两者均为`E170→E3D0`。

## 5. 验证

定向测试通过。workpack双生成稳定为`52/227`，SHA256均为`78d3553aa39a50f6bdeaa493e65d0d6f1b3edcc89f56accb7ca4236e46d11185`；下一单元为`0x0043E250`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
