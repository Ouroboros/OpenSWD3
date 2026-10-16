# 黄金数据（golden）

用旧逐句转写实现（标签 `legacy-transliteration`）对同一组场景生成哈希，作为新架构实现的逐像素对照依据。旧实现本身仍以汇编对照为准（第 3 阶段），黄金数据只证明新旧实现等价。

## 绘制核心（tests/golden/draw_golden.txt）

场景定义：`tests/golden/draw_scenarios.h`（两侧共用）。生成器：`draw_golden_legacy.cpp`。

```
git worktree add -f tmp/legacy legacy-transliteration
mkdir -p tmp/legacy/tests/golden
cp tests/golden/draw_scenarios.h tmp/legacy/tests/golden/
cp reverse/analysis/golden/draw_golden_legacy.cpp tmp/legacy/tests/
cd tmp/legacy
cmake --preset linux -DSWD3_DEPS_PREFIX=<工作区>/third_party/install/linux
cmake --build --preset linux
cd build/linux
GOLDEN_OUT=<工作区>/tests/golden/draw_golden.txt SWD3_GAME_DIR=/mnt/e/Game/swd3 ./swd3_tests golden
```

`GOLDEN_DUMP=<场景名前缀> GOLDEN_DUMP_FILE=<文件>` 可在两侧导出某个场景后的整屏像素，用于定位差异。

不能在旧实现上运行的场景（原程序会崩溃）：未分配的模式（空表项）、带 0x8000 段的镜像 Alpha 映射（空查找表）、原始图片的翻转与镜像半透明模式（空表项）。新实现对这些情况的行为由 `tests/painter_test.cpp` 覆盖，并记录在 `deviations.md`。

## RLE 编解码（tests/golden/rle_golden.txt）

场景定义：`tests/golden/rle_scenarios.h`。生成器：`rle_golden_legacy.cpp`（同样复制到旧树的 tests/ 下），运行 `./swd3_tests "rle codec"` 并设置 `GOLDEN_OUT`。

## tsw 档案（tests/golden/tsw_golden.txt）

场景定义：`tests/golden/tsw_scenarios.h`。生成器：`tsw_golden_legacy.cpp`，运行 `./swd3_tests "tsw archives"`。场景只请求实际存在的帧（越界帧号会把图形数据当作帧记录读取，原程序同样不可用）；直接载入的魔法图片不比较调色板（原程序带未初始化指针）。

## act 档案与动作脚本（tests/golden/act_golden.txt）

场景定义：`tests/golden/act_scenarios.h`。生成器：`act_golden_legacy.cpp`，运行 `./swd3_tests "act archives"`。脚本指针以“是否存在 + 前两个字”代替比较。

## TextFont（tests/golden/font_golden.txt）

场景定义：`tests/golden/font_scenarios.h`。生成器：`font_golden_legacy.cpp`，在旧树中以 `SDL_VIDEO_DRIVER=dummy` 运行 `./swd3_tests "text font"`。Linux 上两侧都不栅格化字形，缓存填满后按字符码写入相同的合成位图，再比较各样式、裁剪、背景与渐变的绘制结果。

## 投影与图片操作（tests/golden/projector_golden.txt）

场景定义：`tests/golden/projector_scenarios.h`。生成器：`projector_golden_legacy.cpp`，运行 `./swd3_tests "golden: legacy projector"`。爆炸场景先标记为已开始并以场景种子调用 srand（首帧原本以当前时间播种）。原程序越界读写的位置，旧实现一侧的缓冲区补上跳过色、填充色或 0，使其结果等于新实现按 deviations.md 记录的处理。

## 寻路（tests/golden/path_golden.txt）

场景定义：`tests/golden/path_scenarios.h`（随机地图，四周四格宽的阻挡边框）。生成器：`path_golden_legacy.cpp`，运行 `./swd3_tests "golden: legacy path finder"`（不需要游戏数据）。旧实现一侧的阻挡数组前后补上全阻挡格。

## mon.dat 与 level.dat（tests/golden/database_golden.txt）

场景定义：`tests/golden/database_scenarios.h`。生成器：`database_golden_legacy.cpp`，运行 `./swd3_tests "golden: legacy database"`。对象编号 1–2000 全部比较；技能编号 0–2000 中只比较脚本在 0x400 字节内以 5 结束的编号（其余不是技能，两侧都会读出缓冲区）；等级表比较第 0–4 组（游戏使用 1–4 组，第 5 组在表外）。

## 填色框与窗口边框（tests/golden/ui_golden.txt）

场景定义：`tests/golden/ui_scenarios.h`。生成器：`ui_golden_legacy.cpp`，运行 `./swd3_tests "golden: legacy ui drawing"`。边框图片取 all_sys.tsw 中帧数不少于 9 的条目（0x234A 需 10 帧）；混色模式不比较 1 像素宽的框（原程序循环失控），由 `tests/ui_draw_test.cpp` 单独覆盖。
