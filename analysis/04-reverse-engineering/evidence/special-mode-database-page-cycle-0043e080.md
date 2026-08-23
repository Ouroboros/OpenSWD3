# 标准模式数据库页来源循环 `0x0043E080`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与直连

唯一行为真值为`swd3.exe.lst`。函数范围`0x0043E080..0x0043E16A`，109行；DA30有两个direct call点，B480另以callback地址绑定。直接callee F000/BCC0/F880/F1E0均已关闭；物品查询和sample保留平台边界。

DA30命中E080时直接调用本helper；F000现直接typed调用并在其内部保留F080/F0D0/D5D0最小边界，input ports继续继承BCC0 missing-node边界。shared-text buffer成为D530 database state的typed owner，MAPS payload由caller显式传入。

## 2. phase 1

交互phase1先对页选择执行32-bit减1，signed负值回绕为2。随后：

1. 直接调用已关闭F000重建forward head、count及window owner。
2. window/local清0，visible预置16。
3. 直接调用BCC0，重算完整count、current head、visible count、selected node及shared text。
4. BCC0 typed-stop时保留上述副作用，不调用后续边界。
5. 成功后依次F880、F1E0，最后sample `0x2E`返回EAX；E080不改display flags。

UT以三节点FFDC链锁定页0→2、count3、内建CP950 `B5 4C 00`、五步调用与flags保持；F000返回空且missing insertion不发布时，在BCC0原不安全点typed-stop，helper count2且不sample。

## 3. phase 2/3

phase2先读取toggle：非零先sample `0x107`。随后无条件查询物品`0x1BA9`，查询EAX覆盖sample EAX；缺物品返回0，runtime bit0置位返回1，两个gate均通过才清toggle。UT锁定sample→query顺序、缺物品仍保留toggle及成功清0。

phase3写countdown `0xC8`并返回EAX0；其他phase保留DEC链EAX。

## 4. DA30耦合修正

DA30上面板进入E080前已要求runtime bit0清；E080成功路径又会清toggle。因此旧的“同帧double-E080”导航假设不可达。真实双button路径为：外层物品查询→E080内部可选sample/再次物品查询→清toggle→DA30读取toggle0→E3D0。UT锁定toggle1时sample107一次、物品查询两次以及`E080→E3D0`；toggle0时无sample但同样进入E3D0。

## 5. 验证

定向测试通过。workpack双生成稳定为`51/227`，SHA256均为`0f52bb8ee251b986257b5e984793f79cdd66fc50d5f329adfb19a40f1d94e272`；下一单元为`0x0043E170`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
