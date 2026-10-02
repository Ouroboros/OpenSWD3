# Workpack 316：原版战斗角色动作子调用诊断采集 v4

本工具保留v3父函数四处入口/出口和四项全局观察，额外在动作更新器
和两种ACT装载子函数的入口/出口读取处理器状态与栈。嵌套钩子只记录
当前已完整采样的父调用，不把未采样父调用当作完整样本。
它不是完整原版差分，也不观察子函数内部每条读取指令的瞬时值。
v1/v2/v3工具源码、便携目录和既有采集保持原样。
不写游戏业务状态或寄存器；Frida仍会插桩并改变时序。
只由用户启动原版。构建、自检和独立测试程序都不启动原版。

## 一键运行

把整个 `build/vm/battle-actor-frame-oracle-v4/` 目录复制到实际原版游戏目录内，
保留独立子目录；确认原版没运行，再双击 `battle-actor-frame-oracle-v4.exe`。
它校验父目录的 `swd32.exe`，SHA-256必须为：
`4c4c226876fd2f3169bfe62c58ede86bba59e0036b7cef4cfaf7d49475c03f2a`。
安装采集后才恢复原版。不要再手动启动第二份游戏。

进入战斗，可以按A自动战斗，正常打完后退出游戏。
把新生成的 `battle-actor-frame-oracle-v4-output/run-.../` **完整目录**交回。
不要只交截图或摘录。工具报错时同样保留原始输出和错误文字。
不要覆盖或删除既有v3便携目录、原版回传和其他已有采集。

输出包括：

```text
run.json
summary.json
events.jsonl
actor-NNNNNN-enter.bin / actor-NNNNNN-leave.bin
stack-NNNNNN-enter.bin / stack-NNNNNN-leave.bin
frame-header-NNNNNN-enter.bin / frame-header-NNNNNN-leave.bin
list-head-word-NNNNNN-enter.bin / list-head-word-NNNNNN-leave.bin
nested-stack-NNNNNN-enter.bin / nested-stack-NNNNNN-leave.bin
nested-action-record-NNNNNN-enter.bin / nested-action-record-NNNNNN-leave.bin
```

## v4采集边界

- 角色快照扩为 `0x2B28` 字节，涵盖目标的 `+2B00/+2B04/+2B08/+2B20`
  访问及组B记录全长；不是组A全部记录，也不是全部嵌套内存。
- 逐段确认连续映射可读后读取，允许跨可读映射；不可读不填假数据。
- 保存ESP起128字节、帧头16字节、链头首dword及所列全局值。
- 沿用v3在`globals`增加的四个地址：`0x004A0E78`矩形宽上界、
  `0x004A0E7C`矩形高上界、`0x004AB784`首相位音频实参、
  `0x004FB308`动作流缓存设置。各保存实际32位读取值；不可读时保存
  `unreadable:`错误，不填零。入口四值也参与重复样本筛选。
- 四值在父函数及嵌套函数**入口/出口**和可用的异常快照读取；
  不证明`0x0041700A/17019`等真正读取指令时的值，也不证明原版
  异常CPU或四caller均已覆盖。
- 仅当已采样父函数同线程活跃且返回地址分别为`0x00479926`、
  `0x00432426`、`0x00432439`时，记录更新器`sub_4321E0`、直接
  装载`sub_432A50`、缓存装载`sub_432BC0`的子调用入口/出口。
  记录真实GPR、可用FLAGS、入口ESP起128字节；更新器从调用实参
  指针另存0x98字节记录，分别与父actor+0x3D0比较归属。不可信或
  不可读值明确记不完整，不能补造回复或父样本输入。
- `events.jsonl`的`nested-call`/`nested-return`按子ID与父序号关联；
  `summary.json`区分子调用是否返回、物理快照是否齐备。
  子返回现场并非原版与C++已经差分相同的证明。
- 限定Frida16.5.1、Windows ia32和原版基址。原生桥按该版源码的保存区
  读取真实PUSHFD字，不依赖缺失的JS `eflags` 属性。
- v4默认**不启动Stalker**，将内层CPU采样与父块轨迹分离。早期自有PE
  FLAGS断言未考虑夹具的`SUB`/`ADD`对标志的修改，不能据此推断Stalker
  导致失真；调用点显式FLAGS与返回路径须另经原生探针核验。
  v3保留父级块轨迹，v4的子调用FLAGS和父级块轨迹不是同一份同时取得的证据。
  不补造缺失的块轨迹或把原版尚未采集的子调用视作已验证。
- 四处调用分别有4096份完整快照预算。每次识别到的调用都记录调用/返回台账，
  不再用一个512次总预算令其他调用方失去采样机会。
- 对同一调用方/角色的**已捕获输入片段**，首次、片段变化和每64次重复保留快照。
  这不是全部外部输入相同的证明；被筛掉的调用没有完整状态，
  本版已采样调用也不采集块轨迹。
  达到预算的调用明确标为 `sample_limit`，不能据此宣称覆盖整场战斗。
- 异常FLAGS仅在Win32 ia32 CONTEXT含完整`CONTEXT_CONTROL`（`0x10001`）且
  EIP与Frida现场相等时可用；缺标记、不可读或EIP不符均不补造FLAGS。
  只复制到独立对象，不写原始上下文，异常回调返回false。
  完整SEH执行链仍不在当前采集范围。

`summary.json`区分全部观察到的调用、完整快照、缺失FLAGS、未返回调用、
调用方预算耗尽和子调用配对；v4的`trace_drain_completion_verified`为false。
`original_diff_verified`始终为false；
最终差分必须另与OpenSWD3同输入逐字段、逐块比较。

## 打包和验证

运行 `analysis/tools/battle-actor-frame-oracle-v4/build.cmd`。
输出是独立v4目录，绝不重建v1/v2/v3目录。
TEMP、Python缓存、PyInstaller缓存都在仓库 `build/tmp/runtime/`，禁用UPX。
整目录复制，包括exe和 `_internal/`。

`--self-test`只检查SDK、设备接口和打包源文件，不spawn或attach任何目标。
`agent_test.cjs`、`capture_test.py`使用模拟数据测试筛选、预算、嵌套配对、边界和记录器。
`native_probe_test.py`及其 `--full-agent` 模式只生成并启动本项目自有32位测试PE，
验证真实原生ABI、七项FLAGS、四处父调用、子钩子和同步采集，不启动或修改原版。
独立测试通过不替代原版v4运行，也不能升级先前v2/v3材料的输入等级。

源码运行可在仓库根目录执行：

```bat
py -3 -B analysis\tools\battle-actor-frame-oracle-v4\capture.py --game-dir .. --output build\vm\capture-v4
```

预算可用 `--max-samples-per-site` 调整，重复间隔用 `--repeat-interval` 调整。
采集锚点仍以LST为准：入口 `0x00479850`，四处CALL为
`0x004554F6 / 0x0045650F / 0x0045AA33 / 0x0045ACBF`。
FLAGS桥布局依据固定版本源码：
`https://github.com/frida/frida-gum/blob/16.5.1/gum/backend-x86/guminterceptor-x86.c`。
