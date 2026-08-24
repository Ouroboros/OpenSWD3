# 特殊模式回调绑定器 `0x0043B480`

状态：`platform_adapted`

## 1. LST锁与既有机器清单

权威范围为`swd3.exe.lst`的`0x0043B480..0x0043B977`。函数共有9个调用者。既有生成器已从完整LST锁定：

- 9组配置。
- 13个主回调槽。
- 107次实际槽写入。
- 92种目标表达式。

逐赋值、逐槽和二级表清单分别为：

- `special-mode-callback-targets.tsv`。
- `special-mode-callback-slots.tsv`。
- `special-mode-secondary-dispatch.tsv`。

现代实现以这些锁定地址作为legacy callback ID；目标地址出现于表中不等于目标函数已实现，目标入口继续按workpack独立关闭。

## 2. 九组选择条件

参数按`(secondary_word, primary_word)`解释：

- G01：secondary `2`，primary `0x1E..0x20`。
- G02：secondary `2`，primary `0x24..0x29`。
- G03：secondary `2`，primary `0x2A..0x2E`。
- G04：secondary `2`，primary `0x30..0x34`。
- G05：secondary `2`，primary `0x36..0x3A`且flag49为0；或primary `0x3C..0x3E`且flag49非零。
- G06：secondary `2`，primary `0x36..0x3A`且flag49非零；或primary `0x3C..0x3E`且flag49为0。
- G07：secondary `2`，primary `0x42..0x47`。
- G08：secondary `1`。
- G09：secondary `0xEA60`。

flag49分支使用零/非零，而不是字面等于1。只有G05/G06范围查询一次flag；其他组不查询。

无匹配条件时函数直接返回，不写任何槽，也不调用helper。

## 3. helper顺序

G01..G07直接写主槽并返回。

G08在主槽写入前直接调用已关闭的`0x00444FC0`安装三张7槽分派表并查询flag49。G09在主槽写入前调用`0x00448700`初始化高模式运行时；仅该callee继续保持typed端口。

## 4. 必须保留的旧值

13槽顺序与`0x0043A470/0x0043A610`消费边界一致。

- 动态slot0 `0x004FB808`只有G08/G09写入；G01..G07必须保留旧值。
- slot4 `0x004FBF7C`在G04/G05/G07缺失写入；这三组必须保留旧值。
- G09明确把动态slot0写为0；这是清除，不是“缺失写入”。
- 其他槽按TSV矩阵覆盖。

现代矩阵以专用preserve sentinel编码缺失写入，应用时跳过对应槽，不能先清空整个表再覆盖。

## 5. typed owner与验证

`LegacyStandardModeCallbackState`保存13个legacy目标ID。`0x0043A2A0`现调用真实绑定器；输入分派的动态前置门直接读取slot0是否为0。具体目标调用仍由后续目标owner接通。

`special_modes.legacy_initial_menu`覆盖：

- G01..G09全部组条件、write count和helper count。
- 13槽完整矩阵的逐字节FNV哈希。
- G04/G05/G07保留slot4。
- G01..G07保留动态slot0。
- G09显式清动态slot0。
- `0x36..0x3A`与`0x3C..0x3E`两段相反的flag49选择。
- G08/G09 helper先行事件。
- 无匹配selector零写入。

Linux完整门通过后，workpack只新增关闭`0x0043B480`。
