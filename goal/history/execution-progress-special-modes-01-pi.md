# 执行历史：模块9特殊模式前段

状态：冻结历史；不得作为当前执行状态或行为真值。

来源：重构前`execution-plan-pi.md` v853第3073..3650行，模块9前段工作包。

完整性与当前资料入口见[`../execution-history-index-pi.md`](../execution-history-index-pi.md)。当前状态见[`../execution-state-pi.md`](../execution-state-pi.md)。

---


- 模块9标准特殊模式总入口`0x00439FD0`闭环。bit31初始化门、bit30低模式变体、bit29保留、
    mode3/4/5/6 selector、初始化公共尾、帧回绕、`update -> input -> conditional draw`和退出
    清理均按LST实现；SDL主帧已统一接线，mode3继续消费真实初始菜单路径，其余未关闭子mode不
    伪造业务成功。workpack两轮稳定为`1/227`，hash为
    `2f654cc75235148ab08330efc25ff9a072b75f4233adaa45bb5ce45aa800c167`；Linux core186/186、
    Linux app192/192及Windows LLVM app192/192通过。

- 模块9标准特殊模式总初始化`0x00439DE0`闭环。启动与世界/MAPS重载两条调用路径、共享退出位
    清零、回调安装边界、18个Act记录部分重置、16个固定动作键、两个保留动作键、剧情标志
    `0x49 == 1`分支和机器尾`0x232B`均按LST实现。workpack两轮稳定为`2/227`，hash为
    `895ddf3dc0d11da21a3fe4e216719878e8da3901923999183aa88fa9a172c34e`；Linux core186/186、
    Linux app192/192及Windows LLVM app192/192通过。

- 模块9双参数初始化`0x0043A2A0`闭环并完成参数方向纠正。mode1/2参数为`(0x1E,1)`或
    `(0x24,2)`，mode3–6参数为`(0..3,0xEA60)`；`word_4FC900`取secondary，三份资源word及
    有符号派生索引取primary。五个callee边界、`0x200`字节输入区清零、共享token及三owner
    token/sentinel顺序均按LST实现。参数修正后workpack两轮仍稳定为`5/227`，hash为
    `dfb6620cabbf4b2fad12886f501dc0e4c2d680babb9ac6fe3892e3e45b4d0c0f`；Linux core186/186与
    Linux app192/192重新通过，Windows BUILD留到模块9大阶段统一执行。

- 模块9可用项目状态初始化`0x0043A380`闭环。4个固定剧情标志、部分重置顺序、非零可用语义、
    可用序号选择、共享索引`8..11`及`records[available_count]`的原始terminal覆盖均按LST实现。
    workpack两轮稳定为`4/227`，hash为
    `0a4e0ab3cada3c7463ba367cf94842e97c4c0a6207c73585165ab8a6a28e93ba`；Linux core186/186与
    Linux app192/192通过。本单入口按阶段门禁不重复执行Windows BUILD。

- 模块9标准模式输入分派`0x0043A470`闭环。13个调用点、12个回调槽、共享overlay cooldown、
    严格/重复输入门、记录3/5误用记录7旧AL的原始异常及记录1优先的退出二选一均按LST实现。
    workpack两轮稳定为`5/227`，hash为
    `dfb6620cabbf4b2fad12886f501dc0e4c2d680babb9ac6fe3892e3e45b4d0c0f`；Linux core186/186与
    Linux app192/192通过。本单入口按阶段门禁不重复执行Windows BUILD。

- 模块9标准模式画面呈现`0x0043A610`闭环。entry extent、有符号transition算术、surface建立、
    三个动作加载点、flag49分支、callback清mode早退、blocking gate、普通/扩张路径、软件鼠标、
    颜色调整、提交和终态快照均按LST实现。workpack两轮稳定为`6/227`，hash为
    `a5d803139a08653d833dda9ed715afd66496b9edd37534e477e29bec3679b98b`；Linux core186/186与
    Linux app192/192通过。本单入口按阶段门禁不重复执行Windows BUILD。

- 模块9标准模式面板准备`0x0043A880`闭环。8–16 step状态、三次flag49、ghost variant与flags、
    step16通用动作桥、20/21交换、step8–15手工frame、40/41交换、三个signed delta和实时offset
    坐标均按LST实现。workpack两轮稳定为`7/227`，hash为
    `7926b61e47ea21deda9ae8f6b90c495c7086c7af70b32fe1f52f66c7668e3615`；Linux core186/186与
    Linux app192/192通过。本单入口按阶段门禁不重复执行Windows BUILD。

- 模块9标准模式transition项目块`0x0043AAA0`闭环。4项byte stage、可用门、4次ghost、3条比例线、
    5段数值文字、level读取、全宽线与可选flags动作的地址顺序、坐标和参数均按LST实现。
    workpack两轮稳定为`8/227`，hash为
    `f4725f41ceba9417b1751f0dc8e5df121f4b56b1d1dd55bed41716e0b95e3c9d`；Linux core186/186与
    Linux app192/192通过。本单入口按阶段门禁不重复执行Windows BUILD。

- 模块9通用分段bar`0x0043AE40`闭环。10参数ABI、两段tile、动作更新失败后继续、x87向零截断
    分割、四个输出、三次矩形及variants `0x1A/0x1B/0x1E/0x1F`均按LST实现。workpack两轮
    稳定为`9/227`，hash为`51f6fa2f32795996d7688c1f84d3ff203de46092fd17e81fc623336a02bfba33`；
    Linux core186/186与Linux app192/192通过。本单入口按阶段门禁不重复执行Windows BUILD。

- 模块9ghost动作绘制`0x0043B080`闭环。update失败早退、frame解析、caller值存储、
    `(mode_flags & 0x80000017) | 0x14`、live offset坐标和opacity0均按LST实现；panel与transition
    两个上游已接真实桥。workpack两轮稳定为`10/227`，hash为
    `bfd4559554dfc5113b0981e1a5b372fe645c62caae16003d227199c009ba276e`；Linux core186/186与
    Linux app192/192通过。本单入口按阶段门禁不重复执行Windows BUILD。

- 模块9特殊模式回调绑定器`0x0043B480`闭环。9组选择条件、13个主回调槽、107次实际写入、
    flag49两段相反分支、G08/G09 helper前置调用及缺失槽保留均按LST生成清单实现；SDL selector
    已接typed callback owner，输入动态前置门读取slot0。目标地址只作为legacy callback ID保存，
    目标函数继续独立关闭。workpack两轮稳定为`11/227`，hash为
    `275a5c1894af8f6f1db777d5d769eff1cbe944003990bc985d8c144e2c104080`；Linux core186/186与
    Linux app192/192通过。本单入口按阶段门禁不重复执行Windows BUILD。

