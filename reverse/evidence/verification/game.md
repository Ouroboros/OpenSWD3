# 逐函数汇编验证：game 模块

每节一轮对照：函数范围、对照的源码修订、原有 BUG、发现与修正的差异、结论。

## 第 1 轮：0x00401000–0x004014E0 全局对象的动态初始化（78 个）

- 源码：`src/game/game.cpp` 中 `Game::Game` 之前的注释块（提交 bb1e648）。
- 汇编：每个对象四个函数——初始化入口 `call ctor 包装; jmp atexit 包装`；构造包装 `mov ecx, offset 对象; jmp 构造函数`；atexit 包装 `push offset 析构包装; call 0x487BA0 (atexit); pop ecx; retn`；析构包装 `mov ecx, offset 对象; jmp 析构函数`。例外：`0x00401180` 组的构造包装 `0x00401190` 以参数 (1, 1, 0, 0, 1, 1) 调用 `0x00430C60`；`0x00401490` 组只构造（`jmp 0x004014A0` → `0x00437560`），不登记析构。
- 对象与重建：MemMgr 0x004B6C78（构造 0x438650／析构 0x4386E0，全程序无其他引用，不创建）、Miles 0x004A9AC8（0x485340／0x485350，`AudioManager::system`）、音效 0x004C8450（0x485930／0x485960，`AudioManager::effects`）、MIDI 0x004B7EB8（0x486580／0x484DC0，`AudioManager::midi`）、流音乐 0x004C9288（0x486580／0x4865A0，`AudioManager::streams`）、乐曲队列 0x004BA6E8（0x4853E0／0x485440，`AudioManager::queue`）、脚本对象链表头 0x004AC990（0x430C60／0x430CF0，`Scene::objects` 以指针代替头对象）、三种字体 0x004AB998／0x004C9A28／0x004A9ED0（0x4350E0／0x435130，`largeFont`／`mediumFont`／`smallFont`）、文件对象 0x004B74A0／0x004B8868／0x004B9F18／0x004A98C8／0x004CAA28／0x004A9380（0x438000／0x438030，`GameDatabases::maps`、`MapCache::lmf`、`GameDatabases::paths`、`GameDatabases::story`、`MapCache::unitFile`、`FieldView::file`）、未被引用的文件对象 0x004CAD50／0x004B7AD0（不创建）、DirectDraw 包装 0x004AB870（0x437560，`graphics`）、DirectInput 0x004B8748（0x436F70／0x436FA0，`input`）。
- 原有 BUG：无。
- 差异：构造顺序由 `Game` 的成员声明顺序决定，与原登记顺序不同；各构造函数只置初值、不做输入输出，顺序不影响可观察行为。
- 结论：通过。

## 第 1 轮：0x00402F80 RoleControl::ControlPlayer（src/role/player_control.cpp，含内联辅助 RoleControl::Approach）

- 调试键：调试模式下右 Ctrl＋右 Shift＋Home＋End 置旗标 0x52；有旗标时依次判断 F12（无领队、无对话窗时暂停 500 ms 后打开存档，菜单模式 1）、Ctrl 组合（F9 切换跳过 350 ms、Q 结束、S／L 存读档、F1／F2 切换调试显示、F3 翻转旗标 0x14，均停 300 ms；Tab 加小键盘 ± 调整 unk49E0BC 于 1..0x40）、F4（作弊对话框）、F7（金钱 +1000，停 250 ms）、F6（0x65..0x4AF 中种类 <0xA 或属于 {0x12,0x13,0x11,0x10,0xF,0x14,0x15,0x18} 的物体各加 10，链表参数取背包首节点的 next，停 500 ms）。
- 键位 16 按 0x004993C0 表切换跑步；四个移动量都为 0 才算静止；计数器淡出每帧 −2。
- 静止、鼠标未屏蔽、无菜单时：镜头卷动与单步的八个判定（0x130、0xF0、0x258、0x1C2 及地图边界）；空格／回车推进对话窗；有选项区时按下即释放（标志 0x1000）后返回；空格对前方角色（0x004993CC／0x004993EC）发起对话；回车开菜单（0x80000001）；playerHold 且无选项区时到此为止。
- 方向键：左右先于上下，斜向 2→4／6、3→7／5；有选项区时按住每 8 帧移动选项（无符号比较）；滑步（旗标 0x14 未置时）；两次 StepEvent（滑步后的与原始的）；角色事件取其 +0x1E；区域事件找不到时原程序读空指针（已记偏差），区域旗标已置则不移动；领队为区域事件时放在身后 0x10 步（0x00499380／0x004993A0）、id 0xFFFD；角色事件经 Approach（走路路径第二次运行的是对方的动作，与原程序相同）。
- 移动：卷动或单步的选择、走／跑动作（双击或跑步旗标）、方向或动作变化时清 delay、速度（跑步加倍、跳过时 0x10）、遇敌（地图旗标 0x1770+地图号、旗标 0xC、无领队；成功时停乐音、关档案、进入战斗、解除地图单元映射、清各角色 SoundOnce）。不动时：动作 0，待机 0x80 帧后 0x34，计数器淡入至 0x10。每帧 idleFrames、unk4CB220 计数。
- 表 0x00499380、0x004993A0、0x004993C0、0x004993CC、0x004993EC 已由 tests/game_state_test.cpp 与 exe 对照。
- 结论：通过。

