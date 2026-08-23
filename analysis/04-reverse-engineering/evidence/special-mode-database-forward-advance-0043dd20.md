# 标准模式数据库向前推进 `0x0043DD20`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与caller

唯一行为真值为`swd3.exe.lst`。函数范围`0x0043DD20..0x0043DDE1`，94行；direct caller是已关闭DA30动态X矩形，B480另把地址写入callback表。直接callee B9A0/BB80/BC90/F880/F1E0均已关闭；sample保留平台边界。

DA30不再把DD20交给通用地址port：命中`address_0043DD20`时直接调用本typed helper。通用input ports继承DD20的三个最小外部边界，因此已关闭caller保留callback计数/EAX，同时DD20主体不被平台层吞掉。

## 2. phase 1

`FCD20==1`时严格按LST执行六步：

1. BB80以完整forward count、`FCAD0` window offset、`FCBA4` local selection和`FCB98` visible count推进cursor。
2. B9A0从共享`FCAE0` head前进window offset项，把结果发布到复用raw owner `FCD10`的typed current forward head。
3. BC90从current head最多计16项，重写visible count；其返回节点仅保留为typed审计值，原函数不发布额外裸global。
4. 直接调用已关闭F880 typed helper。
5. 以first/second inline `0xB0` records调用F1E0边界。
6. `FCAA4`只执行低字节`OR 0x30`，等价于完整u32 `|=0x30`；最后sample ID固定`0x2E`，返回其EAX。

D530此前扫描时把`FCD10`当计数器，F000后同一raw owner被forward head覆盖。typed state因此分别保留扫描审计计数和当前head，DD20只读取/重建后者。`FCAD0`与`FCAA4`分别明确命名为window offset和display flags，不再作为匿名reset。

UT建立18节点链，初始local15/visible16。BB80把window推进到1并保持local15；B9A0发布node1，BC90返回node17并保持visible16。F880先写first record字节，F1E0观察到该写入后再改两个record，证明顺序；最终flags `0xAB00→0xAB30`，sample `0x2E`返回77，六步计数为6。

## 3. phases 2、3与其他phase

`FCD20==2`时：

- toggle不等于1才先初始化sample `0x107`。
- 随后检查runtime input flags bit1；置位则不写toggle。
- bit1清时写toggle=1。

UT锁定sample发生在gate之前：toggle0且bit1置位时sample返回88，但toggle仍为0；toggle已为1时不调用sample，EAX保持两次DEC后的0。

`FCD20==3`只把`FCAB4`写`0xC8`，EAX为三次DEC后的0。其他phase不写owner并返回DEC链留下的EAX；UT锁定phase4返回1。

## 4. 验证

定向`special_modes.legacy_initial_menu`通过，覆盖phase1六步、链owner、records调用顺序、flags/sample EAX、phase2 sample/gate顺序、phase3 countdown及phase4返回。

workpack双生成稳定为`47/227`，SHA256均为`680834374fe3d6063b702c7443a86df5d9e143d2a3a16648e5353fd61be0b278`；下一单元为`0x0043DDF0`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
