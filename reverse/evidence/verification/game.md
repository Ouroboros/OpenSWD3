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