## 0x004272C0、0x004272D0、0x004272E0、0x004272F0（静态对象 0x004CF6E0）

0x004272C0 构造 cFile 0x4CF6E0（0x00438000）并以 atexit（0x00487BA0）登记析构 0x004272F0（0x00438030）。该对象在其余代码中无引用（反编译只在 0x004272D0、0x004272F0 出现），重建不建立，源码注释说明。exact。

## 补正：0x00402F80 RoleControl::ControlPlayer 的调试日志（第 1 遍复查）

此函数的反编译把 nullsub_1 显示为无参数调用，早先的核对漏看了日志。按汇编逐处核对 13 个 nullsub_1：
- 0x004030EC／0x00403105：Ctrl+F9 切换跳过文字、Sleep(0x15E) 后「啟動快速模式」（0x0049F714）／「關閉快速模式」（0x0049F704），随后返回。
- 0x004032B9：Ctrl+F3 切换旗标 0x14、清 0x4CB220 后，旗标置位为「阻擋可通過」（0x0049F6F8），否则「阻擋不可通過」（0x0049F6E8），再 Sleep(0x12C) 返回。
- 0x0040336F：F4 显示光标后无参数调用，无文字可记。
- 0x004034DC：跑步键按下时先记「切換移動速度」（0x0049F6D8），再切换并 Sleep(0xC8)。
- 0x004037B7／0x0040380C：空格对话时对方转身失败记 "Act Err(KeyCole:Talk1)" + 0x0049F6B0「設定錯誤\nGUID%d ,act[%d] qq[%d] dir[%d]」（对方），控制角色转向失败记 "Act Err(KeyCole:Talk2)" + 0x0049F66C（控制角色）。
- 0x0040386D："\nRoleEventBy.GUID[%d].Talk[%d].Ridx[%d].RArgu[%x]"（标识 & 0xFFFF、事件、找到的序号、旗标）。
- 0x00403B84：区域找不到时 wsprintfA「錯誤的劇情點(%d)」后记录（已有）。
- 0x00403C42：区域事件 "\nMapEvent.Talk[%d]"（区域事件，未掩码）。
- 0x00403CEB／0x00403D38：走路碰撞时对方转身失败记 "Act Err(KeyCole:Collision1)"（对方），之后再次执行对方的动作（原版问题，重建保留）失败记 "Act Err(KeyCole:Collision2)"（控制角色的字段）。
- 0x00403DA8："\nRoleEventByCollision.GUID[%d].Talk[%d].Ridx[%d].RArgu[%x]"。
- 差异（diff 轮）：重建缺 8 条，另有 3 条自编文字（"Act Err(approach)"、"talk to role"、"map region event"）。已全部按原文改正，Approach 增加序号参数。复核一致（exact 轮）。

## 0x0042E790 Game::ChangeMap（第 1 遍）

清按键与步长，置 0x004C9A18 位 0，动作参数 0xFFFF 时取受控角色的 act、action、direction（dword），调用 LoadCity 后清位 0，返回 1。通过。