- 用户插入的内部逻辑时钟与显示刷新时钟解耦完成。`[display].fps`允许`0..1000`，默认0
    保持旧同步呈现；正整数使用独立纳秒display deadline，idle中的原视频/音频/yield/游戏帧/
    pause动作均先完成，再检查显示刷新。legacy framebuffer composition与纹理上传仍由原请求
    驱动；VSync无法关闭时回退旧模式。35/70ms逻辑门、输入、剧情、RNG与音频状态均不由
    display clock访问。初始边界Linux core187/187与Linux app193/193通过；按阶段门禁未运行
    Windows BUILD。

- 用户实际验证120/240 FPS后要求普通世界运动真正平滑。新增
    `[display].world_motion_interpolation`布尔开关，默认false且不强制启用；仅当该值为true且
    `fps > 0`时，composition入口才于任何stage前复制背景、空间索引、相机、角色记录和空间
    音频数组。独立display deadline在独立framebuffer中纯重绘indexed object、背景、flagged
    role与普通role；current纯运动底图和完整primary surface的差异作为残差覆盖，保留UI、文字、
    粒子和其他非运动层。重绘端口不执行输入、碰撞、路径、剧情、RNG、动作更新或音频。首帧、
    跨地图、角色身份变化、重绘失败及非普通世界presentation均回退当前完整纹理。

    初始previous/current方案经用户实测存在明显拖拉，随后改为older/previous/current三快照混合
    显示：镜头与非剧情受控角色从current开始，仅在连续两tick速度完全一致且单坐标增量不超过
    128时向下一tick投影；启停、转向、变速、传送及历史不足snap current。非受控角色与bit15
    剧情路径角色维护16.16显示轨迹，把一次逻辑位移铺满`action.wait_remaining + 1`个tick，保持
    坐标的后续逻辑帧不打断轨迹；X轴单调就近量化，Y轴使用256相位低差异时域量化。全部状态只
    存在于显示副本，35/70ms逻辑、输入、碰撞、路径、剧情、RNG、动作与音频owner不变。定向UT
    覆盖配置、60/120/240 FPS、快照时基、稳定投影、启停转向、action等待轨迹、分数采样、传送
    和残差；Linux core188/188、Linux app194/194及Windows app194/194通过。用户240 FPS实测确认
    纵向较平滑、横向移除时域回退后明显改善，最终评价“虽然比原生效果差点，目前来说还能接受”，
    据此关闭插值门禁。原生高频逻辑模拟留待整个执行计划完成后另行评估。

- 模块9标准模式单链计数helper`0x0043B980`闭环。LST范围`0x0043B980..0x0043B993`
    固定从有效head变量读取链头，只跟随节点偏移0的next，以`u32`只读计数；空链返回0，原循环链
    不终止，不新增环检测或上限。11个直接调用点归并为10个caller及6个链owner。typed实现以
    `LegacyStandardModeForwardNode`隔离32位裸指针宽度，定向UT覆盖空链、1/2/3节点和调用后链
    不变。workpack连续两轮稳定为`12/227`，SHA256为
    `6c9092652090a83464cc5b5cf491be7ed409abc52f27aa3cb3e2a69ee6615ec4`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式单链推进helper`0x0043B9A0`闭环。LST范围`0x0043B9A0..0x0043B9BD`
    先无条件复制source head到output head；signed count小于等于0时直接返回output变量地址，正值
    时严格沿节点偏移0推进count次。保留source/output变量别名、短链越界解引用和无null保护。
    37个直接调用点归并为35个caller及固定/动态链owner族。typed pointer-to-pointer实现与定向UT
    覆盖零/负count、1/2/3步推进、distinct source不变和source/output别名。workpack连续两轮稳定
    为`13/227`，SHA256为
    `6b2f98c7c36ca283c5f58aa0a0b028275cb3febc7f918ddb5aa24acabe8ddcd3`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式单链索引helper`0x0043B9C0`闭环。LST范围`0x0043B9C0..0x0043B9D3`
    无条件读取head变量；signed count小于等于0时返回head，正值时严格只读跟随节点偏移0的next
    count次。保留短链越界解引用和循环链有限步行为，不新增null/环保护。45个直接调用点归并为
    37个caller、一个动态head及四个固定head owner。typed实现与定向UT覆盖空head零步、负/零
    count、1/2/3步、链不变及循环链有限步。workpack连续两轮稳定为`14/227`，SHA256为
    `59848c73b0139fe2c720bdf6cce6d4ad5a8230d7e9a82d94581db525ade46783`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式共享文本解析helper`0x0043B9E0`闭环。LST范围`0x0043B9E0..0x0043BA39`
    读取输入记录`+4` u16；`FFDC`以固定`B5 4C 00`输出CP950“無”，普通索引通过MAPS payload
    `+0x4C`相对目录和u32回绕定位记录，再逐字节复制到首个unaligned `%Q`并追加NUL。保留
    embedded NUL、路径相关raw EAX信息和已提交copy顺序；目录/terminator越界及128字节buffer
    越界在原危险点typed-stop。46个直接调用点归并为38个caller。synthetic覆盖FFDC、embedded
    NUL、u32回绕、terminator截断和buffer边界；真实`MAPS.DAT`锁定payload162417字节、目录
    `0x1D993`及索引1的`Nullitm6  `十字节文本。workpack连续两轮稳定为`15/227`，SHA256为
    `84d8a2d5bb81a2af2ca1d6f495be9a0aff3c2681b67cd56a9937337440fe8346`；Linux core188/188与
    Linux app194/194完整门通过，真实测试保留`legacy_real_assets`锁，按阶段门禁未运行Windows BUILD。

