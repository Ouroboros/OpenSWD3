# 剧情 VM 角色等待覆盖 `0x00429F7B`

## 结论

opcode77/78共用入口。两者先读取`+2` selector，均把`0xFFF0`替换为当前source GUID并lookup；只有lookup成功后，opcode77才读取`+4` payload。77把`u16(payload)|0x8000`写入角色action的`wait_override`，成功长度6；78写零，成功长度4。两者随后清`wait_remaining`、调用`sub_4321E0`刷新，按对应固定长度推进，置`ESI=1`，经公共join发布normalized previous并same-call继续。

现有C++成功域字段正确，但错误地在lookup前预验77完整6字节，并漏发previous77/78；现已修正。

## 分阶段与字段宽度

- opcode/selector共4字节是共同第一阶段；
- selector命中后，77才读取payload word并以word OR保留原低15位、强制bit15；
- 78无payload，不读取`+4`；
- 两者把action `+0x4C`等待余量word清零并刷新；刷新失败原版只诊断，仍继续；
- 成功宽度由handler内`var_40=6/4`明确写定。

## missing selector原始异常

lookup失败进入诊断路径，但没有给局部`var_40`赋值；尾部仍用该陈旧栈dword同时推进物理指针与16位IP。因此失败宽度依赖此前栈内容，不能确定为4或6，也不能安全复刻。

现代在同一lookup unsafe点返回`role_not_found`，不推进IP、不发布previous。该typed适配还保留关键时点：opcode77 missing时即使`+4`不可读也先返回`role_not_found`；selector命中后`+4`缺失才返回`operand_out_of_range`。

## 精确尾

- 77完整记录位于`0x7FFA`：写覆盖值、清等待、刷新、IP=`0x8000`、previous77完成后，下一fetch失败；
- 78完整记录位于`0x7FFC`：同理完成清零、刷新、IP与previous78后下一fetch失败。

## 真实资产锁

### opcode77

- 442条物理记录、447个entry probes；
- TALK1/2/3/4分布`137/109/113/83`；
- 全部raw`0x004D`、长度6；
- 104种selector，无`0xFFF0/0xFFFE`；payload共10种，实际样本集中于0、1、2、3、4等小值。

### opcode78

- 4条物理记录、4个entry probes；
- TALK1/2分布`1/3`，TALK3/4为0；
- 全部raw`0x004E`、长度4；
- 3种selector，无`0xFFF0/0xFFFE`。

真实回放使用`TALK1.DAT@0x00004E24`（77，GUID250，payload3）与`TALK1.DAT@0x00042697`（78，GUID8），均在精确尾验证字段、刷新与previous。

## 测试覆盖

- ordinary 77/78 same-call继续到opcode14；
- 两opcode×四raw alias missing，且77 payload不可读；
- 77 selector命中后payload截断；
- 77 `FFF0`与payload`0xFFFF`精确尾；
- 78 `FFFE`受控角色精确尾；
- 两条TALK1真实记录；
- 剧情VM三项测试通过。

## 分类

分类：`platform_adapted`。成功域selector、字段宽度、刷新、固定IP、previous与same-call行为均与汇编一致；missing selector的陈旧栈宽度推进改为typed stop。
