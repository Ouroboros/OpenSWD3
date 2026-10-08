# 战斗当前画面复合效果 `0x00453580`

历史状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。
B11正在复核实际画布接线；灰度分带修正见第15节，复制返回处理见第16节。
角色移动与背景平移的共享量修正见第17节。
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

## 6. 公共全屏clip与遭遇ID门

入口效果结束或被双抑制门跳过后，再次固定恢复`0,0,640,480`clip。

当前遭遇ID按i16符号扩展后，与完整expected i32比较。不同则跳过surface阶段和cadence，直接携带当前stage word进入尾部fade状态机。

ID相等后，只有`primary_suppression==1 || secondary_suppression==1`才执行surface阶段；其他非零值虽然跳过入口绘制，却不会进入此阶段。

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
- 当前遭遇ID写`0xFFFF`；
- primary suppression；
- secondary suppression；
- alternate surface mode；
- fade active。

不清pending rotation、split状态、颜色循环、cadence、fade block、selected surface或surface表。

### 10.2 stage不小于1

先执行stage word减一并写回。

若selected surface完整dword不等于全1且alternate mode为0，则以减一后的stage读取surface表，effect flags固定0，执行一次虚操作后立即返回；不执行fallback source blit。

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
- `0x00453799..0x004537D5`：公共全屏clip、遭遇ID与双stage门；
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

当前没有原版DirectDraw surface对象、三项surface表、共享source/clip/blitter状态、旋转缓存、颜色格式、遭遇ID、全部阶段word与target framebuffer联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。

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
特殊动作405的473411实际写53BF94；该旧`frame_refresh_pending`字段
保留并标明抑制门待绑定，不能按名称把此处迁成闪光。

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
