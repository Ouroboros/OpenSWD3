# 战斗模块核对记录（第 1 遍，2026-10-18）

## BattleHud 0x0044FFE0..0x00451730

- 0x0044FFE0 DrawTileBox（第 4 帧取色填充 16 × (n + 2)，帧 0..2 顶、3/5 两侧、6..8 底；有符号字节填充色）、0x00450270 DrawPictureFrame0（参数 0）、0x00450490 DrawBar（宽 > 0 才画）、0x004504E0、0x00450530 DrawGauge、0x004505B0 DrawGaugeAlt（恒返回 1）、0x00450630 DrawGaugeWide（宽 + 2）：模式 0 的复制画法不读第 6 参数，省略调色板参数等价；通过。
- 0x004506B0 DrawSmallNumber（四位有符号除法，至少一位，从个位向左以当前图宽递减）、0x004507A0 DrawNumber（十位，字体在 0x004FDD08 高字，个位必画）、0x00450900 DrawNumberDigit（无符号除法，0x004FD784 为 0x8000 时模式 0x20，返回当前图宽）、0x004509D0 DrawSlotAnimation、0x00450A50 FillColor（参数本身为源，模式 8）、0x00450B40、0x00450B60、0x00450BD0 DrawDigit（字体 0/1 取 0x2359/0x2358，其他重画上一张；0x4000 时模式 0x20）：通过。
- 0x00450C50 DrawScrollBar（顶 0x1A/0x1E、轨道 0x18、滑块 0x19 长 Ftol(7 / 总数 × 轨道)、步长 (轨道 − 长) / (总数 − 7)、最短 5、到底时伸到轨道末、裁剪 x..x + 0x20、底 0x1B/0x1F；第一个动作沿用上次状态，之后清零）、0x00450F90 DrawCoinToss（帧计数为 0 时：高字非 0 全清，否则 Random(2) 定低字；画 0x2329 的帧 0（图片号 0）；每 25 帧延迟加倍（16 位），< 0x40 时倒数到 0 翻面并以音量 1 播 0x2E，否则延迟复位 0x10001、落在 1 面时高字加一、清状态返回 1）、0x00451100 DrawTimingBar（三条标记 0x004A74D0、指针 (计数 >> 1) + 1 在标记 0..+2 内为命中，等于 0x004FDD08 低字时取消，否则清该低字；到 0x3D 时置 0x8000 返回 1）、0x004512B0 DrawTallGauge（第 2 帧高 / 6 × level，level < 6 时裁剪，光泽模式 0x14 透明度 8）：通过。
- 0x00451420 LoadAnimation（+0x88 字节非 0 时记波形；按图片号载入一次并右移 640 / divisor；+0x42 为 0 结束；越过 6 张的图片号原版读写栈外，重建跳过）、0x00451540 DrawAnimation（波形 0x004CD724 只在这一笔）、0x004515E0 PlayAnimationShifted（每张新图片按正负左右移后画一次，+0x44/+0x46 清零）、0x00451730 FreeAnimation：通过。
- 0x004502B0 DrawPortrait（0x2390 动作 index；高亮时 0x07E007E0 描边；dim 为 1 时绿蓝 −10 红 0、模式 +0x18 | 0x10 再画一次）、0x00450400 DrawArrow（四个槽 0x004FDD10 + 0x98n，0x2392 动作 0，模式 +0x18）。差异（diff 轮）：原版这两处把图片 +4 的调色板指针作为第 6 参数（模式来自动作脚本或含 0x10，可能读取该参数），重建传默认参数；改为 PaletteArgument。修正后通过（exact 轮）。
- 0x00450A80 DrawEffectAt：动作运行一步，mirror 为 1 时翻转位 0。差异（diff 轮）：镜像时原版 `mov dx, [eax+0Ch]; sub dx, si`（0x00450AE2）只把偏移的低字替换为 宽 − x，高字保留 x 的高字，不镜像时为完整 32 位 x；重建两种情况都截成 i16。改为按原式组合。修正后通过（exact 轮）。

## 战斗开始 0x00451940..0x00451B10

- 0x00451940 LoadBattleBackground（释放背景动画与图片；all_map2.tsw 的图片转 16 位、右移 640 / scene；阵型有动画且参数非 0 时载入动画；成功时三个暗色副本色调字设 −1）、0x00451A20 ResetBattle（释放三条名声表并清链头、清 0x004ACF50 的 0x180 字节、八个敌人与十个角色都以基类 0x0047D350 复位）、0x00451AE0 ReleaseBattleSurfaces：通过。
- 0x00451A90 CreateBattleSurfaces：两个离屏面，三个色调字 −1。原版按 GetSystemMetrics 的屏幕宽高建立，重建按逻辑画面（deviations.md 平台替换行）。通过。
- 0x00451B10 StartBattle：清零与填 −1 的各块逐地址对照 globals.tsv（含 0x00524788 起 18 条记录、0x004FF578 四个 1、结束动作 0x2329/0xC）；在队旗标 0x1E..0x21 计入角色数（原版此处不清零，重建同）；队伍顺序表；旗标 0xC9 置面板位 2；计时窗口 0x3C（旗标 0x1BB0 时 0x12）；中字间距 0x10；两色 (0x1F,0x1D,0x17)/(0xF,0xE,0xB)；投影器初始化；指针 (0x140, 0xC8)；重建离屏面；战斗对话；battle.ffd 文件头与阵型；无敌人时 ErrorLog "第%d號戰鬥場景資料錯誤!!" 并返回；Random(4) + 1 为场景数；背景；敌人（复位、0x5246E0 清零、位置与镜像、Setup、守卫）；角色站位 1..4 人（未用的站位保留旧值）与四个面板偏移（含旧站位）；角色 Setup 与倒下计数；整理道具与技能；装备、技能、背包与名字；三条量表 Ftol(值 / 上限 × 56)（灵力与体力按 i16）与区块值；守护者旗标表 0x004A7600 计数不清零地加到守护者数，≤ 2 时按顺序最多两个，否则 Random(8) 取未用且旗标置位的两个（0x00478670 作用于第一个战斗角色）；MeasureUnits、排序；每个敌人 Random(6) 次推进行动条；每个角色随机行动条；倒下数 ≥ 角色数 − 0x53BF26 − 守护者数（无符号）时结果 0x67。原版的 GetWindowRect 取窗口矩形为平台替换（deviations.md）。差异（diff 轮）：角色战斗动作表 0x004A75D8（按字读双字）为 {1, 2, 8, 0x11}，全程序只读；重建第 4 项写成 0x17，李靖在战斗中用错动作；改为 0x11。修正后通过（exact 轮）。

## 战斗开场、帧与背景 0x004527E0..0x00453580

- 0x004527E0 BattleIntro：音效 0xC0；主表面复制到第二个离屏面并按跨度逐行打包为图片（0x004014F0，0x10）；画第一帧（背景、排序、单位、底栏 0x234D、面板）并复制到第一个离屏面，style ≠ 0 时再打包；34 帧过场：0 为 −12 色调、高度 640 + n 的拉伸（模式 0x20），1 为 sin(n × 3.14f × 1/33) × 256 的左右摇摆缩放（先 +后 −），2 为缩放 4 × (0x100 − n)、上移 0x20n；之后 1 摇入 33 帧（先 −后 +），2 清屏，0 重画并以行滑入，且多取一次 Random(3) 不用；乐曲按战斗号选 Europa/Arab/China，不在三段内时播放安装目录（原版如此）；面板位 0x40 未置时 Random(2) 与 Random(100) 决定我方 (27..32) 或敌方 (55..60) 先攻，消息 "我方搶先攻擊!!"/"敵方搶先攻擊!!" 并置位 0x80。双精度常量 0x00499CF8..0x00499D08 的位模式已对照：通过。
- 0x004530A0 SlideBattleIn：先为每行取 Random(20) + 15（不用），之后每帧自底向上每行 x −= 0x1E + 2 × Random(20)，到 0 计数，计数 > 480 结束（临时离屏面的 GetSystemMetrics 为平台替换）：通过。
- 0x00453200 BattleFrame：曲停时重播；鼠标、按键、自动战斗、测量、排序、调试键（返回 0 时不画）；暂停时返回；行动单位记录每 0x10 帧取一次；0x0053C044 = 无行动单位且无当前角色；背景、行动排序、单位、石壁、命中、HitFrame 置位 1、底栏、当前角色面板框（0x233B、x 0x004A75A8[队伍]、0x18D..0x1D3）与光标 0x2391、面板、菜单、结束帧、消息、线、头、对话、抗性面板、两个计时器；旗标 0x11 返回 3；调试显示；结束判定与提示；淡出结束时亮起；震动或显示；截图 "c:\snap\%d.bmp"（1000 + n）：通过。
- 0x00453580 DrawBattleBackground：未暗化时背景（震动时左右移位并移位播放动画，否则播放动画与 0xC0 行起的灰带：< 0x14 加倍否则 + 0x16，到 0xC0 为止，模式 0x28 上下两段）、清震动、闪光（字节 0x004A75FE 有符号，640 × 384 像素的红与绿蓝对移位，每帧 −4 到 0 复位 0x10）；行动单位为 0x004A7630 且暗化时：着色方式重画后 TintPixels（i16 等级 × (i16 色调 >> 1)），否则复制 surfaces[等级 − 1]，每 2 帧等级加一到 2；结束暗化时逐帧减一（等级 0 时原版对空指针 Blt 失败，重建不画）：通过。

