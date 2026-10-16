# 逆向还原工作规则

## 执行边界

Skill负责准备工作区、AGENTS.md、GOAL.md及最小构建配置，并完成IDAT初步分析。交付前必须等待自动分析完成，将工作数据库（IDB／I64）保存至reverse/analysis/，正常关闭后用IDAT重开，确认输入身份及函数、汇编可读取；同时记录输入基线、结构概览和范围缺口，从数据库填写真实初始函数表。新函数为pending_implementation、验证次数为0，源码映射留空；已有工作与进度保留。准备结果及数据库入口写入GOAL后，Skill结束。

由用户自行START GOAL，接续已保存的分析成果。函数实现、逐函数一致性验证与成品构建在GOAL启动后执行；初步分析不计入函数验证次数。

禁止以任何方式访问OpenSWD3（读取、搜索、引用、复制或比对均不允许）。

临时文件、临时数据库与中间产物一律放在工作区tmp/目录，不使用/tmp或其他系统临时目录；用完即删。

## 核心流程

目标：原程序逻辑100%一致，包含原有逻辑BUG，即bug-for-bug compatibility。越界读写、破坏内存类BUG不复现，读取未初始化内存时改为确定初值；这类偏差及显示方式、平台API替换须在reverse/analysis/deviations.md逐条记录（类别见reverse/analysis/architecture.md）。实现语言由用户指定，记录在根目录GOAL.md中。

1. 读取Skill已保存的IDAT工作数据库、输入基线、结构概览和范围缺口，核对GOAL停点。
2. 按已有模块与函数表推进，补充未决分析，在reverse/functions.tsv维护module；无法确认归属的暂记unknown。
3. 按模块推进，以原函数为最小单位，从汇编还原为指定语言的完整代码。
4. 还原中根据新识别的职责和依赖，修正未知及划错模块的函数归属，同步源码位置和相关引用。函数实现后立即标记“已实现、未验证”。
5. 循环选择函数、还原逻辑和修正归属，直到所有函数实现。未知模块中的函数同样纳入处理。
6. 函数全集全部实现且范围缺口处理完毕后，从函数表第一项开始第二遍汇编验证。逐函数修正差异，通过后再继续下一函数。

在reverse/analysis/中记录扫描范围、未识别代码与间接调用缺口及其处理依据，不能仅凭表内函数全部实现或通过验证就认定范围完整。验证中发现遗漏函数时，先补表并完成还原，再继续验证。

## 输入与工具

- target（目标目录）：`/mnt/e/Game/swd3`（Windows：`E:\Game\swd3`）
- main（主程序名）：`swd32.exe`，主程序路径为`/mnt/e/Game/swd3/swd32.exe`
- idat（IDAT路径）：`D:\Dev\Crack\IDA\idat.exe`（WSL：`/mnt/d/Dev/Crack/IDA/idat.exe`，IDA Professional 9.3）
- lang（实现语言）：C++20

参数采用key=value格式，含空格的值使用引号。四项输入均为必填。已有明确输入直接使用；缺失时询问补齐，不自行猜测主程序。启动分析前核实目录、主程序和IDAT路径有效，并将输入记入reverse/analysis/的输入基线资料。

反汇编、数据库分析及IDAPython执行只使用指定的IDAT。禁止启动GUI版IDA，也不得用GUI可执行文件的批处理模式替代IDAT。IDAT不可用时报告原因并要求修正路径或环境，不得回退到GUI版本。

工作数据库只有一个：reverse/analysis/swd32.i64（输入为工作副本reverse/analysis/work/swd32.exe），已禁用FLIRT（AF_FLIRT关闭）。不得重新启用FLIRT，不得并列保留多份IDB／I64；需要试验时只在工作区tmp/建立一个临时库，结束后删除。IDAT批处理调用方式见reverse/analysis/README.md。

## 平台层与第三方依赖

使用SDL3负责跨平台窗口、画面呈现、键盘／鼠标／手柄输入和音频输出；沿用FFmpeg 9.0（n9.0）最小LGPL静态构建及其脚本，仅启用Bink视频与音频、MP3的解封装和解码、本地文件读取及必要的音频重采样和像素格式转换。原程序的游戏逻辑、资源格式解析、绘制规则、播放控制和逻辑时序仍依据原汇编还原并保留原有BUG。

