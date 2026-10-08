# 战斗当前画面复合效果 `0x00453580`

历史状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。
B11正在复核实际画布接线；灰度分带修正见第15节，复制返回处理见第16节。
角色移动与背景平移的共享量修正见第17节。
闪光触发与强度见第18节，刷新阶段与画布标记见第19节。
双抑制门、当前颜色与刷新回调重读见第20节。
不把历史标记或定向测试视作完整生产接线完成。

## 1. 完整LST范围

权威LST函数为`0x00453580..0x004539A9`，完整508行、21个静态call站点、22个标签，无外部FUNCTION CHUNK。ABI为一参数cdecl/plain `retn`，caller清理参数。三个直接caller站点为：

- `0x00452904`与`0x00452D75`，同属已关闭战斗画面转场；
- `0x00453322`，属于已关闭战斗逐帧协调器。

三个caller均忽略返回EAX。本函数不同出口可能保留surface虚调用、软件blitter或普通状态运算残值，不把它伪造成稳定业务返回。

## 2. 入口source发布与全屏clip

入口先读取当前source token并发布到共享blitter source槽，然后固定调用clip owner：

```text
left = 0
top = 0
right = 640
bottom = 480
```

modern以typed source保存token、可写命令流、布局和u16宽高；token发布严格早于clip。所有通用blit固定使用入口source宽高、目标`(0,0)`和空palette/辅助尾，直接复用已关闭软件blitter及其正常公共后缀。indexed布局仍因固定空palette在原palette首次读取点停止，不擅自借用外部palette。

## 3. 双抑制门与零旋转路径

两个抑制dword任一非零时，完整跳过入口source绘制、旋转缓存、split带、pending rotation清零和颜色循环，直接进入公共全屏clip及阶段状态机。

两者都为零且入口rotation amount为0时：

1. 全屏绘制当前source，flags 0；
2. 直接调用已关闭`0x00451540`旋转缓存单帧绘制；
3. `53BF30`的完整DWORD精确等于1时更新u16 extent并绘制上下两条灰度带。
   `4535FA`与EBX=1比较，`453600 JNZ 453716`在不等时跳过整段。
   旧字段名`split_suppression`保留，但原语义是等于1启用；0、2和全1均跳过。
   检查仍在旋转缓存单帧返回之后，跳过时保留extent并继续公共后缀。

extent更新严格为：

```text
extent >= 192: 不变
20 <= extent < 192: extent = u16(extent + 22)
extent < 20: extent = u16(extent << 1)
```

两条clip与绘制顺序为：

```text
clip(0, wrapping_i32(192-extent), 640, 192)
blit(flags=0x28)
clip(0, 192, 640, wrapping_i32(192+extent))
blit(flags=0x28)
```

extent为0时仍执行两次零高clip和两次clipped-out调用；不现代化短路。

## 4. 非零旋转路径

入口rotation amount非零时：

- signed正值：直接调用已关闭literal图像模式3，shift为原值；
- signed负值：以低32位二补数取负，调用模式2；
- `INT_MIN`取负后bit pattern不变，closed rotation按非正shift普通返回，caller仍继续。

之后全屏flags 0绘制当前source，再直接调用已关闭`0x004515E0`旋转缓存动作播放，传入原signed rotation amount。动作初始/后续更新返回0属于原callee普通返回，外层继续；非法缓存索引、空owner、rotation/blit故障和非终止域才typed-stop。

零或非零路径完成后都把共享pending rotation dword清零。若此前source blit、source rotation或缓存callee在真实访问点typed-stop，不执行该清零。

## 5. 三通道颜色循环

只在双抑制门均为零且前述绘制完成后检查颜色循环dword。完整值等于1时：

1. 对颜色delta byte做signed扩展；
2. 把同一i32依次发布到红、绿、蓝三个共享槽；
3. 对固定target framebuffer的`0x3C000`像素依次直连红单通道、绿双lane、蓝双lane调整；
4. 三次全部正常返回后才执行delta byte加`0xFC`的u8回绕；
5. 新byte为0时写回`0x10`并清颜色循环dword，否则保持循环有效。

`0xFC`按-4传入颜色函数；不按252处理。颜色helper保留末像素dword look-ahead与绿/蓝双lane合同；modern使用framebuffer只读guard，不扩大逻辑像素范围。

## 6. 公共全屏clip与角色比较

入口效果结束或被双抑制门跳过后，再次固定恢复`0,0,640,480`clip。

4537A5第二次全屏clip返回后，4537AA读取4A7630角色WORD并做MOVSX；
4537B1读取53AE70完整优先角色DWORD。4537B9比较两者完整32位位形。
不同则跳过surface阶段和cadence，直接携带当前stage WORD进入尾部fade。
它们是角色索引，不能按遭遇编号或两个同宽WORD理解。

角色比较相等后，只有`primary_suppression==1 || secondary_suppression==1`才执行surface阶段；其他非零值虽然跳过入口绘制，却不会进入此阶段。

## 7. 标准surface阶段

alternate surface mode为0时读取signed stage word：

- stage小于1：不读取surface表；
- stage不小于1：以完整signed stage作索引读取surface token，再由固定surface对象执行虚操作；effect flags固定`0x01000000`。

surface表越界在首次实际索引读取停止；此前source发布、两次clip与全部入口门副作用保留。DirectDraw对象和虚调用由窄平台端口表达。

`453808`正常返回后沿`45380B`跳到`4538B2`，重新读取stage WORD；
后续cadence和fade必须使用该读值。HRESULT无论0、正值或高位置位均被忽略。
端口另报`callee_returned`，未返回时保留已执行副作用并停止，不能按HRESULT推断。

## 8. alternate framebuffer阶段

alternate surface mode非零时：

1. rotation amount非零则再次执行source literal旋转和旋转缓存播放，再清pending rotation；
2. 全屏flags 0绘制source；
3. 读取red/green/blue三个i16 factor与signed stage；
4. 每个factor先算术右移一位，再按低32位乘stage；
5. 以红、绿、蓝顺序一次调用已关闭三通道颜色调整，固定`0x3C000`像素。

负奇数factor的右移保持x86算术右移，不使用C++向零除法。乘法保留低32位回绕。

## 9. cadence与stage上限

标准或alternate阶段完成后读取signed cadence dword：

- cadence小于等于1：只执行低32位加1；
- cadence大于1：先置0，stage word执行u16加1，再按signed i16比较；大于2才夹为2，最后cadence加1成为1。

因此stage为`0x7FFF`时加一得到`0x8000`，signed小于等于2，不会被夹为2。modern使用显式bit-pattern回绕。

## 10. fade尾状态机

只有`fade_active==1 && fade_block==0`继续；否则直接返回。

### 10.1 stage小于1

严格清零：

- red、green、blue三个factor word；
- stage word；
- 4A7630实际角色WORD写`0xFFFF`；
- secondary suppression；
- primary suppression；
- alternate surface mode；
- fade active。

不清pending rotation、split状态、颜色循环、cadence、fade block、selected surface或surface表。

### 10.2 stage不小于1

先读取4A7574的active surface token，再执行stage WORD减一并写回。

若token完整DWORD不等于全1且alternate mode为0，则以减一后的stage
读取surface表，effect flags固定0，执行一次虚操作后立即返回；
不执行fallback source blit。token只是哨兵标记，不作为surface数组索引。

