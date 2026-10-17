# 逐函数汇编验证：存档界面与存档（src/save/、src/game/game.cpp）

## 第 1 轮

- 0x00406D30 Game::ResetSaveSlots：清 0x004B7CB0 起 0x200 字节（Controls::buttons，32 槽×16 字节）、0x004CC160=0、0x004CC161=1、0x004CC298=0、0x004CB240=0、重置 0x004CC200 播放，再释放三张预览并载入 3·(最新槽/3)+0..2、0x00409B80 复位动作（SaveSlots::LoadPage）。顺序一致。通过。
- 0x00406E00 Game::FreeSavePreviews：三次 0x004099C0。通过。
- 0x00406E30 Game::SaveScreenFrame：计数减一后 js／jle 0x3E8 判断（有符号）、帧数加一、状态 3 改 1、指针动作 0xD、按键、0x004CAEBC 为 0 走旗标页、为 1 走存档页、状态非 0 时绘制。通过。
- 0x00406EB0 Game::SaveScreenKeys：右键首帧按下转 Esc（0x00437300(1)）、F1／F2 仅在 0x004FBAB0 为 0 时 Sleep(500) 并切页、Esc／左 Ctrl／右 Ctrl 离开。通过。
- 0x00406F70 Game::FlagViewKeys：空格首帧切换当前旗标；左、右、上、下、Page Up、Page Down 依次覆盖步长（−1、1、−6、6、−120、120；按住超过 3 帧或首帧）；小于 0 回到 0x1FFF、≥0x2000 回到 0。通过。
