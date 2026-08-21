# 剧情 VM 角色头像动作遣出 `0x0042A2C6`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原程序动态差分仍为`blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042A2C6..0x0042A33C`，共享尾`0x00429F61..0x00429F76`

opcode：`82`

## 1. 查找与写入合同

原版从`dword_4BA6E0`链头开始，按顺序寻找第一个同时满足下列条件的节点：

```text
node.action_id == zero_extend_u16(ip+2)
node.base_variant == zero_extend_u16(ip+4)
```

命中后只改该节点`horizontal_motion(+0x9A)`：

- 原motion bit15置位：写十进制10000（`0x2710`）；
- 否则先写-1；若signed `current_x(+0x98)>320`再覆盖为+1。

因此current X等于320、负数都写-1；重复key只改链表中第一个节点。未命中静默消费。所有完成路径推进6、common join发布normalized previous82并same-call继续。

## 2. 分阶段读取与unsafe推进

读取顺序不是整条记录预检：

1. 先读全局链头；空链直接跳共享+6尾，完全不读`+2/+4`；
2. 非空链才读`+2` action ID；
3. 只有遇到首个ID相同节点时才读`+4` variant，此后复用该值；
4. 所有ID都不同时从不读variant，仍+6；
5. ID命中但variant缺失才在现代typed边界返回`operand_out_of_range`。

测试锁定空链仅有opcode位于`0x7FFE`时仍推进到`0x8004`；非空ID miss仅有`+2`位于`0x7FFC`时推进到`0x8002`。这些是原始访问/推进顺序，不能用whole-record precheck改变。

固定全局list映射为nullable`runtime.role_head_actions`；owner缺失在原始全局head访问点返回`runtime_unavailable`。这是唯一平台适配。

## 3. alias、精确尾与资产锁

四raw alias `0052/4052/8052/C052`均归一为82。完整记录位于`0x7FFA`时先修改首匹配节点、推进到`0x8000`并发布previous82，下一fetch才失败。

线性TALK目录含1889条物理记录/1889 probes：

```text
TALK1.DAT 489
TALK2.DAT 340
TALK3.DAT 398
TALK4.DAT 662
```

全部raw `0x0052`、长度6。real CTest回放`TALK1.DAT@0x0000614D`（action `0x2711`、variant0），以current X=100的匹配节点验证写-1与精确尾合同。

## 4. 测试与分类

覆盖四alias、首匹配、第二重复节点不变、bit15→10000、signed X左右分支、same-call、空链不读operand、ID miss不读variant、variant截断、owner缺失和真实记录。剧情VM三项为3/3。

分类：`platform_adapted`。合法域遍历顺序、分阶段读取、motion位语义、+6、previous与same-call保持；固定全局owner改为typed nullable list。
