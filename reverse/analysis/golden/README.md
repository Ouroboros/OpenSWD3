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
