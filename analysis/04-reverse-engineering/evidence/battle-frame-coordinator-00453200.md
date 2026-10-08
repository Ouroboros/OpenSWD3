# 战斗逐帧画面协调器 `0x00453200`

状态：续玩接线前重审中。发现历史实现的条件方向及调用顺序错误，旧测试通过不能证明本函数收敛；条件、顺序及重复端口修正已通过定向和Linux core/ASan完整门禁；Linux app完整门禁亦通过；SDL生产绑定及共享状态复核仍未完成。

## B11：画面效果借实际角色

453322调用直接借context.action_dispatch.current_actor_index与实际metrics。
效果终态清同一角色WORD；不在入口复制，也不从局部默认值构造选择。
正常增长、淡出与未返回surface的停止传播通过core/ASan定向验证，见
[角色共享证据](battle-frame-effect-00453580.md#21-b11画面效果借实际角色word及优先角色dword)。
SDL仍在45331C效果首读前停止，完整帧与实际续玩未验收。

## 1. 完整LST范围

权威LST函数为`0x00453200..0x00453570`，完整412行、44个静态call站点、18个标签，无外部FUNCTION CHUNK。函数无参数；导航调用图中同一上层战斗状态机存在33个直接call站点。

## 2. 入口、音乐与首个早退

入口固定把活动dword写1。随后调用music gate；只有完整EAX等于1且抑制byte为0时，才把共享路径地址压栈调用`sub_4856C0`；caller先压入的0未被该callee读取，三项参数在`0x00453236`由caller一并清栈。随后从独立的`dword_4C9A0C`读取32位音乐音量调用`sub_485850`（`0x0045322B–0x00453231`），不是传播放句柄。协调器上下文直接借用脚本的`0x300`字节路径缓冲区与调用方的有符号音量owner，按原始dword位模式提交；SDL战斗`frame`回调现用同一音乐前缀函数、同一脚本路径及独立音量owner执行`0x00453200–0x00453236`，再复用`sub_45FC60`鼠标首门。若同坐标且门为零，进入下一CALL `0x0045323E → sub_45F2A0`入口前缀；否则借用与世界帧同一对话选择owner构建当次热点视图、执行首命中并发布输入选择，消息6、7、9..26、28、29及大于30时经原版跳表默认零返回、进入上层下一CALL入口前缀；非默认消息case 1空角色、case 3三重抑制和case 5/8横向未命中分别执行原版默认零返回并进入上层下一CALL入口前缀；其余未绑定消息在各自首读／行扫描前保守停止；case 2/4在各自悬停项清全1后停在`0x0045FFD9/0x00460124`垂直分支前，case 27/30仍在已读状态后的`0x0045FCEF`前typed-stop；case 0使用真实消息、actor metrics及选择字执行计数／Y／组A数量门，随后复用启动期十项角色来源与八项横向偏移扫描、发布选择及EAX／ECX／EDX；可映射的零／一返回进入上层下一CALL `0x0045323E → sub_45F2A0`入口前缀，越界在首次`0x0045FD49/4B`读前typed-stop；已正常返回的输入路径读取协调器现有`render_abort_latch`并清共享`menu_action`，latch精确等于1时在`0x00453243→sub_45D490`前typed-stop，否则按有符号消息、角色及共享对话链三门分流，键盘路径依序查询DIK2..9并在首个按下的键分支前typed-stop；无键按下与非键盘路径借用共享输入状态及记录1执行三门／latch按位或，再按有符号记录9门发布记录0两项，停在`0x0045F5E0`记录2首读前。上述停点均不返回原版EAX。start EAX被音量调用覆盖，但后续阶段不使用该值。LST `0x00453218`所读的抑制byte是`0x0053C4C0`；`0x00467031`的消息阶段写1、`0x0045BB4B`的重置写0。生产代码现直接读取port的`battle_frame_input_resolution_state().target_selection_suppression`（同一物理byte），不再维护帧协调器内的独立副本。同址修正及共享音乐前缀后定向`battle.legacy_battle_setup` 1/1覆盖gate=0、抑制=1、gate=1的路径借用／有符号音量位模式和三项调用顺序；共享前缀及SDL帧输入状态owner改动后，定向core 1/1、完整Linux core／ASan各200/200及app206/206通过；最初ASan中`battle.legacy_battle_setup`测试函数栈溢出，隔离调用帧后完整重跑通过。SDL现在从与协调器共用的虚基类状态端口借用抑制byte，并在战斗初始化时复位同一状态；新增鼠标首门及热点前缀定向core 1/1通过；首门版本完整Linux core／ASan各200/200及app206/206通过，热点前缀版本完整Linux core／ASan各200/200及app206/206通过；上述门禁自身未执行实际SDL战斗`frame`回调。case 0短路路径已借用SDL消息owner，但完整消息阶段未绑定；用户随后在Windows从新游戏开场进入战斗，独立实际运行日志`build/vm/wp316-sdl-frame-user-run/openswd3-2026-10-03_18-15-50-20916.log`记录战斗id98、setup=ready、frame回调`music_started=1`及`0x0045F5E0`记录2首读前typed-stop，menu/latch/记录0前后为0，脚本opcode=1、offset=37、status=32。它证明生产帧回调到达明确物理停点，不证明完整协调器、角色帧或原版同run差分。用户第二次实际日志`build/vm/wp316-sdl-frame-user-run-v2/openswd3-2026-10-03_19-14-31-66208.log`把已观测首停点推进到输入分派RET前`0x0045FC5B`：记录2／18及后续12项rapid均零，`idle_records_inspected=12`，脚本仍以typed-stop结束。该run未执行预帧。随后SDL依LST把已证实的输入正常返回路径接到`0x00453243→sub_45D490`三道首门，读取同一物理`0x0053C018` owner，早退路径实际进入`0x00453248→sub_45B0E0`清两张18-dword表，再读取共享启动owner的组B数量；非零停在`0x0045B11F→sub_4783B0`前、零停在`0x0045B13E`组A数量读前。预帧、metric清表和计数首读各自定向core 1/1、Linux core／ASan各200/200及Linux／Windows app各206/206通过，没有新的实际回调，仍不构成Workpack 316验收。

接着严格执行六个已关闭typed阶段：第一项组合帧鼠标输入与目标解析，第二项组合逐帧输入与指令分派，第三项组合角色预处理，第四和第五项是metric与角色顺序重建，第六项组合战斗调试快捷键总处理。第六项的E键路径返回0时立即返回EAX 0：

- 活动dword保持1；
- 不锁目标surface；
- 不执行任何绘制、输入、截图或尾阶段；
- 音乐与前五阶段副作用保留。

帧鼠标输入直接复用live鼠标、热点vector、启动party映射、角色顺序、两组数量、最终角色、输入选择和已关闭TSW像素查询；完整普通ECX/EDX直接进入输入分派，启动表、映射、角色顺序、完成槽、marker或命令流typed-stop保留音乐与帧输入前缀并阻断全部后续阶段。输入分派继续复用20条输入记录、typed DIK快照、动作工作区、prompt、对话和相同热点owner；普通返回不形成成功门，完整ECX/EDX传给角色预处理，其typed-stop保留第一阶段及自身前缀。角色预处理typed-stop保留前两阶段与自身前缀并阻断metric以后全部流程。调试快捷键复用相同DIK与共享状态；其typed-stop保留metric和顺序重建副作用并阻断surface lock。六项前置阶段已无opaque call。

## 3. 目标surface与渲染门

第六阶段非零后：

1. `45325E`读取target surface当前引用，`453265`调用`416F10`；
2. `45326A`重读surface引用，`453272`把Lock返回值发布到当前像素地址槽；
3. `453277`以重读的surface与原Lock返回值立即调用`416F60`。

`45327C`在Unlock返回后重读渲染中止dword，精确等于1才进入`453569`，
返回活动dword当前值。该值虽在帧入口写1，但不得用常量替代尾部重读。
lock/unlock副作用已完成，固定帧和后续阶段均不执行。

### B11实际画布绑定

`prepare_legacy_battle_frame_surface`由完整核心协调器与SDL实际帧入口共用。
Lock正常返回零也发布零并调用Unlock；Unlock的HRESULT不控制后续分支。
未正常返回的Lock保留旧发布值，未正常返回的Unlock保留已发布的新值，
两者均显式停止，不能借中止门伪造正常返回。

`416F10`先清零0x7C字节描述，以`Lock(NULL,desc,1,0)`调用旧平台；
HRESULT非零返回0，成功才把signed pitch右移1写入无消费者的pitch shadow，
再返回lpSurface。`416F60`仅透传surface和像素参数给虚表+0x80。
适配沿用[稳定软件画布合同](framebuffer-and-display-presentation.md)：
`LegacyBattleFramebufferSurface`借用SDL既有`game_framebuffer_`，不复制像素。
画布不可移动且无resize入口；guest预约覆盖真实物理像素和已有末尾WORD读护栏，
地址可反查至同一字节存储，解锁不撤销该稳定映射。未知surface或guest预约失败
属于适配typed-stop，不伪装为原Lock的正常零返回；软件解锁无宿主租约要释放。

SDL仅在此前全部阶段正常继续时执行本批；中止门精确1正常返回当前活动值，
其他值继续进入第4节的共享选择前缀。原输入分支停点未被绕过。
本批不建立原版CPU现场证据，不代表整帧绘制、四处角色帧绑定或实际续玩已通过。

本批向量覆盖Lock返回0/非零/全1、surface引用重读、Unlock回调看到发布值、
两次callee未返回的不同前缀、Unlock HRESULT忽略、回调改写中止值和活动值，
以及中止0/1/2/全1。真实画布覆盖1280和1344字节pitch、重复锁取得同一身份、
解锁后同址写入、边界地址映射和与共享前缀的实际组合；完整协调器另覆盖两处
callee停点的传播及后续绘制零调用。guest地址耗尽未单独注入，不声称动态覆盖。

最终`proc_b221`完成定向`battle.legacy_battle_setup`：core及ASan各1/1，
总时间分别5.37/8.04秒；SDL目标构建通过，三份日志无warning/error。
日志为`build/tmp/runtime/battle-frame-surface-publication-{core,asan,sdl}.log`。
此前构建发现移除旧调用路径后留下未使用的`invoke`包装器；删除后才取得本轮结果。
未启动游戏，未新增原版动态差分；WP316仍待审。

## 4. 选择延迟与交互发布

入口读取选择mode和选择值。仅当`mode==1 && value==0xFFFFFFFF`时把mode写0。

刷新分支要求同时满足：

```text
input_source != 0xFFFFFFFF
selection_active == 0
action.frame_enabled == 1
selection_mode == 0
```

- 16位延迟按unsigned小于`0x10`：只执行word递增；FFFF进入出队分支；
- 延迟不小于`0x10`：直接组合已关闭攻击顺序出队；它按无界28字节步长跳过角色查询精确返回1的组A记录，把首个可用或空记录完整七dword复制到共享输出，随后按原规则左移并重置尾部；
- 新值不是全1：延迟word清零、active写1、实际脚本workspace的coordinate_y保存位形；
- 新值仍为全1：不清延迟、不置active、不改脚本坐标。

旧选择刷新callee枚举只保留reserved数值且不再调用。完整协调器保留窄查询端口，SDL按实际组A地址读取启动角色的mode_gate和action的special_mode。未知映射传播callee未完成，保留lock/unlock和出队前缀，阻断全部后续帧。固定调用域不关闭通用查询。

选择值与角色metric优先索引是同一物理七dword输出记录的首dword，后六dword也收敛到同一metric owner；全局重置按原物理写集合清完整七dword。输入源复用启动状态第一条`0x1C`记录的`+0x00`，选择active与mode分别复用最终角色selection gate与action_pending_aux。调试快捷键或后续角色阶段的同址写入会真实影响本帧后续判断，不保留旧独立副本。

交互可用dword最终严格等于：

```text
selection_value == 0xFFFFFFFF && selection_source == 0
```

该值直接发布到action.actor_progress_gate，供双方角色进度调用读取。
帧开关统一到action.frame_enabled，脚本50处写入和角色帧、调试显示共同借用。
核心与SDL共用选择前缀，SDL正常后停在45331C画面效果首读前。
原指令顺序、实际存储及验证见[选择等待与出队绑定](battle-frame-selection-runtime-binding.md)。

## 5. 主绘制阶段与固定帧

随后固定执行：

1. 直接调用`0x00453580`画面效果，以其共享pending rotation作入口参数；
2. 当`conditional_mode == 1 || conditional_submode == 1`时调用`0x0045B280`角色优先级更新；
3. 调用`0x0045B5E0`角色帧序列；
4. 直接组合已关闭双方完成数协调：扫描组A对象双门和组B mask链，满足阈值后发布message及组门；
5. 直接组合已关闭待执行动作提交：按入口总数遍历live角色顺序，处理ready标记、角色发布和记录移除；
6. 直接调用已关闭`0x0045C010`效果总协调步进。

画面效果typed-stop保留交互可用发布及效果内部真实前缀，阻断角色帧和全部后继stage；选择帧在HUD之后才执行，不能提前产生副作用。双方完成数协调复用当前角色、双方数量、最终角色计数、动作phase/packed计数、结果暗化门、启动组门与message唯一owner；前一角色帧的post-call ECX/EDX由显式snapshot进入，正常尾EDX成为待执行动作零角色早退快照。其子typed-stop阻断待执行动作和后续帧，旧第一后继stage只保留reserved枚举值且不再调用。待执行动作提交复用唯一metric顺序/数量、启动ready槽、actor publication和activation latch，并直接组合攻击顺序移除；移除左移尾源与效果总协调器`intensity_records[0]`共用同一物理owner，旧pending移除端口槽只保留reserved数值且不再调用。其typed-stop保留角色帧、完成数协调及publication前缀并阻断效果协调和固定帧。旧第二后继stage只保留reserved枚举值且不再调用。效果总协调器复用主帧端口内唯一角色metric、效果步进和18槽记录状态；子typed-stop阻断固定帧，普通返回值精确等于1时只对共享UI dword的低word OR 1，高16位原样保留。旧opaque完成门枚举和测试桩已删除。

然后直接调用已关闭`0x00450270`，固定资源`0x234D`、帧0、坐标`(0,384)`。frame unavailable或blitter typed-stop在原首次访问/绘制点阻断后续流程；不伪造选中角色、跨模块队列、输入或截图尾。

## 6. 选中角色面板

选择来源为0时跳过整个面板，后续低word调用使用固定帧callee后的显式陈旧ECX snapshot。

选择来源非零时，原顺序为：

1. 清零固定`0x26`个dword动作记录；
2. 写动作号`0x233B`与base variant 0；
3. 直接调用已关闭动作更新器；返回失败也继续；
4. 第一次读取角色映射表；
5. 用第一次映射完整u32作为第二次映射索引；
6. 第二次映射值作为面板left；
7. 第一次映射值只覆盖DX为动作记录`field_4a`，保留其高16位，形成九宫格资源号。

映射缺失typed-stop发生在动作记录清零、字段写和动作更新之后，保留这些前缀副作用。该面板动作记录同时是已关闭列表框所访问的同一物理owner；两个caller各按权威时序清零和更新，不建立第二份记录。

面板直接调用已关闭九宫格helper：

```text
resource = (first_mapping & 0xffff0000) | action.field_4a
left = second_mapping
top = 397
right = wrapping_i32(left + 0x74)
bottom = 467
opacity = 0
flags = 0x80000008
```

九宫格失败在typed状态阻断。

特殊面板抑制dword非零时跳过角色对象与独立动作帧，后续ECX高字取九宫格callee后snapshot。

## 7. 角色对象与独立动作帧

未抑制时，选择来源必须在`8..17`。caller按原两次SHL和两次SUB从`selection_source-8`同时形成`1007*index`入口EAX与组A物理token；入口flags来自最后一次SUB，EDX显式复用九宫格callee后的残值。随后直接组合已关闭`0x004787C0`，从canonical`actor+0x26B8`完整dword返回原bit31。旧角色查询端口槽只保留reserved ordinal且生产零调用。字段或RET typed-stop保留面板动作与九宫格前缀，阻断post-call TEST及全部后缀。

leaf正常返回后以完整EAX执行`TEST EAX,EAX`。非零时按原`JNZ`跳过独立帧，后续ECX沿用actor token；零时首次读取选择来源对应的X/Y坐标，再直接调用已关闭`0x00450B60`：

```text
action = 0x2391
x = selected position.x
y = selected position.y
```

独立动作helper内部仍需要其动作更新callee后的ECX/EDX陈旧高字，聚合请求显式提供这两个snapshot；helper完成后，后续低word调用使用独立帧callee后的另一个ECX snapshot。三个snapshot不互相替代。

## 8. 陈旧ECX低word与跨模块直连

面板路径汇合后，LST只执行`mov cx, gameplay_word`。modern保留此前选定snapshot高16位并替换低16位，再把完整u32传给下一战斗stage。

随后按LST直接组合四项：HUD、`0x00464270`选择帧、消息阶段分派与文字消息逐帧协调。选择帧处理完成角色替换、message绘制、目标轮转和角色标记；typed-stop保留HUD及此前副作用，阻断消息阶段。旧`post_render_stage_1`枚举只保留数值，不再调用。消息阶段在HUD和选择帧后执行，其中消息98直连炼符结果面板，消息99直连过渡控制选择，消息100继续依次直连胜利奖励、结算面板与升级提示面板，消息101在actor缺失时直连角色升级属性提交，消息102在战利品非零时直连战利品清单面板，消息103普通路径直连战败提示面板，消息110在transition存在时直连角色成长对照面板，消息111固定直连成长标题框，消息112在actor缺失时直连成长角色选择、actor有效后直连成长完成标题框，消息113在actor缺失时直连法宝成长结果角色选择并固定直连法宝完全成长提示框；任一typed-stop保留对应前缀并阻断第三后置阶段及全部跨模块链。主帧端口保留炼符结果面板的reserved stage槽，并映射成功标题、成功格式、成功详情、失败标题和失败详情六类服务，使用显式文字长度并回传live stage与结果byte；战利品清单的字体、标题、reserved stage槽、行格式和行绘制五类服务，使用独立发布位与显式长度区分合法空文字；战败提示另映射标题、reserved stage槽、字体和详情四类服务；成长角色选择映射组A完成查询、道具定义加载、道具存在查询与节点分配四类服务，并以固定256-byte说明载荷加显式长度避免复制第二份道具owner；成长结果选择另映射完成查询、结果选择、定义加载、说明释放和标题复制五类服务；法宝完成提示映射格式、长度、reserved stage槽、字体大小和文字五类服务。消息阶段正常完成后，主帧直接遍历共享文字消息链，先绘制活动节点并递减16-bit计时，再以第二轮完成滑出、原位摘链与释放；旧第三后置槽保留reserved数值且生产零调用。子typed-stop阻断全部跨模块链。之后立即直连已关闭跨模块helper，顺序不可交换：

1. packed-row效果链更新/绘制；
2. 角色头顶动作链更新/绘制；
3. 对话链更新/绘制；
4. 调试状态面板；
5. 倒计时`(400,8,0)`；
6. 倒计时`(10,8,1)`。

packed-row、头像链和对话使用各自真实typed owner，不在battle复制平行列表。对话只有`idle/completed`继续；surface、文本或控制typed失败阻断。倒计时只接受completed、inactive和suppressed三种正常状态。

## 9. 内部bit 17与返回3

两个倒计时后调用`0x0040DC50(0x11)`。该callee不是键盘DIK查询；它读取内部bit表：

```text
byte_index = 0x11 >> 3
mask = 1 << (0x11 & 7)
return (flags[byte_index] & mask) != 0
```

modern直接对typed bit-span执行相同访问。span缺失只在byte 2真实访问点typed-stop，保留此前全部绘制和倒计时副作用。bit置位时立即返回完整EAX 3，不执行后续stage、surface尾或截图。

## 10. 后置stage与surface尾

bit未置位后：

- 独立调试叠加门dword精确等于1时直接组合已关闭战斗调试叠加层；旧opaque模型的条件方向相反，现已按LST纠正；
- 叠加层正常返回后直接组合已关闭结果判定前置流程；其双侧计数门可调用全帧暗化、暂停音频与尚未关闭的结果整理；
- 结果判定正常返回后直接组合已关闭上下文提示绘制；叠加层、结果判定或提示typed-stop保留各自前缀并阻断后续颜色与surface阶段；
- 共享颜色计数小于等于0且共享初始化门精确等于1时，直接调用已关闭颜色初始化器并传入`24,24,24,0,0,0,8`，再把共享门写0；
- `0x004534D2`固定以参数1直接调用三通道颜色累加，之前没有额外finalize调用；旧`finalize_overlay`槽只保留枚举数值且不再调用。

三通道颜色累加：固定递减请求，按共享九float与计数执行step零门、`current += step`或`step = target`、x87向零转换，并只调整framebuffer前`0x3C000`像素。overlay门与颜色累加读取同一typed计数；颜色framebuffer失败阻断surface与截图尾。

随后：

- special surface gate不等于1且mode flags的bit`0x100`未置位：以固定selector`0x2711`解析primary surface并从target surface执行整surface虚操作；零token只在立即vtable访问点typed-stop；
- gate精确等于1或mode bit已置位：直接组合已关闭纵向位移，以16项signed表构造两组矩形Blt，中间按固定1280字节行宽清framebuffer暴露带，并按live节拍推进phase。

## 11. 截图尾与最终返回

截图请求与调试P键复用唯一typed状态。截图请求dword等于1时：

1. 16位计数器递增并回绕；
2. 零扩展新word后加1000；
3. 格式化`c:\\snap\\%d.bmp`；
4. 直接调用已关闭16位framebuffer BMP writer，固定`640×480`；
5. 无论writer结果如何，截图请求清零。

截图编号不会携带陈旧EDX高字。测试锁定`0xFFFF -> 0 -> 1000`和精确路径。

普通尾在`0x00453569`重新读取活动dword；它在入口写1，但后续结果处理可以改写。三类汇编正常出口分别是调试快捷键门返回0、内部bit返回3、其余路径返回活动dword当前值。

## 12. 双向追溯

- `0x00453200..0x0045325D`：活动、音乐、鼠标解析与输入分派直连、角色预处理、metric、顺序、完成门与零早退；
- `0x0045325E..0x00453286`：target lock/unlock、发布、渲染门与当前活动值返回；
- `0x0045328C..0x0045331C`：选择mode、延迟刷新和交互可用发布；
- `0x0045331C..0x00453379`：画面效果直连、条件阶段、角色帧顺序、双方完成数与待执行动作提交直连、效果协调、UI低word和固定帧；
- `0x0045337F..0x00453431`：选中动作记录、双映射、九宫格、角色对象和独立帧；
- `0x00453434..0x00453482`：ECX低word、HUD、选择帧、消息阶段分派、文字消息逐帧协调、三类跨模块队列和两倒计时；
- `0x00453491..0x004534A3`：调试叠加精确门、结果判定前置流程及上下文提示typed直连；
- `0x00453485..0x00453490`：内部bit与返回3；
- `0x00453491..0x00453514`：可选/固定stage、overlay、三通道颜色累加直连、整surface提交与纵向位移分支；
- `0x00453514..0x00453570`：截图word、路径、BMP写入、请求清零和活动返回。

历史记录声称C++到LST反向追溯覆盖完整412行、44个静态call站点和18个标签；本轮发现下述反例，该完成结论撤回，须重新执行完整正反向核对。

B11初始化门共享：453322画面效果借该端口实际53C030，
不再读取效果状态副本。十五组计数/门向量覆盖DWORD高位与非正计数，
同时检查画面效果先等待、4534CB随后条件清门，不补执行已返回的效果。
core/ASan setup及actor316各1/1、SDL链接通过；详见
[初始化门共享](battle-frame-effect-00453580.md#22-b11画面淡出借实际颜色初始化门)。
完整SDL帧与实际续玩仍待验收。

B11旋转缓存共享：453322效果context直接借startup.background_rotation_cache。
三组0/1/-1检查实际记录身份、播放入口清写及未返回前缀，
停止后不继续角色更新和绘制。core/ASan两组定向及SDL链接通过，见
[旋转缓存共享](battle-frame-effect-00453580.md#23-b11画面效果借背景初始化的实际旋转缓存)。
完整SDL帧与实际续玩仍待验收。

## 13. 验证与动态差分

定向测试覆盖：

- 音乐启动/commit、鼠标解析与输入分派直连、typed角色预处理与调试快捷键顺序，以及E键第六阶段零早退；
- target lock/unlock后渲染门返回活动1；
- 选择延迟、攻击顺序出队直连、七dword共享输出、角色查询转接、旧opaque槽清零、active/auxiliary发布与交互门，以及选择帧typed-stop传播与旧frame-stage槽零调用；
- UI dword只改低word且保留高16位；
- 角色更新之前画面效果直连及其typed-stop前缀；
- 角色帧后双方完成数协调直连、post-call寄存器snapshot、组A/组B消息发布与旧第一opaque槽清零；
- 完成数协调后live数量/顺序改写、待执行动作提交直连、旧第二opaque槽清零及子typed-stop阻断效果与绘制；
- 固定帧直连、ECX高字/低word组合；
- 消息96–113阶段分派直连、消息后文字链两轮遍历、活动计时、三类滑出、摘链释放、旧第三后置槽清零、消息98炼符结果五类服务与reserved stage槽映射、消息99过渡控制选择直连与旧槽零调用、消息100胜利奖励与升级提示面板嵌套直连、消息101角色升级需求/双模板/音频映射、消息102战利品清单字体/标题/reserved stage槽/格式/文字映射、消息103战败标题/reserved stage槽/字体/详情映射、消息110成长reserved stage槽/格式/文字/音频映射、消息111标题名称/reserved stage槽/文字/括号格式映射、消息112成长角色选择的完成查询/定义加载/道具存在/节点分配映射及完成标题的同类映射与内部sample播放映射、消息113成长结果选择的完成查询/结果选择/定义加载/说明释放/标题复制映射及法宝完成格式/长度/reserved stage槽/字体/文字映射、对应旧槽零调用及子typed-stop阻断第三后置阶段；
- packed-row、头像、空对话与双倒计时直连；
- 内部bit 17返回3及缺失bit表真实访问typed-stop；
- 调试叠加精确等于1门、正常组合和子typed-stop后续阻断；
- 结果判定双侧算术、全帧暗化直连、窄结果端口和子typed-stop后续阻断；
- 上下文提示300帧门、30项switch、鼠标/角色四路提示、偏移动作帧直连及子typed-stop后续阻断；
- 角色预处理工作区typed-stop阻断metric与后续帧；
- 三通道颜色初始化、共享门与尾寄存器，以及同帧颜色累加、计数递减与`0x3C000`前缀；
- surface门0/1/2与mode bit两侧、整surface零token typed-stop、纵向位移双矩形提交及子typed-stop截图阻断；
- 截图计数word回绕、路径、writer调用与请求清零；
- 面板动作更新、双映射、九宫格、角色组A token、`actor+0x26B8`高位查询的入口EAX/EDX/SUB flags与真实返回地址、post-call TEST/JNZ、独立动作帧和第三类ECX snapshot；
- 映射缺失发生在面板动作更新副作用之后；
- battle聚合目标零warning，普通定向通过。

## 14. 旧存档进入战斗后的重新核对

用户确认读档正常，但战斗23在SDL输入前缀`0x0045FB29`停止。核对原版完整帧发现，不能直接将当前核心库视作已正确绑定的实现：

- `0x0045332F/31`的相等跳转直接进入角色更新；不等时`0x00453333/39`仅在第二字不等于1时跳过。旧C++错误使用第一个字不等于1。
- `0x00453354/56`的`JNZ`跳过`OR word,1`；旧C++对不等于1做OR，方向相反。
- `0x004534AE`的`JG`和`0x004534B6`的`JNZ`均跳过颜色初始化；旧C++错误要求初始化门不等于1。
- `0x004534DF/E1`的相等跳转进入纵向位移，`0x004534E8/EB`的位0x100非零也进入位移。整画面提交要求门不等于1且该位清除；旧C++将前者误写为非零。
- `0x0045343C→sub_459D10`之后才在`0x00453441`调用`sub_464270`。旧C++在画面效果之前调用选择帧，并在HUD后额外调用旧opaque槽。现将真实选择帧整体移动到该物理调用处并移除额外调用。

- `0x004534D2→sub_45D2F0`只有一次颜色更新。旧C++在真实颜色更新前仍调用`finalize_overlay`端口，缺少对应原版call；现移除该调用，新增旧槽零调用及真实颜色更新恰好一次断言。

修正后首次定向测试暴露旧断言错误；新增门值0/1/2、计数负/零/正与位0x100的独立组合向量，并更新错误顺序导致的停止前缀断言。`build/tmp/runtime/battle-frame-lst-gates-retest.log`记录定向`battle.legacy_battle_setup`通过；新增颜色旧槽零调用检查后，`battle-frame-coordinator-core.log`与`battle-frame-coordinator-asan.log`分别记录205/205通过。构建同时发现音乐适配器忽略返回值警告，已以显式void转换保留原有忽略语义；该最终源码的Linux app门禁211/211通过，日志为`battle-frame-coordinator-linux-app.log`，未出现warning/error。尚未据此开放SDL完整战斗或关闭第316包。

上阶段先将菜单取消六处、目标刷新四处写入接到帧协调器当时读取的`final_actor.frame_gate_b`。随后完整扫描确认`0x0053BFC0`实际被拆成五份存储；当前统一借用`action.action_pending_aux`，原final-actor、input、撤退和转场副本均已删除。帧入口、菜单取消、角色更新和全部其余既有读写方的映射及本轮验证见[共享选择等待状态](battle-shared-selection-gate-0053bfc0.md)。不能以同步副本或默认初值相同代替共享状态。

上一阶段限定修正上述帧条件、调用顺序和两个现有输入callee的十处共享门写入，不包含SDL完整战斗绑定或全部共享状态回收。当时工作树经`./build.sh core --test`、`./build-asan.sh --test`、`./build.sh app --test`验证，依次205/205、205/205、211/211通过，无编译warning/error；日志为`build/tmp/runtime/battle-existing-publication-{core,asan,app}.log`。源码、测试和证据差异已重新完整审查。本轮未重建Windows，也不证明实机战斗生命周期通过。

当前没有原版剩余战斗callee、攻击顺序出队角色查询与无界相邻内存轨迹、共享选择/队列/对话/倒计时状态、调试叠加字体/文字/角色查询状态、结果判定计数/音频/整理状态、上下文提示计数/鼠标/动作帧状态、九float与计数、DirectDraw target surface、内部bit表、寄存器snapshot与BMP文件联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