否则执行一次全屏flags 0 source blit后返回。surface表越界发生在stage已减一之后；fallback blit typed-stop也保留stage写回。

## 11. closed callee回收

21个call站点分类为：

- clip owner 4次：直接typed状态转换；
- 通用软件blitter 6次：直接typed绘制；
- 旋转缓存单帧1次、播放2次：直接复用已关闭共享cache；
- literal source循环平移2次：直接复用已关闭可写命令流旋转；
- 红/绿/蓝单通道3次、RGB三通道1次：直接typed颜色函数；
- DirectDraw虚操作2次：保留窄平台端口。

已关闭callee没有复制平行实现。函数自身不创建source、frame cache或surface owner。

## 12. caller回收

`0x00453200`在主frame stage之后、条件stage之前直接调用本函数。typed-stop保留主frame stage并阻断全部后继stage、固定帧、跨模块队列、输入与截图。

`0x004527E0`在首张640×480 raw快照转换后、任何场景准备前第一次调用；mode 0第二阶段在首个临时surface操作后、重复场景准备前第二次调用。caller从同一raw快照直接复用已关闭`0x004014F0`编码结果建立可写命令流，不用token冒充主机指针。首次调用typed-stop保留双raw分配、480行复制与转换，不执行合成cleanup。

## 13. 双向追溯

- `0x00453580..0x004535BF`：source发布、全屏clip、双抑制门；
- `0x004535C5..0x00453711`：零/正/负rotation、全图、split带、旋转缓存；
- `0x00453716..0x00453793`：pending清零、颜色循环与delta byte回绕；
- `0x00453799..0x004537D5`：公共全屏clip、角色WORD与完整优先DWORD比较及双stage门；
- `0x004537D5..0x0045380B`：标准staged surface；
- `0x00453810..0x004538AF`：alternate旋转、全图与RGB factor；
- `0x004538B2..0x004538E3`：cadence与signed stage clamp；
- `0x004538E5..0x00453941`：fade门、stage减一、selected surface早退；
- `0x00453942..0x00453968`：fallback全图返回；
- `0x00453969..0x004539A9`：stage小于1的精确终态清零。

C++到LST反向追溯覆盖完整508行、21个静态call站点和22个标签。

## 14. 验证与动态差分

定向测试覆盖：

- source token入口发布、全屏绘制、两条split clip与最终clip恢复；
- extent小于20双倍、20起加22及192冻结三类分支；
- color byte正值归零重载16、`0xFC`负值符号扩展与非零回绕；
- 标准surface stage、effect flags、cadence和stage上限；
- alternate RGB三个half-factor乘积；
- fade stage减一、减一后surface索引、虚调用早退和stage零精确清理；
- surface表首次读取typed-stop前缀；
- 正rotation逐行字面像素结果、空缓存播放与`INT_MIN`非正shift；
- 空source首次全图blit typed-stop；
- 逐帧caller effect时点与阻断后继stage；
- 转场caller一次/两次调用计数及首次effect缓存故障的分配、复制、转换前缀；
- battle聚合目标零warning，普通定向通过。

当前没有原版DirectDraw surface对象、三项surface表、共享source/clip/blitter状态、旋转缓存、颜色格式、角色WORD与优先DWORD、全部阶段word与target framebuffer联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。

## 15. B11灰度分带条件修正

旧C++与本证据第3节曾把`453600 JNZ`方向写反。修正限定于完整DWORD
等于1才进入分带路径，未改变原extent计算、两次裁剪/绘制顺序及公共后缀。
测试从原CMP/JNZ独立覆盖0/1/2/FFFFFFFF，并覆盖unsigned WORD的
0/19/20/191/192/FFFF；extent=0仍保留两次clipped-out调用。
颜色与fade夹具显式选择是否启用分带，不再依赖旧反向默认行为。
完整帧caller同时验证开关0/1的绘制次数与extent写回。

flags=28h对应`421850`的目标灰度化，并非镜像。
`421A38..421A93`按三通道求和后右移2位；RGB555白色7FFF变为5EF7。
新增1×384固定图像验证extent从10增至20后，仅172..211行变灰，
同时核对上下clip边界内外像素；其余开关值保持全部白色。
旧实现运行新增门和extent向量产生7条预期失败，记录在
`build/tmp/runtime/battle-frame-split-gate-red.log`。
修正后`proc_c2b2`的core与ASan setup目标各1/1，SDL构建通过。
新增像素断言后`proc_ee2c`的相同core与ASan目标各1/1，分别4.13秒和6.72秒。
最终日志为`battle-frame-split-pixel-core.log`、`battle-frame-split-pixel-asan.log`
及`battle-frame-split-gate-sdl.log`，均在`build/tmp/runtime/`，无warning/error。
未启动游戏或新增原版动态差分。

实际画布接线仍待完成。surface回调后的stage重读修正见第16节；共享source
重读仍待接线。本批不宣称整个453580重新收敛。

## 16. B11复制返回后的强度与停止传播

独立范围为`453808`、`45393B`两个surface调用和`4538B2`的stage重读。
未改变表索引、两个复制标志、WORD增长/夹取/淡出算法及三个普通出口。

- `453808`正常返回才重读stage，接着读取cadence并执行原增长和fade。
- `45393B`调用前已减stage；正常返回后直接RET，不再次增长或清理。
- 两处都忽略正常返回的HRESULT。端口的未返回标记独立于HRESULT；
  未返回时保留已经写入的像素和状态，阻断本函数及caller后缀。
- 结果保留最近一次端口回复。表索引失败仍为零次调用，未伪装成callee返回。

六组回调向量把stage写为0、7FFF、FFFF、3、4并改变cadence与fade门，
覆盖增长、WORD回绕、signed夹取、终态清理和第二次surface索引。
旧实现产生5条预期断言失败，记录在`battle-frame-surface-return-red.log`。
另以两处调用×返回/未返回×HRESULT 0/1/80004005覆盖12组回复，
并写入固定像素1234检查失败前缀。完整帧caller及转场两个物理caller
分别覆盖后缀阻断，未将函数返回值伪造成游戏业务结果。

首轮core发生段错误；ASan确认新增转场回归使测试函数栈溢出。
新增大型结果改为堆上直接构造，生产算法不因测试栈限制改变。
最终`proc_522c`退出0：core setup目标1/1（4.23秒）、ASan同目标1/1
（6.81秒），SDL链接通过。最终三份日志无warning/error或sanitizer finding，
分别为`battle-frame-surface-return-final-core.log`、
`battle-frame-surface-return-final-asan.log`和
`battle-frame-surface-return-final-sdl.log`，均位于`build/tmp/runtime/`。
首轮core/ASan编译中的既有outcome-resolution数值转换warning未修改。

实际共享状态、source/尺寸/颜色重读、真实旋转缓存及SDL画布接线仍未完成。
WP316仍为pending_audit，未运行游戏或新增原版动态差分。

## 17. B11角色移动与背景平移共用实际位移

`53BD5C`共28个text访问。沿用已有`EffectShiftState.actor_delta`唯一存储，
脚本、动作和转场端口通过virtual state port共同借用。效果context借i32引用，
删除原`FrameEffectState.pending_rotation`副本。转场两次调用及逐帧协调器
在各自入口读取共享量，仍按值传入旋转参数；后续清零操作写回真实存储。
已有位移效果、调试键和全局reset继续使用该存储，不增加逐帧同步复制。