## RoleActionFrame 0x004539B0

- 倒下返回 1；命令 A 为 0 时命令 B 为 0 返回 1，否则 switch(0) 落入 default 返回 0；0x63 解除控制（0x004A762C 的敌人 SetHeld(0)/SetFacing(0)）；0x64 返回 1；0xC8 走向目标；0x12C 返回原位；0x190/0x195/0x196 命中两点、击倒五点并置伤害 −1；0x192 暗化着色且不记命中；0x194 连招（第一帧只写低字 1 并标记：角色先 4 后 0x40、跳过守卫者，敌人先 0x40 后 4；第二帧写 0x10002 即灰带 1；解除时不看守卫者）；0x199 击倒不置 −1、以 0x12C 结束返回 1；0x1F4 只记命中不扣血；其余返回 0：通过。
- case 2/3（法术与道具：置 0x8000、0x53BDEC、清表、0x004A75FC 低字 1、法术带 ActFlag13 时建 0x280 × 0x1E0 网格 0xC8 的涟漪；结束后清伤害，抗性面板显示时等待，道具以 0x00470E20 消耗（两个参数在被调函数中未使用））、4（魔法攻击）、5、6（灵契：已知 ≥ 0x14 次或可灵契时开始，失败等 0x28 帧，成功 0x1E 帧，消息帧 0x28，"靈契成功"/"靈契失敗"，全部敌人离场时结果 0x63（有符号比较 jl））、7、0xB/0xC、0xD（符咒：失手位 0x2000，行动队列前 8 格）、0xE（结果 0x62）、0xF（护驾：最多 2 个，面板位 4，0x00524268 越界项不写）、0x11、0x16（掷硬币：0x00478AE0 作用于第一个角色记下的敌人，计数用 != 1，找人用非 0，物品 0x5FD）、0x17（偷窃 "成功偷到[%s]!!" 帧 0x32，失败 0x1E）、0x18（姿势横扫，伤害按 i16 封顶 9999）、0x19（控制 "控制成功!!"/"控制失敗!!"）、0x1A（计时攻击）、0x1B、0x1C/0x1D/0x20（饰品增益：0x1D 先给敌人再落到 LABEL_66 再给自己，原版如此）、0x1F（五秒计时连击）、0x21（必杀：0x00474B60 的 70% × 熟练度 / 100 + 10 与 Random(100) 的 16 位比较）、0x22..0x24（自身，0x23 数字 movsx）：ecx 与比较方式逐处对照汇编，通过。
- case 1 普通攻击：目标为角色时先吸取（仍取同号敌人）再算吸血戒指（只算份额），目标为敌人时吸血戒指治疗自身后再吸取。差异（diff 轮）：击倒时原版只记击倒、闪白并以 0x12C 结束（0x00453ACF..0x00453B10，`or eax, -1` 只是填充值），不把命中伤害置 −1；重建照特殊招式的写法置 −1，使之后的吸取与吸血份额按 −1 计算；已删去。修正后通过（exact 轮）。

## 单位帧与命中 0x00455D60..0x00458DE0

- 0x00455D60 EnemyActionFrame：> 0x64 只认 0xC8 走向与 0x12C 返回；0x64 返回 1；1 攻击（打角色时先吸取后扣血（含灵力体力），击倒只闪白不记 0x502984，以 0x12C 结束返回 0；打敌人时击倒记 0x502984，返回 1）、2 法术（涟漪同角色）、6 吸取角色（暗化 −12）、7 角色倒下（原版以 target + 4 移除记录与比较当前单位，照做）、0xA 淡出、0xB/0xC 淡入淡出（0x1000 标记、字 2A0A = 8，0xC 在离场数 ≥ 敌人数时结果 0x63）、0xF 召唤（0x8019 帧，0xF0 × 0xDC/0x15E 两个，镜像、载入记录、Setup、标记 0x400）、0x11 逃跑（离场 ≥ 敌人数时清 18 条记录为 −1、结果 0x63；只剩被召唤者时复原单只）：通过。
- 0x00456680 RoleFrame：暗化或暂停时待机保持；对话允许时效果计时与行动条，满时回复，AI 与守护者 Random(敌人数) + 1 取未倒下的敌人并选指令（ActFlag1/0 记入 0x520E98/0x520E94），队员音效 0x2C 后入队（StatusBit5Or8 时直接开始回合）；队首未被控者取得面板；未入槽者插入记录（被瞄的敌人只剩一个或结果 0x63 且无待行动时先复位回合，比较为有符号 jg）；单体与横扫指令（0x00520EA0 计数与 jl 有符号比较、SetAC0 作用于目标单位）；行动帧（RoleActionFrame，结束后命中数、RetargetRoles、被控角色、只剩一个敌人时角色效果到期、面向复原、回合复位与各表清零、对话倒数）；撤退（位 0x4000 时每人后退，全员后 BeatsLevel 作用于第一个角色比敌方最高等级（0x502984[角色号] ≠ −1 时取 200，原版如此），成功置 0x8000，失败 "撤退失敗!!"；0x8000 时按位记离场，全员离场 "撤退成功!!"、结果 0x68、记录全清零）；倒下移除：通过。
- 0x004576A0 EnemyFrame：效果计时与行动条（0x2AB0 为 0 且结果非 0x67 时入记录）；0x53BF64 或 0x53BF1C（按字比较）时不行动；被控或倒下的行动敌人结束回合；横扫汇集（jl 有符号）；选目标（StatusBit5Or8 时按场上敌人数在随机角色与随机其他敌人间选择；否则随机非 AI 角色，ChooseHeal 时打自己）；选指令（捕获判定以队伍首位等级逃跑 0x11/字 2；无命令时 ChooseCommand；命令位 0x8000/0x4000/0x2000 与低字节目标；倒下目标顺延到第 9 号为止）；行动与收尾（横扫逐个减命中与归还护驾（多调 0x004750C0），单体先归还后减命中；0x53AE70 < 8 时（有符号 jge）清行动记录；目标角色效果旗标 3 时以 0x235E 反击）；待机、命中动作与倒下移除：通过。
- 0x004582B0 HitTargetFrame：涟漪战（移动攻击沿线走到目标并在 0x3E8 帧后保持 0x28 帧，非移动每 9 帧在目标处 Random(100) + 0x32、Random(80) + 0x3C 溅射，0x64 帧结束）或直接读图（攻击者偏移、锚点或目标位置，镜像，音效按侧平移 ±0x10，移动攻击走线），暗化；跟随动作（暂停保持、目标位转交、按侧平移）；目标位（2 跟随、0x8000 地震、8 推移 0x1000/0x2000/0x4000 与 0x400 场地淡化、4、1/0x10 伤害（ActFlag14 时抗性面板，按攻击者种类取伤害，封顶 9999，−1 不显示，灵力 0x2367 与体力 0x2366 数字各延 8 帧））；淡出、推挤与结束：16 位截断与 ecx 逐处对照，通过。
- 0x00458DE0 HitAllFrame：偏移只在 x、y 都非 0 时取用；四种攻击者 × 镜像的落点（0xA0/0x1E0、y 0xEB）；沿 y 0 走线（槽 0 计数）；跟随动作不保持、只翻攻击者种类、清命中动作的音效（原版如此）；推移只有 0x1000；全体命中时角色侧（0x53BF64 且 0x53AE7C，或 0x53BF68）与敌人侧（数字延迟不复位，原版如此）分别处理，灵力体力显示后清零故记录为 0；推挤时着色暗化：通过。

## 命中动作、面板与排序 0x004599B0..0x0045B5E0

