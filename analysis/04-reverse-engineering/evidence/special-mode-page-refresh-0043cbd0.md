# 标准模式page可见项刷新 `0x0043CBD0`

状态：`assembly_exact`、`unit_tested`

## 1. LST范围与caller

唯一行为真值为`swd3.exe.lst`。函数物理范围为`0x0043CBD0..0x0043CBF6`，24行，无callee、无自身外置chunk。七个direct caller为：

- `0x0043C520`：`0x0043C552`。
- `0x0043C590`：`0x0043C5C2`。
- `0x0043C3C0`外置chunk `0x0043C600`：`0x0043C632`。
- `0x0043C670`：`0x0043C6A2`。
- `0x00446420`外置chunk `0x0043C6E0`：`0x0043C720`。
- `0x0043C760`：`0x0043C79E`。
- `0x0043C9C0`：`0x0043CBBA`。

其中已关闭的C520/C590/C3C0/C670/C760/C9C0全部直接复用typed helper；未关闭`0x00446420`及其chunk留待该owner独立审计。

## 2. 精确控制流与EAX

函数读取active entry alias指针到EAX，随后无条件把visible count `FC914`清0，并立即读取alias首项。

- 首项0：直接返回首项指针。
- 首项非零：每轮先比较signed visible count是否大于等于15；达到15时返回当前指针。
- 未达到15：visible count加1、entry指针加4、发布visible count，再读取下一项；下一项0时返回该终止项指针，否则继续。

因此：

- N个连续非零entry后接0，N小于15时，visible=N，返回终止0的指针。
- 至少16项非零时，会读到index15，visible=15，然后因count cap返回index15指针；不会读取index16。
- 递增路径先发布visible，再裸读下一entry。这一顺序对alias63非零尤为重要：visible先变1，随后读取entry64越界。

modern结果使用`const u32*`表达原32位EAX entry指针，不把宿主64位指针截成i32。

## 3. typed-stop

runtime state只有64项entry和typed alias index：

- 负alias或大于63，在首项读取点typed-stop；visible已清0。
- alias63且entry63非零，在下一项读取点typed-stop；visible已写1。
- alias63且entry63为0正常返回entry63指针。

停止前的visible写入保持；不执行原程序的越界读取。

## 4. caller回接与既有假设纠正

原`refresh_page`port已从共享接口和全部测试fake删除。C520/C590/C670/C760及C3C0 page-advance在alias重建后直接调用本helper，并在CBD0 typed-stop时停止后续selected读取、consume、flags和sample。C9C0 tail直接返回本helper的entry指针。

真实CBD0会改写visible count，这纠正了此前窄port保持旧visible的synthetic行为。C3C0重叠upper→dynamic→page测试中：

1. upper后alias0只见entry0非零，visible变1。
2. dynamic page-retreat再次得到visible1。
3. page-advance据实时visible1把offset推进15、cursor保持0；alias15首项0，visible变0，最终消费entry15的0。

旧期望offset0/cursor14/entry14来自未实现CBD0时保留旧visible15，不符合LST，已删除。

## 5. 验证

`special_modes.legacy_initial_menu`覆盖：

- alias2的两个非零项加终止0：visible2、返回entry4指针。
- entry0..15均非零：visible15、返回entry15指针。
- 负alias：visible先清0再停止。
- alias63非零：visible先写1再在next读取点停止。
- alias63为0：正常返回末项指针。
- C9C0三entry tail：visible3、返回terminator entry3指针。
- C670 alias64停止传播，不执行selected/consume/sample。
- C3C0重叠路径按实时visible产生offset15/cursor0/entry15。

定向测试通过。workpack连续生成两轮均为`37/227`，SHA256均为`45af9d23d04ae8fe96324b1a3d52bf5c8610b0ecb97619feaea414d8c9832f3e`；只新增关闭`0x0043CBD0`，`0x0043CC00`仍为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
