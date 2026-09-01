# OpenSWD3 执行规则

最后更新：2026-09-01

本文件承载稳定执行合同。当前步骤和即时门禁见[`execution-state-pi.md`](execution-state-pi.md)；阶段与模块顺序见[`execution-roadmap-pi.md`](execution-roadmap-pi.md)。

## 1. 行为真值与兼容目标

- [`../../swd3.exe_export_for_ai/swd3.exe.lst`](../../swd3.exe_export_for_ai/swd3.exe.lst)的机器码字节与指令是原程序行为唯一真值，高于IDA伪码、符号名、字符串解释、ASM和现有C++。
- 每个目标必须覆盖完整proc/endp主体、全部出口及所有外部`FUNCTION CHUNK`；caller、callee、反编译和名称只用于导航。
- 初步还原要求bug-for-bug可观察兼容。不得现代化原有BUG、整数宽度、符号/零扩展、回绕、随机调用顺序、扫描顺序、帧内时序、异常分支或陈旧寄存器/局部状态。
- 平台层只允许隔离阻断启动或新系统兼容的问题，且必须记录原行为、失败原因、最小改动和验证。
- 不添加无来源nil防护，不以零值、空对象或“成功”掩盖调用链错误。

## 2. 固定技术决定

- 语言：C++20。
- 构建系统：CMake命令行，默认Ninja Multi-Config；不依赖Visual Studio IDE，不硬编码本机编译器路径。
- 平台边界：SDL3；业务核心不直接依赖DirectDraw、DirectInput或其他旧Windows图形输入接口。
- 音视频：FFmpeg n9.0最小LGPL静态归档只链接进项目自有`openswd3_ffmpeg`平台库；主程序只依赖OpenSWD3媒体ABI。发行包必须包含精确源码、非FFmpeg目标文件和重链接材料。
- 剧情文本：脚本内核继续按原始字节和字节偏移解析；`[scripts].encoding`的`big5`对应CP950、`gbk`对应CP936，缺省`big5`，不自动猜测；公共接口使用`char16_t`/UTF-16。
- 游戏内分辨率固定`640×480`；TOML窗口宽高和最大化只描述SDL宿主窗口，内容等比缩放。
- 诊断日志只观测行为，不改变原逻辑时序和返回合同；每次游戏进程启动建立独立`openswd3-YYYY-MM-DD_HH-MM-SS-{PID}.log`，不共用或追加单一日志。
- 文档、源码、测试和生成结果都放在`OpenSWD3/`对应分类；构建材料留在`build/`且不提交。

## 3. 执行模型

- 同一时间只允许一个阶段、一个模块和一个当前工作包处于执行状态。
- 默认使用单一主Agent；除非用户明确改变，不并行引入第二写入者或竞争性结论。
- 新发现先写入负责它的模块、evidence或待确认项，不自动扩展新阶段，不打断当前已收敛工作包。
- 每轮逆向开始前完整读取`AGENTS.md`和Pi的`APPEND_SYSTEM.md`；按仓库格式、命名、提交和发布规则执行。
- 已关闭caller必须回收已关闭callee的opaque边界；`pending_audit`caller不得提前修改。
- 共享物理状态只能有一个typed owner；跨模块调用通过低层公共依赖或窄端口组织，禁止复制owner或形成循环依赖。

## 4. 单工作包闭环

每个函数、handler、格式规则或紧密耦合小组按以下顺序执行：

1. 从LST锁定完整地址范围、ABI、物理行/指令/call/jump/label/return数量及外部chunk。
2. 在不参考现有C++的情况下记录基本块、输入、条件、数据宽度、调用顺序、副作用、异常与全部出口。
3. 从汇编条件独立推导测试向量，覆盖跳转两侧、相等、零、正负、哨兵、截断、符号扩展、回绕和原始unsafe域。
4. 识别唯一状态owner、allocator、调用方和依赖边界；先决定caller是否已关闭，禁止越界回收。
5. 实现C++20 typed路径和原访问点typed-stop；stop必须保留此前副作用、当时寄存器和陈旧状态，并阻断未到达后缀。
6. 执行LST→C++和C++→LST双向逐基本块追溯；发现差异时同步修正实现、测试、规格和证据，并从入口重做完整追溯。
7. 更新单项evidence、负责模块文档、机械inventory及其生成器；inventory连续双生成并逐字节一致。
8. 执行定向测试、Linux core、AddressSanitizer、Linux app、格式和release审计；需要大阶段Windows门时按路线图执行。
9. 只在实现、证据和全部规定门禁通过后标记工作包完成、推进当前状态和下一`audit_order`。

UT通过、文档自洽或固定次数复核不能单独证明汇编收敛。

