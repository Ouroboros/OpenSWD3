# 战斗角色成长对照面板 `0x00467F00`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00467F00..0x00468927`，从proc到endp共1007行、608条带机器码和真实助记符的实际指令、48个静态call、33个跳转、14个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。唯一静态caller是已关闭消息阶段分派的消息110；原caller在transition对象存在时无条件调用本函数并立即返回，不消费返回值。

48个callsite包括动作记录更新1次、固定矩形1次、九宫格2次、成长面板查询1次、`wsprintfA` 14次、文字绘制22次和sample播放7次。动作、矩形和九宫格复用已关闭typed实现；查询、格式与文字通过成长面板窄端口保留；sample播放按live signed mix level进入typed音频边界。

## 2. 入口、底板和角色标题

入口按原指令把64-byte局部缓冲首byte写`0xFF`，其余63 byte写零。过渡actor精确为`0xFF`时立即返回，不更新动作、画面或阶段。

actor存在时复用胜利结算唯一动作记录，只写动作`0x233B`和variant 0后更新。固定矩形参数为`196,76,212,transition_stage+40,0,4,4,0`。第一九宫格为`200,80,400,96`，资源低word读取动作记录`+0x4A`，高word保留矩形返回EDX。随后按i8符号扩展actor访问十项动作标签，名称token为`0x0049E148 + label*16`，在`280,80`以颜色`0xFFC0`绘制；负actor或第十一项在首次真实标签访问typed-stop，保留动作、矩形和第一九宫格前缀。

第二九宫格为`200,112,400,transition_stage+112`；资源低word仍为动作记录`+0x4A`，高word保留名称文字返回ECX。之后固定查询`112,268,2`；返回EAX不精确等于1就直接返回。查询及后续格式/文字callee可发布live actor或transition stage；阶段动画使用另一物理u16 owner，不与画面transition stage合并。

## 3. 七条CP950基线文字

查询成功后，函数从上一升级提交保存的完整56-byte角色快照读取七个u16：三个limit和`+0x10..+0x16`四个派生属性。前三项按i16 sign-extension传给`wsprintfA`，后四项按u16 zero-extension。七条连续CP950格式依次为：

```text
生命力:%4d
魔法力:%4d
體  力:%4d
力  量:%4d
耐  力:%4d
智  慧:%4d
敏  捷:%4d
```

文字固定在x=216、y=`120,140,160,180,200,220,240`，颜色`0xFFFF`、字号16。modern按CP950原byte和`%4d`最小宽度生成缺省结果，同时保留每个`wsprintfA` callsite的EAX/ECX/EDX准备与可替换端口返回。64-byte缓冲最多容纳63个非NUL byte；第64个非NUL byte在格式副作用后、对应文字绘制前typed-stop。

## 4. 阶段门与九项差值

七条基线绘制后，函数读取输入分派唯一owner中的u16成长阶段，并读取快照第一limit作为DI基线：

- 阶段精确100：把九项成长差值全部清零，阶段保持100并进入当前值对照；
- 阶段精确29：按live actor和live标签读取当前56-byte角色记录，以u16回绕计算当前值减快照值的九项差值，写入三项primary和六项secondary物理owner；随后阶段加一为30并进入对照；
- 其他阶段小于30：阶段按u16加一；结果仍小于30时立即返回，等于30才进入对照；
- 阶段不小于30且不等于100：不再递增，直接进入对照。

九项依次对应三个limit和`+0x10..+0x1A`六个派生属性。原函数虽然计算并在阶段100清理最后两项`+0x18/+0x1A`差值，却不绘制也不递减它们；modern保留这一遗漏，不补画第八、第九行。阶段29的标签越界在首次真实当前记录读取typed-stop，七条基线已经绘制，阶段仍为29，九项差值保持此前前缀。

## 5. signed/unsigned增长对照

进入对照后，七个显示字段都重新读取live actor与live标签，不缓存跨callee角色地址。前三个limit以i16 signed `current > baseline`判断，后四个派生属性以u16 unsigned `current > baseline`判断；不增长的字段不画箭头、不格式化当前动画值，也不递减差值。

增长字段先在x=310绘制固定`" ==> "`，再以`current - remaining_delta`格式化`%4d`并在x=344绘制，y与基线行相同、颜色`0xF000`。前三项的current和delta均按i16 sign-extension后做i32减法；后四项均zero-extend后相减。每个字段的重新寻址、箭头、格式和新值绘制均保留各自原寄存器链；中途callee改写actor会影响下一字段，越界停止保留此前已经绘制和递减的行。

## 6. 两条串行动画队列

每个已显示字段在绘制后才尝试递减其u16 remaining delta：

- primary链：第一项只看自身signed正数；第二项要求第一项精确为零且自身signed正数；第三项要求第二项精确为零且自身signed正数；
- secondary显示链：第四项只看自身unsigned正数；第五、六、七项分别要求前一项精确为零且自身unsigned正数。

一次调用最多各推进primary链和secondary链一项，因此最多播放两个提示音。递减后前三项若成为signed负数才夹零且不播放；在正常正数门下结果不会为负。后四项不做signed夹值。每次成功递减都以sample `0x125`和live mix level播放；sample返回寄存器可流入下一字段，但下一字段仍按权威链重载actor与地址。

阶段29首帧先计算全部九项差值，再以旧remaining值显示基线起点，之后才各递减两条链的首项。阶段100先清零全部九项，因此直接显示完整当前值且不播放sample。

## 7. owner、caller回收与验证

动作记录、actor、标签、transition stage、成长阶段、sample mix、四项角色资源和完整56-byte快照均复用既有typed owner。三项primary与六项secondary成长差值加入角色升级状态，分别承接原非连续物理区域，不复制快照或角色记录。全局重置没有这些地址的原写集合，本工作包不新增伪清零。

消息110在既有transition状态非零时现直连本实现；旧`advance_message_110`枚举槽改为reserved并保持生产零调用。本函数typed-stop由消息阶段原样传播，caller没有任何后续副作用可执行。

定向测试覆盖actor FF早退、底板/双九宫格/名称、七条CP950基线、阶段0与29/30/100、九项差值、最后两项只计算不显示、前三signed与后四unsigned比较、两条串行动画队列、sample寄存器、阶段29角色资源越界、格式缓冲边界、第十一actor、消息110直连/旧槽零调用/子stop传播和主帧四类generic映射。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。app配置仅出现白名单ALSA开发库缺失警告，源码编译零warning。

当前缺少原版动作/矩形/九宫格/字体surface、成长面板查询callee、角色名称与当前记录联合状态、动态栈地址、`wsprintfA`返回、sample对象及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
