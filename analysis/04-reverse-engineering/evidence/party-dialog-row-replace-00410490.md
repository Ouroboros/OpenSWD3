# 角色/道具对话旧行替换 `0x00410490`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x00410490..0x004105F6`，共175行，无外部`FUNCTION CHUNK`。三个caller都在对话页填充`0x00410730`。

参数中的row、名称、数量、编号、附加值和分母与`0x00410600`相同。唯一新增行为是在写cell前发送`0x1007`删除指定旧行。

## 2. 严格资源与消息顺序

机器顺序固定为：

1. 申请64字节scratch。
2. 删除指定row。
3. 复制名称。
4. 按column0..3写四个cell。
5. 释放scratch。
6. 返回1。

申请发生在删除之前。不能为避免暂时资源而先删除，也不能在删除失败后释放scratch，因为原后续清理未执行。

## 3. 直接复用四列规则

现代把`0x00410600`拆成“申请scratch”和“已申请后的四列核心”。普通填充先申请再调用核心；本入口先申请、删除，再直接调用同一核心。因此以下异常保持完全相同：

- denominator 0的百分号被十进制覆盖。
- denominator 1复用编号文本。
- denominator大于1显示比值，小于1显示十进制。
- added value `-1`为空串。
- 名称64字节边界和逐cell停止前缀。

没有复制第二套格式化规则。

## 4. typed-stop与验证

scratch申请、删除消息、名称复制或任一cell不可用时，只保留此前副作用，不执行后续释放。完整成功才释放一次并返回1。

`special_modes.legacy_initial_menu`覆盖正常事件`allocate→delete→四cell→release`、denominator1残值，以及申请失败、删除失败和第二cell不可用前缀。
