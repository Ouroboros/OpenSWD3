# 标准模式数据库反向推进 `0x0043DDF0`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与caller

唯一行为真值为`swd3.exe.lst`。函数范围`0x0043DDF0..0x0043DECA`，105行；DA30有两个direct call点，B480另以callback地址绑定。直接callee为已关闭B9A0/BBC0/BC90、尚未关闭F880/F1E0、物品查询及sample owner。

DA30命中`address_0043DDF0`时现直接调用本helper；通用地址port不再承载DDF0。input ports继承retreat专用边界，保留物品查询和DD20/DDF0共用的F880、F1E0、sample三个最小边界。

## 2. phase 1

`FCD20==1`时按LST执行：

1. BBC0递减`FCBA4` local selection；负值时钳0，并仅在`FCAD0` window offset正数时递减window。
2. B9A0按window offset从共享`FCAE0` head重建`FCD10` current head。
3. BC90从current head最多计16项并重写`FCB98` visible count。
4. F880边界。
5. first/second inline `0xB0` records进入F1E0边界。
6. display flags低字节`OR 0x03`，最后sample `0x2E`返回EAX。

UT建立18节点链，初始window1/local0；BBC0得到window0/local0，B9A0回到node0，BC90返回node16。F880写first record后F1E0观察并覆盖，证明顺序；flags `0xAB00→0xAB03`，sample返回79，六步计数为6。

## 3. phase 2

`FCD20==2`先查询物品`0x1BA9`：

- 不存在时立即返回查询EAX 0，不读写toggle。
- 存在后检查runtime input flags bit0；置位时立即返回查询EAX 1，不读写toggle。
- gate清时读取toggle。toggle非零才sample `0x107`，随后无条件清toggle；toggle为0时不sample且EAX保持0。

UT锁定物品查询ID、query→gate→toggle→sample→清零顺序：bit0置位时toggle1保持且EAX1；gate清时sample返回89再清0；物品缺失时toggle7保持。

## 4. phase 3与其他phase

`FCD20==3`写`FCAB4=0xC8`，返回三次DEC后的EAX0。其他phase不写owner，保留DEC链EAX；UT锁定phase4返回1。

## 5. DA30真实重读路径

闭环DDF0和DD20均不修改鼠标X，因此移除早期伪port通过闭环callee强改X的测试。DA30现以真实行为分别锁定：

- `x=80`先直接DDF0，X仍80；第一动态边界调用未关闭DFA0后把测试X改205，再命中DED0，共3次callback。
- `x=460`先直接DD20，X仍460；随后DFA0和DED0，共3次callback。

这既验证caller直连，也保留DA30每次callee后重读X，而不向已关闭callee注入不存在的副作用。

## 6. 验证

定向`special_modes.legacy_initial_menu`通过。workpack双生成稳定为`48/227`，SHA256均为`e73871d107cef775a9cf4178ae9ec763039546f1c8d8b8d15c73f3caeda7f966`；下一单元为`0x0043DED0`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