- 模块9标准模式输入状态位组合helper`0x0043BA40`闭环。LST范围`0x0043BA40..0x0043BAA1`
    每次先清output；两组gate只接受精确1，各自state按signed i32把1映射为低组1/高组4，
    大于1映射为低组2/高组8，形成0/1/2与0/4/8的9种组合。保留原`or al`和路径相关EAX；
    两个直接caller均不消费返回。typed结果与定向UT覆盖9种组合、gate失配、负state和legacy EAX。
    workpack连续两轮稳定为`16/227`，SHA256为
    `b2be6a6ee527034544e7fd9a7222c54db40dcb3a4117231651a4fa8c03307445`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式窗口游标归一化helper`0x0043BB40`闭环。LST范围`0x0043BB40..0x0043BB7B`
    先按signed i32判断local cursor是否越过visible count；未越过时零写入并返回原cursor，越过时
    把cursor写为`max(visible - 1, 0)`。随后以32位回绕计算`visible + window offset`，signed结果
    小于total时回绕递增offset；改写路径返回最终offset。4个caller均额外压入一个未读立即数，
    modern只表达函数体实际访问的四指针ABI；`0x0043F880`传播路径相关EAX，其他3个caller覆盖。
    typed结果与定向UT覆盖早退、visible 0/1/多项、负值、相等边界和`INT_MAX/INT_MIN`回绕。
    workpack连续两轮稳定为`17/227`，SHA256为
    `9fa7ef74a307b40c6ff041ee187482160bd35825de828c2ca2af2486fdbbc5e5`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式窗口游标预增helper`0x0043BB80`闭环。LST范围`0x0043BB80..0x0043BBBE`
    先以32位回绕无条件递增local cursor；递增值按signed i32仍小于visible时零额外写入并返回
    local cursor参数地址，越界时钳为`max(visible - 1, 0)`。随后按32位回绕计算
    `visible + window offset`，signed结果小于total时回绕递增offset；越界路径返回最终offset。
    8个callsite、7个caller均额外压入未读立即数；`0x00445E3C`分支传播指针/整数联合EAX。
    modern以显式return kind隔离64位typed指针与i32值；定向UT覆盖预增、边界、负值、
    `INT_MAX/INT_MIN`回绕和两类返回。workpack连续两轮稳定为`18/227`，SHA256为
    `3c78e939eb8fff41dbdf4a848fba4052917c07d4e52293b35221b390452340ed`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式窗口游标回退helper`0x0043BBC0`闭环。LST范围`0x0043BBC0..0x0043BBDE`
    以32位回绕无条件递减local cursor；结果非负时不读取offset并返回local cursor参数地址，
    结果为负时把cursor钳0，仅在window offset为signed正数时递减并返回最终offset。8个callsite、
    7个caller虽均压入5项，但函数只读取offset与cursor两个指针；total、visible和立即数均未读。
    `0x00446037`分支传播指针/整数联合EAX，modern以显式return kind隔离typed指针与i32值。
    定向UT覆盖预减、零边界、正/零/负offset、`INT_MIN/INT_MAX`回绕和两类返回。workpack连续
    两轮稳定为`19/227`，SHA256为
    `1ab952b8fbe8927855dc0644a28926d30b3beaa5b54df64a2cb84c2676dea681`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式窗口分页前移helper`0x0043BBE0`闭环。LST范围`0x0043BBE0..0x0043BC52`
    非末项cursor归一为`max(visible - 1, 0)`；末项先以32位回绕推进offset，再按
    `new offset + step < total`的signed结果选择继续当前页或重建末页。当前页返回
    `total - offset - 1` cap并仅在cursor大于cap时改写；末页按`total - step`重算offset，负值
    钳0，再重算visible与cursor。6个callsite、5个caller均使用完整五参数，`0x00446209`分支传播
    i32 EAX。typed结果锁定三类路径与owner写入；定向UT覆盖visible 0/负值、cap、精确边界、
    负step及`INT_MIN/INT_MAX`回绕。workpack连续两轮稳定为`20/227`，SHA256为
    `aea28751bf54f377927b19787250cd7256f9963978769eaba4a8c548e354bbb0`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式窗口分页回退helper`0x0043BC60`闭环。LST范围`0x0043BC60..0x0043BC8E`
    若local cursor非零则只清cursor；cursor为0时以32位回绕执行`window offset - step`，signed
    结果为负时再按原顺序把cursor与offset清零。函数始终返回local cursor参数地址。6个callsite、
    5个caller虽均压入total/offset/cursor/visible/step五项，但函数只读取offset、cursor与step；
    modern只公开两个typed引用和step值。定向UT覆盖正负非零cursor、正常/精确/负结果、
    `INT_MIN/INT_MAX`双向回绕及typed指针返回。workpack连续两轮稳定为`21/227`，SHA256为
    `eac0784e38c2ae6dbde7aca7b15de9c632ac50ce689e93748254bbcd131ab3d5`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式有界单链计数helper`0x0043BC90`闭环。LST范围`0x0043BC90..0x0043BCB7`
    入口先无条件清输出count；非空链每轮先按signed i32判断count是否达到limit，未达到时count加1
    并只跟随节点offset0 next。limit先达到时返回当前节点，null先达到或恰好同时达到时返回null；
    limit非正返回head且不解引用。22个caller使用固定或动态limit，`0x0043F06B`传播返回节点。
    typed实现保持i32 count/limit与只读链；定向UT覆盖空链、负/零limit、1/2/3/超长limit、
    循环链有限步、count入口清零和`INT_MAX`短链。workpack连续两轮稳定为`22/227`，SHA256为
    `30d3b81078ac67ee36c669effd889f5c3ce79cd1ef25c630a9731273298f09f2`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式窗口链选择与文本解析`0x0043BCC0`闭环。LST范围`0x0043BCC0..0x0043BD66`
    先统计source链并发布total；空链以精确`FFDC/1/0`通过窄port请求未关闭`0x0044D2D0`，且不重计
    total。随后按signed回绕归一offset/cursor，复制推进output head，以`0x0043BC90`覆盖visible，
    再按`offset + cursor`从当时source变量选择节点并解析共享文本。保留source/output变量别名、
    5处最终文本返回传播，以及window/selected null原危险点typed-stop；不提前计入callee。
    定向UT覆盖首窗、cursor/offset修正、fallback插入、不发布fallback、文本失败、变量别名及两处
    unsafe边界。workpack连续两轮稳定为`23/227`，SHA256为
    `e7b5c8b17175a8d999425ee11631f10a0b8ae63b7975de72dc0e3a569fb8785e`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式纵向动画面板`0x0043BD70`闭环。LST范围`0x0043BD70..0x0043BE3B`；inactive
    条件为velocity等于0且position不等于`0x154`。活动路径先以x86 SAR折半velocity，再以32位
    回绕更新position；正速度越过`0x154`或非正速度越过`0x1E0`时钳位置并清速度。随后严格按
    已关闭的矩形效果、九宫格边框、格式化文字顺序调用，边框资源合成矩形返回高字与共享低字，
    最终返回文字callee EAX。typed port复用三个rendering owner；定向UT覆盖inactive、顶部静止、
    正负移动/钳制、速度1、坐标回绕、调用顺序和全部常量。workpack连续两轮稳定为`24/227`，
    SHA256为`8faa1d23648b963401dee5a701dc7b237e975b80ad205a650793d603e3709621`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式MAPS值分组查找`0x0043BE40`闭环。LST范围`0x0043BE40..0x0043BE8B`，从
    MAPS payload `+0x58`相对目录进入分组；每组跳过6字节头后按unaligned u16扫描到`FFFF`，
    命中返回group起点，组终止后再遇`FFFF`表示全表结束。value先zero-extend再与full i32 target
    比较，因此负数及大于65535不截断匹配。modern以相对offset表达指针并在原越界读点typed-stop；
    定向UT覆盖两组命中、双sentinel未命中、full-width target、空目录、缺目录、u32回绕和缺终止。
    workpack连续两轮稳定为`25/227`，SHA256为
    `1675589164a3f4811b024c4bbf4146bb838c3944d2c24f03a2a8b6e2ec50a2cf`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式MAPS条件筛选记录构建`0x0043BE90`闭环。LST范围`0x0043BE90..0x0043BFB4`，
    每次释放旧表、建立`0x800`字节/512项新表并清count；从MAPS `+0x5C`记录流解析`%Q`名称、
    6字节头与`FFFF`终止条件。每个condition查询`u16 + 0x1388`，只有精确返回1命中且不短路；
    命中记录保存头与lstrcpy可见C字符串。modern用typed vector/array和窄query port替代裸分配，
    在缺目录/marker/sentinel、64字节lstrlen过读、65字节写及第513项表写处typed-stop。
    定向UT覆盖双记录全查询、embedded NUL、精确1门、空流、各解析失败和512容量。workpack连续
    两轮稳定为`26/227`，SHA256为
    `3b4136d16656acd22dabeaf2d93b693733cbc8cecf41c1210e68c3bb66e3c772`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式对话/场景准备`0x0043BFC0`闭环。LST范围`0x0043BFC0..0x0043C08F`，先清
    `0x96000`字节surface，再以服务`0x2711`配置接口；随后按当前索引读取216字节stride记录并
    调用绘制owner。绘制后填128字节`CF`，发布输入word、清零字段、仅改packed dword低16位为1，
    并把记录`B8/AC/B0`字段发布到三个state owner，最终返回B0。typed state/record/port保留顺序；
    索引越界在首次表读点停止但保留clear/interface副作用。定向UT覆盖全部常量、字段映射、半字
    写、高位保持、返回与越界顺序。workpack连续两轮稳定为`27/227`，SHA256为
    `1889fde199030e1d4b75d152cfd8a3628b9d388beedf57f292fa61b30cd9b3a9`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式16字节记录可用性判断`0x0043C090`闭环。LST范围`0x0043C090..0x0043C0C8`，
    只读取记录offset0 enabled与offset12 state；enabled为0不可用，state精确1可用，否则state须
    signed大于10且为偶数。12个callsite、7个caller消费0/1结果。typed span在原`index * 16`
    表读取点隔离负/越界index；定向UT覆盖enabled真值、负/0/2/10、11/12/13、`INT_MAX`奇偶和
    index边界。workpack连续两轮稳定为`28/227`，SHA256为
    `79f22567a8aca4a9a2e73bb42912ddeb26c57d4184927d8981507e0893f5caaf`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式运行时表初始化`0x0043C0D0`闭环。LST范围`0x0043C0D0..0x0043C2DC`，typed
    state重建`0xB0` scratch、两张`0x200`状态表、16×32与64×16字符串槽及64项entry表。严格
    执行1..500 load循环与1..500 query循环；成功load发布`+0x5E`、释放`+0xAC` token并清token。
    后续只清字符串首字节，先把entry alias写typed index 0，再清`FC974/FC90C/FC928/FC914/FC910`
    五个owner，直接调用已关闭C9C0执行classification/status各500次扫描、page刷新与真实entry表，
    再只写共享17项action表record0的`0x232A/0x33`、消费entry[0]并最后清mode flags。未关闭的
    数据库/load/release/refresh/consume callee仍由共享typed port隔离；定向UT锁定两组500次顺序、
    表边界、token、未初始化字节保持、alias、
    action字段和EAX。workpack连续两轮稳定为`29/227`，SHA256为
    `4d43482df73105a50a831f9da35a35f89fab5916af1d7ffae22e5e3f9ad3f940`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式运行时输入分派`0x0043C3C0`闭环。完整函数由主块与`C2F0/C600/C7E0/C800`
    四个外置chunk组成。第一矩形按24像素行计算并保留钳制后额外减1；第二矩形保留负delta减2、
    正delta不改mode但仍刷新/发声的原BUG。availability记录15后的upper/bottom使用固定范围，
    两段动态范围读取runtime state中由`0x0043C820`发布的split-bar边界，四段按顺序独立命中；
    翻页严格执行step15、alias重建、page刷新、entry消费、flags低字节OR `0x30`
    与sample `0x2E`。exit必须精确500，随后条件释放record token并按4个固定块、16 long、64 short、
    entry表顺序释放，最终`FC974=64`、action写`0x232A/0x43`并保留末次release EAX。原表越界在
    availability15或selected entry读取点typed-stop；已关闭`0x0043C520`、`0x0043C590`、
    `0x0043C670`与`0x0043C760`直接复用typed helper，其余未关闭callee继续由窄port隔离。定向UT
    覆盖所有分支、严格边界、重叠顺序、路径EAX和
    85项storage释放。workpack连续两轮稳定为`30/227`，SHA256为
    `f9dc7e93f48dba950d034372bfb2830b0cef40e93357d0c9afebc4e353c3e137`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式运行时游标推进组合器`0x0043C520`闭环。LST范围`0x0043C520..0x0043C58F`，
    三个callsite来自`0x0043C3C0`两次与`0x00445C90`一次。先调用已关闭`0x0043BB80`，再严格
    执行alias重建、page刷新、原entry base的`window_offset + local_cursor`读取、entry消费、
    flags `| 0x30`和sample `0x2E`，最终返回sample EAX。selected index以u32回绕计算并只在原
    entry读取点typed-stop。两个`0x0043C3C0`caller已真实回接：第一矩形tail路径返回sample EAX，
    bottom路径保留全部副作用后以pointer Y覆盖EAX。定向UT覆盖正常滚动、顺序、flags、sample、
    selected越界与caller集成。workpack连续两轮稳定为`31/227`，SHA256为
    `3b77b39c834345e45eb3b555273d34ac609db66abcf7482273be302a39bd3e0a`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式运行时游标后退组合器`0x0043C590`闭环。LST范围`0x0043C590..0x0043C5FF`，
    两个callsite来自`0x0043C3C0`与`0x00445E90`。先调用已关闭`0x0043BBC0`，再执行alias重建、
    page刷新、selected entry读取/消费、flags低字节OR `0x03`和sample `0x2E`。selected index
    只在原entry读取点typed-stop。`0x0043C3C0` upper caller已真实回接；重叠upper→dynamic→page
    路径锁定两轮rebuild/refresh/consume/sample、实时cursor归一化和最终flags `0x33`。定向UT覆盖
    后退钳制、offset retreat、entry/flags/sample、selected越界和caller集成。workpack连续两轮
    稳定为`32/227`，SHA256为
    `23447b5e37aeac637e55272f7920bbfa8c75af181e445a531859f7ff0e4303c9`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式运行时翻页后退组合器`0x0043C670`闭环。LST范围`0x0043C670..0x0043C6DF`，
    两个callsite来自`0x0043C3C0`与`0x00446260`。先调用已关闭`0x0043BC60`，再执行alias重建、
    page刷新、selected entry读取/消费、flags低字节OR `0x03`和sample `0x2E`。非零cursor只清0，
    零cursor使offset按u32回绕减15并signed钳0；selected越界只在原entry读取点typed-stop。
    `0x0043C3C0` first-dynamic caller已真实回接；CBD0关闭后，重叠upper→dynamic→page路径使用
    实时visible1推进offset15/cursor0，三轮消费entry13/0/entry15(0)，最终visible0与flags `0x33`。
    定向UT覆盖两种page retreat、CBD0停止传播、entry/flags/
    sample、selected越界及caller集成。workpack连续两轮稳定为`33/227`，SHA256为
    `96b94897a246543feac8c35a3ab60bf66f907f10f63a5ec78480fd2c00d75298`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式运行时模式推进组合器`0x0043C760`闭环。LST范围`0x0043C760..0x0043C7D0`，
    两个callsite来自`0x0043C3C0`外置chunk与`0x00446550`。mode先u32回绕递增，再signed大于11
    才钳11；因此10→11、11→11、`INT_MAX→INT_MIN`。随后直接调用已关闭C9C0按mode映射重建
    entry/text/status并清window/cursor/alias、第一次刷新page，再执行alias重建、第二次page刷新、
    entry0读取/消费和sample `0x2E`，不改flags。`INT_MAX→INT_MIN`紧接着在C9C0 mode-map读取点
    typed-stop；合法调用不再保留旧window制造synthetic selected越界。`0x0043C3C0` mode caller已
    真实回接：负/正delta的中间值3/5经本函数变为4/6，本函数内部sample后caller再无条件播放第二次
    sample。定向UT覆盖回绕、signed钳制、C9C0停止传播、双refresh、真实entry0消费、双sample及
    早退边界。workpack连续两轮稳定为`34/227`，
    SHA256为`d2053fd736fee3f30bf0b0edab19ff0f41a812ba397b7a42a91060cad38e20b7`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式运行时列表渲染`0x0043C820`闭环。LST范围`0x0043C820..0x0043C9B5`，运行时
    caller仅`0x00447100`。signed total大于15时独立衰减flags两个nibble并形成overlay位，按x87
    signed比例向已关闭split-bar owner发布`0xCE/0x62/0x15E`几何与四个动态边界。alias链从typed
    index迭代，selected行读取short text与entry，写独立preview action `entry/0x44/variant0`，再按
    `0x1FC/0x3C`绘制；每行调用entry owner，selected行调用已关闭矩形效果。无条件post-row next
    alias读取与Y=`0x1C6`上限顺序保持。runtime state修正为共享17项action表；C0D0/cleanup只写
    record0，split bar使用records6–9。C3C0动态判断改读本函数发布的state边界。定向UT覆盖nibble、
    float、两行链、preview/frame、total15跳过、bar停止和三类原表typed-stop。workpack连续两轮
    稳定为`35/227`，SHA256为
    `4856353c4390b409498d64a75f8fb47c9137c4ffe1f5753ff8eb10bd39066fe6`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式entry初始化`0x0043C9C0`闭环。LST范围`0x0043C9C0..0x0043CBC6`，callsite来自
    C0D0、C760与`0x00446420`的`0x0043C6E0`外置chunk；第一个stack参数未读。先清entries和short
    首字节，按15项mode→classification表读取mode，再清64-byte status与total；signed i8分类精确
    查询1..500并写entry，独立隔离第65项写入和恰好64项后的terminator越界。第二轮status严格
    1..500；匹配entry使用一次性清零且不逐次重置的`0xB0` scratch加载文本，成功后再次读取status，
    load成功或失败均释放并清`+0xAC` token。无NUL和长度大于15分别在原`lstrcpyA`点typed-stop。
    正常尾部只清window/cursor/alias并返回CBD0 EAX。C0D0/C760已真实回接；`INT_MAX→INT_MIN`在
    C9 mode-map停止，合法C760先清旧window再消费真实entry0。定向UT覆盖三entry、signed分类、
    502次status读取、load失败、scratch/token、五类typed-stop及两个caller集成。workpack连续两轮
    稳定为`36/227`，SHA256为
    `6d00895fa07920a2aa3d71411fe434ce98aeacf38e907603a8e9b8b51b0719f7`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式page可见项刷新`0x0043CBD0`闭环。LST范围`0x0043CBD0..0x0043CBF6`，无callee，
    七个caller覆盖C520/C590/C3C0 chunk/C670/`0x00446420` chunk/C760/C9C0。无条件清visible0并
    读取alias首项；非零时每轮先检查signed count15，再递增visible与entry指针、发布visible后读取
    下一项。小于15项时返回terminator指针；至少16项时读entry15并返回其指针。typed实现用
    `const u32*`表达EAX，不在64位宿主截断。负/超界alias在首读停止；alias63非零先写visible1再在
    next读取停止。全部已关闭caller移除`refresh_page`port并直接回接，且在CBD0停止时不执行后续
    selected/consume/flags/sample。真实visible纠正C3重叠路径为offset15/cursor0/entry15(0)。定向
    UT覆盖terminator、15上限、首读/next边界、C9 tail与caller传播。workpack连续两轮稳定为
    `37/227`，SHA256为
    `45af9d23d04ae8fe96324b1a3d52bf5c8610b0ecb97619feaea414d8c9832f3e`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式entry alias重建`0x0043CC00`闭环。LST范围`0x0043CC00..0x0043CC1C`，无callee，
    六个caller覆盖C520/C590/C3C0 chunk/C670/`0x00446420` chunk/C760。先把entry base写入alias；
    signed offset大于0才逐项加4，非正值保持base。EAX始终返回alias owner指针。typed index等价为
    `offset > 0 ? offset : 0`，结果以`i32*`表达owner；正`INT_MAX`不提前钳制，真正越界留给CBD0
    原解引用点。全部已关闭caller移除alias重建port并直接回接。定向UT覆盖`-7/0/7/INT_MAX`、
    owner返回和既有CC00→CBD0链。workpack连续两轮稳定为`38/227`，SHA256为
    `fd99444cb9fe7102c3218207b6361d26c7b0a1933bd04a6bcf5d68fe72ef4a5c`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式entry绘制`0x0043CC20`闭环。LST范围`0x0043CC20..0x0043CEE2`，唯一caller为
    C820；14次raw text与最终格式化文字owner均已在rendering关闭。空名称绘`????????`，非空名称
    按`%-12s`右填充；status按signed i8乘5并用`%3d%%`。row坐标保持u32回绕。selected精确等于1
    才绘9个固定详情；非空名称再按首字节`'?'`抑制3个可选详情，并保留异常X=`0xF228`。最后以
    scratch `+0xAC` token调用格式化owner，参数`242/336/5/360/style4`并返回其EAX。返回联合区分
    selected值、空名称short指针和formatted EAX；字符串无NUL在原扫描点typed-stop。C820删除
    `draw_entry`占位并直接回接，完整u32 color不再截低16位。定向UT覆盖`Hero`/`-10%`、9+3详情、
    问号跳过、空字符串仍绘制、token、返回联合与边界。workpack连续两轮稳定为`39/227`，SHA256为
    `8028d9e05900777cf5f8f89860330050bd76523189757ad63340cb66edb5eca1`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式entry消费`0x0043CEF0`闭环。LST范围`0x0043CEF0..0x0043D04E`。入口无条件
    release scratch `+0xAC` token并清整个`0xB0` record及两个derived offset；entry为0直接返回
    EAX0，不调用loader/D050。非零entry写low16 header，但把完整u32 ID传loader；失败仍继续。
    按`+0x60` base与`+0x72/76/7A/86/8A`累计第二offset，按`+0x7E/82`重建第一offset；最后以
    `window_offset+local_cursor`的u32回绕值直接调用已关闭D050并传播返回联合。旧`consume_entry`
    高层占位全部移除，六个已关闭caller直接回接；第七caller `0x00446420`保留独立审计。定向UT
    覆盖release/clear、零entry、完整ID/low16 header、七开关、`INT_MAX+1`、loader失败继续及
    C0D0/C3C0真实顺序。workpack连续两轮稳定为`40/227`，SHA256为
    `5d0095ca98c73d02f6cf8296e51d854713f0511e34801b9832cf5911a8fb46fd`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式selected record显示`0x0043D050`闭环。LST范围`0x0043D050..0x0043D36B`。
    按C0D0真实分配锁定12个32-byte display owner；signed status依次门控category、`%4d` base和
    `%-12s` selected name，无条件发出六个D370精确请求。slots9..11先写12问号；status≥19按
    `+0x72/76/7A`加载三条related名称并first-wins去重，每轮无条件release token。第三loader保留
    `scratch legacy address high16 | ID`原BUG。status<19返回slot11 owner指针，≥19返回临时storage
    release EAX，均以联合表达。CEF0删除synthetic D050 port并直接回接。定向UT覆盖category、base42、
    `Hero`填充、六次请求、三次load/release、同名抑制、第三高16位BUG和EAX。workpack连续两轮稳定为
    `41/227`，SHA256为`13f61f90e5d60664520c11df18d26ad5d47a972710b477d3b7f8318d6de6e92f`；
    Linux core188/188与Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式derived text `0x0043D370`闭环。LST范围`0x0043D370..0x0043D46F`，六个caller
    均来自D050，唯一callee为RNG。入口先发布`%4s  `；delta按`threshold-status`回绕。delta≤0用
    immediate CP950模板和signed `%4d`；delta1/2按value选择10/100/1000 scale，计算
    `rng(scale/(3-delta))-upper/2+value`并钳到0..maximum，分别使用两套CP950模板；delta≥3追加
    ` ???`并返回destination owner指针。返回联合区分formatter长度与指针。D050删除整函数port并
    直接调用六次，传播typed-stop。定向UT覆盖delta0/1/2/4、RNG上界、居中、钳制、exact字节和EAX。
    workpack连续两轮稳定为`42/227`，SHA256为
    `f481edf973f9909147d6eda7a6f6e7d51369a69183f1ae1e46c0f3675ac3e538`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式mode strip `0x0043D470`闭环。LST范围`0x0043D470..0x0043D52F`，唯一caller为
    C820。入口viewport为`10,1,206,478`；candidate按current mode±2回绕，X从6每轮加40，只绘
    signed 0..11且非current项，资源`0x2439`/variant candidate。每次draw后重读mode并重算上界，
    保留动态扩展BUG。循环后以最后current加载`0x243A`，固定X86/Y58绘center；最后恢复viewport
    `0,1,640,478`并返回EAX。C820删除整函数`prepare_frame`占位并直接回接。定向UT覆盖mode0/5、
    draw后mode5→6扩展、资源顺序、坐标、handle和load失败不恢复viewport。workpack连续两轮稳定为
    `43/227`，SHA256为`33cc4028a439a928bb19dd5af0b63664af1eda7993a6dbac96dc03dbb49de35d`；
    Linux core188/188与Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式数据库初始化`0x0043D530`闭环。LST范围`0x0043D530..0x0043D873`，由callback
    间接调用。四个1200项表先填-1；逐ID清0xB0 record、加载、成功发布`+5E/+60/+2C/signed +A7`，
    并无条件release `+AC` token。扫描后release storage。两个runtime record清零，adjustment链按
    u16回绕写两字段和，随后写action常量、reset/enable、调用尚未关闭F000边界并直接复用B980/BC90。
    两文本索引为FFDC，sample固定136。四个F0、四个1B8 buffer保留malloc未初始化字节；0x400表只按
    127个source写`mirror[128+i]=v/2`和`mirror[128-i]=v/-2`，未写位置保持。定向UT覆盖1200扫描、
    signed字段、链表、forward helpers、未初始化字节、镜像范围/EAX及source短缺停止顺序。workpack
    连续两轮稳定为`44/227`，SHA256为
    `87f11ac9fefc9d31d6e69347fb52003c4dfe4797db9df11036fbd719a6133e91`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式数据库清理`0x0043D880`闭环。LST范围`0x0043D880..0x0043DA2E`，由E770尾跳转
    及callback间接调用。入口改写两action原字段后先调用尚未关闭F080；随后条件release两个heap
    token、清两个inline B0 records、条件release两个runtime `+AC` token。F080后forward残留节点
    逐个无条件release token（含0）再release node。最后固定顺序release两个runtime、四表、四F0、
    四1B8和mirror共15类storage，返回mirror release EAX，生命周期phase写1；固定storage内容和悬空owner不清。
    forward/adjustment统一typed node。定向UT覆盖F080耗尽/残留、22事件顺序、条件token、inline清零、
    15 storage、动作字段保持、悬空字节及EAX。workpack连续两轮稳定为`45/227`，SHA256为
    `f8c52a97a3ac329afa39acee09d78e2a1863d3f6b9eef2fa179a0637381d508e`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式数据库输入分派`0x0043DA30`闭环。LST范围`0x0043DA30..0x0043DD1F`，由B480
    callback间接调用。权威owner重映射确认dword FCD20是交互phase，独立于word 4FC900生命周期phase；
    FCAD8是完整forward count，FCB98是16界count。phase1保留页/列表/方向/hover矩形、C090 record15
    门控及动态strict-X边界，前三个callee后重读X，可同帧执行DDF0→DD20→DFA0→DED0。E080/E170闭环
    后确认上/下面板成功路径分别清toggle0/写toggle1，双button真实均进入E3D0，旧double-E080/E170
    假设不可达。phase3/4/5按低4位调用E3D0/E770。定向UT覆盖signed count边界、C090 typed-stop、
    链式X重读、真实toggle耦合、物品1BA9及未知phase。workpack
    连续两轮稳定为`46/227`，SHA256为
    `53c46bdf47c9719b2ee102b907d48d820ec10bbf52731ebca6de7dfb53d9c184`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式数据库向前推进`0x0043DD20`闭环。LST范围`0x0043DD20..0x0043DDE1`，由DA30
    直接调用及B480 callback间接绑定。phase1直接复用已关闭BB80/B9A0/BC90，依次推进cursor、从共享
    head重建FCD10 current head并重算16界count；随后仅保留F880/F1E0边界，display flags低字节OR30，
    sample 2E返回EAX。DA30不再把DD20交给通用地址port。phase2在runtime bit1 gate前条件sample107，
    gate清才写toggle1；phase3写countdown200，其他phase保留DEC链EAX。同步明确FCAD0 window offset、
    FCD10扫描计数/current head复用和FCAA4 display flags。定向UT覆盖18节点链、六步顺序、records依赖、
    flags/sample、phase2 gate、phase3及phase4 EAX。workpack连续两轮稳定为`47/227`，SHA256为
    `680834374fe3d6063b702c7443a86df5d9e143d2a3a16648e5353fd61be0b278`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式数据库反向推进`0x0043DDF0`闭环。LST范围`0x0043DDF0..0x0043DECA`，DA30有
    两个direct call且B480 callback间接绑定。phase1直接复用BBC0/B9A0/BC90，重建window/local、
    FCD10 current head和16界count；随后F880/F1E0边界、display flags低字节OR03及sample2E。DA30
    不再把DDF0交给通用地址port。phase2严格按物品1BA9→runtime bit0 gate→toggle→条件sample107→
    清toggle顺序，保留缺物品EAX0与gate EAX1。phase3写countdown200，其他phase保留DEC链EAX。
    同步移除旧测试通过闭环DD20/DDF0伪造X变化，改以真实不改X的两条三callback重读路径。定向UT覆盖
    18节点链、六步顺序、records/flags/sample、三条phase2分支、phase3/4及DA30直连。workpack连续
    两轮稳定为`48/227`，SHA256为
    `e73871d107cef775a9cf4178ae9ec763039546f1c8d8b8d15c73f3caeda7f966`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式数据库分页推进`0x0043DED0`闭环。LST范围`0x0043DED0..0x0043DF91`，由DA30
    direct call及B480 callback间接绑定。phase1直接复用BBE0固定step16、B9A0和BC90，重建window/
    local、FCD10 current head及16界count，再执行F880/F1E0边界、display flags低字节OR30与sample2E。
    DA30不再把DED0交给通用地址port。phase2保持toggle条件sample107先于runtime bit1 gate，phase3
    写countdown200，其他phase保留DEC链EAX。定向UT以40节点链锁定window16、node16/node32、六步
    records/flags/sample顺序、phase2两分支、phase3/4及DA30 last target/EAX。workpack连续两轮稳定为
    `49/227`，SHA256为`eedad1b039d678862dae44defa748f89ef2b32459ec2948067cece165ff1fe8e`；
    Linux core188/188与Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式数据库分页后退`0x0043DFA0`闭环。LST范围`0x0043DFA0..0x0043E07A`，由DA30
    direct call及B480 callback间接绑定。phase1直接复用BC60固定step16、B9A0和BC90，重建window/
    local、FCD10 current head及16界count，再执行F880/F1E0边界、display flags低字节OR03与sample2E。
    phase2按物品1BA9→runtime bit0 gate→toggle→条件sample107→清toggle，phase3写countdown200，
    其他phase保留DEC链EAX。DA30的DDF0/DD20/DFA0/DED0分页链现全部直接typed回接，测试用重叠动态
    strict边界锁定两条三callback链，所有闭环callee均不伪造X变化。定向UT覆盖40节点反向页、六步
    records/flags/sample、phase2 item/toggle、phase3/4及DA30空通用port事件。workpack连续两轮稳定为
    `50/227`，SHA256为`a3c74b1f054f6886676e1956a6da3086df39ed202b646b1ac26210792533becb`；
    Linux core188/188与Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式数据库页来源循环`0x0043E080`闭环。LST范围`0x0043E080..0x0043E16A`，DA30
    有两个direct call且B480 callback间接绑定。phase1对page做32-bit减1、负值回绕2，F000重建head，
    清window/local并预置visible16后直接调用BCC0，发布count/current/selected/shared text；BCC0失败在
    原不安全点typed-stop，成功才F880/F1E0及sample2E，不改display flags。phase2按toggle条件sample107
    →物品1BA9查询覆盖EAX→runtime bit0 gate→清toggle；phase3写countdown200。闭环后修正DA30旧
    double-E080假设：外层bit0清门与E080清toggle共同保证双button真实进入E080→E3D0。定向UT覆盖
    三节点FFDC内建文本、missing insertion typed-stop、sample/query顺序、toggle清零、phase3及DA30耦合。
    workpack连续两轮稳定为`51/227`，SHA256为
    `0f52bb8ee251b986257b5e984793f79cdd66fc50d5f329adfb19a40f1d94e272`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式数据库页来源正向循环`0x0043E170`闭环。LST范围`0x0043E170..0x0043E243`，DA30
    有两个direct call且B480 callback间接绑定。phase1对page做32-bit加1、signed值大于2写0，F000
    重建head，清window/local并预置visible16后直接调用BCC0，发布count/current/selected/shared text；
    BCC0失败在原不安全点typed-stop，成功才F880/F1E0及sample2E，不改display flags。phase2中toggle1
    跳过sample并保留EAX1，其他值sample107覆盖EAX；runtime bit1清才写toggle1。phase3写countdown200。
    闭环后确认DA30 lower-panel外层bit1清门保证E170成功写toggle1，双button真实为E170→E3D0，旧
    double-E170假设不可达。定向UT覆盖三节点FFDC内建文本、missing insertion typed-stop、toggle/sample/
    gate、phase3及DA30耦合。workpack连续两轮稳定为`52/227`，SHA256为
    `78d3553aa39a50f6bdeaa493e65d0d6f1b3edcc89f56accb7ca4236e46d11185`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式数据库方向循环`0x0043E250`闭环。LST范围`0x0043E250..0x0043E305`，无direct
    code caller，由B480 callback地址绑定。phase1对direction做32-bit加1、signed值大于1写0，然后
    sample107。phase2严格按toggle加1/大于1回0→物品1BA9存在写1→runtime低字节bit0清写1→同一AL
    的bit1清写0→sample107，后写可覆盖前写；phase3写countdown200，其他phase保留DEC链EAX。
    定向UT覆盖方向回绕、query/sample顺序、物品与四种gate组合关键分支、phase3及helper counts。
    workpack连续两轮稳定为`53/227`，SHA256为
    `e99f41464c78cb7d81fb9189765f2b210f5237edf311fa6601eb7cf1c9ba6541`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式数据库主方向循环`0x0043E310`闭环。LST范围`0x0043E310..0x0043E3C2`，DA30
    有两个direct call且B480 callback间接绑定。E310与E250共享typed内部实现，仅phase1 sample ID
    参数化：direction加1、signed值大于1写0后sample2E。phase2仍严格执行toggle回绕→物品1BA9→
    runtime低字节bit0/bit1覆盖→sample107；phase3写countdown200。DA30两个方向矩形现直接调用E310，
    先写1/0再由callee回绕为0/1。定向UT覆盖主sample差异、共享phase2、两个矩形直连及空generic事件。
    workpack连续两轮稳定为`54/227`，SHA256为
    `29bf22b0a4bc9e09ac71098a6dc0c5fc8e9ccb76f18dc28a0e6a8f7d23323e69`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式数据库交互提交`0x0043E3D0`闭环。LST范围`0x0043E3D0..0x0043E741`，DA30
    四个direct call且B480 callback间接绑定。phase1严格执行物品1BB0→E770短路、F1E0失败转phase5、
    4404D0后重读phase加1、动作232A/39、物品1BA9与两个runtime +60差值/flags、sample2E。phase2
    两类toggle/flag组合sample8C拒绝，其他路径phase3/countdown-40/FDE0/sample2E；phase3经4405C0
    后小于-35写35及动作232A/46。phase4依次解析两个inline和toggle选择的runtime record，按F7C0
    EAX选择shared/alternate destination，发布三个44D2D0节点、条件释放两个heap token，再直接执行
    B980→B9A0→BC90→BCC0并成功复位phase1/动作232A/3B；B9A0与BCC0 typed-stop保留此前副作用且
    不发布复位。phase5/10写phase1并保留EAX4/9。DA30直接调用并传播`database_commit_stopped`。
    定向UT覆盖全部phase关键分支、三record materialize、token释放、12 helper、FFDC文本、两类typed-stop
    和DA30真实phase副作用。workpack连续两轮稳定为`55/227`，SHA256为
    `9c4abc1d3061dbb831d8b66fb43355da754fb4f0c9b0732c7c779a52006b54b6`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式数据库交互退出`0x0043E770`闭环。LST范围`0x0043E770..0x0043E7DF`，DA30
    两个direct call、E3D0一个direct call并由B480 callback间接绑定。phase1按u16减lifecycle，结果0时
    清lifecycle-zero owner，再直接执行B480→D880，返回D880最终storage EAX；callback primary word与
    callback state纳入typed owner，cleanup通过adapter复用D880。phase2写phase1及动作232A/3B；phase3/4
    直接尾调E3D0并传播commit typed-stop；phase5写phase1，其他phase保留`phase-1` EAX。E3D0物品1BB0
    短路及DA30所有E770路径均直接typed调用，不再产生generic target事件。定向UT覆盖lifecycle2→1的
    B480 G08、lifecycle1→0清owner、15 storage cleanup、phase2/3/5/default及最终EAX。workpack连续
    两轮稳定为`56/227`，SHA256为
    `223801a61e31fb0f348ba9ad946424aa7ac1490b0d6ddcc7ac5fcdab209aa243`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式数据库绘制`0x0043E800`闭环。LST范围`0x0043E800..0x0043EFFE`，910行、29个
    基本块，无direct caller并由B480及后续callback表绑定。入口双颜色及物品1BB0提示早退；phase1/5
    精确恢复page/direction动作坐标、display双nibble衰减、AE40双比例、16项列表/marker、双inline/runtime
    record面板、1BA9 gate、threshold资源2465/2463及139h/25Bh blit坐标。phase2/10恢复两个FA70 flags
    与公共panel；phase3恢复signed countdown、动画offset、-35 action边界、140→141后snap200及405C0；
    phase4按toggle选择record action；phase5按len*12居中提示。固定/索引文本、资源和未关闭callee保持
    最小typed render port；resource缺失在原`[eax]`点typed-stop。定向UT覆盖全部phase及精确坐标/flags/
    比例/typed-stop。workpack连续两轮稳定为`57/227`，SHA256为
    `2515ab3d64a6961f4ae9389efd852d47b363e4e4818311b788e3de569669cffe`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式数据库forward刷新`0x0043F000`闭环。LST范围`0x0043F000..0x0043F073`，55行，
    caller为D530/E080/E170各一次。严格恢复F080→F0D0→空head时D5D0→B980→owner reset→BC90；
    F080/F0D0/D5D0保持三个最小typed边界，B980/BC90直接复用已关闭helper。移除原来把F000整体
    压成`initialize_*forward_list`的port及caller重复/遗漏owner：D530、E080、E170现直接调用F000，
    其他函数的refresh边界不误接。独立UT覆盖旧head观察、page实参、三节点count/bounded/window/current、
    null后fallback及helper数；既有三caller测试继续通过。workpack连续两轮稳定为`58/227`，SHA256为
    `174d13dfe6e7583041a1df0f5e876cea5eb469ce3ac253eba3a79c92f78a76cc`；Linux core188/188与
    Linux app194/194完整门通过，按阶段门禁未运行Windows BUILD。