- 0x004599B0 PlayHitAct（地震位、动作一步（失败清跟随动作）、直接读图、镜像、偏移只在 x 与 y 都非 0 时取用、音效按侧平移、等待后清动作）、0x00459BF0 FadeHitAct（无动作返回 1，亮度（有符号字节 0x00521374）≤ −0x20 时归 0 返回 1，否则以亮度着色画一帧后减 4）：通过。
- 0x00459D10 DrawRolePanels：AI 角色列表框（名字取记录首部、56 像素生命条、行动条蓝 0x18、最后一项在 listGlow 位 7 时 +3 到 0x1C 的渐变高亮）；各队员面板（当前角色高亮取 0x004A75FF 的低 7 位，每 3 帧按位 7 升降、超过 8 置位 7，降到 0 的同一帧回升到 1）、名字条 0x234F 或倒下名字 0xF000、被控者名字闪烁（%4，0xF3F0）、头像（倒下变暗）与效果图标、区块条 0x2352（相等时取新值并以旧值画，下降 2/5、上升 1/3）与区块 0x38 时的箭头、生命/灵力/体力框与条（下降 1/3）、滚动数字（差值测位 27 并按位取反后 /10 + 1 为步长，体力的回零下限为 1，值 ≤ 0 时数字与显示一并归零）、生命数字字体按 > 10 与 > 上限 / 3 选 0x2354/0x2355/0x2356；比例为单精度（x87 以 56.0f 相乘后存 float）；自动战斗框滑入 (0x44 − x) / 3、"自動攻擊"：常量逐一对照，通过。
- 0x0045A980 DamageNumbersFrame、0x0045AA00 RemoveDefeated（敌人：奖励与 0x14/3 点、名声计数、离场数（有符号 jl）≥ 敌人数时结果 0x63；角色：被控者计数并清表尾站位、离场、取消指令与记录、当前角色时全体 SetAC0(0)、轮次表左移、全员倒下时 18 条记录全字节 0xFF 与结果 0x67，否则效果旗标 2 的角色接下一回合）、0x0045ADF0 RetargetRoles、0x0045B0E0 MeasureUnits（y movsx 存入）、0x0045B280 SortActingDrawOrder（插入排序与目标—行动者成对放置，读 darkFrames 的越界情形照做，表外写入不做）、0x0045B5A0 ListEnemyOrder、0x0045B5E0 UpdateUnits：通过。
- 0x0045AF90 PrepareDarkenedCopies：色调变化时两张副本分别为背景按 n × (i16 色调 >> 1) 的红与绿蓝对移位，暗化等级 1，0x004A7574 = 0x0053BD0C。差异（diff 轮）：循环中原版调用 AIL_serve，重建写成 AudioPump（0x0040CF10）；改为 audioDevice.Serve()。修正后通过（exact 轮）。
- 0x0045B190 SortDrawOrder：每轮取第一个 y 非 0 且未取的单位，再在其后的敌人与全部角色中取最低 y（相等时敌人优先）。差异（diff 轮）：没有可选单位时原版首选下标 18 并以 unitY 之后的 dword（0x00520E88，darkFrames）作 y；重建取 0（与 0x0045B280 读 darkFrames 的做法不一致）；改为读 darkFrames，表外写入仍不做（deviations.md 第 0x0045B190 行已改）。修正后通过（exact 轮）。

### 补正：0x00453200 BattleFrame

截图回调中原版 0x004303D0 调用的是 AIL_serve（与 game.cpp 中另一处截图相同），重建传入 AudioPump；改为 audioDevice.Serve()（diff 轮），修正后通过（exact 轮）。

## 战斗结束、命中帧与调试 0x0045B630..0x0045E7D0

- 0x0045B630 EndBattle：用脚本把伪代码约 230 条赋值或 memset 按地址映射到成员，再在重建中逐条找到对应行（tmp/endbattle_map.py）。非零值逐条核对：0x1F8 字节 0xFF、unk4FF578 四个 1、0x4A7568/0x4A7570 = 2、0x4A7580..0x4A7594 = 0x60, 0x60, 0xF0, 0xFA, 0xF2, 0x64、0x4A7574/0x4A7564 = −1、四个 word 0xFFFF、flashLevel 0x10、0x4A7628 = 6、0x4A763C = 1；浮点 0x4FDFA4、0x521394、0x525468、0x4FDF8C、0x521388、0x525448、0x52151C、0x525430、0x520D58 与 0x520FB8 即 fieldFade（由 0x0045D3E0 的写入确认）。随后关闭档案、停音效与音乐、关闭数据库文件，0x53BFFC 清零，0x53C154 清空：通过。
- 0x0045BD10 FadeFieldOut（三色以 0x4B000 像素移位、减 2、有符号 > −0x20）、0x0045BD90 PushUnits（推力 > 0 时减半（正数，sar 与除法相同）、镜像时为正、picture 与 movsx word_53AE7A 比较（jle）、滑回最多 0x1E、darkenByTint、restart 1 时设 0x1A4）：通过。
- 0x0045C010 HitFrame：角色与敌人两半逐个分支对照（全体与横扫、单体、HitAllFrame 与 HitTargetFrame、0x4A7626 = 0 或目标、全体时 flashing、单体打敌人与打角色写 0x4A7620 的高或低 word、5242B0[0] 的用法、吸取灵力块 0x472C70 → 0x4787D0(0x2367) → 0x47D640 → 0x47CEC0(1) → 0x47F150 的 ecx 都是被打角色（ebp = 0x5029D0 + 目标，0x0045C1C4、0x0045CA40）、横扫计数 53BD80 与 53AE74/53BD7C、结束时 4A75FC = 1 与清零的表）：通过。
- 0x0045D2F0 FieldFadeFrame（计数到负时把结束值复制进步长）、0x0045D3E0 StartFieldFade（ftol 的对象依次为三个起始值，x87 除法后存 float）、0x0045D490 AutoBattleTurn（SetTarget 等调用的 ecx 均为 0x5029D0 + 0x2F34 × (53BD50 − 8)）、0x0045D690 ApplyDrain（吸生命只作用于攻击者且不清伤害；吸灵力/体力取目标的第二个输出值（var_8）movsx 比较，目标显示数字只在截取时设置）、0x0045D810 WalkLine：通过。
- 0x0045D8F0 BattleDebugKeys：键码 0x1D/0x9D、0x3D、0x3B、0x2D、0x25、0x2C、0x20、0x21、0x2F、0x43、0x12、0x2E、0x3F、0x3C、0x11、0x23、0x24、0x19 与常量、字符串 0x004A7838/0x004A782C/0x004A7820 逐一对照；C 键的越界调用已列入 deviations.md：通过。
- 0x0045DEE0 DrawBattleDebug：坐标、格式字符串 0x004A792C..0x004A78C4 与英文格式；敌人行的参数顺序为生命（0x484500 的第一个输出）、0x4786D0、0x4786B0（均 and 0xFFFF）、记录 +0x54 的 movzx 等级；word_4A7630、word_4A7644 与 byte_4A7646 movsx；TswMem 为有符号 /1000；FRAME 为 idiv：通过。
- 0x0045E580 CheckBattleEnd（第一比较 jb 无符号，第二比较 jge 有符号）、0x0045E660 DrawBattlePrompt、0x0045E7D0 QuakeFrame（movsx 表 0x004A75EC = 4,3,4,3,4,3,2,3,2,3，两次 Blt 的源与目标矩形、清除 0x500 × step 字节）：通过。

## 归还、槽记录、战斗文件与按键 0x0045E9C0..0x0045FC60

