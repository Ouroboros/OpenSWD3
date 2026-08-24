# 护驾系统初始化 `0x00440630`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。完整函数范围`0x00440630..0x00440742`，110行；444FC0把本函数地址写入初始化callback slot，函数体无直接call caller。callee为B9E0、442050、4429B0及三次487C10 allocation。

业务术语由同函数簇原版CP950文本确认：`4A6F24`为“護駕”，442130详情标签包括“攻擊、防禦、敏捷、熟練、成長、辨識”，元素标题为“火、冰、風、土、毒”。因此本簇称为护驾记录系统，不再使用无意义“数据库”。

## 严格顺序

1. 清零0xB0 scratch record；发布alternate record list owner。
2. 仅当party selector low16等于5时把low16清0，高16保持不变。
3. 复制interface source owner，依次allocation两块0x38 storage；本函数不清这两块malloc内容。
4. 清primary/secondary accumulator、two totals、selection、two selection values及record head，再调用442050列表准备边界。callee可改selection/head，返回后必须重读。
5. 清action scratch与panel offset，allocation第三块0x190 storage；原函数立即对返回地址清400字节。typed端仅在该精确memset点对token0停止；成功时清零后调用4429B0属性cache准备边界。
6. 以调用后`party_selector.low16*16 + selection`读取7×16护驾record表，取record `+4`文本ID并直接调用已关闭B9E0发布shared text。表越界和B9E0失败均在原读取/发布点typed-stop。
7. 成功后才清render/双scroll owner，写viewport extent480、previous selection -1、panel X488、Y120。EAX为B9E0 formatter返回值。

新增`LegacyStandardModeGuardianInitializationState`、最小allocation/list/cache ports及`initialize_legacy_standard_mode_guardian_system`。UT覆盖party low16重置且保留high16、三次allocation与两个callee顺序、前两块不清/第三块先清、FFDC文本、最终常量、第三allocation失败、record表越界及文本typed-stop。

定向测试通过。workpack双生成稳定为`71/227`，SHA256均为`35961a02fac7661d9a8022bd470d5a74ede7c8f3b96d7843baf0b088cf1c64b6`；下一单元`0x004407F0`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
