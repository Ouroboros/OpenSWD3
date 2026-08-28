# 战斗胜利奖励与结算面板 `0x00467710`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00467710..0x00467AB9`，从proc到endp共418行、249条带机器码和真实助记符的实际指令、22个静态call、18个跳转、15个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。唯一静态caller是已关闭消息阶段分派的消息100路径；原caller先压入两个未被本函数读取的零参数，返回后才发布目标选择准备门和递增timer。

22个callsite包括动作更新1次、矩形效果1次、九宫格2次、音乐渐隐1次、全部sample停止1次、sample播放1次、组B掉落查询1个静态位置、玩家道具数量2个静态位置、组A六类业务call各1次、`wsprintfA`3次和文字绘制4次。动作、画面、音频、玩家道具和格式化已直接组合typed实现或typed平台接口；六类未审组内业务callee继续由窄端口保留。

## 2. 固定面板前缀

函数建立64-byte局部文字区，并按原BSS空首byte加其余零初始化。它不清整个共享动作记录，只把动作编号写`0x233B`、基础变体写零后调用动作更新。随后以固定`x=196,y=176,width=188`和`transition stage+40`作为高度执行模式0、颜色`0/4/4`矩形效果。

第一九宫格使用动作记录资源低word与矩形返回EDX高word组合资源，范围为`200,180..376,196`、flags `0x80000008`。完成后以CP950`戰鬥勝利`、坐标`248,180`、颜色`0xFFC0`、字体16绘制标题。第二九宫格资源低word相同，但高word来自标题文字返回EDX；范围为`200,212..376,transition stage+212`。任一modern画面安全状态typed-stop时保留此前动作和绘制前缀。

## 3. 一次性奖励门与音频顺序

奖励word位15已置位时跳过全部音频、掉落、角色奖励和银币写入，只继续结算面板stage推进。位15未置位时严格执行：

1. 固定stream 100开始divisor 1渐隐；
2. 停止全部sample，包装返回EAX固定1；
3. 重新装载live signed mix level，以sample `0x12C`播放。

sample调用前EAX是mix level的完整32位bit pattern，不沿用全部停止返回。音频返回不产生现代成功门。

## 4. 组B掉落合并

组B循环从索引0开始，以live数量按i32 signed比较；每次迭代尾重读数量，不增加现代上限。对象地址使用基址`0x00525508`、步长`0x2B28`；第九对象在首次真实掉落查询typed-stop。

查询返回只使用DI低word；零跳过。非零道具先线性扫描`0x004FF2F0`起十个u16槽：

- 命中既有槽：以数量选择1直连玩家道具数量，再对对应u16结算数量加一并回绕；不增加唯一道具计数，也不重写payload或道具编号。
- 未命中：以共享u16唯一道具计数作为目标槽，先直连玩家道具数量，再增加目标u16数量，然后增加共享计数，最后依次保存payload token和道具编号。

因此唯一道具计数为10时，第十一个新掉落已经完成玩家道具副作用，随后在首次真实数量表访问typed-stop；计数、payload和编号尚未推进。`0x00524468..0x0052448F`现由十项payload数组唯一承接；消息99写入该数组第0项，炼符结果面板成功路径读取同一第0项作为名称token，不再误建单一标量owner。

## 5. 组A角色奖励

组A循环同样按live i32 signed数量并在尾部重读。对象地址使用基址`0x005029D0`、步长`0x2F34`；第十一对象在首次读取`+0x2B00`停止。每个对象依次检查`+0x2B00`与`+0x2B04`，只有值精确等于1才跳过；这两个物理字段由胜利奖励state独立承接，不复用启动记录的抽象active值。

两字段都不为1时查询奖励阻断，返回非零跳过。返回零则：

1. 从共享动作标签表读取角色标签；
2. 以56-byte步长访问世界剧情VM唯一四项角色资源owner；
3. profile `+0x2C` byte低于共享u16阈值时，把每人经验按u32回绕加入profile首dword；
4. 以固定profile token和奖励经验u16调用角色奖励；返回精确1时置共享奖励门；
5. 标签对应的`0x004ACF54+n*0x60`计数先加一；组B live数量按i32不小于3时再加一；
6. 依次调用角色准备参数1和角色配置参数`0,0`。

profile、计数或动作标签只在原首次真实访问停止；此前callee及共享写全部保留。奖励call前锁定组A对象、奖励word和每人经验EDX；准备call前锁定计数地址EAX与live组B数量EDX。

## 6. 银币提交与结算文字

双方循环结束后读取奖励银币word，把位15置位并写回，再清共享transition timer。入账金额只取原word低15位，按u32回绕加到世界剧情VM `script_variables[0]`，即进程期银币唯一owner。共享变量owner缺失时，typed-stop发生在置位和清timer之后。

最后直连已关闭stage推进，以`base=212,target=284,divisor=2`更新共享transition stage；signed商为零、返回精确1时才使用同一64-byte局部缓冲连续执行三次signed `%d`等价格式化和文字绘制：

- `每人得到經驗值:%d`，坐标`208,220`；
- `得到銀幣:%d`，坐标`208,240`；
- `得到法寶經驗值:%d`，坐标`208,260`。

三行均使用CP950、颜色`0xFFC0`和字体16。每次格式化只覆盖新字符串和NUL，保留局部缓冲其余旧尾；首次真实64-byte越界才停止。动态栈地址不转换为宿主指针，由调用请求提供token并在动态差分中继续标记缺口。

## 7. owner、caller回收与验证

共享双方数量、动作标签、sample mix、transition stage/timer/item count、玩家道具链、世界角色资源和银币均复用既有唯一owner。新增state只承接胜利面板动作记录、十项道具编号/数量/payload、四项稀疏奖励计数、两组角色跳过字段、三项奖励word、profile阈值和奖励完成门。后续已关闭战利品清单直接消费同一item count、十项payload和u16数量，不复制展示数组；战败提示继续复用同一面板动作记录。全局重置按原写集合只清编号/数量前两项、全部十项payload和三项奖励word；未写的其余槽、稀疏计数、阈值、奖励门及角色字段保持原值。

消息100现先直连本实现，再直连已关闭升级提示面板；旧消息100 opaque枚举槽改为reserved且生产零调用。本函数子typed-stop阻断升级面板和caller全部写入；升级面板子stop则保留已完成胜利奖励与自身画面前缀，再阻断actor-retarget、双cache、target-ready、queued和timer写入。

定向测试覆盖双面板几何与画面停止前缀、固定CP950标题、奖励位15幂等门、音乐渐隐→sample全停→播放顺序、mix level预调用EAX、组B signed live循环和第九对象、十槽命中/新增/第十一槽停止顺序、玩家道具子stop、组A两字段精确1门、第十一对象、profile阈值、奖励/准备寄存器、组B数量3双计数、共享银币owner、三行CP950格式、全局重置物理范围、消息100正常直连与子stop传播。验证：定向测试、AddressSanitizer、Linux core `188/188`、Linux app `194/194`全部通过。源码构建零warning；app仅保留既有ALSA开发库CMake warning。

当前缺少原版两组角色对象、六类未审业务callee联合状态、动作/画面/字体surface、动态栈地址、音频对象及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