- 0x0045E9C0 ReturnBattleItems（0x4FF2EC、0x4FF2EE 两个 word，非零才加；每个仍在外的投掷物加一个 0x300；清 0x53BF26、0x53C4B0、0x4FF2EC）、0x0045EA30 ReleaseBattle（10 个角色、8 个敌人）、0x0045EA80 TryRetreat（0x118, 0xA, 0x32, 0x004A7954 "無法撤退!!", 0x40000002；音效 0x8C）、0x0045EB40 ApplyHits（角色清 0x524400 + 4 × 单位 = 0x524420[角色下标]）、0x0045EC60 SetReturnedItem、0x0045EC80 CheckStoneSides（角色侧的人数为 8 位、0x67；敌人侧 0x63）、0x0045EDF0 AddSlotRecord：通过。
- 0x0045EE70 InsertSlotRecord：指令表字段对应（+0x14 ← +4、+0xC ← +0、+0x18 ← +8、+0xA ← +0xC 低 word、+4 ← +0x10），下移次数 v3 − at + 1（jl 有符号）。差异（diff 轮）：kind 1 且单位 < 8 时重建提前返回，漏掉原版之后的 0x53BF80、0x53BFD0 清零（越界取指令不做，已列入 deviations.md）；改为只跳过取指令。现有调用者 kind 1 时单位都 ≥ 8。修正后通过（exact 轮）。
- 0x0045EFB0 RemoveSlotRecord（最后一项读表外后被 0xFF 覆盖，无影响）、0x0045F020 TakeSlotRecord（单位 ≥ 7 视为角色；搬移到第 16 项为止；无空记录时从 at 清起）：通过。
- 0x0045F0F0 BattleFile（30 项 {i, 5i/4}）、0x0045F130 ReadHeader（0x2714 字节，计数表在 +0x1F44）、0x0045F1B0 ReadFormation（计数 movsx，jle/jg 有符号；记录号表在 +4；0x10C × 记录 + 0x2714）：通过。文件以共享方式打开属用户要求的平台替换。
- 0x0045F2A0 BattleKeys：数字键 2..9 对应指令 1..8，可用字节 0x524414..0x52441B，指令 5 的特殊流程；按钮编号 0 Escape、1 Space、2 RightShift、3..6 方向、7/8 翻页、9 RightCtrl、0xC Enter、0xE/0xF 鼠标、0x11 A、0x12 G 与 Controls 一致；G 键检查 0x53BF1C 为 word（cmp word）、撤退标记 or byte 0x53BF1D, 40h（= word 0x4000），SetTarget 的 ecx 为 0x5029D0 + (53BD54 − 8) × 0x2F34；滑块比较为无符号；确认键 6 帧重复：通过。
- 0x0045FC60 MouseHover：各状态的矩形、带宽与行列常量逐一对照（含 0x1E 状态的列从固定 0xEA 起、行找不到时换下一列、失败时清 0x4A7644；3 状态角色路径 RoleCycle 0x004A7970、循环后 SetAC0(0) 的 ecx 为 role[0x53C4AC]；0x433AA0 的 ebp 参数只作临时寄存器）。差异（diff 轮）：原版所有指针坐标比较都是无符号（jbe/jnb/jb/ja），指令菜单下标用无符号除法（mul + shr）；byte_53BDF2、byte_53BDF3 用 movsx。重建原先用有符号比较并把两个字节当无符号；改为一致。修正后通过（exact 轮）。

## 战斗菜单按键 0x00460C40..0x00462740

- 0x00460C40 MenuUp、0x00461240 MenuDown：各状态的步进、上下限与音效逐一对照；状态 3 的瞄准（AimStep）：我方路径人数 ≥ 4 走 RoleCycle（0x004A796C，检查 role[r − 1]，瞄准 role[r]；GetShadowRect 的 ecx 为 role[r]），否则以 0x4A754C 逐个角色（检查与瞄准都是 role[n − 1]），全体只处理 4 个角色并清头像高亮；敌人路径经 0x520DF4[n]（= enemyOrder[n − 1]）。差异（diff 轮）：① 原版回绕只检查移动方向的那一端（MenuUp 只查 < 1，MenuDown 只查 > count），0x4A7550/0x4A754C 留有瞄准另一侧时的较大值时照原值继续；重建 StepWrapped 两端都夹，改为单端（RoleCycle 表外的情形停住，deviations.md 新增一行）。② 状态 2 的 byte_53BDF2 与状态 8 的 byte_53BDF3 为 movsx（MenuUp 0x0046115A 赋值 0x4A7560 也是 movsx），重建按 u8；改为 i8。修正后通过（exact 轮）。
- 0x00461900 ScrollListBack：通过。0x00461A30 ScrollListForward：差异（diff 轮）为 byte_53BDF2 的 `cmp cl, 7; jl` 与 movsx，改为 i8；修正后通过（exact 轮）。
- 0x00461C10 MenuCancel（自动战斗分支 SetAC0 的 ecx 为 0x5229E0 + 0x2B28 × 0x4A754C）、0x004620D0 ConfirmKey（0x6E 状态的 0x1D/0x64 跳转、ItemName/SkillName 的 ecx 为 movsx word_4A762C 的敌人、GetPosition 为当前角色）、0x00462320 SelectPreviousRole、0x004623A0 SelectNextRole、0x00462420 SelectRoleUnit、0x004624C0 CanSelectRole、0x00462510 MenuRight、0x00462630 MenuLeft：通过。
- 0x00462740 MenuConfirm：状态 1（0x4FE5C0 + 2 × 指令的种类 word、各指令分支、SetCursorPos 与 0x437430 的 0xF0, 0xC6、末尾 NextEnemyTurn/PressConfirm）、2、4（含护驾 0x004A7980 "護駕召喚中!!"、0x004A7990 "體力點數不夠!!"、位置表 +0 actId +2 x 0x12C +4 y 0x10E/0x154 +8 kind、镜像 0x280 − x）、5、7、8、0x1B、0x1E 与结束状态 0x62..0x71、0xC8 逐一对照；ActSpeed（0x47D8B0）在零路径只清 ax，但各调用点紧接 `and eax, 0FFFFh`，+0xE 恒为 0。差异（diff 轮）：状态 3 在自动战斗且无当前单位时，重建设完几个状态后直接返回，漏掉原版其后与越界无关的处理（全体取消标记、头像高亮、0x53BD90/94、0x4FDC60、0x4A755C、0x53BFC0/C4/C8、PressConfirm、0x4A7564 = 0x4A754C、清指令名）；改为只跳过对 role[−8] 的调用和指令表之前的写入（deviations.md 对应行已改）。修正后通过（exact 轮）。

## 战斗菜单绘制 0x00464270..0x00466C00

- 0x00464270 DrawBattleMenu：被抓角色交出面板（清列表状态、取消指令、敌人取消标记、在 unk520DD0 中找下一个；名字下标沿用循环位置）、不能行动的角色立即行动；状态 1 的面板位置（指针 −0x10/−0x30、0x96 帧或指针出界（jl/jg 有符号）回到 0x122, 0xA0 并移动输入坐标）、状态 2/4/0x1B 的列表与滚动条（0x53BDF2 movsx、0x53BDF4 movzx，> 7 才画，x 0x190/0x19E/0x192）、状态 3 的瞄准指针（我方 ecx 为 role[0x4A754C − 1]，+0xA；敌人先 SetAC0 再取矩形与偏移，偏移 movsx；指令 6 由小型机械决定指针 0x238F/0x2393）、5/6/7/8/0x1E：通过。
- 0x00464CC0 StartRoleTurn（随机数 Below(enemyCount)，最多 enemyCount 次跳过倒下的敌人）、0x00464DD0 NextSelectableRole、0x00464E40 PreviousSelectableRole、0x00465090 NextEnemyTurn、0x00465170 NextRoleTurn（上界比较无符号，表外停住已列入 deviations.md）：通过。
- 0x00464E90 BuildCommandMenu：启用表 0x01010101/1、灵契（UnkPanel2，种类 6，0x004A79A0）、两个饰品槽（0x4C8AD0 + 4 × (16 × 角色 + 7)）种类 0x15..0x31 取 0x4A74A0 的名字、MarkBit8Clear/HasBlock38/MarkBit1 的处理。差异（diff 轮）：MarkBit1 时按 0x4A7548 读启用表（byte_524413[cmd]），重建直接以 cmd − 1 下标，cmd 为 0 时越界；改用 CommandEnabled（表外视为未启用，deviations.md 新增一行）。修正后通过（exact 轮）。
- 0x004651D0 DrawCommandMenu（largeFont 0x4AB998；怒气满（块 0x38）且非 MarkBit1 先启用指令 4 再重建；五个固定指令 4 行一列，x + 0x30 × (i >> 2)、y + 0x18 × (i % 4)，光标项以 0xF000 底色重画于 −1, −1；饰品指令在第二列第 2..4 行，CanUseAccessory 不通过时置暗并清启用位）、0x00465480 DrawListTabs、0x004655B0 DrawSpellList（GetSpirit/GetStamina 的第二个输出、i16 比较、说明取 role+0x2EC0 的节点）、0x004659C0 DrawItemList（说明取 role+0x2EC8，列出时已指向当前节点）、0x00465E50 DrawThrowList（0x004A76A0 "投擲"）、0x00466190 DrawRefineList（标题为 0x4A74A0[0x1E] = 0x004A7688 "煉符"）、0x00466500 DrawSpecialList（0x004A7648 "絕招"；0x53C184 只在列表内当场绘制）、0x004667B0 DrawEscortList（0x004A79D0 "佈置護駕"，0x12 字距）、0x00466C00 DrawControlMenu（标题为 0x4A74A0[0x19] = 0x004A76B0 "控制"、"特殊%d"、"釋放"；ItemName 只在返回 1 时写 id，v11 保留与否结果相同）：通过。
- 0x00466950 DrawTargetTag：名字宽 0x14 × (长度 / 2)、镜像时 0x276 − 宽 − x、名次表 0x4B9F00（FameList）≥ 10 显示 "生命:%d/%d"、≥ 15 画暗红行动条（0x484500、0x478340 的 ecx 为 0x5229E0 + 0x2B28 × 0x4A754C）。差异（diff 轮）：自动战斗无当前单位时重建以 currentUnit − 8 作下标读 pendingOrders（越界）；改为视为未设（deviations.md 新增一行）。修正后通过（exact 轮）。

