# 剧情 VM 角色头顶标记 `0x00429CBC`

## 结论

`sub_427920` 的 opcode71 固定长6字节。它原样读取`+2` selector，不替换`0xFFF0`；lookup命中时才读取`+4` slot，并把角色`field_3c`写成：

```text
0x004B9F68 + zero_extend_u16(slot) * 0x98
```

lookup缺失时不读取slot、不写角色，但仍按6字节消费。命中与缺失两路都保持`ESI=0`，共同出口发布normalized previous71并yield。

现有C++的token算术正确，但错误地预验完整6字节、同调用continue且漏发previous；现已按LST修正。

## 分阶段读取

1. 先要求opcode与selector共4字节可读；
2. 调用`sub_40C0D0(raw selector)`；
3. lookup缺失：直接推进6并yield，`+4`可不可读都不影响；
4. lookup命中：此时才要求slot word可读并写field。

因此在`0x7FFC`只有opcode+selector时：literal`0xFFF0`缺失可把IP推进到`0x8002`并yield；命中角色则在slot读取点typed-stop，IP/previous不变。

## selector与slot

opcode71不替换`0xFFF0`，但共享lookup仍支持`0xFFFE`受控角色。slot先零扩展，再按`0x98`乘法；原版没有0..3范围检查。现代`legacy_world_head_sign_action_token`保留u16零扩展和legacy 32-bit token，不把slot钳制到真实HeadSgn数组。

## 流控与边界

命中路径在写field后推进6；缺失路径也推进6。两路均发布previous71并yield，不读取下一opcode。

完整命中记录位于`0x7FFA`时，token写入、IP=`0x8000`与previous71完成后直接yield。该yield使真实story100中每条71成为独立帧边界；长链测试已显式跨过71及其后可能紧随的67等待，不再依赖旧错误的same-call行为。

## 真实资产锁

- 331条物理记录、338个entry probes；
- TALK1/2/3/4分布`127/22/138/44`；
- 全部raw`0x0047`、长度6；
- 54种selector，无`0xFFF0/0xFFFE`；
- slot仅0/1/2/3，分布`193/113/6/19`；
- 原始offset、word、长度逐条核验零错误。

真实回放使用`TALK1.DAT@0x00004B9D`：selector GUID191、slot0；写入base token，推进6、发布previous71并yield。

## 测试覆盖

- 四raw alias与slot`0/1/3/0xFFFF`无钳制；
- literal`0xFFF0`缺失且slot不可读仍消费；
- 命中角色slot截断typed-stop；
- `0xFFFE`受控角色；
- `0x7FFA`完整精确尾；
- opcode72既有覆盖保持独立；
- TALK1真实记录与story100全部71帧边界；
- 剧情VM三项测试通过。

## 分类

分类：`platform_adapted`。原版裸HeadSgn地址以legacy u32 token表示；有效域selector、slot算术、分阶段读取、IP、previous与yield保持汇编语义。
