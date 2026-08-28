# 战斗成长完成标题框 `0x00468AD0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00468AD0..0x00468C7E`，从proc到endp共187行、125条带机器码和真实助记符的实际指令、11个静态call、3个跳转、2个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。唯一静态caller是已关闭消息阶段分派的消息112公共完成路径；caller固定压入两个零参数，本函数返回后重新读取timer并按u32加一。

11个callsite包括sample播放1次、`wsprintfA` 2次、`lstrlenA` 2次、动作记录更新1次、固定矩形1次、九宫格1次、标题框查询1次和文字绘制2次。它与相邻成长标题框共享确定性的格式、动作、矩形、九宫格和文字业务实现，但以独立variant保留新增sample门及五个generic callsite不同的EAX/ECX/EDX布局。

## 2. 入口seed、模式门与sample

入口先读取原`ValueName`首byte写入64-byte局部缓冲，再从byte 1开始精确清零63 byte；EAX/ECX归零。modern请求携带该live seed快照，默认值锁定原始映像的`0xFF`，不建立第二份持久全局存储。seed在mode为1时会被第一次格式完全覆盖，mode非1时局部缓冲不外泄，但权威读取顺序仍保留。

只有transition mode精确等于1才继续；其他值保持入口EDX并返回EAX/ECX零。继续路径先读取live transition stage。stage为零时，以sample `0x160`和live signed mix level调用音频边界；调用前EAX为零、ECX为mix完整bit pattern、EDX保持入口值。stage任何非零值都不播放。sample返回后才读取actor，因此负一actor的typed-stop仍保留此前sample副作用。

## 3. 名称格式与动态底板

actor从共享transition actor byte按i8符号扩展，并在第一次真实访问十项动作标签时检查边界。合法时名称token仍为`0x0049E148 + label*16`，以`%s`格式进入局部缓冲。该变体在第一次格式调用前保留EAX=名称token、ECX=局部缓冲token、EDX=符号扩展actor；不能复用相邻函数的EAX=局部缓冲、ECX=actor、EDX=名称token布局。默认CP950名称表与相邻函数共享；格式端口可替换真实结果。64-byte缓冲最多容纳63个非NUL byte，越界在格式副作用后、第一次长度调用前typed-stop。

第一次`lstrlenA`后执行`cdq/sub/sar 1`，得到signed向零修正后的名称长度除二。随后复用唯一胜利面板动作记录，只写动作`0x233B`和variant 0。设`half=floor(name_length/2)`：

- 固定矩形为x=246、y=176、width=`half*20+8`、height=`live_stage+8`、颜色`0,4,4`、mode 0；
- 九宫格为left=250、top=180、right=`half*20+250`、bottom=`live_stage+180`、flags `0x80000008`；
- 资源低word读取动作记录`+0x4A`，高word保留矩形返回EDX。

stage在动作更新后重新读取用于矩形，矩形返回后又重新读取用于九宫格，保持callee可改live值。任一渲染typed-stop保留此前seed、sample、名称格式、长度和动作副作用。

## 4. 双行文字与寄存器variant

九宫格后固定查询`180,236,3`；EAX不精确等于1时保留底板并返回。成功时第一行在`260,188`以颜色`0xFFFF`、字号16绘名称。该变体第一次文字调用前为EAX=查询返回1、ECX=字体token、EDX=framebuffer token。

随后完整清零64-byte局部缓冲，以`[%s]`包裹角色升级状态中的共享24-byte标题。第二次格式调用前为EAX=局部缓冲token、ECX=0、EDX=第一行文字callee返回；24 byte内缺NUL时保留左括号、24个源byte和第一行文字后，在首次读取下一物理byte时typed-stop。格式结果达到64 byte则在第二次长度调用前停止。

第二次`lstrlenA`的输入token通过ECX准备。位置仍按原整数链计算：

```text
name_pixels   = floor(name_length / 2) * 8
detail_pixels = floor(detail_length / 4) * 16
x             = 250 + name_pixels - detail_pixels
```

第二行在`x,210`以颜色`0xF000`、字号16绘制；调用前EAX=framebuffer token、ECX=字体token、EDX=局部缓冲token。相邻函数在这三个位置分别使用另一套寄存器布局，测试逐callsite锁定，不能以“同画面”合并。

## 5. owner、caller回收与验证

transition mode/stage/actor、sample mix、动作标签、动作记录、framebuffer和24-byte共享标题均复用既有唯一typed owner；共享标题继续与紧邻的输入选择文字工作区保持独立数组，并由战斗全局重置末尾六dword写在全部尾部callee之后清零。入口seed只作为请求快照，不形成持久副本。

消息112现于actor有效后直连本实现；actor缺失时既有选角、sample、完成查询与transition分配顺序不变。正常返回后caller按u32递增timer；本函数typed-stop模拟callee不返回并阻断timer。旧阶段112槽改为reserved且生产零调用。消息113仍保留其自身下一工作包边界，不提前回收。

定向测试覆盖mode精确1门、live seed请求、stage零/非零sample门、sample前寄存器、sample后负一actor停止、CP950名称、五个variant generic调用寄存器、动态矩形/九宫格、查询非1、双行文字、消息112直连/旧槽零调用/timer及主帧sample映射。组合战斗Debug测试已有多个单函数栈帧超过1MB；仅Windows该测试目标以`/STACK:8388608`锁定8MB PE栈保留，生产目标不变。修正后Windows定向测试连续三次通过；Linux定向、AddressSanitizer、Linux core `188/188`和Windows app `194/194`全部通过，源码构建零warning。

当前缺少原版入口seed全局、sample对象、动作/矩形/九宫格/字体surface、标题查询callee、完整名称与共享标题联合状态、动态栈、格式/长度及寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