## 0x004303D0 SaveScreenshot、0x004306C0 Game::DrawWrappedText、0x00430B60 Game::StartTimer（第 1 遍）

- 0x004303D0：复制并转回 RGB555 后才打开文件（OPEN_ALWAYS、GENERIC_WRITE），失败返回 0；文件头 0xE 字节、信息头 0x28 字节（24 位），行自下而上，3 字节一像素（蓝 = 低字节 << 3、绿 = >> 2 & 0xF8、红 = >> 7 & 0xF8），行宽 4 字节对齐，每 15 行 AIL_serve；最后取文件位置写到偏移 2，偏移 0xA 写 0x36，关闭后 AIL_serve。通过。
- 0x004306C0：大字体，颜色 (0x19,0x17,0x11)、阴影 (6,4,3)，样式 4；%N 或超宽时画出当前段，未满行数则换行（+0x19），否则继续在本行；%C 后数字直接作颜色值；x = 前段字数 × 11 + x。通过。
- 0x00430B60：第二计时器清 0x004BAB78/7C 并置旗标 0x4A；第一计时器清 0x004C97E8/EC、记事件并置 0x10、0x12；帧数 = 30 × (秒 + 60 × 分)。通过。

## 0x00431150 MaintenanceAssert、0x004311C0 ShowDeviceError（第 1 遍）

- 0x00431150：wsprintfA(消息, 值)，再按 0x004A5DD0 "\n檔案: %s\n行數: 第%u行\n錯誤: %s" 排版，标题 0x004A5DC4 "維護錯誤"，MB_ABORTRETRYIGNORE | MB_ICONHAND（默认按钮为中止）；中止：SendMessage(WM_CLOSE) 后 int3；重试：int3；忽略：返回。差异（diff 轮）：重建 Windows 对话框的回车默认按钮是“忽略”，改为“中止”；中止与重试的处理记为平台替换（deviations.md 0x00431150）。修正后通过（exact 轮）。
- 0x004311C0：22 个 DirectDraw 结果码各显示原文（含尾随空格），标题为参数，MB_OK；其他值不显示。通过。

## spline 0x00437EB0..0x00437F90（第 1 遍）

- 0x00437EB0 WeighColumns：每列 row0 × v0 + row1 × v1 + row3 × v3 + row2 × v2（依次相加）。0x00437EF0 Weighted：x 按点 2、1、3、0，y 按 0、2、1、3 相加。0x00437F40 BasisWeights：t² 留在寄存器（0x00437F5B fld st）再乘 t，权重 (t³, t², t, 1)。0x00437F90 Evaluate：t 存为 double 取 floor + 2.0 经 _ftol 得段号，t − floor(t) 存 float，取段号 − 2 起的四点。通过。

## 0x004517A0..0x00451930 战斗全局对象的动态初始化（第 1 遍，22 个，2026-10-18）

- 0x004517A0/0x004517B0/0x004517D0/0x004517E0：0x005029D0 的十个战斗角色（每个 0x2F34，向量构造迭代器 0x0048A4C0 以 0x0046E490 构造，atexit 登记以 0x0048A560 与 0x0046E4D0 析构）→ `battleRoles`。
- 0x00451800/0x00451810/0x00451830/0x00451840：0x00525508 的八个敌人（0x2B28，0x00475560/0x00475590）→ `battleEnemies`。
- 0x00451860/0x00451870/0x00451880/0x00451890：0x00521598 的战斗单位（0x00478250/0x00478300），全部汇编中只在这四个函数出现，重建不创建。
- 0x004518A0/0x004518B0/0x004518C0/0x004518D0：0x0053B0B8 的投影器（0x00433C40 构造，atexit 0x00433D70 释放）→ `BattleState::projector`。
- 0x004518E0（跳到 0x004518F0）/0x004518F0：0x004FF5B8 的 battle.ffd 读取器以投影器地址构造（0x0045F0F0，0..29 与 n × 5 / 4 的表），未登记析构 → `BattleState::file{&projector}`（只取地址，成员声明顺序不影响）。
- 0x00451900/0x00451910/0x00451920/0x00451930：0x00521538 的 cFile（0x00438000/0x00438030），全部汇编中只在这四个函数出现，重建不创建。
- 结论：通过。