- `4528FE`、`452D6E`、`45331C`取DWORD参数快照；
  `45371B`及`45384A`保持原调用完成后的清零时机。
- 脚本22在`46BB29`写sign-extended参数WORD；两个坐标查询返回后，
  分别在`46BB4E`、`46BBA7`重读DWORD。正常脚本出口保留平移量。
- 脚本40在`46C8D6`或`46C8FA`写目标居中差值或负商，
  `46C93B`、`46C98F`读取同一存储的低WORD。
- 脚本73在`46CA3E`暂停帧，`46CA49`写有符号商，随后发布位置。
  `46CA89`、`46CADD`按低WORD加到角色坐标。删除对`value_a`的错误写入；
  除零仍在发布平移量之前停止。
- 组A动作完成在目标清空调用正常返回后，于`457106`清共享平移量。
  不再清`action_runtime_word`；该字段在`456F4A`对应另一个DWORD `53C02C`。
  目标清空失败时，两者均保留前缀现场。

独立向量及28处访问导航见
`build/tmp/runtime/battle-frame-rotation-sharing-audit.md`。
新增脚本22/40/73各正负一像素向量，以真实命令流确认首像素变化和消费后清零；
脚本22另覆盖0、1、FFFF、8000符号扩展。既有两组角色坐标与寄存器回归继续执行。
坐标发布故障检查脚本73保留原临时量并已写共享商；组A成功/失败收尾、
完整帧正负旋转失败与成功消费、转场抑制及旋转失败均检查共享存储。
坐标查询为已直接组合的封闭callee，源实现按原站点重读，未虚构回调行为。

