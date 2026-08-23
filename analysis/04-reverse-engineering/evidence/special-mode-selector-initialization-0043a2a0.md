# 标准特殊模式双参数初始化 `0x0043A2A0`

状态：`platform_adapted`

## 1. LST锁与真实参数方向

权威范围为`swd3.exe.lst`的`0x0043A2A0..0x0043A378`。函数接收两个32位栈参数，本文严格按栈位置称为`primary`与`secondary`，不能把它们统一解释成resource/selector。

五个机器调用点全部位于`0x00439FD0`，覆盖六组逻辑参数：

- mode1/2普通分支：`(primary=0x1E, secondary=1)`。
- mode1/2 alternate分支：`(primary=0x24, secondary=2)`。
- mode3：`(primary=0, secondary=0xEA60)`。
- mode6与mode3共用一个机器调用点：`(primary=3, secondary=0xEA60)`。
- mode4：`(primary=1, secondary=0xEA60)`。
- mode5：`(primary=2, secondary=0xEA60)`。

这组方向由每个调用点的实际push顺序与callee栈槽共同确定。高模式不是`(0xEA60, 0..3)`。

## 2. 固定字段与有符号索引

函数在任何callee之前提交：

- `word_4FC900 = low16(secondary)`。
- `word_4FBECC = 5`。
- `word_4FB8A8`、`word_4FC200`和`word_4FBAB8`均为`low16(primary)`。
- `word_4FC3C4 = trunc_signed((primary - 0x1E) / 6) + 0x0B`的低16位。

机器使用`imul 0x2AAAAAAB`和符号修正实现除以6，语义是C/C++向零截断。实际派生值为：

- primary `0x1E`得到`11`。
- primary `0x24`得到`12`。
- primary `0`得到`6`。
- primary `1..3`得到`7`。

## 3. callee与输入同步顺序

字段提交后的调用及副作用顺序固定为：

1. `0x0043B480(low16(secondary))`绑定模式回调。
2. `0x0043A380(5)`建立可用项目状态。
3. 从`0x004B7CB0`开始以`rep stosd`清零`0x80`个dword，即严格`0x200`字节。
4. 清`dword_4FC320`。
5. `0x004239D0(6, 4, 3)`建立共享输入token。
6. 按`0x004AB998`、`0x004C9A28`、`0x004A9ED0`顺序，以`0x00435670`发布同一token。
7. 按相同owner顺序，以`0x00435660`发布sentinel `0xFFFE`。
8. EAX保留最后一次sentinel发布结果；五个直接调用者均不读取它。

`0x0043A380`现已独立关闭。`0x0043B480`、`0x004239D0`、`0x00435670`和`0x00435660`继续保持`pending_audit`，当前函数只锁定其参数、次数和顺序。

## 4. 平台接线

SDL低模式路径按`(setup_resource_id, setup_selector)`传入；mode3–6路径按`(0..3, 0xEA60)`传入。typed状态分别保存`secondary_word`和三份`primary_words`，避免再次用业务猜测交换ABI参数。

现代输入状态只公开20个已确认的`LegacyInputRecord`；端口清除全部公开记录，兼容核心及单测锁定原机器`0x200`字节清零合同。未关闭的回调和三owner容器callee不伪造业务结果。

## 5. 验证

`special_modes.legacy_initial_menu`覆盖：

- 高模式真实参数`(2, 0xEA60)`，派生索引`7`。
- 低模式真实参数`(0x24, 2)`，派生索引`12`。
- 两个参数各自的低16位截断。
- 负primary差值按有符号向零截断。
- callback、item、`0x200`字节清零、mode清零、token建立、三次token发布和三次sentinel发布的固定顺序。
- `0x004239D0`参数`6, 4, 3`、token复用、sentinel `0xFFFE`和机器尾返回传播。

参数方向修正后，Linux core `186/186`与Linux app `192/192`重新通过；按阶段门禁不重复执行Windows BUILD。当前workpack连续两轮生成均为`5/227`，SHA256均为`dfb6620cabbf4b2fad12886f501dc0e4c2d680babb9ac6fe3892e3e45b4d0c0f`。