## 战斗结束面板与讯息 0x00466F70..0x00469960

- 0x00466F70 BattleEndFrame：各结束状态逐一对照（0x61 GlowAct 的 ecx 为 role[roleCount]；0x65/0x6E 的 RestartMainAct（0x482030）只返回 0/1；0x63 中 0x475160 的对象为 movzx word 0x53BF1E 的角色（伪代码显示整个 dword 是假象）、炼符的等级取 0x4AB7BC + 0x38 × 队伍角色、目标敌人为 0x5229E0 + 0x2B28 × pendingAction）：通过。
- 0x00467710 DrawVictoryPanel（掉落按 0x4FF2F0 的 10 项找同种、新种追加；经验只给等级低于 0x4A762A 的角色；0x4B8A00 熟练度；战斗次数 0x4ACF54 + 0x60 × 队伍角色，敌人 ≥ 3 时加 2；"戰鬥勝利"、"每人得到經驗值:%d"、"得到銀幣:%d"、"得到法寶經驗值:%d"）、0x00467AC0 DrawLevelUpPanel（格式串 0x004A7A38 为 "%s"，压栈的等级参数不显示）、0x00467C50 CheckLevelUps（经验比较 jl 有符号；0x5028C0/0x520F80 只在本函数内使用；+0xA..+0x1E 各字段按 after − before 累加）、0x00467F00 DrawLevelUpStats（旧值 movsx；最大值比较 jle 有符号，属性 jbe 无符号；滚动在 0x1D 帧取差值，0x64 清零；7 个标签与 " ==> "、"%4d"）：通过。
- 0x00468930 DrawNewSpell、0x00468AD0 DrawNewSpecial（"%s學到新魔法"/"%s學到新絕招"、"[%s]"，后者在面板高度 0 时播 0x160）、0x00468C80 LearnSpecials（类型 0x1F 的技能设定所需战斗数 unk44 与下一招 unk42 | 0x80000000；0x1BB0 旗标时跳过 0x665/0x669；新节点挂在表尾）、0x00468ED0 DrawMastery（不运行面板动作，0x11 字距）、0x00468FF0 AnnounceMastery（0x46F850 的 ecx 为当前角色）：通过。
- 0x00469080 DrawLootPanel（"%-12s X %2d"）、0x00469220 DrawDefeatPanel、0x00469340 DrawCraftResultPanel、0x004694E0 TakePendingAction（4 × 10 队列）、0x00469550 DrawMessageBox、0x00469620 GrowPanel（有符号除法）、0x004698E0 Add（Rise 时 y = 1、+0x14 = −32）、0x00469960 Draw（绘制与滑出两段：0x50 步长、Rise 速度加倍、−32 以上保留）：通过。
- 0x00469650 DrawResistancePanel：分支与坐标一致；原版条长用单精度常量（0x00499D10 = −0.1f、0x00499D14 = 120f、0x00499D18 = 0.1f）并以 fstp dword 存 float，重建用 double 的 0.1；对全部 i8 抗性值逐一计算两种算法的 ftol 结果，差异 0 处（tmp/resist_check.py），等价：通过。

## 0x00469D20 FightTalkFrame（战斗剧本解释器）

逐条指令对照（0xFFFF、1..6、8..0x1F、0x21..0x25、0x27..0x53；7、0x20、0x26 与其他值为 default：返回 1 且游标不动）。各指令选择单位时的比较类型按汇编整理：jbe（无符号）为 2、0xA、0xB、0xE、0x14、0x15、0x1A、0x1B、0x1C、0x1D、0x27、0x2C、0x34、0x3B、0x3D、0x40、0x4C、0x4F；jle（有符号）为 5、8、9、0xD、0xF、0x12、0x17、0x28、0x36、0x3A、0x44；0x32、0x33、0x38、0x46、0x4A、0x4B、0x50 不比较、总按敌人表（0x525508 + 0x2B28 × 单位）取对象。其他核对要点：命令 2/0x2C/0x3B 的窗口（0x40AFF0 的 x、y 参数为 __int16）、命令 1 结束时 FightTalkEnd、0xE/0x3D 的列表计数与 LABEL_410..413 的跳转（空列表时游标不动）、0x13 只有多于一项时消费随机数、0x24/0x25 的线条效果、0x2A 的说话者（0x20 字节）与默认名替换、0x3A 敌人分支测试的是两个函数地址（恒非零，只前进游标）、0x3E 的 ftol 依次作用于三个起始值、0x41/0x42 的数量比较（movsx + jg）、0x4F 的伤害取负前为 movsx。

差异（diff 轮）：
1. 0x14：0x0046B978 用 jbe 无符号选择单位，重建先转 i16；改为 u16。
2. 0x17：0x0046AC17 `cmp word ptr [esi+4], 7; jle` 为有符号，重建用无符号；0x476160 把 act 零扩展写入整个 dword 0x53BD40，重建只替换低 word；角色分支原版以 dword 写入 order+0xC（+0xE 随之为 0），重建补写 +0xE = 0。均改为一致。
3. 0x1C：spirit 用 movsx word_53CE9A（0x0046BE23），重建按 u16 零扩展；改为 i16。
4. 0x24/0x25：原版 4 处 nullsub_1 日志（"對話系統(GAON) 代碼值太大(%d)"、"對話系統(GAON) 代碼值重複(%d)"、"對話系統(GAON) 位置超出螢幕(x%d ,y%d ,w%d ,h%d)"、"對話系統(G_AREA_OFF) 代碼值太大(%d)"，参数为第三个字或矩形的四个 movsx 值），重建缺；补上。
5. 0x3E：0x0046D55C `xor ecx, ecx` 后 0x0046D580 `mov cx, [esi+0Eh]`，帧数为零扩展；重建按 word 2 的符号填高 word（起始红色为负时帧数变负、淡化立即结束）；改为零扩展。
6. 0x38、0x46、0x4A、0x4B、0x50：原版总按敌人表取对象，重建对编号 ≥ 8 改写了角色或越界下标；0x32、0x33 也直接下标。改为只对八个敌人生效，超出时不读写（deviations.md 合并为一行）；0x49 除零补记一行。
修正后通过（exact 轮）。

## 战斗剧本文件与角色设定 0x0046E090..0x0046F850

- 0x0046E090 Unk2B00、0x0046E0A0 Unk2B04、0x0046E1E0 FightTalk::Seek（0x1000 字节，从偏移 + 0x200 读）、0x0046E260 FightTalk::Close、0x0046E390 FightTalk::Reset（不清 0x53CCE8）：通过。
- 0x0046E0B0 FightTalk::Load：0x204 + 4 × battle − 4 处读偏移、0x8000 字节脚本。差异（diff 轮）：原版以 `!result` 检查 CreateFileA（OPEN_ALWAYS）的返回值，失败返回 −1 通不过检查，于是照样置已打开、读取失败得到全零脚本；重建打开失败时直接返回。改为与原版相同（置已打开、脚本全零、游标 0）。修正后通过（exact 轮）。
- 0x0046E490 BattleRole::BattleRole、0x0046E4D0 ~BattleRole、0x0046E520 StepGauge（agility >> 2，Slowed >> 3，加速 30%、减速 30%、隐身再减 unk26DC% 的一半 + 4；满时清主动作效果、施法动作待机、转回朝向）、0x0046E6A0 ClearExtras、0x0046E730 Setup（放置记录从 0x53AF70 起，actId 在 +0x14、kind 在 +0x1C；"戰鬥角色actNumber為0!!" 第 0xDE 行；六项上限 9999）、0x0046E850 SetInventory、0x0046E870 SetSkills、0x0046E890 SetupEscort（"護駕actNumber為0!!" 第 0x123 行）、0x0046E9C0 SetupGuest（乘积截为 i16/i8 再除 10；"Npc角色actNumber為0!!" 第 0x14E 行）、0x0046EBB0 ApplyEquipment（"武器%d號的強攻擊act沒設!!" 第 0x182 行，投幣式鎧甲 0x39D）、0x0046EE60 Regenerate、0x0046F030 ApplyAccessory（元素宝珠的 i8/i16 截断与 /−10）、0x0046F1F0 UseAccessorySkill（0x24 取当前体力 +8；0x4830A0 的第三个参数 kind 在函数体中未使用，重建不传等价）、0x0046F6E0 StepOrbCounters（+12、u16 比较）、0x0046F850 FinishProficiency：通过。
- 0x0046F5B0 StepWornCounters：差异（diff 轮）：原版先 u16 相加（0x0046F60B `add [esi+6], bp`）再与上限 jb 比较，重建用 min<int> 不回绕；改为 u16 相加后封顶。修正后通过（exact 轮）。

