# 战斗效果记录帧协调器 `0x004582B0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整LST范围

权威函数为`0x004582B0..0x00458DDA`，完整1257行、65个静态call站点、64个`loc_`标签，无外部FUNCTION CHUNK。唯一caller为尚未关闭的`0x0045C010`，共有8个静态调用点。

ABI读取actor index、参数对象token、source value和slot index。第八十二项caller审计确认主/备用物理工作区均为18个固定槽；每槽主记录与备用记录均为152字节：

```text
primary:   0x005202A8 + slot * 0x98
alternate: 0x004FE600 + slot * 0x98
```

slot越界只在首次主记录complete读取处typed-stop。参数对象和resource owner仍是`compat::u32` token，不转主机指针。

## 2. 主记录未完成分派

主记录complete为0时分两类。

### 2.1 animation mode等于1

先查询argument对象的animation mode并读取一个输出word。

mode完整EAX等于1时：

1. 分别查询actor与argument对象两组坐标；
2. animation counter按signed小于1000且collision完整EAX等于1时，播放固定sample、只OR status低byte的bit0，并把counter写1000；
3. 绑定effect surface；counter原值为0时发布初始位置与固定particle；
4. counter低32位加1；signed达到1000时把共享X写`actor_x-20`、共享Y写actor_y；
5. 发布后续位置、present与advance；
6. signed达到1040时complete写1、counter清零。

因此collision把counter写1000后，同一调用继续递增到1001并穿透status/reward公共尾，不在碰撞点截断。

mode不等于1时：

- 查询actor坐标并绑定surface；
- counter按signed执行`idiv 9`；余数为0且counter不大于30时发布位置，counter为0时播放固定sample，再按原顺序调用`random(100)+50`与`random(80)+60`构造particle；
- counter加1后固定发布位置、present与advance；
- signed达到100时counter清零、status OR 1、complete写1。

两个动画分支都在公共尾复制三项record word，调用共享发布callee，并把两个suppression dword写1。

### 2.2 animation mode不等于1

先把source value写主记录、固定字段清零、按global mode发布mode snapshot，再初始化主记录。初始化完整EAX为0时，原程序清的是同槽**备用**152字节记录，不是当前主记录；随后alternate active清零并直接返回1。

初始化成功后按两个u16 key查询resource owner。owner token为0时在首次`[owner]`读取typed-stop。owner输出提供resource value、width、height和data token。

## 3. 主记录镜像与宽度陈旧高word

本地render flags来自主记录完整dword；本地width value由render flags高word与width-adjustment低word拼接。

参数对象内部mode为0时：

- render flags完整bit0翻转；
- base offset=`u16(resource_width)-record_base`；
- x86只执行`mov dx,resource_width`，因此width寄存器高word保留参数对象token高word；width adjustment低word非0时，用该陈旧完整EDX减原width value。

全局flip mode等于1时再次翻转render flags bit0，重新计算base offset；width adjustment非0时把width value恢复为record高/低拼接值。两次翻转可互相抵消。

参数对象token只在原`[arg+0x2AA0]`读取点验证；此前record初始化、resource查询、owner字段和共享resource token副作用均保留。

## 4. 坐标、sample与主声像

先查询argument offsets；任一低word非0才查询base coordinates并以完整dword相加。

- width value低word或record Y adjustment任一非0：只有坐标任一低word非0时，X做完整dword减width value，Y只减低word；
- 两者都为0：清两项offset local，重新查询actor坐标，X做完整dword减base offset，Y只减record base-Y低word。

sample参数先保留上述路径形成的ECX高word，再用record pan覆盖CX。play sample之后：

- `signed16(X)+base_offset`按低32位相加并以i32比较320；
- 小于320时，pan参数保留play callee EAX高word；
- 大于等于320时，保留play callee ECX高word；
- 两者都只覆盖低word为record pan，并分别传`-16`或`16`。

随后record pan清零，finalize-coordinates输出写共享signed X/Y。

## 5. 二次collision与resource释放

再次查询animation mode。完整EAX等于1时查询argument坐标，X完整减base offset、Y只减base-Y低word；若当前X低word为0，则collision X与本地X都写完整`0-base_offset`。collision完整EAX等于1时status OR 1并把complete写1。

resource render严格传共享X/Y、u16宽高、本地render flags与resource data。resource value非0才释放value；owner在成功读取后总是释放。释放顺序固定value后owner。

## 6. 备用记录

进入公共备用阶段前，EDX先重载alternate-active完整dword；这也是最终gate陈旧EDX的初始来源。

alternate active完整值等于1时：

