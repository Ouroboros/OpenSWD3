# 执行历史：阶段与模块基线

状态：冻结历史；不得作为当前执行状态或行为真值。

来源：重构前`execution-plan-pi.md` v853第307..630行，A/B阶段清单与模块基线。

完整性与当前资料入口见[`../execution-history-index-pi.md`](../execution-history-index-pi.md)。当前状态见[`../execution-state-pi.md`](../execution-state-pi.md)。

---


1. `[x]` A1：恢复顶层执行路径和首版模块骨架。
2. `[x]` A2：函数归属。
3. `[x]` A3：状态所有权。
4. `[x]` A4：依赖、生命周期和平台边界。
5. `[x]` A5：原程序模块到重写模块映射，并冻结首轮模块顺序。
6. `[x]` B1：`compat + platform_sdl3 + app` 已完成接口级逆向、实现、逐基本块复核、Windows LLVM `core`/`app` 构建、23 项 CTest 和真实 SDL3 窗口创建/关闭 smoke；状态为 `module_closed_pending_oracle`，只保留已登记的原程序动态差分阻塞。
7. `[x]` B2：历史 63 项范围已完成有限收口审计；修正 5 项模块归属并补齐 6 个真实缺口后，当前 58 项为 57 项实现与 1 项不可达。Windows LLVM `core`/`app`、34/34 CTest 和全套真实资产回归通过；状态为 `module_closed_pending_oracle`。
8. `[x]` 日志基础设施：独立实现 UTC 毫秒时间、级别、线程 ID、`file:line`、单行消息、线程安全文件写入、逐条刷新、级别过滤，以及 `stderr`/Windows 调试器失败回退；Windows LLVM `core`/`app` 均通过 31/31 CTest，命令行早退和真实 SDL3 窗口正常关闭 smoke 均产生完整日志。
9. `[x]` B3：26 项函数全部具有实现映射；两套 RNG、帧时钟、默认绑定、DIK 快照、鼠标合同、整帧 20 条输入记录和 DBCS/IME 编辑驱动均已按完整汇编复核。Windows LLVM `core` 为 39/39、`app` 为 41/41，WSL Linux Clang 22.1.8 为 39/39 CTest；唯一缺口是已登记的原程序动态 oracle，状态为 `module_closed_pending_oracle`。
10. `[x]` B4：B6 归属复核转入 `0x004350E0` 后，152 项有限收口矩阵现为 95 项实现、16 项内部物理分支、36 项平台替代、2 项当前资产不可达、2 项等待 B10 owner 的战斗 surface 接线和 1 项归属修正；没有 B4 自有缺口。20/16/12 字体 renderer 已接入启动、显示停用/恢复和总退出，Linux `core` 64/64、Windows LLVM `app` 66/66 CTest 通过。原程序 framebuffer/RECT 动态差分仍为已登记的 `blocked_runtime_oracle`，状态为 `module_closed_pending_oracle`。
11. `[x]` B5：73个自有地址及全部核心状态机已关闭；官方签名FFmpeg n9.0源码由项目脚本构建为最小LGPL双平台静态包，五个归档通过项目自有共享媒体库接入BGM/MP3与BIK/OP。Linux五归档3.57 MiB，Windows五个clang-cl/MSVC ABI归档4.36 MiB，双clean构建的十个归档哈希一致；运行目录不再复制拆分FFmpeg库，Windows媒体DLL为1.22 MiB且无FFmpeg或MinGW运行时依赖。真实`Map_Ca12`与`Map_Eu08`循环、短Bink和完整opening解码通过；Linux core188/188、Linux app194/194及Windows LLVM app194/194均通过。两平台LGPL合规包均实际重新链接成功；Windows构建未删除输出目录，实机退出只重排TOML段落且配置语义不变；原版Miles/Bink动态差分保留`blocked_runtime_oracle`，状态为`module_closed_pending_oracle`。
12. `[x]` B6：78 个候选函数、TSW/ACT/ANI 运行时、公共动作记录、缓存状态、SND
    借用边界、生命周期和验证入口已形成唯一工作包，单模块开始条件满足；首个缓存容量
    策略单元已按 `0x00424330/0x004315C0/0x00432010` 实现；TSW 物理读取、六包
    路由、低 16 位键、特殊资源端口、命令流转换和原版十桶缓存顺序已经形成最小闭环；
    ACT 六包独占读、完整 32 位键、44 字节索引、零洞变体切片、两级缓存、失败节点
    保留和原版先淘汰顺序也已形成最小闭环。`0x98` 动作记录的选择性初始化、三段键
    重置、等待协议和 33 个命令字更新器已经按汇编实现；六个真实 ACT 包均已通过
    runtime→provider→updater 集成。Linux `core` 85/85、Windows LLVM `app` 89/89
    CTest 通过。ANI 的 `0x24` 头、独占打开、一基帧链、首次/重读差异、调色板和
    span 输出已形成首个闭环；19 个真实文件的 5,312 帧、6,057,767 个 span 和
    178,426,082 个索引像素全部通过。`0x004154A0` 的冻结快照、`-13` 揭幕、逐帧
    推进、`10030/10015` 两种结束阈值、释放顺序与外部端口也已实现。`0x00430C60..`
    `0x0043114C` 的 `0x2C` framebuffer 变形节点、双工作场、采样、衰减、径向注入，
    以及 `0x00416CC0` 的全链调度与完成摘链已形成闭环。`0x004163C0` 的 64 项状态、
    48 项有效块、16 帧刷新周期、第二套 RNG 顺序和下一物理行复制也已实现。Linux
    `core` 90/90、Windows LLVM `app` 94/94 CTest 通过。`0x00416B30` 的 service
    `0x13` 门、变体 78/79 双帧、宽高交叉裁剪 BUG、`0x2C` 绘制标志和目标跟随
    状态已形成闭环，真实 ACT/TSW 路径通过；Linux `core` 92/92、Windows LLVM
    `app` 96/96 CTest 通过。`0x00416590` 的 48/64 槽重置差异、条件 service 8、
    四步 RNG 创建、第 0 行跳过、包含上界拖尾、逐点饱和增亮和越底边计数
    异常已形成闭环；Linux `core` 93/93、Windows LLVM `app` 97/97 CTest 通过。
    `0x004167B0` 的 96 槽、只清计数器、条件 service `0x16`、四步 RNG、八段
    相位、九点饱和增亮、横向跨扫描线和越底边计数异常已形成闭环；Linux `core`
    94/94、Windows LLVM `app` 98/98 CTest 通过。`0x004161C0` 的四槽/四动作记录、
    只写 x 的重置、service 6、重生与扰动 RNG、八字节变体表、ACT→TSW→blitter
    及帧尾移动已形成闭环；真实四变体 framebuffer 哈希为 `0x53695F8D8D2219DF`，
    Linux `core` 96/96、Windows LLVM `app` 100/100 CTest 通过。`0x00415EE0` 的
    四发射器、角色 selector 符号不对称、节点链、完整 RNG 顺序、`i16` 回绕、
    map 50 颜色和复制后继删除异常已形成闭环；真实 variant 59 framebuffer 哈希为
    `0xFA22737232A60CF6`，Linux `core` 98/98、Windows LLVM `app` 102/102 CTest
    通过。`0x00415B70` 的三组四槽状态、四初始化/二更新差异、同值概率门、四向重生、
    共享动作绘制和 `i32` 回绕也已闭环；真实 variant 0 framebuffer 哈希为
    `0xE216591950463029`，Linux `core` 100/100、Windows LLVM `app` 104/104 CTest
    通过。ANI 组 11 个自有入口已经全部完成。最终逐地址审计把 78 个机械候选修正为
    59 个 B6 自有地址与 19 个跨模块转交项；三个公共动作绘制桥和真实
    ACT→TSW→blitter 路径均已验证，Linux `core` 102/102、Windows LLVM `app`
    106/106 CTest 通过。唯一缺口为已登记原程序动态差分，状态为
    `module_closed_pending_oracle`。