## 角色技能、道具与攻击 0x0046F8C0..0x00470AC0；补正 0x00467AC0

- 补正 0x00467AC0 DrawLevelUpPanel：上一批依 IDA 注释把 0x004A7A38 当作 "%s"，实际为 "%s升第%d級"（名字、movzx 的等级字节 0x4AB7BC + 0x38 × 队伍角色）；重建只画名字（diff 轮），改为完整格式。修正后通过（exact 轮）。全量复查见 save.md 补正一节：74 处注释截断中，其余均已在重建中有完整字面量（0x434DD0 的两条 particle_put 消息框只在内存分配失败时出现，重建无此路径；0x4A7600 为数据表）。
- 0x0046F8C0 AttackFrame（淡入、居中 0x140/0x136、暴击动作 0x29 与武器施法动作、强制单位动作 0x28/0x30/0x31、反冲、地震、跟进与场地淡化、命中、飞行动作 slotActs[1] 的起点、等待后以 4 递减的暗化（> −32 有符号）、清除与连击）、0x0046FF00 RollCritical（随机数只在 8 次以上且不足 roleCount + 8 时消费）、0x0046FFE0 ShowAllSkills、0x0046FFF0 PrepareCommand、0x00470180 ListSkill（16 字节缓冲溢出得到元素号）、0x004702E0 CountSkills、0x00470380 CanPaySkill（"物品不夠!!"、"靈力值不夠!!"、"體力值不夠!!"）、0x004705C0 SelectSkill（两条失败路径都 `or ax, 0FFFFh` 返回 0xFFFF）、0x004707B0 LoadItemAction（局部记录 +0x50/+0x3E/+0x34；释放的是自身对象的说明）、0x00470820 FinishCommand、0x00470890 PayStamina、0x004708C0 TakeCommand2F18、0x00470900 ShowAllItems、0x00470910 ListObject、0x00470A10 CountObjects：通过。
- 0x00470AC0 SelectObject：流程一致；差异（diff 轮）：断言消息 0x004A7CDC 实为 "%d物品特效號碼沒設"（注释显示 "%d"），重建只用 "%d"；改为完整字符串。修正后通过（exact 轮）。

## 角色道具、吸收、投掷与法术帧 0x00470E20..0x00474BA0

逐函数对照汇编，全部一致，无代码改动（exact 轮）：

- 0x00470E20 UseSelectedItem（多次使用 0x400、次数掩码 0xFF、保留种类 0x80、0x8000000、无动作 0x2000）、0x00470F70 ListedObject、0x00470FE0 ThrowableObject、0x00471080 CountUsableObjects。
- 0x004710D0 StartAbsorb：y 取 `movsx edx, [esp+20h+var_E]`（16 位符号扩展）；屏幕尺寸按已记录的平台替换取逻辑 640×480。0x00471270 AbsorbFrame：火花表 5 项、动作 0x186A、粒子标志 0x56（WithinRange 0x40 | Mixed 0x16）。
- 0x004714B0 BeatsLevel（高于角色 35/100，低 1..7 级 70/100，8..12 级 90/100，≥ 13 必中，同级 0）、0x00471540 StepBack（2/6 终点、动作 0x2A、音效 0x2F 声像 ±0x10、每帧 0x10）、0x004717D0/0x004717E0 Word2A86Bit13/Bit12。
- 0x004717F0 ThrowAct、0x00471AD0 CatchAct（动作 0x186B，每帧多 8 线段步，到达判定为 x 相等）、0x00471D60 GlowAct（0x24 动作、±2、峰值 0x3E、tint = glowLevel/2 − 0x1F、结束音 0x6A；原版清的是反冲动作的音效，保留）。
- 0x00471FC0 FlyExtraAct：伪代码的 a5 − x 为 0x0047204F 的 16 位 `sub dx` 后 movsx，与重建 i16 一致。0x004721F0 PoseAtTarget（动作 0x2B、0x8000 镜像一次、阴影 −6/−1、声像以 0x140 为界）。
- 0x00472430 Steal（无物 25000；熟练度 < 100 时 0..9 掷骰减 熟练度/25 下限 0，≥ 难度失败）、0x004724D0 AttackPose（0x28；返回施法动作 | 0x8000 或 2）、0x00472710 CanTarget、0x00472730 CanAbsorb（三处掷骰比较均为 setle 有符号）、0x004728D0 MarkBit11Clear。
- 0x004728E0 DirectAttack：0x00472B7C `movsx eax, ax` 对应 HitDamage 返回 i16；阴影 y 为 0x00472A04 的 16 位 `sub di` 后 movsx。0x00472C70 DrainShare（0x37、步进 0xC、熟练度 >> 1，0x00472CAB 有符号 imul/除 100 + 1）。
- 0x00472CE0 CastAt（0x2D，不动 hold；0x00472E1C movsx 偏移）、0x00473010 MagicFrame、0x004731A0 DrainSpellFrame（飞回 0x47F940 参数逐个对上；0x47F150 第三参为 word 0x52441C）。
- 0x004735B0 BeamSpellFrame：0x004737F4 `cmp cx, 0FFE2h; jle`、0x0047397D/0x00473B45 jg 均为有符号；0x17FE、0x1F88、−0x1F、+2。
- 0x00473C10 ChargeAttackFrame：冲刺 `idiv 96h` 有符号（0x00473E26），0x10F4 = 2、0x10F8 = 3、0x10F9 = 1、0xC 蓝色；跟进坐标在 0x00474269 movsx（重建 i16）；0x53CEB4 只加低字节（重建 u8）。重建把内联的冲锋部分拆为 ChargeOnTarget，结果一致。
- 0x004745B0 MagicAttackFrame：0x004749CB `cmp ax, 0FFE0h; jle` 有符号，每帧 −8、hold 时加回；TargetsUnit 起点取站立动作 0x3B0/0x34C。0x00474B60 RollChance（0x46 × p / 100 有符号 + 0xA，16 位 cmp）。
- 0x00474BA0 SwarmSpellFrame：偏移表 0x004A7C04/0x004A7C14 与 SwarmOffsetX/Y 逐项相同；每 3 帧（i16 有符号取模）、上限 8、0x28、0x34、音效 0x3E、拖影 0xC。

## 角色收尾与敌人行动 0x00474E60..0x00476330

- 0x00474E60 LeapSpellFrame（阶段 0..3，0x00474F80 `cmp [esi+267Ch], 3; jl` 有符号）、0x00474FC0 Strike（sub_477830 参数 0x2F1A、0xF8、0xF6；0x00475062 `movsx edx, word_521520` 加成仅在伤害非 0 时加；闪避时只置 0x2B14）、0x004750C0 CancelCommand（命令 0x28 字节、名字前 0x10 字节）、0x00475160 Unk1EProficiency、0x00475180 Release、0x004751C0 BoomerangFrame（0x1BF3、去 0x1C 步、回 0x20 步抖 7 帧、无冲击 −1、音效 0x114）：一致（exact 轮）。
- 0x00475560/0x00475590 BattleEnemy 构造/析构、0x00475720 Setup（命名角色 agility + 6、attack + 0xA；maxHitPoints 取 movsx 的 hitPoints；撒旦 0x1C → 41000、蚩尤 0x2E → 120000）、0x00475820 Reload、0x00475870 Unk62And8A：一致。
- 0x004755E0 AdvanceGauge：差异（diff 轮）：0x0047561B `and eax, 0FFFFh; cmp eax, edx; jl` 为零扩展 gauge 与 dword 0x004A74CC 的有符号比较，重建把 gaugeFull 转 u32 作无符号比较（gaugeFull = 每档 × (档数 − 速度) 为负时结果不同）；改为 i32 有符号比较。修正后通过（exact 轮）。
- 0x004758A0 EnemyAttackFrame：动作 0x28/0x30/0x31 或 +0x2A0E，反冲、跟进与场地淡化、地震、0x11 命中、颜色暗化 0x53BF98、飞行用槽 0x6C8、每帧 −4 且 0x2C 标志直接到 −0x20、结束时 gauge + 10..129。两段内联命中合并为 EnemyStrike：RollEvade 的 ecx 为 arg_0（目标）；第二段闪避参数取被 sub_482E90 返回值覆盖的 var_4（0x00475C23），重建以 `chance = kind` 保留。差异（diff 轮）：目标反射（sub_47F360 = 1）时 0x00475C03、0x00475E41 `push 1; mov ecx, esi` 对敌人自身调用 sub_47CEC0，重建对目标计数；改为敌人。修正后通过（exact 轮）。
- 0x00476080 ChooseHeal（0..4 第一种、5..9 第二种，hitPoints 或 0x26B4 低于 0x4C/3）、0x00476140 CommandStatusBit（0xD98 bit 28、0xD94 bit 12）、0x00476160 LoadSkillAction（act 取 object + 0x50 = this + 0x60）、0x004761D0 PrepareCommand、0x00476250 LoadEnemyCommand（0x72/0x76）、0x004762F0 RollDrop（0..19 < dropChance）、0x00476330 CatchRoll（0x00476368/0x00476383 jle 有符号；0..11 掷骰丢弃）：一致。