1. 从主记录复制两个source字段，写mode snapshot并初始化备用记录；失败时清备用记录、active清零、直接返回1；
2. 备用status非0时完整覆盖主status并清备用status；
3. 按备用key查询resource owner；
4. **在首次owner解引用前**，先以owner token高word+备用pan低word播放sample；
5. 边缘小于320时pan保留play EDX高word，大于等于320时保留play EAX高word；随后set pan并清备用pan；
6. 此时才首次读取owner字段，零owner typed-stop保留play、set-pan与pan清零副作用；
7. 参数对象mode和global flip各只翻转render flags最低byte的bit0；
8. resource render的最后参数固定0；
9. resource value与owner都无条件调用release，即value为0也调用；
10. 备用complete等于1时，EDX改为alternate-active槽物理地址并active清零。

备用完整路径最后一个owner-release callee的EDX是未完成备用记录的陈旧EDX来源；complete路径则由槽地址覆盖。

## 7. status word

主status bit1置位时先把整个word清零并把alternate active写1。

随后重读status：

- signed负值：battle gate清零，唯一共享战斗消息/阶段写1；该dword与startup、动作和预帧路径共用typed端口；
- bit3置位：依次处理bit`0x1000/0x2000/0x4000`，每项先清自身再发布固定`(30,1/2/3)`；高byte bit2即word bit`0x0400`置位时，把七个record u16按i16符号扩展后直连已关闭颜色初始化器，清该bit并把共享颜色初始化门写1；
- bit2即word bit`0x0004`置位时发布actor index并把整个status word清零。

status-mode、颜色初始化与actor callee的完整EDX按最后执行者更新最终陈旧EDX；颜色初始化精确保留蓝current向零qword的高dword。没有这些call时保留备用阶段来源。

## 8. 奖励三行

status bit0置位时先发布actor，再无条件进入奖励；bit`0x10`置位也进入奖励。

奖励门完整EAX等于1时只OR battle byte低byte的bit`0x20`，resolve actor结果写共享值并清message。随后首次读取参数对象mode：

- mode等于1：调用带auxiliary word与packed-high-word输出token的reward callee；auxiliary写u16，第二输出只覆盖packed dword的**高word**；
- 其他mode：调用普通reward callee，保留既有auxiliary与packed highword。

reward返回只取AX并signed扩展。signed大于等于9999夹到9999；仅`-1`改为0且不发布基础行，其他负数仍按原位形发布。

三行发布顺序：

1. 基础reward，无ID，成功后offset加8；
2. auxiliary非0时ID`0x2367`、signed辅助值、当前offset、mode 1，再加8；
3. packed highword非0时ID`0x2366`、signed高word、当前offset、mode 1。

最后把辅助值和packed highword分别signed扩展写槽数组，reward按u32回绕累加，发布display total，pending清零、status清零。此路径明确执行`movsx edx,auxiliary`，所以最终陈旧EDX完整变为辅助值的signed扩展。

## 9. pending、final gate与收束

pending slot等于1时调用pending step `(argument token, resource key, resource aux, slot)`；其完整EDX更新陈旧寄存器。完整EAX等于1才清status和pending。

final gate word按i16大于0时：

- 保留当前陈旧EDX高word；
- 只以主记录第二lookup key覆盖DX；
- 直连已关闭全角色数值步进`0x0045BD90`，参数为`(stale_value, primary_complete)`，入口ECX保留完整complete；
- 子返回0立即返回0，角色typed-stop阻止后续收束。

正常成功还要求：主记录complete完整值等于1、pending为0、alternate active为0。否则返回0。成功时先清共享Y，再清同槽备用152字节记录，随后清共享X与animation counter，返回1。

主记录与主record complete不会在最终成功尾被清除。

## 10. callee、测试与动态差分

原32个唯一直接callee中的`0x0045BD90`与`0x0045D3E0`已关闭并直连；其余30个资源、动画、角色、奖励、音频或owner边界继续使用专用typed token端口。全角色步进内部两个尚未关闭actor callee也复用同一端口。第八十二项进一步把本函数与群体效果函数的主记录、备用记录、活动槽、公共渲染字段和奖励数组收敛为同一18槽虚共享状态。

定向测试覆盖：

- slot越界；
- 主记录初始化失败清备用记录并直接返回1；
- 主resource owner零token；
- 参数对象在resource字段发布后的真实访问停点；
- 主记录镜像、sample EAX/ECX高word、render flags与双release；
- mode-one collision同调用穿透奖励尾；
- alternate动画的cadence、双随机与sample；
- 备用owner在play/set-pan之后停点；
- 备用完成的owner高word、AL翻转、双release和槽地址陈旧EDX；
- signed status、三mode、七项i16颜色初始化、共享门、蓝转换EDX与actor清word；
- reward 9999夹值、负auxiliary、packed highword与三行顺序；
- pending callee EDX进入已关闭全角色步进；
- 子返回0早退、角色越界typed-stop与共享状态直连。

当前缺少原版双记录内存、30类剩余直接callee与效果步进内部两类actor callee共享副作用、resource owner、参数对象字段、随机状态、sample manager、奖励输出和寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