- 模块9标准模式数据库forward释放`0x0043F080`闭环。LST范围`0x0043F080..0x0043F0CA`，49行，caller为D880/F000。逐节点先推进forward head；非FFDC节点回收到adjustment池，FFDC节点严格先释放`+AC` token再释放节点。F000与D880现直接调用typed helper，原整块release port删除；D880后续forward循环保留为原始死路径。定向UT覆盖回收、双释放及caller顺序。workpack稳定为`59/227`，SHA256为`102301e70a3dbe32d400c0dac85fd0360cf8e0d323732186a16a726b1fe00875`。

- 模块9标准模式数据库forward构建`0x0043F0D0`闭环。LST范围`0x0043F0D0..0x0043F159`，99行，唯一caller F000。直接恢复F7C0逐节点筛选、从adjustment摘链及u16 key插入；保留输出block `+4`陈旧sentinel不清导致的小key异常追加BUG，仅清`+8` word。F000直接调用typed F0D0，原整块build port删除。UT覆盖正常3→5排序、sentinel10时5→3反常排序及E080/E170零source零F7C0事件。workpack稳定为`60/227`，SHA256为`5d77602f24d93396086abdf6653387e67d22e60a7a41996b102588148ea90920`。

- 模块9标准模式数据库forward排序`0x0043F160`闭环。LST范围`0x0043F160..0x0043F1DE`，71行，无callee。以清零0xB0本地块逐节点摘链/清next并按u16 key升序插入，最终写回head且仅清`FCAE8/FCAEA`，保留`FCAE4`。UT锁定3→1→5变1→3→5及owner写入。workpack稳定为`61/227`，SHA256为`aafa7a79eceafd91431f24d3968462f30e96a2f1efd5f87f9bd7e038dc568253`；F880 caller待独立关闭。
