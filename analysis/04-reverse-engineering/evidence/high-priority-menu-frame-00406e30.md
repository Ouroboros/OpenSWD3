# 高优先级菜单逐帧协调 `0x00406E30`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x00406E30..0x00406EA5`，共64行，无外部`FUNCTION CHUNK`。唯一caller为主帧`0x0040A570`。

入口直接读取和写入高优先级菜单共享状态；公共输入`0x00406EB0`、子模式0 `0x00406F70`、子模式1 `0x004070A0`及活动画面`0x00408CF0`分别由后续工作包或跨B11 owner关闭，当前以逐调用窄端口保留原重读和停止位置。

## 2. 计时和固定前缀

每帧先把delay按u32减1并立即发布。减后值按i32小于0，或按i32大于1000时写0；因此入口0下溢后归0，1001减后保留1000，1002减后1001再归0。

随后：

1. frame count按u32加1并允许回绕。
2. activity字面等于3时改写为1；其他值不变。
3. 软件鼠标frame固定写13。
4. 调用公共输入。

公共输入不可用时，以上全部副作用保留。

## 3. 子模式与活动尾

公共输入返回后重新读取submode：

- 0调用子模式0。
- 1调用子模式1。
- 其他值不调用子模式handler。

输入回调同帧改写submode会立即影响该分派。子模式返回后再次读取activity；值为0时以0返回，不调用活动画面。非零时尾调用活动画面并使用其返回值。

这两个重读不可用入口snapshot替代：测试锁定输入把9改为0后调用子模式0，以及子模式0把activity清零后跳过绘制。

## 4. typed-stop与验证

公共输入、选中的子模式或活动画面不可用时，只在对应原调用点停止；timer、frame count、activity 3→1、鼠标frame以及此前callee修改均不回滚。

`special_modes.legacy_initial_menu`覆盖delay边界0/1001/1002、frame count回绕、activity折返、输入改submode、子模式清activity、submode 2跳过handler、活动尾调用，以及三个callee停止前缀。
