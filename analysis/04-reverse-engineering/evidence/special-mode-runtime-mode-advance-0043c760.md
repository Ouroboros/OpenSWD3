# 标准模式运行时模式推进组合器 `0x0043C760`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与调用点

唯一行为真值为`swd3.exe.lst`。函数物理范围为`0x0043C760..0x0043C7D0`，无外置chunk，下一workpack入口为`0x0043C820`。

LST有两个运行时callsite、两个caller：`0x0043C3C0`外置chunk在`0x0043C7E0`调用一次，`0x00446550`在`0x0044655C` tail-jump一次。函数地址另由`0x0043B480`保存；workpack caller字段仅作导航。

## 2. 模式推进与固定顺序

函数先对mode index执行32位回绕`+1`，再做signed `> 11`判断；大于11才钳为11。因此：

- 10推进到11。
- 11推进到12后钳11。
- `INT_MAX`回绕到`INT_MIN`，signed不大于11，不钳制。

随后严格：

1. 以实时mode index直接调用已关闭`0x0043C9C0`；其按固定mode→classification映射重建entry/text/status，并清window/cursor/alias后刷新page。
2. 用window offset、原entry base与entry alias owner调用`0x0043CC00`。
3. 调用`0x0043CBD0`刷新page。
4. u32回绕相加offset与cursor并读取entry，调用`0x0043CEF0`。
5. 播放sample `0x2E`并返回sample EAX。

本函数不改mode flags。entry初始化、alias重建与page刷新已分别直接复用`0x0043C9C0`、`0x0043CC00`、`0x0043CBD0`typed helper；未关闭的classification/status数据库、record load/release、consume/sample继续由共享typed port隔离。

## 3. typed边界与caller回接

`advance_legacy_standard_mode_runtime_mode`先传播C9C0的typed-stop。C9C0正常返回后window/cursor已清0，随后执行alias重建和第二次CBD0；CBD0 alias读取停止时不进入selected。CBD0完成后才在原selected entry读取点保留64项检查；任一停止均不伪造后续entry消费或sample。

这纠正了此前synthetic entry port允许任意mode并保留旧window的测试假设：`INT_MAX→INT_MIN`仍由本函数回绕产生，但紧接着在C9C0 mode映射读取点停止；旧window64也会被合法C9C0清0，后续消费真实entry0。

`0x0043C3C0`第二矩形caller已真实回接。原caller先保留其特殊delta规则，再调用本函数；本函数内部播放一次sample，返回后`0x0043C7E0`无条件再播放一次sample。因此刷新路径有两次相同`0x2E`/handle调用，最终EAX来自第二次。

例如初始mode5：

- 负delta先按原BUG减2到3，本函数再推进到4。
- 正delta先保持5，本函数再推进到6。

mode0负边界、mode14正边界及delta0在caller早退，不调用本函数。

## 4. 验证

`special_modes.legacy_initial_menu`覆盖：

- mode 10→11、11→11钳制并映射classification14；`INT_MAX→INT_MIN`回绕后传播C9C0 mode-map typed-stop。
- C9C0第一次refresh、alias重建、第二次refresh、真实entry0消费和sample顺序。
- 旧offset64/cursor7由C9C0清0，后续消费entry1，不再伪造selected越界。
- `0x0043C3C0`负/正delta最终mode4/6、C9C0双扫描与第一次refresh、真实entry1消费和两次sample事件/参数。
- caller的mode0/14/delta0早退仍无port副作用。

定向测试通过。workpack连续生成两轮均为`34/227`，SHA256均为`d2053fd736fee3f30bf0b0edab19ff0f41a812ba397b7a42a91060cad38e20b7`；只新增关闭`0x0043C760`，`0x0043C820`仍为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