SDL3与FFmpeg由构建脚本自行下载源码并构建，不查找或复用本机其他位置的现成副本。FFmpeg启用x86汇编，汇编器为nasm（WSL中`/opt/nasm`，`/usr/local/bin/nasm`链接；Windows目标在WSL中交叉编译，同样使用该nasm）。

平台定位：Windows（x86）是主平台，最终产物必须能正常游玩，画面内容、输入、音频、视频播放与原程序一致。显示改为窗口化：逻辑画面640×480与窗口大小分离，窗口默认1280×960、可用鼠标拉伸，最近邻任意比例等比缩放，不成原比例时留黑边，Alt+Enter切换无边框全屏，鼠标坐标换算到逻辑画面。平台层是C++类（Display、Surface、Input、AudioDevice、MediaDecoder、GlyphRasterizer、FileSystem），不模拟Win32、DirectDraw、DirectInput、Miles与GDI的API。Linux只要求编译通过、单元测试通过并可用于代码验证，不要求音频输出或可正常游玩。

## 动态采集

遇到必须启动原游戏、通过动态DUMP才能验证的数据，编写Frida辅助采集脚本，并提供运行方法、游戏操作步骤、采集时机和输出位置。由用户启动游戏并完成操作，Agent根据采集结果继续分析和验证；采集脚本与原始输出统一保存在reverse/下。尚未取得所需数据时，保留待验证状态，不以推测代替结果。

## CRT 与命名

已识别的静态链接 MSVC CRT 函数使用 C++ 标准库，不重写 MSVC 的堆、启动代码与库内部实现；只在游戏可观察到差异时保留原行为：`rand`／`srand`（MSVC LCG）、`_ftol` 截断与溢出值、`difftime` 的 32 位减法等，集中在src/crt/。函数表中 CRT 函数的source记录对应的平台 CRT 调用。

类名、函数名和变量名不必照搬原名，使用本项目的命名风格；每个还原函数定义前写`// swd32.exe 0x<地址>`，函数表source记录`路径:符号`。

## 汇编依据与代码结构

原程序的机器码、汇编和原数据是逻辑依据。IDAT用于分析，IDAPython通过IDAT执行必要查询；反编译辅助理解。名称、已有代码和测试结果不能单独证明原行为。

尽量还原原模块、函数归属、数据结构和接口。结合符号、调用关系、共享状态与生命周期组织src目录和文件；推断注明依据与未决项。函数是工作粒度，相关函数按原模块组织。

代码按reverse/analysis/architecture.md组织，要求如下：

- 每个原模块是正经的C++类或一组相关类型（头文件加命名空间），原全局变量是所属类的成员，由根对象Game持有。不允许可写的自由全局变量、全局状态结构、数据镜像，也不允许C风格的逐句转写。
- 记录与文件格式定义为带命名字段的struct；含义未定的字段在类型内以unkXX（XX为偏移）占位并注明读写者。不得按数字偏移访问记录。
- 只读常量表与可写全局的初值写成带类型的constexpr表或成员默认值，并以单元测试与exe字节比对。
- 常量、模式、状态、声音编号写成具名常量或enum class；函数指针表按用途改写为虚函数、成员函数指针或枚举分派。
- 命名写出依据（读写它的函数、取值范围、对应的界面行为）；推断与已确认结论分开标注。
- 提交前运行scripts/format.sh（clang-format 加空行规则，配置为 .clang-format）与scripts/check_style.sh（成员前缀、地址占位名、匈牙利前缀、C 风格内存管理），并检查改动文件没有数字偏移访问。函数登记用scripts/register.sh。

## 代码风格

命名：