13. `[>]` B7：114 个函数的单模块工作包和真实地址范围已经建立，开始条件已满足；
    `0xD8` 角色布局、`0x00404FD0/0x004050B0` 格占用查询和 `0x00425BE0` 的 LMF
    会话子链已经实现，地图 22/24/500 真实验证通过。`0x00426840..0x004272B8` 的 CM
    命中、miss、容量淘汰、24 槽初始化、分块生成和完整槽读取也已按汇编实现，地图 24
    的 3,706,880 字节 RGB565 输出与再次命中通过；`0x00404610` 的九分支足迹碰撞、
    事件低字节、角色命中提前返回、原版异常步长和受检地图会话适配也已按汇编实现。
    `0x004261CE..0x00426798` 的事件、两种地图角色与三组空间链，以及
    `0x0040F2C1..0x0040F31B` 的受检格索引和 flags 投影已经闭环；地图 22/24/500
    的业务角色数固定为 49/29/1。Linux `core` 112/112、Windows LLVM `app`
    116/116 CTest 通过。`0x004038DB..0x00404094` 的四方向输入、八方向覆盖、列表重复、
    移动边界、动作/速度状态，以及 `0x004120F9..0x00412197` 的角色/相机坐标更新
    已实现；`0x0040BB50` 八方向占位、`0x00404510` 三带扫描和 `0x004040B0` 的
    单轴绕行/对角裁剪也已闭环。`0x00403AD7..0x00403DB6` 的修正/原始方向两次碰撞、
    地图事件 bit 副作用、地图/角色 Talk 构造、双方朝向与原始双次目标 action 刷新
    不对称也已实现。`0x0040D9E0/0x0040E672/0x0040DA60` 的 MAPS 区域、阈值、
    两次 RNG 选择，以及 `0x00403F43..0x0040406F` 的全部遇敌门和立即战斗切入也已
    闭环；当前游戏数据的 11 个阈值组、115 条区域和全部候选列表验证通过。Linux `core`
    119/119、Windows LLVM `app` 123/123 CTest 通过。`0x00412930` 的三条主体路径、
    清屏/clip、service/control 短路、公共尾部与四条地图底图路径已经实现，地图 24 的
    真实组合哈希为 `0x947C15A53487BF9A`，Linux `core` 123/123、Windows LLVM `app`
    127/127 CTest 通过。`0x00413EA0/0x00413F00` 的 group 0 bit-29 扫描、
    `0x00413870/0x00413910` 的 group `2→0→1` 普通角色绘制，以及 `0x00413CA0`
    距离音频已有实现证据；共享 jitter 保留残影继承上一笔绘制状态的原始顺序，普通
    角色 runtime adapter 已接入真实 TSW 与软件 framebuffer。两个角色路径哈希为
    `0xA6C3E08156F06060`、`0xA4766C928B05DC88`，Linux `core` 128/128、Windows LLVM
    `app` 132/132 CTest 通过。两条空间 stage 现已在 `0x00412930` 的实际 runtime
    原槽接线，共用角色数组、clip、framebuffer 与 jitter；真实 TSW 双路径叠加底图的
    整帧哈希为 `0xA6144A91E57939F9`。但这五个函数的权威 inventory 行均不从上述实现、
    集成、哈希或测试证据继承闭环状态：`0x00413EA0`、`0x00413F00`、`0x00413870`、
    `0x00413910`、`0x00413CA0` 各自仍为 `pending_audit`，必须分别完成独立的
    LST→C++→LST 审计后方可关闭。其余十七个 stage 保持显式转交，任何受检失败
    都在原 stage 停止而不伪报完成。Linux `core` 130/130、Windows LLVM `app`
    134/134 CTest 通过。`0x0041287F..0x00412923` 的地图 tile 层折返动画、单帧
    counter 保留、零帧异常、32 位回绕和选择序列视口恢复已按汇编实现；Linux `core`
    131/131、Windows LLVM `app` 135/135 CTest 通过。前置 `0x004148F0` 的选择序列
    入口门、游标/计数器、有符号相机增量及回绕也已实现，并和帧尾恢复形成组合 UT；
    Linux `core` 132/132、Windows LLVM `app` 136/136 CTest 通过。`0x004120B0` 外层
    coordinator 已按原槽接起玩家/相机位移、选择滚动、两次 audio service、世界组合与
    唯一呈现、对齐门控的帧后 transition、tile 动画及视口恢复；未恢复的动作/角色 stage
    继续显式转交。Linux `core` 133/133、Windows LLVM `app` 137/137 CTest 通过。
    `LegacyWorldRenderSession` 现已按 `header → 0x00426044/sub_426840 CM → surface`
    的外部数据顺序共同拥有 LMF 会话、CM 字节和底图 source；同时纠正 `sub_411620`
    为地图高度空间工作区而非 CM。8 位地图从 CM 首 `0x200` 字节建立并 forward 转换
    256 项 palette，地图 4/24 的真实整帧哈希固定为 `0xF00691829E9FE2D5`、
    `0x947C15A53487BF9A`。Linux `core` 135/135、Windows LLVM `app` 139/139 CTest
    通过。`sub_40F160` 的 MAPS 去前 `0x200` 字节载入、四个世界目录和七 word 初始
    记录，以及 `sub_40C130` 在 LMF 业务状态后追加 MAPS 角色、ACT 更新、最终格绑定和
    `sub_40D0C0` 相机的顺序已经形成初始世界 owner。当前游戏数据记录自身选择逻辑/归档
    地图 81，9 条既有角色加迁入的 GUID `1/10000/10001` 共 12 条；真实
    `MAPS + LMF + ACT` 集成通过。Linux `core` 140/140、Windows LLVM `app`
    144/144 CTest 通过。`0x004492BA..0x00449311` 的实际新游戏提交事件现已按原顺序
    接到该 owner；`sub_40AD10` 当前地图私有 TSW 帧加载器也已接入资源号 `0xFFFF`，
    覆盖 direct-16 与内嵌调色板两条转换路径。地图 81 的真实
    `MAPS + LMF + ACT + TSW + framebuffer` owner 已完成一帧普通世界组合，并抵达
    唯一画面提交槽及两次音频维护槽；Linux `core` 142/142、Windows LLVM `app`
    146/146 CTest 通过。`0x0040D200..0x0040D552` 的切换前角色同步现已闭环：load
    flags bit 0 在目标 LMF 载入前把旧运行时角色写回 MAPS，保留 GUID 查找、完整字段
    写回、`0xFFFF` patch 哨兵、16 位坐标截断、PATH type 8 固定 72 槽覆盖及原始回绕。
    当前游戏数据 1,371 条角色源中有 136 条初始 Path 非零，当前命令均为五；真实回归
    和合成边界 UT 通过，Linux `core` 144/144、Windows LLVM `app` 148/148 CTest
    通过。`0x0040CAD3..0x0040CCBC` 的角色物化后附加状态也已闭环：GUID 1 action
    覆盖、MAPS `+0x64` 的 18 字节门控目录、`sub_40D610` 在该调用点可达的 72 个
    `0x21C` 对象槽扫描/清空、MAPS flags 回写、八槽队伍追加，以及 flags bit 9 的四条
    16 字节记录均已接入初始世界 owner。当前首图十二条角色的 bit 7/bit 9 均为零，队伍
    索引零精确指向 GUID 1；Linux `core` 146/146、Windows LLVM `app` 150/150 CTest
    通过。`sub_40D610` 的共享角色转移 owner 随后完成全函数闭环：Path 非零时固定扫描
    72 个 `0x21C` 活动对象槽；移动中的角色先用 `sub_40AE20` 精确清除旧表面占用，再按
    八方向四像素步长回退到 16 像素网格，并复现 `sub_411530` 按 GUID 解链和重插；最后
    才执行 MAPS flags、对象槽和队伍副作用。MAPS 物化调用点已经复用该 owner，剧情
    opcode 65 的解释器接线仍留在 B7 后续范围。Linux `core` 147/147、Windows LLVM
    `app` 151/151 CTest 通过，不据此宣称剧情脚本或任意地图切换完整。普通世界帧最前
    `0x004120B7..0x004120F7` 的八个 HeadSgn 动作记录也已从 delegated stage 替换为
    真实 owner：启动时按原布局初始化动作 `0x232E` 的变体 `0..3`，每帧从槽 7 倒序
    到槽 0 跳过零 action id，并保留单次更新失败只诊断、不终止后续槽或整帧的行为；
    真实初始世界 ACT 路径通过。Linux `core` 148/148、Windows LLVM `app` 152/152
    CTest 通过。呈现后的 `0x00412719..0x0041287C` 也已从三个 delegated stage 收敛为
    真实 owner，按原顺序完成玩家空间链重插、表面格清除/重标、格索引移动、transition
    清零、三组 32 项历史、地图格 flags 投影和非致命动作校验；表面足迹 owner 同时由
    角色转移复用。Linux `core` 150/150、Windows LLVM `app` 154/154 CTest 通过。
    `0x004121A1..0x004124D1` 的固定 72 槽地图角色路径循环现已替代外部占位，按原槽
    恢复整槽跳过门、action wait、两级步长翻倍、对齐后的空间/表面迁移、到达 flags、
    三个有符号 action 覆盖、`sub_42D920` 后游标重读、GUID 1 清理、自动 Talk、非致命
    action 更新和 `sub_40D0C0` 镜头重定位；剧情路径完成仍保留为明确跨模块端口。
    Linux `core` 151/151、Windows LLVM `app` 155/155 CTest 通过。
    紧随其后的 `0x004124DC..0x00412681` 队伍角色循环也已替代外部占位：只遍历槽
    `1..count-1`，保留停用路径/等待状态仍更新 action、步长不翻倍、对齐后空间链只移除
    不重插、表面迁移、游标门位和 flags 投影；首图角色物化产生的队伍数量与槽已接入
    实际 frame state。Linux `core` 152/152、Windows LLVM `app` 156/156 CTest 通过。
    `sub_414570` 的脚本相机平移也已在原帧槽闭环：四个 remaining/step 字段、视口四边
    同步移动、逐轴精确归零清步长、共享更新体的非规范状态及全部 32 位回绕均按汇编
    保留；原 precompose 占位已删除。Linux `core` 153/153、Windows LLVM `app`
    157/157 CTest 通过。
    已在 rendering 模块闭环的 `sub_4308C0` countdown 随后接回普通世界
    `0x004126C7` 原槽，复用实际 action updater、TSW provider、framebuffer、clip、
    effect 与 jitter；primary/抑制门、静态动作延迟写入、`M:SS` 五片顺序及资源失败
    边界均由 coordinator 回归固定。原 fixed UI 占位已删除，Linux `core` 153/153、
    Windows LLVM `app` 157/157 CTest 通过。
    `sub_413FE0` 开发调试叠层也已完成全函数闭环：入口 16 点文字样式、两个精确等一
    内部门、五次 38×28 cell flags 扫描、原版奇数轮廓、七条诊断基线、地图事件高位后
    低位 flag 查询及附近角色/重叠文字均按汇编保留。调用者多压入的常量 2 已确认从未被
    被调函数读取，不再伪装成第三参数；最后一个 generic outer stage 已删除，SDL 文字
    端口接到 legacy glyph renderer。Linux `core` 154/154、Windows LLVM `app`
    158/158 CTest 通过。
    `sub_4147E0` 图片动作链随后在主/副两个原帧槽完成闭环：`0xA4` 节点继续固定
    `+0x08` 动作记录和 `+0xA0` 旧 next 槽，动作更新失败仍继续取帧/绘制，`+0x58`
    位置音效按 `screen + camera` 播放后清零，只有 `+0x8C == 1` 才摘链。两条链由
    `story_scene` 端口借出而非转成 world-map 全局状态；generic inner stage 从十七项
    减为十五项。Linux `core` 155/155、Windows LLVM `app` 159/159 CTest 通过，
    Windows app 成功链接且未启动任何 EXE。
    `sub_414B60` 世界移动动作链进一步完成精确布局与原帧槽闭环：早期语义子集已替换为
    完整 `0xB4` 节点，固定 `0x98` 动作记录、四个 i16 坐标、四个 float 运动字段和
    `+0xB0` 旧 next 槽；动作更新失败继续、严格可见边界、hold 门、移动前绘制、移动后
    严格目标窗口删除均按 LST 保留。真实 TSW 路径已进入同一世界帧，generic inner stage
    从十五项减为十四项。Linux `core` 156/156、Windows LLVM `app` 160/160 CTest
    通过，Windows app 成功链接且未启动任何 EXE。
    `sub_414CE0` 角色头顶动作链也已从早期中性精灵视图替换为独立精确 owner：固定
    `0xB4` 节点、`+0x98 current_x`、`+0x9A horizontal_motion`、`+0x9C target_x`、
    `+0x9E y`、16 字节保留段和 `+0xB0` next；动作更新失败继续、运动前绘制、
    `0/0x8000` 趋近分支、低 16 位三倍飞出和 `-120/760` 包含边界删除均按 LST 保留。
    opcode 81 创建、82 驱离和 86 改键的生产/变更合同已经反向锁定，最终 VM 接线仍留在
    `story_scene`。本链已接入 `0x00412930` 会合后的原帧槽，generic inner stage 从十四项
    减为十三项；两条 `0xB4` 链共同进入真实 TSW 后的逻辑 framebuffer 哈希为
    `0x3EAF7C3143994E65`。Linux `core` 157/157、Windows LLVM `app` 161/161 CTest
    通过，Windows app 成功链接且未启动任何 EXE。
    normal 世界帧中的七个连续环境效果 stage 随后一次性完成运行时接线：
    `sub_4161C0` drift、`sub_416590` streak、`sub_4167B0` spark、`sub_415B70`
    directional、`sub_4163C0` row-copy、`sub_416CC0` deformation 和 `sub_416B30`
    follower 现在共用实际地图尺寸、相机、16 位 framebuffer、pixel conversion、
    ACT/TSW 绘制端口及启动时播种的同一 secondary RNG。禁用门、streak/spark 的无条件
    概率 RNG 消耗、形变空链和启用后的 drift/row-copy 路径均由 runtime UT 固定；generic
    inner stage 从十三项降为六项，activity 分支另保留一项外部边界。
    公共尾部的 `sub_414E50` packed-row、`sub_4146F0` 全帧颜色过渡和 `sub_4153D0`
    限时消息也已从 generic stage 接回真实 owner：共用 owned framebuffer、当前 pixel
    conversion、启动期 16 项颜色表、secondary RNG 与 12 点 legacy glyph runtime；
    `sub_420490` 最后像素的 dword 读取由不参与上传/哈希的单 word guard 精确承接。
    `sub_4151F0` 的 LMF indexed object 随后完成装载期规范化与原帧槽接线：保留
    `0..30` 序号扫描、头插链反序、每序号首个相交对象、两种位移公式、16 位坐标回绕
    和逐对象 clip 恢复；地图 72 的 `1072x1024x16` 真实 command stream 已进入 runtime
    blitter。Linux `core` 158/158、Windows LLVM `app` 162/162 CTest 通过。normal 世界帧
    的外部 stage 由六项降为两项，activity 分支仍另有一项。`sub_4149B0` 的两个
    `0x2329` 动作记录随后接回软件鼠标/右边条原槽：Delete 变体 15、移动/空闲滑动、
    右上角特殊模式请求、Talk 门、主动作更新失败后继续和完整 flags/opacity 均按 LST
    保留；SDL 直接传入原始 DIK、逻辑鼠标和实际特殊模式状态。真实组合 framebuffer
    哈希更新为 `0x5889E0547682E179`，normal 世界帧只剩 `0x0042ED40` 一个外部 stage。
    Linux `core` 159/159、Linux/Windows `app` 163/163 CTest 通过，Windows app 成功
    链接且未启动任何 EXE；activity 分支仍另有一项外部边界。
    `0x0042ED40..0x0043017C` 已确认不是 world-map 私有的限时 UI，而是世界/其他场景
    共用的 `story_scene` 对话消息链驱动。首个有限检查点已完成精确 `0x4C` 物理记录、
    `0x0042EDCF..0x0042F11E` 四路开窗几何和 `0x0042F43A..0x0042FE14` 文字字节协议：
    `%Q/%N/%L/%P/%S/%C/D%/%G/%B/%A/%K`、DBCS、逐字显示、分页、选择热点和原地
    marker 变更均按 LST 固定，原 256/16 字节栈缓冲区只在越界点增加现代受检边界。
    Linux `core` 161/161、Windows LLVM `app` 165/165 CTest 通过，Windows 应用成功
    链接且未启动。随后 `0x0042F11E..0x0042F43A` 的输入/超时/分页/关闭门和
    `0x0042FA7C..0x0043017C` 的外层驱动全部闭环；固定 `440×121` 临时文字 surface、
    20/16 点字形、两套 16 色表、ACT/TSW 面板与 end/next action、caption 混色、
    合成、角色 owner 清理及链尾顺序已经接到实际 framebuffer。normal 世界帧不再有
    外部 stage；剧情 VM 的消息生产与选择输入仍按 owner 边界留给后续接线。Linux
    `core` 164/164、Windows LLVM `app` 168/168 CTest 通过，Windows应用成功链接且
    未启动任何 EXE。
    为形成首个可操作闭环，模式 3 的有限新游戏切片现已按 LST 接入 SDL：保留正常入口
    `-10` 计数、四个严格 hitbox、键盘回调顺序、默认 Big5 姓名的两段确认和第 105 计数的
    `0x004492BA` 提交，不使用临时快捷键。提交后沿既有 owner 打开地图 81；普通世界
    方向输入随后按四 transition 全零门、地表阻挡修正、地图步长和四帧格移动接入实际
    世界帧。当前数据回归确认 GUID 1 在起点向右每帧移动 4 像素，四帧完成 16 像素格并
    清 transition。Linux/Windows 完整应用 169/169 CTest 通过，Windows EXE 成功链接
    且首次实机已抵达模式 3 菜单；该次运行同时暴露 SDL 瞬时状态遗漏短按，以及原失焦
    路径主动最小化两个宿主问题。平台层现把 key/button-down 锁存到下一接受帧，并让
    焦点变化保持后台运行；原输入记录、重复节奏和显式最小化/恢复核心不变。Linux/Windows
    完整应用 170/170 CTest 通过。第二次实机日志确认输入已令选择值 `0→1→2`，但也暴露
    菜单坐标轴被实现层转置：`0x004A9924/0x004A9928` 经窗口中心初始化证明分别为 X/Y，
    原版菜单使用共同 X 和四个纵向 Y。ACT 选中残影、四个鼠标 hitbox 与姓名确认按钮现已
    按 `0x00448840/0x004490C0` 修正并由坐标回归固定；Linux/Windows 仍为 170/170，等待
    实机复测选择显示与新游戏提交。实机同时发现 SDL 主循环在 35ms 帧门之间纯忙等并占满
    一个逻辑核心；平台层现于每轮末尾执行 `SDL_DelayNS(100)`，不改变接受帧和输入时点，
    Linux/Windows 170/170 CTest 通过，CPU 降幅等待实机确认。后续实机日志确认主菜单约
    5 秒后才响应并非原版时序，而是把 `sub_448700` 的 `word_4FB8A8==1/2` 分支 `-120`
    错套到了正常启动。正常入口的参数为 0、计数器为 `-10`，现已按汇编纠正。另发现选择
    “离开”后，SDL owner 漏掉了 `0x00449320` 事件，导致阶段二计数器越过 105 后无限增长。该事件现已接到原版
    `0x04` 进程关闭位，并由同一接受帧的公共尾部同步关闭。随后按 `0x00448F8C` 与
    `0x004496B0..0x00449953` 接回两个 `0x20` 姓名输入对象：容量 8、坐标 `(300,230)`、
    `0x2449` 面板、20 点原字形、字节光标、编辑键、Enter/Esc、SDL 文本输入及平台边界
    UTF-8→CP950 转换均已接线；Linux/Windows 完整应用仍为 170/170 CTest，Windows EXE
    已重新链接且未启动。实机进入世界后追加现代窗口状态需求：`640×480` 游戏内分辨率
    保持不变，SDL letterbox 继续等比呈现；EXE 同目录 TOML 新增 `[window].width/height`，
    启动恢复、正常退出写回最后客户区大小，缺失或非法配置回退 `960×720`。世界交互/碰撞
    Talk 和遇敌入口仍是后续显式缺口。配置保留、读写回归及 Linux/Windows 完整应用
    170/170 CTest 均通过，Windows EXE 已重新链接且未启动。实机发现宿主最大化事件进入
    原 `SIZE_MAXIMIZED` 恢复分支后又调用 `SDL_RestoreWindow`，导致窗口立即弹回原大小；
    平台层现将最大化保留给 SDL 管理，显式最小化与恢复仍保留原生命周期。该修复在
    Linux/Windows 完整应用 170/170 CTest 通过，Windows EXE 已重新链接且未启动。配置
    随后补充 `[window].maximized`：最大化退出时保留此前普通窗口尺寸并写入 `true`，启动
    同时恢复普通尺寸与最大化状态；旧配置缺少该键时按 `false` 兼容。配置回归及
    Linux/Windows 完整应用 170/170 CTest 均通过，Windows EXE 已重新链接且未启动。
    原版 GUID 248/249 角色位置 oracle 已取得 71 组稳定样本，确认蓝衣角色使用
    `TSW 188/4` 与 `mode_flags=1` 水平反转，红衣角色使用 `TSW 188/8` 与
    `mode_flags=0` 正向绘制。由此定位当前角色装载把 `sub_40F280` 的 action mode 清零
    错放在 updater 之后，抹掉了 `IV` 产生的 bit 0；实现现已恢复为汇编的“清零→更新
    action→格绑定”顺序，并以真实 `MAPS + LMF + ACT` 固定初始化及首帧后的两个模式位。
    Linux/Windows 完整应用 170/170 CTest 通过，Windows EXE 已重新链接且未启动。
    随后 `sub_427300` 普通世界鼠标交互完成全函数恢复并接入 `step_world_interaction`
    原槽：选择链先于 NPC/地图 Talk、cache-only TSW 命中、双方朝向与动作刷新、地图格
    事件、光标覆盖、右键八方向合成和左键延迟输入复制均按 LST 实现；选择结果同帧交给
    已有对话 runtime。实现后已按新增规则再次从函数入口到全部返回独立核对，并据此纠正
    hover 查询不得 load-on-miss 及调用者 `0x0040A753` 每帧光标重置。剧情持久位和 Talk
    脚本解释仍是明确后续 owner，不据此伪报地图切换或对话已经完整。Linux/Windows LLVM
    完整应用 171/171 CTest 通过，Windows EXE 已重新链接且未启动。
    `sub_402F80` 的正常玩家入口随后接入配置键 16（默认 R）速度切换、200 ms 防抖、
    控制门、`sub_404C00` 面向角色搜索、面向 Talk、菜单模式 1 请求及两次碰撞 Talk；
    第二次完整汇编核对纠正东北搜索第五纵向端点。控制/对话列表仲裁、菜单模式 1 消费、
    剧情 VM、持久剧情位和随机遇敌 SDL 接线继续保留为明确后续 owner，隐藏调试热键不在
    本切片。Linux/Windows LLVM 完整应用 174/174 CTest 通过，Windows EXE 已重新链接
    且未启动任何 EXE。
    下一有限切片已完成 `0x00402030..0x00402F77` 的 23 函数 A* 内核：八节点池、
    反向搜索、open/closed 链、邻接传播、足迹外围阻挡、掩码覆盖和路径回写均已实现；
    每个函数完成后再次对照汇编，核对中纠正路径字节方向不能沿用搜索展开编号。原版
    超过 510 步只警告但继续产出的分支保留为观测位，不擅自终止。本节点当时尚未接回
    四类调用者，不伪报剧情路径可用；Linux/Windows LLVM 完整应用 175/175 CTest
    通过，Windows EXE 成功链接且未启动任何 EXE。
    `sub_406390/sub_406960` 两个路径请求随后完成并分别再次从入口核对到全部出口：普通
    角色保留 72 槽扫描、槽 32 异常返回和寻路失败直接改坐标；队伍跟随保留历史目标、
    路径复用、离屏四像素预推进、service `0x4F` 两次查询和当帧八像素步长。队伍请求已
    接入 `sub_405430` 对应的 `step_story` 槽并复用进程期节点池；普通角色请求等待
    `sub_405500` PATH.DAT owner，两个剧情调用者仍归后续 owner。Linux/Windows LLVM
    完整应用 176/176 CTest 通过，Windows EXE 成功链接且未启动任何 EXE。
    PATH.DAT owner 已从初始五项切片扩展为完整闭环：`sub_405500` opcode `0..36`、
    `sub_405430` 全部分派门、`sub_406580/sub_406710/sub_406770` 均完成汇编—C++
    双向收敛并接入原 `step_story` 顺序。收敛过程修正了 opcode 36 缺失目标只推进两个
    word，以及真实 path `169/170/171` 的绝对跳转会产生负 32 位 word 光标、寻址时
    必须保留符号。MAPS 的 136 个非零初始 Path、PATH 的 800 个有效非零目录项、当前
    资产使用的 20 种 opcode 和全部跳转目标均通过离线回归；37 个 opcode 另有汇编独立
    UT。动态标签改用受控 owner，opcode 36 的原版未初始化栈分支显式隔离；原程序动态
    差分仍为 `blocked_runtime_oracle`，未启动任何 EXE。
    B7 全集随后独立复核控制/碰撞组：`0x004040B0/0x00404510/0x00404610/`
    `0x00404C00/0x00404FD0/0x004050B0` 与依赖 `0x0040BB50` 已完成 LST 到 C++、
    C++ 到 LST 的双向追溯并关闭；同时删除了方向修正文档中仓库实际不存在的“36,000
    组随机参考模型”声明。`sub_402F80` 没有继承旧完成叙述，现明确保留隐藏调试热键、
    控制/对话列表仲裁、随机遇敌运行时接线和完整入口审计四项缺口。114 项当前关闭
    37 项，剩余 77 项继续逐项审计。
    随后的地图私有资源、占用与角色查询组又关闭七项：`sub_40AD10` 的有效 TSW 读取、
    解压和像素转换顺序已收敛，目录 `index == count` 越界及文件所有权归为平台隔离；
    `sub_40AE20/sub_40AEC0` 的锚点重复、三种足迹特例与精确位掩码已逐块闭环；
    `sub_40C020/sub_40C060/sub_40C0D0/sub_40C100` 已建立独立公共 owner。复核纠正了
    PATH 中“GUID 查找失败操作角色零”的旧结论：普通查找失败会把输出覆盖为
    `0xFFFFFFFF`，相关原调用点的数组越界只在现代消费边界隔离。定向 Linux 测试
    6/6 通过，完整 Linux `core` 179/179、Windows LLVM `app` 184/184 CTest 通过，
    Windows EXE 已重新链接且未启动；114 项当前关闭 44 项，剩余 70 项继续逐项审计。
    MAPS 角色 helper 组随后独立关闭七项：`sub_40D060/sub_40D0C0/sub_40D160/`
    `sub_40D200/sub_40D3C0/sub_40D460/sub_40D560` 已完成双向逐块收敛。复核纠正默认值
    对 `role+0x2C` 只写低 16 位，补齐源角色物化共享 owner，并固定两个相机 helper 的
    X 前导量分别为 `0x130/0x140`。有效 MAPS 目录内逻辑保持原样，损坏终止符、全局数量
    回绕和越界写归入明确平台隔离；Linux `core` 179/179 CTest 通过。114 项当前关闭
    51 项，剩余 63 项继续逐项审计。
    世界装载主链随后完成全函数复核：`sub_40C130/sub_40F160/sub_40F280` 已按完整 LST
    范围双向追溯，补齐描述字段的移动步长、tile 间隔、RGB 偏移、七个地图 service、
    方向效果四槽及 24 次 RNG 顺序、遇敌源顺序、原版非规范重复 GUID 游标，以及逐角色
    `动作初始化 → 格 flags 投影 → 条件表面占用`。Win32 退出、阻塞提示、裸文件/内存、
    音频进度和跨模块销毁明确归入宿主或具体 owner，故三个入口登记为
    `platform_adapted`；定向真实资产回归通过。114 项当前关闭 54 项，剩余 60 项继续
    逐项审计。
    角色转队与随机遇敌组三项随后完成独立矩阵复核：`sub_40D610` 的 72 槽扫描、移动中
    清占用、按方向回格、空间链迁移、MAPS patch 和队伍副作用，以及
    `sub_40D9E0/sub_40DA60` 的 14 字节区域扫描、头插反序、两次 RNG、有符号阈值、
    force 门、矩形首命中和计数清零均完成 LST→C++ 与 C++→LST 双向追溯。有效输入路径
    无差异；固定全局数组/裸链表改为受检容器，损坏游标、终止符、表号、候选数据与原版
    除零状态均显式隔离，因此三项登记为 `platform_adapted`。当前 `MAPS.DAT` 的 11 个
    阈值组、115 条区域和全部候选列表继续通过回归；114 项当前关闭 57 项，剩余 57 项。
    `sub_40D790` 随后恢复为独立角色/地图更新 owner：保留完整八槽物理扫描、逻辑人数外
    残留命中、移动中先清占用再按方向加法回格、`sub_411530(...,0)` 解链后重插、运行时
    七字段更新、MAPS 只同步 Talk/Path 并清 bit 7、两组数组固定搬到槽七和尾槽不清零。
    三轮实现后反向核对分别纠正了 MAPS 字段范围、空间链重插语义和 callee 失败仍继续
    后续副作用；高位零扩展、正负方向、`0xFF`、残留槽及真实 MAPS 派生向量均通过。
    损坏索引、方向、表面、空间链和目录归入平台隔离。opcode 66 的 16 字节分派接线明确
    转交 P1 剧情 VM；Linux `core` 181/181、Linux `app` 186/186、Windows LLVM
    `app` 186/186 CTest 均通过，未启动任一游戏 EXE。114 项当前关闭 58 项，剩余 56 项。
