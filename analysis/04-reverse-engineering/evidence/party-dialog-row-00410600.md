# 角色/道具对话四列行填充 `0x00410600`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x00410600..0x0041072D`，共151行，无外部`FUNCTION CHUNK`。唯一caller为共享Windows对话过程。

原函数接收列表句柄、row、名称、数量、编号、附加值和附加值分母。先申请64字节scratch并用`lstrcpyA`复制名称，再依次发送四次`0x102E`列表项消息。现代以窄scratch/cell端口隔离CRT与HWND边界。

## 2. 固定前三列

消息顺序不可改变：

1. column0：名称原字节串。
2. column1：数量按`%d`。
3. column2：编号按`%d`。
4. column3：附加值规则。

每次消息都使用同一row。只有第四次消息成功后才释放scratch并固定返回1。

## 3. 第四列残值合同

第四列严格保留原控制流异常：

- added value字面等于`-1`：空串。
- denominator等于0：先格式化`"%d%%"`，随后无条件再以`"%d"`覆盖，最终只显示十进制；结果记录该死格式发生，但不得显示百分号。
- denominator大于1：`"%d/%d"`。
- denominator小于1：十进制added value。
- denominator等于1：不执行任何新格式化，直接复用column2残留的编号文本。

不能把分母1现代化为`value/1`，也不能保留分母0瞬时百分号字符串。

## 4. typed-stop

- 64字节scratch申请失败：无cell更新。
- 名称含NUL终止需超过64字节时，在原`lstrcpyA`目标写点停止；scratch已申请但不执行后续释放。
- cell端口不可用：保留之前已更新cell，不释放scratch。

完整成功才释放一次scratch；这些停止前缀不伪造清理。

## 5. 验证

`special_modes.legacy_initial_menu`覆盖四列顺序、负数量、denominator 0/1/2/-2、added `-1`、64字节边界、scratch失败和第三cell不可用前缀。