- 类型（class、struct、enum class、别名）、函数与成员函数用 PascalCase，例如 `TextFont`、`DrawGlyph`。
- 成员变量、局部变量与参数用 camelCase，不加前缀或后缀：不用 `m_`、`g_`、`s_`、尾下划线，也不用匈牙利前缀（`dw`、`n`、`p`、`lp`、`sz`、`b`、`h` 等）。
- 常量（`constexpr`、`static constexpr`）用 `k` 加 PascalCase，例如 `kWidth`；`enum class` 的枚举值用 PascalCase。
- 命名空间用小写（`swd3`）；文件与目录用小写下划线（`text_font.cpp`）。
- 名字表达含义；原地址只写在注释里，不进入标识符。
- 整数类型用项目别名（src/core/types.h）：`u8`、`u16`、`u32`、`u64`、`i8`、`i16`、`i32`、`i64`，不写 `std::uint16_t` 等；显示像素用 `Pixel`。有领域含义的值优先定义具名类型（结构体、`enum class` 或别名）。这些基础别名沿用标准库式的小写，是类型名 PascalCase 规则的例外。

写法：

- 在成员函数内访问本类成员变量与成员函数一律显式写 `this->`，例如 `this->width`、`this->Present(screen)`。静态成员与类内常量用类名限定，例如 `Display::kWidth`。
- 每个还原函数的定义前写 `// swd32.exe 0x<地址>` 注释，可附简要说明（行为、保留的原有BUG、偏差）。默认实现的构造函数也写出定义以附注释。
- 用 C++ 类型转换（`static_cast` 等），不用 C 风格转换；不用裸 `new`／`delete` 与 `malloc`／`free`，用容器与 `std::unique_ptr`；不用宏（单元测试框架除外）。
- 头文件用 `#pragma once`，不在头文件中写 `using namespace`。
- 缩进 4 空格，行宽 120；`switch` 的 `case` 缩进一级。
- 大括号单独占一行（Allman），包括函数、类、命名空间、控制语句与 lambda；所有控制语句都带大括号，不写单行函数体或单行块。
- 每个大括号块前后各空一行：

  ```cpp
  statement;

  if (condition)
  {
      body;
  }

  nextStatement;
  ```

  块位于外层块开头或结尾时不加空行；`}` 与其后续部分（`else`、`catch`、do-while 的 `while`、右括号）之间不加空行；前导注释与函数头之间不加空行。
- 函数调用一行写不下（超过 120 列）时，参数每个一行、缩进 4，右括号单独一行与语句起始对齐：

  ```cpp
  result = Function(
      argument1,
      argument2,
      argument3
  );
  ```

  函数声明与定义的参数表一行写不下时用同样的格式。不使用按左括号对齐的续行。
- 代码注释用英文，说明原行为与依据；不写逐行翻译汇编的注释。

## C++ 构建要求

### 工具建议

项目选择 C++ 时，建议采用：

- CMake：项目配置与构建目标管理。
- Ninja：构建执行。
- LLVM：编译工具链。

### 工作环境与平台

- 日常工作环境为 WSL。
- Linux 目标在 WSL 中构建为 i386 静态程序（与 Windows x86 相同的 32 位整数与指针宽度）：`cmake --preset linux`、`cmake --build --preset linux`，构建目录build/linux/；单元测试`ctest --preset linux`。sysroot 为`/opt/sysroot-i386`（`scripts/deps/setup_i386_sysroot.sh`生成，Ubuntu noble 的 libc6-dev-i386、lib32stdc++-13-dev 等包解压而成，不改动系统目录），工具链文件cmake/toolchain-linux-i386-clang.cmake。
- Windows 目标使用 clang-cl／lld-link 交叉构建 x86 产物（MSVC 14.44 与 Windows SDK 10.0.26100 头文件和库）：`cmake --preset windows`、`cmake --build --preset windows`，构建目录build/windows/，工具链文件cmake/toolchain-windows-x86-clang-cl.cmake。
- 两个平台分别使用独立的构建目录。

### 构建记录

分别记录 Linux、Windows 的构建入口和实际结果。GOAL最终交付时：Linux构建与单元测试通过；Windows构建通过且可正常游玩。构建结果、测试结果与函数汇编验证结论分别记录；测试结果辅助验证，不能代替逐函数汇编对照。