首轮发现旧脚本73测试也断言错误临时量写入，已按LST更正。
新增转场故障夹具诊断显示：普通截图含8000/C000标记行，非零旋转后在blit停止，
未到达目标缓存调用。原旋转函数要求固定literal行，见
[原literal布局](battle-literal-image-cyclic-rotation-00433f70.md#3-literal行布局)。
该缓存故障向量改用全literal截图，并显式注入动作更新typed-stop；
更新返回0在播放路径实际属于正常结束，不能作为未完成回复。
生产旋转和播放算法保持不变，普通截图格式问题留在后续source接线审计中。
最终`proc_52b3`退出0：core setup 1/1（4.50秒）、actor_frame_316 1/1
（25.70秒），ASan同两目标分别1/1（7.29秒、30.12秒），SDL链接通过。
日志为`build/tmp/runtime/battle-shared-rotation-gates-`前缀的
`core.log`、`actor.log`、`asan.log`、`actor-asan.log`与`sdl.log`。
ASan构建仍有既有outcome-resolution:137的u16到u8转换warning；
没有新编译错误或sanitizer finding。未运行游戏或新增原版动态差分。

仅共享平移量在本批回收。其余效果状态、source/尺寸/颜色重读、真实旋转缓存
和SDL画布接线仍未完成。SDL仍停在45331C；不升级WP316或WP379的验收状态。

## 18. B11动作与脚本触发的闪光共用实际状态

`53BFCC`的28处text访问共用`LegacyBattleScreenFlashState.active`。
`4A75FE`由同一对象的独立BYTE `intensity`持有；静态初值16来自
`4A75FC`的原始字节`01 00 10 01`。相邻低WORD计数的DWORD读取在使用前
均掩码FFFF，+3字节也不覆盖+2，不能把这些字段合成另一个闪光计数。

写入和消费范围：

- 动作清屏的453AFA、453C78、454114、4544D8、454DD3、45515A、
  4554A1、4558DE、455ACD、455BCE，以及敌方456046、45610F。
- 组B完成4581C5，以及效果协调器45C13B、45C25E、45C3DA、45C5C8、
  45C7DE、45C943、45CAD3、45CD52、45CF24、45D0D6。
- 脚本10在46AF69、46AFD7写1。46AFFD..46B00B只清工作字、
  53BFC0并恢复帧开关，没有清闪光；删除旧实现返回时的额外清零。
- 全局reset的45BA3E写强度16、45BC5D清触发；增加4A75FE映射，
  不再另存未映射字节副本。释放画布提前失败不执行这两个写入。
- 453716先读触发DWORD，45371B清共享平移量，只有精确1才进入颜色分支。
  453725符号扩展强度，三通道调用正常完成后，45377B重读BYTE并加FC。
  结果为零时45378C恢复16，453793清触发；颜色失败不执行衰减后缀。

动作、脚本、效果调用、转场和reset通过virtual state port借同一对象。
核心帧及两处转场效果context直接借引用，外部绘图端口不持有另一份状态。
组B单效果适配器把const/非const访问都转发到实际动作端口。
旧效果、脚本、协调器、组B触发副本已删除，动作清屏改写共享对象。
特殊动作405的473411实际写53BF94，不能按名称把此处迁成闪光。
当时保留的`frame_refresh_pending`副本现按第20节回收到实际抑制门。

清屏前发布及容量失败保留行为不变。45515A先于47CC40；该callee
在47CC40..47CC4A只把参数写入角色+2AD0，不访问闪光存储。
reset批量写入与类型别名发布之间没有外部调用，不新增逐帧同步副本。
完整访问导航见`build/tmp/runtime/battle-frame-flash-writer-windows.lst`，
独立向量见`battle-frame-flash-sharing-audit.md`。

新增固定状态验证：DWORD门0/1/2/FFFFFFFF、强度0/1/4/80/FC、
16→12→8→4→16的四帧消费、RGB555实际像素、抑制和颜色失败保留。
脚本10两组角色各覆盖真实颜色消费后的正常返回及frame typed-stop；
核心协调器验证消费清零，两处转场验证连续衰减和失败保留。
动作、敌方、组B清屏测试改为检查实际共享触发，reset覆盖成功与提前失败。

首轮core构建通过，旧reset测试仍从未映射表读取4A75FE而失败；
已改为检查实际强度及未映射表无重复副本。随后定向core通过。
`proc_82db`退出0：core setup 1/1（4.63秒）、actor_frame_316 1/1
（26.71秒），ASan同两目标1/1（7.94秒、26.55秒），SDL链接通过。
日志为`build/tmp/runtime/battle-shared-flash-final-`前缀的五份日志。
ASan保留既有outcome-resolution:137数值转换warning；新增脚本测试
WORD循环的转换warning已改为显式WORD数组。`proc_0924`重跑受影响
setup目标，core 1/1（4.71秒）、ASan 1/1（7.55秒），两份
`battle-shared-flash-verified-{core,asan}.log`无warning/error或
sanitizer finding。actor目标与SDL源文件未因该测试类型修正变化。

本批只回收闪光开关与强度。其余颜色发布、抑制门、stage、source重读、
真实旋转缓存及SDL画布接线仍待完成；不升级WP316或WP379验收状态。
未运行游戏或新增原版动态差分。

## 19. B11画面刷新阶段与画布标记共用实际状态

`53BF44`的WORD阶段与`4A7574`的DWORD标记共用既有
`LegacyBattleFrameRefreshState.refresh_pending/active_surface_token`。
全部13个text访问见`build/tmp/runtime/battle-frame-stage-token-lst-references.txt`；
独立范围、位宽、分支与停止条件见`battle-frame-stage-token-sharing-audit.md`。
4A7574的data初值为FFFFFFFF；不把WORD阶段当作布尔值归一化。

- 45B0A6、45B0B6发布阶段1与最终画布token，均先于最终lock/unlock。
  动作14在454977调用刷新，45497F返回后重写同一阶段1。
- 4537DD、453877按signed WORD消费阶段；4538B2在复制或颜色处理
  正常返回后重读。实际状态持有u16，比较与乘法显式bit_cast为i16。
- cadence增长先4538CA写WORD回绕结果；signed大于2时再4538D6写2。
  7FFF增长成8000仍保留，不现代化成无符号夹取。
- fade的453908先快照标记，再45390E减WORD、453913写回。
  标记只和FFFFFFFF比较；复制表仍按stage索引，不按token或pitch索引。
  45393B正常返回直接RET，保留回调对实际阶段的后续改写。
- stage小于1时45397E清实际阶段；其他终态写入维持既有顺序。
- 全局reset只在45B8CB恢复tokenFFFFFFFF，没有写53BF44。
  增加4A7574映射，删除未映射表中的重复字节；不额外清阶段。
  释放提前失败保留标记及阶段，类型发布前无外部调用。

效果context直接借刷新状态；核心帧与转场两处调用借同一port对象。
转场、reset和动作端口通过virtual state port共同借用；getter支持
const/非const虚转发，组B单效果适配器直接转发实际动作端口。
删除`FrameEffectState.stage/selected_surface_index`副本，不增加逐帧同步。
其余颜色因子、抑制门及资源状态未在本批合并。

新增实际刷新callee→增长→淡出回归，不手工复制刷新发布的阶段与标记。
固定token 0/1/80000000/FFFFFFFF确认只有全1走背景，其他值按stage复制。
复制回调改写8000后仍直接返回；既有六组回调、12组回复矩阵迁到真实owner，
继续检查WORD回绕、signed比较和失败前缀。完整帧与转场检查共享阶段，
reset验证非零阶段保留、token复位、未映射表无副本和提前失败保留。

`proc_61ed`的集成core setup 1/1通过。`proc_0e29`退出0：
core setup 1/1（4.59秒）、actor_frame_316 1/1（25.08秒），
ASan同两目标1/1（7.65秒、26.21秒），SDL链接通过。
日志为`build/tmp/runtime/battle-frame-stage-token-final-`前缀的五份日志。
ASan仍有既有outcome-resolution:137的u16到u8转换warning；
没有新增warning、编译错误或sanitizer finding。
双向审查后补齐4538CA先写回绕结果、4538D6再按signed比较写2的顺序。
`proc_90bf`复验受影响目标：core setup 1/1（4.61秒）、ASan setup 1/1
（7.67秒），SDL链接通过。日志为`battle-frame-stage-token-verified-`
前缀的`core.log`、`asan.log`、`sdl.log`。core和ASan均只有上述既有转换
warning；没有新增编译错误或sanitizer finding。actor目标的接口及处理不变。

本批仅回收刷新阶段与画布标记。颜色、抑制门、source重读、真实旋转缓存
和完整SDL画布接线仍未完成；SDL仍停45331C，实际续玩尚未验收。
未运行游戏或新增原版动态差分，WP316/WP379验收状态不变。

## 20. B11双抑制门与当前颜色共用实际存储

独立核对53BF94/98的全部32个text访问，以及53BF36/38/3A的69个
text访问。五字段由`LegacyBattleFrameEffectControlState`持有：
两个u32抑制门，三个i16颜色因子。静态PE检查确认五字段均位于
`.data`虚拟尾部，loader zero-fill；默认全零，不借用历史颜色快照。
检查日志为`build/tmp/runtime/battle-frame-effect-control-initial-image.log`。

### 20.1 双门的全部访问与对应行为

下列地址按完整机器指令逐站回收，不把同名成员当作同址证明：

- 4535A0在入口clip前快照primary；4535B9在clip后读secondary。
  双零才绘制背景、旋转、分带及闪光；任一非零跳过该入口段。
- 4537C1、4537C9重新读实际双门；只有完整DWORD精确1进入阶段路径。
  45398E先清secondary，453994再清primary；终态清三色仍保持原序。
- 动作4545DE、4548CE、4549B6清primary，分别由XOR EAX、XOR EDX、
  两条入口上的XOR EDI产生零；对应三色也清零。
- 动作4546E1、45484E、454971、455359、4556D7写primary 1。
  4539FB建立EBX=1；各可达分支保留它，三色均写WORD FFF4。
  动作14在刷新返回后45497F重写阶段1；不改变既有失败前缀。
- 敌方45648F写primary 1，4564CA清primary。455DC6建立EBX=1；
  4564BB清EAX后才到清门站点。后者不清三色，保留原写集合。
- 组A4566BC、4566C4与组B45820D、458215读双门；只有精确1开启
  角色效果模式。组A收尾4572FE只查primary，组B4580CA只查secondary。
- 单效果45890C、458911在刷新返回后依次写双门1；群体效果
  459162、459167在刷新前依次写双门1。后者不覆盖最终回调改门。
- reset 45BC0F、45BC15清实际双门；菜单461E3D仅在动作种类2时
  清primary，secondary保持。删除input的`selection_mode_cache`副本。
- 405的473411和动作4/404的474839，在刷新返回后检查当前三色，
  任一非零才写primary 1；全零保留原值，不主动清门。
- 组B475C9D按同一条件写secondary 1。删除旧误接：BF98不是
  `active_effect_gate`，该字段其他站点仍对应独立BF74选择门。

53AF68的27个text访问另行核对，其中45C0D7..45D093的12个反馈
读取继续使用协调器自己的`primary_suppression`成员。它不属于BF94。
灰度分带BF30、选择BF74及两门均保持独立，不按字段名整体合并。

### 20.2 三色的全部访问与实时重读

69个站点分为下列完整集合；三色按WORD存储，使用时按i16解释：

```text
帧效果读：453870 / 453883 / 45388B
帧效果清：453969 / 453970 / 453977
动作清：  4545E3 / 4545E9 / 4545EF
动作写：  4546C1 / 4546C7 / 4546CD
动作写：  454835 / 45483B / 454841
动作清：  4548D4 / 4548DB / 4548E2
动作写：  454958 / 45495E / 454964
动作清：  4549BC / 4549C3 / 4549CA
召唤写：  455347 / 45534D / 455353
402写：   4556BD / 4556C3 / 4556C9
敌方写：  45647D / 456483 / 456489
单效果写：4588EB / 4588F1 / 4588F8
群体写：  459147 / 459154 / 45915B
刷新比较：45AF96 / 45AFA6 / 45AFB6
刷新循环：45B00D / 45B02B / 45B048
刷新尾读：45B079 / 45B080 / 45B086
复位清：  45BAAC / 45BAB3 / 45BABA
405写：   4733CC / 4733D9 / 4733E7
405比较： 4733F6 / 4733FF / 473408
4/404写： 4747F4 / 474802 / 47480F
4/404比： 47481E / 474827 / 474830
组B写：   475C58 / 475C66 / 475C73
组B比较： 475C82 / 475C8B / 475C94
```

完整45AF90..45B0D7复核后，刷新接口只接实际port，不再接三色按值参数。
每轮红读取后有红callee，绿读取后有绿callee，蓝读取后有蓝callee；
下一轮从当前状态重读。SAR半值和低32位乘法回绕保留，不缓存入口颜色。
最终45B079、45B080、45B086按绿、红、蓝重读，分别保留WORD位形。
先发布绿、红快照，再写阶段1，最后发布蓝快照与画布token。
最终lock/unlock可以改变当前颜色，不能回写覆盖此前快照。

角色配置的`shared_word_36/38/3a`仍是配置来源；4A7632/34/36仍是
刷新比较快照。二者都不代替当前颜色存储。FrameEffect及SharedEffectFrame
中的同址三色和双门副本删除，405的`frame_refresh_pending`删除。
动作、效果调用、菜单、转场和reset虚继承同一状态端口；核心帧及转场
context直接借引用，组B单效果adapter的const/非const getter均转发。
不新增逐帧复制，也不把绘图端口改成另一业务owner。

reset新增两个mapped范围，114→116。原234项写序、3300次物理写及
13106字节不变；五字段恢复零，14个地址字节不留unmapped副本。
阶段和历史颜色快照不在新增清零集合；提前释放失败保留五字段。

### 20.3 独立回归、门禁与剩余范围

双门0/1/2/FFFFFFFF的16组矩阵区分双零与精确1；RGB555红在bit10、
蓝在bit0，delta 1/2/3的像素为0443，同时独立断言三通道delta。
双方完整收尾各验证四个门值；另一门及BF30为1不能触发错误淡出。
实际单效果/群体效果调用验证两种相反的门发布顺序、signed配置和快照。
刷新回调逐站改色，两轮RGB参数为3/-5/-6及-32768/32766/-2；
最终快照-3/5/-7在lock回调清实际颜色后仍保留。
动作4/404、405和组B覆盖刷新后清零/改非零；菜单、reset、核心帧、
转场及失败停止回归均借实际存储，已有HRESULT及阶段回绕向量继续执行。

初次矩阵七条断言失败来自RGB555预期颠倒红蓝，已改独立预期，未改算法。
初轮final core setup 1/1通过，actor目标编译发现新夹具复制含独占资源
的角色状态；改为新建角色并只复制动作配置，不改变生产实现。
此前仅setup通过不能证明该actor目标内的动作4/404、405和双方收尾通过。

最终core setup 1/1（4.66秒），actor_frame_316 1/1（26.39秒）；
ASan setup 1/1（7.89秒），actor_frame_316 1/1（26.95秒）；SDL链接通过。
日志为`build/tmp/runtime/battle-frame-effect-control-final-`前缀的
`core.log`、`asan.log`、`actor-asan.log`、`sdl.log`，以及
`battle-frame-effect-control-verified-actor.log`。
仅既有outcome-resolution:137的u16→u8编译warning，无新增错误、断言失败
或sanitizer finding。只格式化本批修改范围，MON既有差异保留并排除发布。

本批仅回收双门及其必要的颜色发布依赖。实际画布、source尺寸/格式重读、
真实旋转cache、actor selector、完整SDL帧和实际续玩仍待完成。
WP316保持pending_audit、315/422；整体暂估60%（粗略工作量，非验收比例）。
未运行游戏或新增原版动态差分，不升级WP316/WP379验收状态。

## 21. B11画面效果借实际角色WORD及优先角色DWORD

### 21.1 原物理读取、终态写序与初值

4537A5的416FF0返回后，4537AA对4A7630 WORD做MOVSX到EDX，
4537B1读取53AE70完整DWORD到EAX，4537B9执行CMP EDX,EAX。
4537BB不相等去4538E5；相等后才逐个检查BF94/BF98是否精确DWORD 1。
因此FFFF只匹配FFFFFFFF，不匹配0000FFFF；8000只匹配FFFF8000，
不匹配00008000。优先角色不是低WORD，比较也不带范围归一化。

453969..45397E按红、绿、蓝、stage清零，453985才写角色WORD FFFF；
45398E清secondary，453994清primary，然后清alternate及fade。
它不清53AE70。453941、453968正常早退，以及先前typed-stop均不补清角色。
原surface调用返回后的颜色、阶段与cadence重读保留；不重做角色比较。

静态PE初始像：4A7630是FFFF，53AE70是零填充DWORD 0。
451B3F清EDX，451C14 OR FFFFFFFF，451C79再写优先角色FFFFFFFF；
中间没有callee或EDX改写。loader初值与入战后值不能混为一项。
静态导航共38个角色WORD text访问和40个优先DWORD text访问；
该计数不是其他函数完整语义关闭的证据。

### 21.2 唯一输入存储与三个核心调用站点

删除FrameEffectState的current_encounter_id/expected_encounter_id副本。
FrameEffectContext借ActionDispatchState.current_actor_index的u16引用，
以及ActorMetricState.priority_actor_index的const u32引用。
MOVSX通过bit_cast i16再扩到i32，转u32后比较完整位形；终态直接写该u16。
两项都不在入口按值复制，且终态不改优先DWORD。

453322核心caller直接借context.action_dispatch与port.actor_metric_state。
452904/452D75两处转场caller借同一实际动作对象与metrics；转场接口以完整
ActionDispatchState替代原单独selection_gate引用，原选门两站仍写其
既有action_pending_aux。全部现有九处转场测试caller已迁移，不构造业务副本。
脚本闪光、平移及刷新后效果的夹具也借实际动作/指标对象。
SDL完整帧仍停45331C，本批没有把库内caller验证称为SDL实机接通。

45B989原本已经列入reset有序写和mapped范围，但实际动作对象漏了写回。
现在在既有映射阶段恢复current_actor_index为FFFF；优先七DWORD仍按
45B701清零。mapped范围仍116，原234项、3300次物理写及13106字节不变；
该WORD的两个字节不留unmapped重复像，提前释放失败保留角色与优先值。

### 21.3 优先角色的实际写入与清写

删除ActionDispatchState.active_effect_target和组A的active_effect_tail。
敌方动作、双方角色帧、最终角色清理、战后重排及脚本78的53AE70访问
直接使用同一ActorMetricState，不在回调之间同步副本。
FinalActorStepState.active_actor_code的非53AE70用途保留；不按字段名合并。

必要改写站点按原顺序对应：

- 455D60：456554比较实际DWORD，45655C命中后写FFFFFFFF。
- 456680：456DE8比较后，456DFB查询idle返回只测试其结果，
  不重复先前CMP；456E9C在下一站重新读取priority。
  456EF8读priority低WORD，456F09发布为当前角色；
  4570BF的另一次角色WORD发布保持原分支。
  4572AB reset返回后，4572C0清priority及其六DWORD尾；
  457320随后写FFFFFFFF；45765C的清写仍在原分支。
- 4576A0：45774B读取，457781/4577C2在两条早退中清FFFFFFFF。
  457E51的比较值保留到动作查询的EAX和flags，不在callee后重读。
  458085在收尾回调之后读DWORD，458090 CMP8与4580A5 JGE为signed；
  仅小于8时执行4580B3七DWORD清写，再于4580B5写FFFFFFFF。
- 45AA00：45AB83/45AB90匹配清写、45AC07终止清写与45AC3E
  继续发布均读写实际priority；其他角色字段不替代该DWORD。
- 45ADF0：45AF28仍在全部前置callee正常返回之后才写FFFFFFFF。
- 脚本78：46DCC9清EDX，46DCD0把操作数WORD读入DX。
  实现于动作模式callee返回后重读workspace的高WORD，保留该零扩展值；
  完成原两组工作区与队列清写后，46DD34发布实际priority。
  失败RET不执行后缀。脚本77只写模式bit40，没有此priority发布。

调试45E2F1的MOVSX也改借实际current_actor_index，删除battle_selector。
前一段文字绘制回调改写角色后，摘要显示重新读取的signed WORD。
45E210的priority显示与其另一项53BD54显示分别保留，不能按名字混同。
53BD54的其他既有存储关系不在本批关闭范围，须另行审计。

40个显式priority访问按原函数分组：451B10(1)、453200(3)、453580(1)、
455D60(2)、456680(6)、4576A0(7)、45AA00(4)、45ADF0(1)、45B280(1)、
45B630(1)、45C010(1)、45D8F0(5)、45DEE0(1)、45EA80(1)、45EC80(1)、
466F70(3)、469D20(1)。既有初始化、出队、指标、热键、消息、反馈和完成
路径已借metrics；本批只收回上述错用副本的站点。
38个显式角色WORD访问分属453580(2)、4539B0(20)、455D60(5)、
456680(3)、4576A0(6)、45B630(1)、45DEE0(1)。动作原WORD发布保留，
效果消费/终态、reset及调试显示借同一实际动作状态。
这些数量只用于检查遗漏，不升级各函数的完整语义或动态差分状态。

### 21.4 独立向量与验证范围

14组MOVSX/CMP向量覆盖0、7FFF、8000、FFFF及完整DWORD高位差异，
context建立后修改原存储，证明借用而非快照。
两组旋转callee回调改变角色、优先值和画面门，后续比较使用新值；
surface回调再次改成不相等，已选分支仍按原顺序增长，且保留两项新值。
24组终态向量覆盖signed非正stage、精确fade门与阻止门，两条操作数保持分离。
缓存及首次source失败覆盖终态写前停止，既有surface HRESULT矩阵继续执行。
核心帧消费实际对象并保留未返回surface前缀；转场首个终态对场景回调可见，
回调发布8000/FFFF8000，第二次效果据此选择stage surface，再阻断其场景后缀。
reset验证正常WORD复位、优先清零、两个失败槽的保留及重复像排除。

新增脚本78正常/失败各两组，效果context先建立再执行实际producer，
观察随后surface分支与阶段；按4538BE/C0设cadence为2，符合增长前提。
组A检查七DWORD正常清写和callee失败前保留；组B七组priority覆盖
0、7、8、7FFFFFFF、80000000、FFFF8000、FFFFFFFF，并在收尾回调改写，
检查signed比较两侧与完整六DWORD尾。调试四组WORD覆盖符号边界及回调重读。

首轮proc_1677通过不覆盖随后producer修改。后续core失败已定位到夹具：
原协调器同时出队角色5、使用旧角色0副本；改为实际出队角色0并检查坐标0。
调试向量补总显示门，画布端口明确正常返回，阶段向量补原cadence前提。
proc_be02的core setup为1/1（5.04秒），actor316两条旧存储断言失败；
分别补实际priority=0的CMP输入、把末尾断言迁到真正的priority字段。
最终producer门禁proc_f040退出0：core actor316为1/1（27.40秒），
ASan setup为1/1（8.25秒）、actor316为1/1（27.31秒），
SDL于129/129链接完成。core setup沿用同源码proc_be02的1/1（5.04秒）；
后续仅修改actor316两处旧存储夹具，没有改变setup源码或生产实现。
仅既有outcome-resolution:137转换warning，无新增错误或sanitizer finding。
当前源diff488行、测试diff2179行已分小块完整阅读；格式化后逐字节相同。
本批增改按21.1–21.3的LST站点反查，复核入口、比较快照、callee后重读、
角色终态及早退/typed-stop后缀；最后一轮未产生本批新的实现差异。
该结论限定于角色WORD/priority DWORD共享，不升级各函数整体审计状态。
七DWORD记录清写已验证；AE74..AE88邻接业务字段的其他别名仍须独立核对，
不能据此认定这些字段的全部消费者已共用存储。
本批只恢复上述消费、必要producer与复位合同；画布、source重读、
初始化门、真实旋转缓存、SDL完整帧和实际续玩仍待完成。
WP316仍pending_audit、315/422；未运行游戏或新增原版动态差分。

## 22. B11画面淡出借实际颜色初始化门

### 22.1 原访问、初值与复位

`0x0053C030`是DWORD，PE初值为零填充`00 00 00 00`。
当前LST九处显式text访问：4534B0、4534CB、4538F7、458B8F、
4593A4、46FAF0、473103、474724、475A57。计数只作遗漏导航。

4538EB先比较53BFEC与EBX=1，不等直接4539A6；随后4538F7
把53C030与EDI=0比较，非零也直接4539A6。门零才消费已加载的
signed WORD强度，进入下降或453969终态。不能截断DWORD或改成精确1。
453941、453968正常早退及453969..4539A9终态均不写53C030。
前置typed-stop阻断后缀，不补执行下降、终态或门清零。

4534AE先按signed计数与零比较；计数<=0且4534B0的门精确1才
调用45D3E0，参数24,24,24,0,0,0,8。4534CB在callee返回后清门，
随后4534D2才更新颜色累积。该阶段在453322画面效果之后，不能提前清门。
45B630原写集合不含该门；既有reset测试同时检查颜色清零与门9保留。
本批不改变234项物理写、3300次写入、13106字节或116项映射范围。

### 22.2 唯一存储与调用方

删除`LegacyBattleFrameEffectState.fade_block`。
效果context借`const u32& color_initialization_gate`，绑定端口实际门；
建立context不复制值，4538F7对应位置读取callee改写后的值。
453322核心帧与452904/452D75转场均绑定同一端口实际颜色门。
转场端口虚继承既有颜色状态接口，共用其生命周期与PE初值，不加每帧同步。
脚本和刷新测试聚合端口显式提供颜色状态，未向生产脚本添加无来源字段。

六个producer沿用既有真实端口写入及原顺序：
458B8F/4593A4在45D3E0返回、清记录bit400后写1；
46FAF0/473103/474724/475A57在45D3E0前写1。
颜色累积算法、七signed参数、尾寄存器与原失败前缀未改动。

审计中曾把组B的4599B0待执行单体效果误当成4582B0颜色效果入口。
前者没有45D3E0调用；新增颜色断言因此失败。
已撤回对应适配器和夹具改动，两文件与本批基线逐字节相同。
未把仅继承而未调用的接口当成必须转发的依赖。

### 22.3 独立向量与验证

25组向量交叉DWORD门0/1/2/80000000/FFFFFFFF与WORD强度
0/1/2/FFFF/8000。context绑定后改门，零门才下降或终态，
非零门保留颜色、强度、角色及画面门。既有24组精确fade门矩阵保留。
十组surface正常返回/未返回及改门向量检查原读取时机与失败前缀；
两组旋转callee改门，后续下降读取实际1。没有在终态额外清门。
核心十五组计数/门组合检查效果先等门、颜色累积后清门。
转场三组入口门分别检查首个结果可见及第二次surface停止的失败前缀；
它们在第二次4538F7之前停止，不能证明后续读门。
最终审查补三组正常到达4538F7的向量：第一次门1等待，
场景回调写0/1/FFFFFFFF；两次效果均正常返回，第二次仅零门执行终态。
不以停在读取之前的测试冒充后续读门覆盖。
正常早退、surface HRESULT、source失败与reset保留回归继续执行。

首轮proc_e19c编译失败：脚本测试聚合端口没有颜色状态接口，
已补显式来源；后续lambda诊断随首个错误消除。
proc_fbe6的core setup通过1/1（4.72秒），actor316只有上述错误夹具失败。
撤回后proc_6350全部通过：core setup 1/1（4.84秒）、
actor316 1/1（25.28秒），ASan setup 1/1（7.90秒）、
actor316 1/1（26.34秒），SDL于130/130链接完成。
仅既有outcome-resolution:137转换warning，无sanitizer finding。
补充转场读门向量后proc_95d3的core setup 1/1（5.02秒）、
ASan setup 1/1（8.67秒）通过；该测试注册在setup目标，actor316不含该文件。
生产代码及actor316测试未变化，先前actor316与SDL结果仍覆盖当前版本。
proc_c75a只格式化十项明确C++路径的新增范围，MON排除；
代码、字符串及注释机械一致，空白变化不改变上述验证版本的行为。
完整源、测试及相关文档差异重新阅读，按22.1的全部门访问与出口反查；
该字段共享的最终正反向审查没有新增差异，不升级各函数整体状态。

本批范围只关闭初始化门共享。真实旋转缓存、source重读、实际画布、
SDL完整帧及实际续玩仍待完成；原版联合捕获后端仍缺失。
WP316仍pending_audit、315/422，不开始317。

## 23. B11画面效果借背景初始化的实际旋转缓存

### 23.1 同一对象与原读取点

本批从LST proc到endp逐段核对451420、451540、4515E0、451730、
451940及453580，导航行数分别为135、65、164、57、103、508。
旧绘制、释放和背景证据多计五行，已按实际物理范围修正；行数不升级语义等级。
4FDFA8的六处显式text引用连接同一缓存对象：

- 451960→451965：背景初始化先释放旧缓存。
- 4519F5→4519FA：加载和旋转正常返回、两个WORD门满足后初始化缓存。
- 4535F0→4535F5：零旋转背景绘制后读取并绘制该缓存。
- 453706→453711：非零背景旋转、绘制正常返回后播放该缓存。
- 453840→453845：另一画面分支的非零旋转后仍播放同一缓存。
- 45B638→45B63D：全局复位在显示释放后、几何释放前释放该缓存。

IDA合并的dword_4FDFA8[387]不作为对象布局依据。
缓存前0x98字节是动作记录，+9C..+B0是六个owner槽；
+B4/+B8为坐标DWORD，+BC为位移DWORD，+C0为存储动作WORD。
451540先读WORD零门；非零时零扩展写动作号、清base variant并更新记录，
忽略正常EAX零返回。随后读取更新后WORD帧号与真实缓存，成功后返回+8C。
4515E0非零动作先清0x98记录，按signed旋转方向消费真实可写图像；
正常完成再次清记录，保留六槽及扩展字段。等待WORD清写、重复帧跳过、
正常零返回和typed-stop保持既有区别，不为共享改动重排这些算法。
453711/453845未返回时，父调用不补执行后续平移清零。

### 23.2 实现与必要caller

删除LegacyBattleFrameEffectState的rotation_cache值成员。
LegacyBattleFrameEffectContext必需借LegacyBattleActionRotationCacheState&，
三个消费站点均直接传该引用，没有入口复制、每帧同步或默认零缓存。
453322核心caller借context.startup.background_rotation_cache。
452904及452D75两个转场caller借同一startup.background_rotation_cache，
不把首次调用时的动作号、记录、帧资源或扩展字段冻结为快照。
入口rotation参数仍是原snapshot，后续真实缓存读取与它分别保留。

背景初始化与全局复位已经使用该startup缓存，未新增producer或重复释放。
SDL背景初始化已借实际ACT updater与会话内旋转资源；
本批未改SDL主循环，SDL仍停45331C，不能把链接通过称为完整画面接线。
source token、bytes、布局、尺寸的原站点重读另批继续。

### 23.3 独立回归与失败收敛

两种完整动作值1/1234FFFF与六帧索引0..5交叉，共十二组。
context先绑定再初始化：检查低WORD动作、初始化清记录、实际updater记录身份，
随后以正常EAX零绘制1357像素并返回CAFEBABE。
释放同一缓存后旧context立即观察WORD零门，不再更新或绘制缓存，
释放顺序为image→owner，不恢复动作或资源，背景像素为001F。
旧实现proc_b7e3出现二十四条对应失败：十二条绘制、十二条释放可见性。

两个播放站点各覆盖1、-1、INT_MIN，共六组。
检查入口真实0x98记录清零、右/左/非正shift像素3/2/1、
等待清零、完成记录清零与六槽、坐标、BC、C0保留。
索引5空owner和索引6未知访问保持更新后的真实记录与父平移量，
不补reset或后缀。核心三组0/1/-1检查updater未返回前缀，
播放路径入口清记录，零旋转路径保留未清字段，均阻断后续角色和绘制。
转场三组在首场景回调把C0改0/2/FFFF并把帧号改1；
两次效果均正常到达，第二次按同一对象的新WORD和帧槽消费。
首场景检查只执行一次，保留第二次prepare_scene的原调用。

proc_0710首轮在转场断言后崩溃，后续四项未执行。
proc_6de6查实栈溢出位于转场角色调用链；新增大夹具移到堆上直接构造。
两条断言由第二次prepare_scene覆盖首次观察值造成，夹具按真实次数修正。
没有为通过测试修改生产算法或扩大宿主栈。
proc_93b3最终五项通过：core setup 1/1（5.63秒）、actor316 1/1（26.80秒）；
ASan setup 1/1（9.94秒）、actor316 1/1（27.62秒）；SDL130/130链接完成。
最终日志无warning/error或sanitizer finding；失败初轮只另有既有转换warning。
proc_4dd2限定九项C++路径格式化，代码、字符串及注释机械一致，MON排除。
上述测试提供固定状态与像素证据，没有运行游戏或新增原版动态差分。

### 23.4 审查范围与未决合同

LST到C++沿六个对象引用、三个消费站点与全部父返回路径核对；
C++到LST反查引用接线、WORD动作门、正常返回与typed-stop前缀。
绘制、播放及释放正常完成直接写同一对象，无新增物理复位写。
本批只关闭缓存共享，既有函数整体状态不升级。

释放接口目前为void noexcept，没有独立未返回回复。
451752/45175D原路径重读owner，45175F再次检查零门；
现有释放实现未显式建模回调把owner槽改为零后的第二零门。
该独立缺口登记保留，本批只验证普通释放对借用者立即可见，
不宣称任意释放回调改写或未返回域已完成。
原版共享缓存、ACT寄存器、资源与画布联合捕获仍为blocked_runtime_oracle。
其余共享状态、source重读、实际画布、完整SDL帧和实际续玩仍待完成。
WP316保持pending_audit、315/422，不开始317。

## 24. B11画面来源与尺寸按原站点读取

### 24.1 两项存储与消费点

502940背景记录与4CD730绘制来源分别保存，不能以一个入口快照替代。
背景记录由startup.background.image_record唯一持有五DWORD；
绘制来源由共享LegacyBlitRequest.source_token持有，效果state不另存副本。
来源接口只借该记录和图像查询端口，不复制token、bytes、布局或尺寸。

453580入口读取背景图像，453596发布绘制来源，45359B设置clip。
此处不解析图像；双门抑制后仍保留发布，未知图像只在实际消费点停止。
六次绘制独立读取尺寸：宽度取DWORD低WORD，高度另取高WORD并零扩展。

- 4535CD/4535D5：初始全图绘制。
- 453644/453652：上带；45364A取背景token，453665重新发布。
- 45368D/45369B：下带；453692取背景token，4536AD重新发布。
- 4536E3/4536EB：图像旋转返回后。
- 453850/453858：可选旋转与缓存播放返回后。
- 453942/45394A：淡出已减stage后。

最后三处不重新发布背景token。45157A、45168F的缓存callee发布实际帧
到同一绘制来源，后续blit必须沿用该帧，即使当前背景记录已经变化。
4536BF/4536CB及453816/453822的旋转参数另读当前背景token。
入口rotation仍按signed快照取方向；负量低32位取负，INT_MIN不修复。
433F7A/433F7D先按signed判断shift，非正值直接返回；433F83才取图像
指针，433F87才读取首WORD。因此INT_MIN取负回绕后不查询图像分配，
只保留旋转调用；后续绘制仍在自身消费点查询当时已发布的来源。

4170E0取当前绘制来源，4170E8立即读首WORD，早于尺寸、裁剪和opacity
早退。查询未知身份或不足首WORD时停止，保留此前发布、clip、分带增长、
stage减一和已经完成的像素。通用像素算法不在本批修改范围。

### 24.2 背景记录producer与生命周期

433380的217行完整范围为96192..96408，433540的234行为96417..96650。
4333F3取局部var_44地址传433540；其+12h/+14h尺寸写的是局部六DWORD
记录，不能当成502940的布局。433501..433530再向最终五DWORD记录发布：
图像、低尺寸WORD、高尺寸WORD、+4、+8、+10h，保留该写序。

4332A0完整69行为96051..96119。4332DC写局部存储深度；深度精确8
时分配200h调色板，4332F8写局部+4，433305读入。43352D将该var_40
指针发布到最终+8，当前背景记录第三DWORD保存同一身份。
现有archive加载适配器沿用已验证的物理文件读取与palette结果；
没有在本批关闭原全局预备模式的额外I/O、缓冲复用或全部失败语义。

401C70把同一背景记录交给转换。非8位路径按图像头重写两个尺寸WORD，
再原地转换实际分配；后续缺行头保留已完成literal像素写入。
8位路径按头部宽高计算低32位width*height*4+800h分配临时转换身份。
转换完成先释放旧palette及图像，再为实际输出长度建立独立最终身份；
最终分配失败保留旧记录指针及已释放前缀，不把临时身份发布为来源。
成功后先写长度，再替换图像指针，最后清palette指针，尺寸保持加载值。
不能从转换后vector的头或最终尺寸反推两种分支的记录字段。

图像与palette身份使用reserve_legacy_guest_bytes，不截断host指针。
背景持有者管理实际字节与分配身份；记录只是原读取的共享字段。
451940先释放旋转缓存，再按背景token非零释放图像并清该槽；
load失败不清其他四DWORD，不补转换、除法、旋转和完成WORD。
未知释放身份在实际释放域停止，不交给仅管理旋转帧的释放端口。

451B10仍在原五DWORD写点清同一记录；45B630仍先读取图像token并调用
条件释放，再执行原固定写程序。删除startup.reset.values_502940副本，
不增加物理写、范围或清写其他画面状态。

### 24.3 真实caller与测试

453322核心帧直接借startup背景记录、字节分配和同一旋转缓存。
452904、452D75两处转场使用同一背景；4528D7、4529C3的截屏仍是独立
转场资源，不能以primary图像或固定640×480替代背景来源和尺寸。
核心、转场、刷新、脚本、startup和reset相关夹具已迁移到实际存储。

旧实现proc_5097的五条来源断言失败，前置断言均通过。
覆盖动作更新后上带换图像、surface返回后新尺寸配旧来源，以及
alternate播放后的1、-1、INT_MIN三种缓存像素3/2/1。
另覆盖surface直接改绘制来源、空owner但图像仍保留的查找，
三种零尺寸组合与三种opacity值的九组首WORD读取门。
producer回归区分文件尺寸9×7与图像头2×1，WORD分支重写、indexed
分支保留；缺后续行头时已发布记录和已转换22/44像素保留。

proc_35d5旧播放夹具缺图像编号，且背景分配误送旋转专用释放端口，
出现三条断言后中止。补齐身份并归还释放职责后，proc_a37c只余转场
literal夹具两条失败，修正实际背景而非仅截屏。proc_bce3的indexed
夹具使用保留marker值，改普通索引4/5；没有反改生产算法通过断言。

proc_dbc6五门全部通过，288秒exit0：core setup 1/1（6.63秒），
actor316 1/1（28.38秒）；ASan setup 1/1（11.22秒），
actor316 1/1（29.28秒）；SDL233/233链接完成。
ASan和SDL仅有既有world-map两条warning，ASan另有既有outcome转换
warning，无sanitizer finding。构建、测试与游戏运行证据分别保留。

复核401DFF/401E07/401E0D发现最终图像不能沿用临时转换身份。
修正释放与最终分配顺序，新增最终输出分配边界回归。
proc_b52d，196秒exit0：core setup1/1（5.57秒，总5.63秒），
ASan setup1/1（10.49秒，总10.54秒），SDL233/233链接通过。
仅复验受影响的producer及setup；actor316未重跑，保留前述独立结果。
core/ASan仍有既有world-map两条及outcome一条warning，SDL两条，
无sanitizer finding。

最终审查补查433F70早退，修正INT_MIN回绕后提前查询图像的差异。
两个旋转站点分别覆盖1、-1、INT_MIN与未知背景身份的六组独立回归；
正负1停在旋转消费，INT_MIN跳过该消费，普通路径停在后续blit，
alternate路径继续缓存播放并消费其发布的有效图像。
最新core setup1/1（5.13秒，总5.19秒）通过。暂停恢复时原受管任务
已停止，ASan旧日志未完成，SDL日志未建立；没有把这两门记为通过。
proc_c718重新执行剩余两门，119秒exit0：ASan setup1/1（11.60秒，
总11.65秒）、SDL233/233链接通过，无sanitizer finding。
actor316没有在这轮重跑，保留proc_dbc6独立结果；没有运行游戏。

### 24.4 核对边界

LST到C++沿入口发布、六处尺寸、两处分带重发、两处旋转参数、
两个缓存发布点、producer写序、同一记录复位及三个caller核对。
C++到LST逐项反查字段、引用、晚查询、合法身份和失败前缀。
没有把现有archive适配器或通用blitter全部行为升级为assembly_exact。
原raw blitter在41742A/417443再次取4CD730，完整像素callee的原全局
联合动态捕获仍未取得，original_diff_verified保持blocked_runtime_oracle。
45AFEA/45B002的共享刷新也读取背景并发布来源，但其历史source_pitch
字段和窄callee端口尚待独立接线，本批不升级该函数整体语义。
其余共享状态、实际画布、完整SDL帧、实际续玩和WP316仍未验收。
