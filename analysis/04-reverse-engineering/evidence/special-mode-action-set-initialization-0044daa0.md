# 初始化九个特殊模式动作记录 `0x0044DAA0`

状态：`assembly_exact`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0044DAA0..0x0044DBB1`，102行，无外部FUNCTION CHUNK。caller为尚待闭环的`0x0044D920`和`0x0044EA60`；唯一callee `0x0040DC00`已由共享`initialize_legacy_action_record`等价实现。

函数严格按九个固定owner顺序调用共享动作初始化。该callee只写：

- `+0x1C/+0x20/+0x3C = 0xFFFFFFFF`
- `+0x42/+0x44/+0x46/+0x48 = 0`
- `+0x90 = 0`

它不清零完整152字节记录，其他字段必须保持入口值。

## 2. 后续覆写

共享初始化全部完成后：

- 记录0..3：`base_variant=5`、`variant_delta=0`；`action_id`依次为`1,2,8,17`。
- 记录8：`action_id=0x232A`、`base_variant=22`、`variant_delta=0`。
- 记录7：`action_id=0x232A`、`base_variant=50`、`variant_delta=0`。
- 记录5：`action_id=0x232A`、`base_variant=28`、`variant_delta=0`。
- 记录6：`action_id=0x232A`、`base_variant=29`、`variant_delta=0`。
- 记录4除共享初始化字段外不再覆写，原动作ID、基础变体、变体增量均保留。

覆写顺序保持LST的`0..3,8,7,5,6`。

## 3. 验证

UT为九条记录写入不同入口哨兵，验证九次共享初始化、未触及字段保持、四条状态动作和四条`0x232A`变体，并验证记录4的动作键完整保留。

workpack双生成稳定为`193/227`，SHA256为`5707bece268a213870c10ed8595588ded0aaec2b20d7ffb41ddaef136d9bc30a`。由于其caller `0x0044D920`尚未关闭，下一单元仍为`0x0044D920`；关闭后必须删除该caller的旧初始化边界并直接调用本helper。