单元测试源码放在tests/，由CTest运行。

## 函数状态与验证次数

reverse/functions.tsv是唯一函数状态表，以binary和address标识原函数。binary相对原始输入目录；source和evidence路径相对工作区根目录。module记录当前归属，unknown表示未知模块；归属调整保留原函数身份、实现状态和验证次数，更新源码位置与依据。

函数实现与验证状态：

- pending_implementation：待还原。
- implemented_pending_review：已实现，尚未通过验证；次数为0表示未验证。
- assembly_exact：完整函数已通过汇编验证，包括原有BUG。

verification_count初始为0。每完成一轮从入口覆盖完整函数的汇编对照加一，无论本轮通过还是发现差异；未完成轮次、编译和测试不计数。修正后继续累计，保留各轮结论及对应源码修订。

展示状态时带次数，例如“已实现、未验证（0次）”“已验证2次，仍有差异”“已验证3次，通过”。次数不能代替通过结论。

第二遍先从汇编独立推导，再对照源码，覆盖全部分支与出口、位宽、溢出、调用顺序、随机数消费、状态副作用和逻辑时序。差异修正后重新核对完整函数，直到整轮无剩余差异。源码或依赖数据、资源的修改使本函数或其他已验证函数的结论失效时，将受影响函数恢复为implemented_pending_review并重新验证，保留累计验证次数。

逐函数验证同时核对所依赖的常量表、全局数据的内容、布局与初值，以及资源的内容和使用关系。依赖项尚未核对或仍有差异时，函数不能标记为assembly_exact。共享数据与资源的原始依据、还原或复用位置及核对结果记入现有reverse/analysis/与reverse/evidence/，由相关函数证据引用；范围内未被函数证据覆盖的数据与资源也须核对并留存证据。

## GOAL编写与更新

根目录GOAL.md是整体执行计划，只回答：要干什么、不要干什么、现在干什么、下一步干什么、整体完成了多少，以及怎样算完成。

- 要写：目标与范围、明确不做的事项、整体阶段与顺序、当前工作、下一步、整体进度和完成条件。当前模块或函数只保留定位所需的简短停点；阻塞只写影响与恢复条件。
- 不写：模块或函数的详细计划、任务操作步骤、汇编推导、字段与代码细节、逐轮验证记录、构建与测试日志、执行流水账和历史进度堆积。
- 详细计划与结构分析放在reverse/analysis/中；函数归属、状态和验证次数放在reverse/functions.tsv中；逐函数验证与差异记录放在reverse/evidence/中。GOAL只保留必要摘要和入口引用。

工作推进后，根据reverse/functions.tsv原位更新GOAL的整体进度、当前工作、阻塞和下一步，替换旧状态，不追加流水账。总范围未确定时写待确定；目标、范围、整体计划或验收条件变化时更新版本。

已实现数为implemented_pending_review与assembly_exact之和，已验证数仅计assembly_exact。待实现数为总数减已实现数，待验证数为总数减已验证数。

AGENTS只保存长期规则，当前进度和断点写在GOAL及函数表中。

## 工作入口与资料

文中工作区路径均相对工作区根目录。AGENTS.md与GOAL.md保留在根目录；逆向函数表、分析资料、详细计划和验证证据统一放在reverse/。源码、头文件、构建配置及其他工程内容放在reverse/之外，按所选语言组织。

开始或恢复工作时，读取AGENTS.md、GOAL.md、reverse/functions.tsv、reverse/analysis/中的范围与缺口记录、当前函数及其依赖数据和资源的证据、拟修改文件，核对当前状态和已有差异。

原程序与数据保留原貌，在工作副本上分析，核对输入身份与加载关系。修改保持聚焦，保留用户已有改动。

src/保存源码，include/按语言需要保存头文件；reverse/analysis/保存输入基线、范围记录、详细计划、结构分析、工作库、汇编导出及必要查询脚本；reverse/evidence/保存逐函数验证、数据与资源核对记录、样本和日志；Frida采集脚本与原始输出保存在reverse/下。记录事实、依据、验证结果与未决项，区分推断和已确认结论。
