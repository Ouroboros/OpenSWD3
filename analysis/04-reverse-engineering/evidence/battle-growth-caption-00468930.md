# 战斗成长标题框 `0x00468930`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00468930..0x00468AC5`，从proc到endp共173行、117条带机器码和真实助记符的实际指令、10个静态call、2个跳转、1个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。唯一静态caller是已关闭消息阶段分派的消息111；原caller固定调用本函数，返回后重新读取timer并按u32加一。

10个callsite包括`wsprintfA` 2次、`lstrlenA` 2次、动作记录更新1次、固定矩形1次、九宫格1次、stage推进1次和文字绘制2次。stage推进、长度、动作、矩形和九宫格使用确定性typed实现；格式和文字通过窄typed端口保留。相邻成长完成标题框复用该业务核心，但通过独立variant保留sample门和不同的generic调用寄存器布局。

## 2. 入口模式门和角色名称

入口按原指令把64-byte局部缓冲首byte写`0xFF`，保留未写的byte 1–3，再从byte 4开始清零15个dword，并留下EAX/ECX为零。只有transition mode精确等于1才继续；其他值保持入口EDX并直接返回。

继续路径按i8符号扩展live actor，再首次访问十项动作标签。与上一成长对照面板不同，本函数没有actor `0xFF`早退；因此mode为1且actor为FF时在负一标签访问typed-stop。标签合法时名称token为`0x0049E148 + label*16`，以连续格式`%s`写入局部缓冲。缺省typed数据复现原表前八项的四个CP950名称循环，后两项为空；端口仍可替换真实格式结果。64-byte缓冲最多容纳63个非NUL byte，第64个byte在格式副作用后、第一次`lstrlenA`前typed-stop。

第一次`lstrlenA`返回signed int长度；原`cdq/sub/sar 1`按有符号向零修正后除二。合法C字符串长度非负，因此得到`floor(name_length/2)`，但modern仍保留原算术链和EAX/EDX结果。

## 3. 动态宽度底板

名称长度确定后，函数复用胜利结算唯一动作记录，只写动作`0x233B`和variant 0并更新。设`half=floor(name_length/2)`：

- 固定矩形参数为x=246、y=176、width=`half*20+8`、height=`transition_stage+8`、颜色`0,4,4`、mode 0；
- 九宫格为left=250、top=180、right=`half*20+250`、bottom=`transition_stage+180`、flags `0x80000008`；
- 九宫格资源低word读取动作记录`+0x4A`，高word保留矩形返回EDX。

矩形或九宫格typed-stop都保留此前名称格式、长度和动作更新。九宫格之后直连`base=180,target=236,divisor=3`的共享stage推进，signed商非零、返回EAX不等于1就返回，保留底板但不绘两行文字。

## 4. 名称与共享标题

stage商零后，第一行把局部名称在`260,188`以颜色`0xFFFF`、字号16绘制。随后函数以16次dword写把整个64-byte局部缓冲清零，区别于入口的首byte `0xFF`初始化。

第二次`wsprintfA`使用连续格式`[%s]`和共享24-byte标题owner。modern在24 byte内寻找NUL；找到时严格输出左方括号、原byte、右方括号并补NUL。若24 byte均非零，格式过程已写左括号和24个源byte后，在首次读取物理下一byte时typed-stop；名称行和部分格式副作用保留，第二次`lstrlenA`及绘制不执行。端口返回长度达到64时则在完整格式副作用后、第二次长度调用前停止。

第二次`lstrlenA`获得括号文字长度。原位置计算为：

```text
name_pixels   = floor(name_length / 2) * 8
detail_pixels = floor(detail_length / 4) * 16
x             = 250 + name_pixels - detail_pixels
```

中间使用`SAR/SHL`顺序而不是现代浮点居中。第二行在`x,210`以颜色`0xF000`、字号16绘制。格式和文字callee可改live actor或transition stage；后续只使用长度和局部文字，不重新寻址actor。

## 5. owner、caller回收与验证

transition mode/stage、actor、动作标签、动作记录和framebuffer均复用既有owner。共享24-byte标题加入角色升级状态，承接原`0x0053C154..0x0053C16B`，与紧邻但起于`0x0053C16C`的输入选择文字工作区保持两个独立typed物理数组。原战斗全局重置写集合会清这24 byte；本函数本身只读，不新增伪写。

消息111现直连本实现；旧阶段111槽改为reserved且生产零调用。正常返回后caller仍按u32递增timer；标题框typed-stop模拟原故障不返回，阻断timer写入。

定向测试覆盖mode精确1门、actor FF负索引、四byte CP950名称、双格式/双长度、动态矩形和九宫格宽度、stage商非零、两行颜色与位置、24-byte源缺NUL、名称/详情缓冲边界、消息111直连/旧槽零调用/timer回绕/子stop阻断和主帧四类generic映射。补齐全局重置末尾写对该typed owner的同步后，定向测试、AddressSanitizer、Linux core 188/188和Linux app 194/194全部重新通过；最终重跑无warning/error/failure。首轮app配置仅出现既有ALSA开发库缺失白名单提示。

当前缺少原版动作/矩形/九宫格/字体surface、完整角色名称表与共享标题联合状态、动态栈地址、`wsprintfA/lstrlenA`返回及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
