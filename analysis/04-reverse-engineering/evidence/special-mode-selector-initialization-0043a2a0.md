# 标准特殊模式选择器初始化 `0x0043A2A0`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0043A2A0..0x0043A378`。五个机器调用点全部位于标准特殊模式总入口`0x00439FD0`，覆盖六种逻辑参数：

- mode1/2普通分支传`resource=0x1E, selector=1`。
- mode1/2 alternate分支传`resource=0x24, selector=2`。
- mode3传`resource=0xEA60, selector=0`。
- mode6与mode3共用一个机器调用点，传`resource=0xEA60, selector=3`。
- mode4传`resource=0xEA60, selector=1`。
- mode5传`resource=0xEA60, selector=2`。

函数接收两个32位栈参数，但选择器和资源字段均只保存低16位。

## 2. 固定字段与有符号索引

函数在任何callee之前提交：

- `word_4FC900 = low16(selector)`。
- `word_4FBECC = 5`。
- `word_4FB8A8`、`word_4FC200`和`word_4FBAB8`均为`low16(resource)`。
- `word_4FC3C4 = trunc_signed((resource - 0x1E) / 6) + 0x0B`的低16位。

机器使用`imul 0x2AAAAAAB`和符号修正实现除以6，语义是C/C++向零截断，不能改成向负无穷取整。对实际资源`0xEA60`，派生索引为`0x2716`。

## 3. callee与输入同步顺序

字段提交后的调用及副作用顺序固定为：

1. `0x0043B480(low16(selector))`绑定模式回调。
2. `0x0043A380(5)`建立可用项目状态。
3. 从`0x004B7CB0`开始以`rep stosd`清零`0x80`个dword，即严格`0x200`字节。
4. 清`dword_4FC320`。
5. `0x004239D0(6, 4, 3)`建立共享输入token。
6. 按`0x004AB998`、`0x004C9A28`、`0x004A9ED0`顺序，以`0x00435670`发布同一token。
7. 按相同owner顺序，以`0x00435660`发布sentinel `0xFFFE`。
8. EAX保留最后一次sentinel发布结果；五个直接调用者均不读取它。

`0x0043B480`、`0x0043A380`、`0x004239D0`、`0x00435670`和`0x00435660`均仍需独立LST闭环。当前函数通过窄端口保持它们的参数、调用次数和顺序，不把callee内部效果计入本入口。

## 4. 平台接线

SDL的`LegacyStandardSpecialModePorts::initialize_mode_selector()`以及低模式初始化内保存的selector请求现在均进入typed selector owner，因此五个机器调用点对应的六种逻辑参数均接通。现代输入状态只公开20个已确认的`LegacyInputRecord`；端口清除全部公开记录，兼容核心及单测仍锁定原机器`0x200`字节清零合同。未关闭的回调、项目和三owner容器callee在SDL不伪造业务结果。

## 5. 验证

`special_modes.legacy_initial_menu`覆盖：

- 选择器与资源低16位截断。
- 实际`0xEA60`资源派生`0x2716`。
- 负资源差值按有符号向零截断。
- callback、item、`0x200`字节清零、mode清零、token建立、三次token发布和三次sentinel发布的固定顺序。
- `0x004239D0`参数`6, 4, 3`、token复用、sentinel `0xFFFE`、调用计数和机器尾返回传播。

Linux core `186/186`、Linux app `192/192`和Windows LLVM app `192/192`通过。workpack连续两轮生成均为`3/227`，SHA256均为`bb501b6c069983b834a14030ac584b96b326f43ea2a5f233f8b2b9a65263b001`，只新增关闭`0x0043A2A0`；所有直接callee继续保持`pending_audit`。