## 敌人辅助、mon.dat/level.dat 读取与计数表 0x004763D0..0x00478330

逐函数对照汇编，全部一致，无代码改动（exact 轮）：

- 0x004763D0 RunAway（≤ 6 有符号结束、首图音效 0x10F、动作 0x24、fadeLevel 0xF 时 0x2F、每帧 0x19）、0x00476600 RefineTalisman（熟练度 0x32..0x4F 两选、0x50..0x64 三选；0x3C/0x5A；类型 0x8000000 返回 +0x48）。
- 0x00476780 ItemName（id 恒取 itemA，保留）、0x00476860 SkillName、0x004768D0 IsSmallMachine、0x00476900 CommandValues、0x00476920 SetResistances、0x004769A0 SetWords66、0x00476A10 SetWords72（第四值写 +0x74，保留）、0x00476A60 BattleEnemy::Release。
- 0x00476A80 Database::LoadSkill：opcode 0..0x19 的操作数宽度与标志位（3→1、9→2、0xB→4、0xE→8、0xF..0x11→0x10..0x40、6→0x80、0x12..0x15→0x100..0x800、0x16→0x1000、0x18→0x2000、0x19→0x4000；0xA 置 unk1E 高位）逐一对上。
- 0x00476DB0 Database::LoadObject：标签与字段偏移逐一对上；0x7D0 共 15 字节（抗性 9、0x9B、0x9C、+0x54、+0x52）；名字/说明的 "$$" 搜索 0xFF 上限与跳过字节数一致；读位置 0x204 后 FILE_CURRENT 4 × id − 4。
- 0x00477290 Database::LoadLevelEntry（偏移 4 × (index + 0x64 × group) + 0x70，经验在 opcode 0 的 +0x16）、0x00477400 LevelUp（0x00477524 `movsx ax, byte ptr [edx]` 取 opcode 低字节；断言 0x004A7D04 "升級魔法編號為0!!" 行 0x330；0x00477584/0x004775A2 16 位 cmp 节点 id，0x8000 不追加）、0x004776A0 CloseFiles、0x004776F0 ClearCounterList。
- 0x00477710 AddCounterCapped、0x00477780 SetCounterCapped、0x00477800 FindCounterValue、0x00477830 StepCounterProgress（伪代码漏掉 0x00477884 `fmul flt_499D28` = 100.0f，重建乘 100 正确）、0x00477920 SetCounterProgress、0x004779F0 FindCounterPercent、0x00477A20 SetProficiency（上限 0x00477A8B `and eax, 0FFFFh` 取 word；非空表从第一节点起找；done 不改）、0x00477B40 GetProficiency、0x00477BD0 LearnSkill（断言 0x004A7D38 "魔法編號為0!!" 行 0x50A；16 位 cmp）。
- 0x00477C90 Fame::Save（三个头部 dword 之和取低 16 位 × 0xE + 0x1E；压缩失败 "save fame compress false !!"）、0x00477F10 Fame::Load（计数写头部低 word；"Load fame decompress false !!"）、0x00478110 Fame::Free、0x00478220 FreeDescription。
- 0x00478250 BattleUnit 构造（targets −1、0x29A2 = 0xFFFF、0x2668 = 0xF、0x266C = 1、0x2A0A = 4、0x2A68 = 2、0x2A6A = 0x18、0x26BC = 0x062B062B，与成员默认值一致）、0x00478300 析构、0x00478330 SetTarget。

## 战斗单位访问器、待机、倒下与受击 0x00478340..0x0047C1F0

- 0x00478340 GaugeLength（× 62.0）、0x00478370 ResetGauge、0x00478380 RandomizeGauge（0x96 / (0..8 + 1) + 0x12C）、0x004783B0 GetDrawPosition、0x00478400 GetOffset、0x00478470 GetOrigin、0x004784A0 GetShadowRect（0x24 / 主动作动作 / 倒下 0x33）、0x004785A0 Move、0x004785C0 SetPosition、0x00478600 GetPosition、0x00478620 StepIdleAction、0x00478670 SelectEntry、0x00478690..0x004786E0 各取值函数、0x004786F0 ClearIdleAction、0x00478710 SetCommand、0x00478770..0x004787C0 Flag26B8 三个函数（0x2000000 阻止）、0x004787D0 PushTarget、0x004787F0 StartDefeat（默认姿势 6）、0x00478830 ToggleAF8、0x00478850 ResetTurn（清前 8 个槽动作；0x2A6A = 8；DelaysFadeIn 置 −0x20 与标记 0x200；角色 gauge 50..189）、0x00478A70 Retarget、0x00478AA0..0x00478B50 计数与取值：一致（exact 轮）。
- 0x00478B60 IdleFrame：晃动/升降（拆为 MoveIdle）的 0x00478BB0 `cmp cx, ax; jl`、0x00478BE5 `jg` 均为 16 位有符号；站姿、弱势 0x26（hp ≤ max/4）、就绪 0x2C/0x2D/0x31（0x1C 号动作取 0x2690，0x8000 回退 5）、护卫 0x48..0x4B、倒下 0x33、阴影（2B00 时 −1）、轮廓 0xFFFFFFFF/0x07E007E0、隐身 0x30、SG 透明度与波浪、屏障闪光、灰 0x28、命令 2 红 0x10、金钱数字 0x2354、命令 6 投掷 0x186A、覆盖图与效果：逐项一致。0x00478D13 对角色调用 nullsub_1 不带参数，按既有写法注释说明、不输出。一致（exact 轮）。
- 0x00479850 DefeatFrame：立即结束分支（角色记录 +0x25 |= 0x80、ExpireEffects(1)）；姿势分派 1..0xF、0x32、0x33、0x64 与各辅助函数逐个对照（剪裁矩形、tint、步长、终点；结束比较 0x004799E3 jg、0x0047A1A7 jle、0x0047A271/0x0047A75D/0x0047AA86/0x0047B752/0x0047B848 jge 等全为有符号）；case 7 第一块高度 h/2、第四块 y + 0x10，case 0x32/0x33 首帧播放施法动作的音效并把站立动作音效设为 0x31，HopHeights 与 0x004A7DB4 相同；LABEL_139/140/52/149/141 对应 EndDefeat / EndParticleDefeat。一致（exact 轮）。
- 0x0047BA80 HurtFrame：动作 0x24/0x25/0x27、0x004A7E08（movsx 字节）与 0x004A7DF0/0x004A7DF4（错开一个 dword，StaggerRise[5] 读到 0x004A7E08 的字节）三表与重建相同；敌人红色残影、阴影、闪光、覆盖图、反冲效果、每两帧一步到 5 后复位：一致（exact 轮）。
- 0x0047C1F0 DrawDamageNumbers：槽位字段（+0x2724 数字、+0x283C 跳动、+0x26DE 延迟、+0x295A 淡出、+0x29C4 字体）、DigitHops（0x004A7D8C）、淡出 0x30、缩放 + 0x1A 一致。差异（diff 轮）：额外数字在槽位已满时，重建不清除 0x2AE8 标志；原版先清标志再越界写入。改为先清标志、有空位才写入，越界部分记入 deviations.md。修正后通过（exact 轮）。

## 战斗单位辅助 0x0047C660..0x0047D640；同一偏移的重复成员