## 5. 验证等级

- `assembly_exact`：全部汇编基本块有实现映射、不可达证据或批准例外；全部C++可观察行为可反查汇编；最后一轮双向追溯零未决。
- `asset_verified`：真实资产或存档验证通过。
- `original_diff_verified`：已与原程序输出或状态差分。
- `platform_adapted`：存在已记录的平台兼容隔离。
- `unreachable_current_assets`：当前资产不可达但原分支仍保留。
- `blocked_runtime_oracle`：缺少原程序运行或捕获环境。
- `hypothesis_only`：不能作为实现依据，且不能与`assembly_exact`并存。

缺少动态oracle时可完成静态闭合并登记`blocked_runtime_oracle`，但不得宣称`original_diff_verified`。

## 6. 构建与运行安全

- 未经用户明确许可，不启动原版或OpenSWD3游戏EXE；单元测试、构建和静态审计不等于启动游戏。
- 构建只使用`./build.sh core`、`./build.sh app`、`./build-asan.sh`；不得另造正式构建路径。
- 长任务必须使用受管process并依赖通知，不使用`sleep`、忙轮询、`&`、`nohup`、`setsid`或脱管进程。
- 项目命令设置仓库内`TMPDIR/TMP/TEMP`。系统TMP审计使用既有`build/tmp/migrate_system_tmp.py`的`classify()`口径，排除`.ctx-mode-*`、`pi-*`等Pi管理运行时，不迁移或删除它们。
- `/mnt/e/Game/swd3/compile_commands.json`必须指向`OpenSWD3/build/linux-core/compile_commands.json`；仓库内不得创建重复链接。
- `goal/HANDOFF.md`必须不存在。
- 格式以仓库clang-format和`AGENTS.md`为准；代码块后保留空行，`switch` case之间保留空行；最终执行`git diff --check`等价审计。
- release审计必须确认零OpenSWD3源码warning、零测试失败、零sanitizer finding、零runtime error、零异常游戏进程、TMP和链接状态合规。

## 7. 提交、推送与阶段汇报

达到已经验证、可独立回退的工作包边界后：

1. 精确暂存本工作包文件，不包含`build/`或无关用户改动。
2. 完整审阅暂存差异和空白错误，按`$commit` Skill创建提交。
3. 提交标题按`AGENTS.md`描述真实游戏界面或数据职责、操作时机和结果，不使用内部流水黑话。
4. 验证暂存区被提交消费后推送当前分支。
5. 推送成功后发送严格五段TG，再推进下一项。

若用户明确要求在门禁完成前创建中间提交，必须标记为中间提交，记录剩余门禁；不得据此推送、发送完成TG、标记工作包完成或推进下一项。

TG命令：

```text
python3 /mnt/d/Dev/Source/Project/stockkit/scripts/tg_notify.py "CONTENT"
```

`CONTENT`正常情况恰好五段，每段一行，段间恰好一个真实空行；标签和顺序固定为`OpenSWD3`、`实现：`、`兼容：`、`验证：`、`进度：`。shell参数使用普通ASCII双引号包住真实多行字符串；禁止单引号、`$'...'`、字面量`\n`或转义拼接。内容只写高层中文摘要，不列地址、helper、字段、flag、测试边界、SHA256或commit ID。

`验证`只写本轮实际执行并从最终日志确认的门禁和实际测试计数，不复制冻结历史中的旧计数。发送前机械检查五段、空行、引号和内容；失败时报告错误，不声称送达。

## 8. 模块开始与关闭

模块开始前必须具备：范围清单、主要调用点、状态owner、职责、输入输出、依赖、生命周期、公共接口关键宽度/错误/顺序证据，以及预先列出的UT、真实样本和差分点。

模块关闭要求：范围内每项有实现映射、不可达证据或明确阻塞；UT、真实样本、双向追溯和适用差分通过；无擅自修BUG、随机或帧序变化。只缺动态oracle时状态为`module_closed_pending_oracle`；存在其他规格、实现或测试缺口时不得移交。

## 9. 文档归属

- 当前指针和即时剩余动作：`execution-state-pi.md`，覆盖更新。
- 稳定方法和门禁：本文件。
- 阶段、模块顺序、里程碑和全项目完成条件：`execution-roadmap-pi.md`。
- 单项语义：`analysis/04-reverse-engineering/evidence/`。
- 模块摘要与集成：`analysis/04-reverse-engineering/modules/`。
- 机械范围和状态：`analysis/04-reverse-engineering/inventory/`及生成器。
- 历史追溯：`goal/history/`冻结文件。

禁止把逐工作包完成流水重新写入执行入口、状态或规则文档。