- 0x0047C660 SetAC0、0x0047C670 IsAE4Clear、0x0047C680 GetWordDA6、0x0047C690 MarkAC8、0x0047CC40 SetHeld、0x0047CC50 SetAD4、0x0047CC60 DrawFlash（2..0x18 上升再回落，绘制参数 0）、0x0047CD60 RollEvade（隐身 (0..9)+1 ≤ 9；指定几率 /10；连续失败 0x18 必中、0x14 起 1/3；闪避 ≤ 6 先过 80% 的否决，短路保持随机数次数）、0x0047CE70 Mirrored、0x0047CE80 IsDefeated、0x0047CEA0 StatusBit13、0x0047CEB0 IsAB8、0x0047CEC0 StepTargetCount（上限 0x22）、0x0047CF00 PushTargetAlt、0x0047CF20 DrawGuardText（跳动表即 0x004A7D8C；第 4 个字母 x 为 (u16)(三宽之和)）、0x0047D2E0 ScaleByTenths、0x0047D640 SetDamageDigits（伪代码的 (__int16) 截断不存在：0x0047D70E/0x0047D754/0x0047D78C 对 32 位 edi 做 imul；末位 0x0047D7B1 `imul cx, 0Ah` 16 位；屏障结束消息 0x004A7E20 实为 "%s抵抗所有攻擊解除"）：一致（exact 轮）。
- 0x0047C6B0 DrawFadeOut、0x0047C950 DrawFadeIn：差异（diff 轮）：原版 DrawFadeOut 的两次绘制与 DrawFadeIn 的非混合绘制以 `[picture+4]` 作第 6 参数（DrawFadeIn 的混合绘制为 0），重建未传；按原版补 PaletteArgument。0x0047CB32/0x0047CB45 锚点坐标 movsx 与重建一致。修正后通过（exact 轮）。
- 重复成员（diff 轮）：BattleUnit 中三处原版同一字段被拆成两个成员，读写互不相通：
  1. +0x2A78：HurtFrame/GlowAct 用 hurtStep，RaiseFlag26B8（0x00478780 清 +0x2A78）与 Reset 清的是 attackStage，再次受击时受击步数不归零。
  2. +0x2B20：SetFacingIfAble（0x00483FF0 写 `[this+0xAC8*4]`）与 AdvanceGauge、Reset 用 name，StepGauge（0x0046E520）与 DefeatFrame 读的 unk2B20 从未被写入。
  3. +0x2A80：Reset 清的是从不读取的 actionCount，AddQueuedAction 等用的 queuedActions 未清。
  合并为 hurtStep、unk2B20、queuedActions 各一个成员；受影响的 0x00478780 RaiseFlag26B8、0x004755E0 AdvanceGauge、0x0047D350 Reset、0x00483FF0 SetFacingIfAble 修正后通过（exact 轮）。StepGauge、DefeatFrame、HurtFrame 代码不变，读写的字段现与原版同一。
- 0x0047D350 Reset：清除的动作槽（0x2A0、0x338、0x3D0、0x468、0x500、0x598、槽 10 = 0xC20、槽 0..7）、数组（0x295A、0x29C4 各 0x46 字节、0xFCC、0x283C）、ExpireEffects(1)、角色释放 +0xC 与各字段逐项对上（0x2724、0x26DE、0x2A70、0x2A76 不清，与原版同）。

## 战斗单位状态、效果链与伤害 0x0047D7D0..0x0047F3A0

- 0x0047D7D0 ResetAfterGuard、0x0047D7F0 GetGuard（标记 0x42）、0x0047D810 SetStatus（0x200 置 brightLevel −0x20）、0x0047D830 ClearStatus（0x1000 同时结束护卫）、0x0047D860 SetWord2A0A、0x0047D870 SetGuarding、0x0047D880..0x0047D8F0 命令动作位与 ActSpeed（bit 15 时低 15 位）、0x0047D900 SetAction（清主动作）、0x0047D930 IsGuarding、0x0047D950 SetVitals、0x0047DA10 SetAttributes（角色 +0x26/+0x28/+0x16）、0x0047DA90 SetLevel、0x0047DAB0 SetB00、0x0047DAC0 SetB04：一致（exact 轮）。
- 0x0047DAD0 TickEffects：计时每次 + 0xF、满 gaugeFull 复位并减一回合；毒按战斗副本 +0x31 的抗性 ScaleByTenths；耗体力每回合 10。差异（diff 轮）：0x0047DB51 调用的是 sub_47E4C0（HasEffectElement，按效果元素 +0xA4 查找），重建调用 HasEffectKind（0x0047E500，按位查找，kind 5 时找位为 0 的效果）；改为 HasEffectElement(5)。修正后通过（exact 轮）。其余调用点（HitDamage 0x00481010 的 0x0047E580/0x0047E500 ×2/0x0047E4C0）与重建一致。
- 0x0047DBD0 AddAilment（MissedAilment 置标记 0x20；1..10 掷骰比 +0x1C/+0x5E；同位效果只刷新回合与数值，查找含头节点；新节点插在头节点之后；各位的动作 0x2AF8 帧号与图标 1..0xB；0x200 角色四项属性 −10%）、0x0047E070 AddBuff（0x4000000 为护罩计数；元素与整段 flags 比较；元素与状态位的 14 条消息 0x004A7E34..0x004A7F28 逐条读字节核对）、0x0047E4C0 HasEffectElement（0x0047E4D4 16 位 cmp）、0x0047E500 HasEffectKind、0x0047E580 HasEffectBits、0x0047E5C0 ApplyPendingDamage、0x0047E650 DrawEffects（第一个带动作的效果，含头节点；石化灰 0x28、0x1000 tint −12/0/−12）、0x0047E7F0 DrawEffectIcons（0x241D）、0x0047E880 CureEffects（复活位 0x8000；清除后从第一个效果重新查找）、0x0047E950 ExpireEffects（释放后跳过下一个节点；各状态位的清除与 21 条消息逐条核对；0x200 的属性恢复在 0x0047EAE1 等处为零扩展 32 位计算，伪代码的 i16 截断不存在；clearAll 清标记 0x142、状态与全部节点）、0x0047F150 Damage（0x0047F1DB/0x0047F278/0x0047F2C1 等比较均为有符号；金钱护卫在金钱非 0 时扣金钱；敌人倒下随机姿势 1..10）、0x0047F340 EffectFlag2、0x0047F360 EffectFlag3、0x0047F380 SetDefeatPose、0x0047F3A0 SetDefeatPoseFixed：一致（exact 轮）。

## 淡入淡出、飞行与排队动作、敌人选指令 0x0047F3C0..0x00480D40

逐函数对照汇编，全部一致，无代码改动（exact 轮）：

- 0x0047F3C0 FadeMainOut（glowLevel 每帧 −2 至 −0x20，清反冲动作；0x47F3C0 的参数未使用）、0x0047F580 FadeMainIn（brightLevel +1 至 0，清标记 0x200）、0x0047F710 FadeInvisible（至 0x18；阴影以主图为源、取阴影图宽高绘制；模式 0 置 Invisible，清标记 0x480）。
- 0x0047F900 SetFacing、0x0047F910 ActId、0x0047F920 IsHeldOrHolding。
- 0x0047F940 FlyAct：锚点 0x0047F9BE movsx；目标坐标差 0x0047FAC6 movsx（16 位）；-1 步数从 1 起倍增、超过 0x20 后 +0x30；到达判定为 x 相等。
- 0x0047FC40 RunQueuedActions：偏移 x 0x0047FD06 movsx；flown 比较 0x0047FD99 jg 有符号；抖动表 0x004A7E10 与 ShakeOffsets 相同；晃动节点沿用上一节点的到达结果（首节点未初始化已登记）；冲击动作未结束时原版在同一帧重跑整个节点，重建 `continue` 不前进节点，行为相同；DrawTrail 不改 0x254C。
- 0x004800F0 AddQueuedAction（第 8 参数为步数、第 9 参数为抖动帧，与调用方一致）、0x004801A0 TrailLastQueuedAction（空队列写表头已登记）。
- 0x00480220 ChooseCommand：选择顺序（治疗 > 物品/技能轮替 > 状态位 5/8 > 无空闲回合时丢弃 0..1 掷骰）、物品 0..4/5..7/8..9 三选（每 7 回合强制第三件）、技能 0..6/7..9 两选（每 5 回合强制第二个，仅第二个复位等待）、0..9 掷骰 < idleTurns + 机率、禁用条件（0xD98 bit 28 的强制单位、0xD94 bit 12 且 0x53BF28）、技能值为赋值、物品值为累加、0x1C 号动作的就绪姿势 0x2C..0x2F；拆出的 ChooseAttack/MarkDelayedAttack 即 LABEL_118/LABEL_123（标志 0x10 且命令 A 为 1 时改为 0xC8）。
- 0x00480AD0 Data、0x00480AE0 PlayIdleEffect、0x00480D40 PlayIdleFollowUp。
