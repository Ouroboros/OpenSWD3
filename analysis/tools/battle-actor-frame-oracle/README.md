# Workpack 316：原版战斗角色动作诊断采集 v2

本工具提供 `sub_479850` 四处父调用的诊断输入，不等于完整原版差分。
不写游戏业务状态或寄存器；Frida仍会插桩并改变时序。
只由用户启动原版。构建、自检和独立测试程序都不启动原版。

## 一键运行

把整个 `build/vm/battle-actor-frame-oracle-v2/` 目录复制到实际原版游戏目录内，
保留独立子目录；确认原版没运行，再双击 `battle-actor-frame-oracle-v2.exe`。
它校验父目录的 `swd32.exe`，SHA-256必须为：
`4c4c226876fd2f3169bfe62c58ede86bba59e0036b7cef4cfaf7d49475c03f2a`。
安装采集后才恢复原版。不要再手动启动第二份游戏。

进入战斗，可以按A自动战斗，正常打完后退出游戏。
把新生成的 `battle-actor-frame-oracle-output/run-.../` **完整目录**交回。
不要只交截图或摘录。工具报错时同样保留原始输出和错误文字。
不要覆盖或删除v1便携目录及其中已有采集。

输出包括：

```text
run.json
summary.json
events.jsonl
actor-NNNNNN-enter.bin / actor-NNNNNN-leave.bin
stack-NNNNNN-enter.bin / stack-NNNNNN-leave.bin
frame-header-NNNNNN-enter.bin / frame-header-NNNNNN-leave.bin
list-head-word-NNNNNN-enter.bin / list-head-word-NNNNNN-leave.bin
```

## v2采集边界

- 角色快照扩为 `0x2B28` 字节，涵盖目标的 `+2B00/+2B04/+2B08/+2B20`
  访问及组B记录全长；不是组A全部记录，也不是全部嵌套内存。
- 逐段确认连续映射可读后读取，允许跨可读映射；不可读不填假数据。
- 保存ESP起128字节、帧头16字节、链头首dword及所列全局值。
- 限定Frida16.5.1、Windows ia32和原版基址。原生桥按该版源码的保存区
  读取真实PUSHFD字，不依赖缺失的JS `eflags` 属性。
- Stalker设置 `trustThreshold=-1`，禁用已在独立测试中破坏OF的可信缓存路径。
  块轨迹使用同步callout，不依赖延迟事件队列；每样本最多16384块。
- 四处调用分别有4096份完整快照预算。每次识别到的调用都记录调用/返回台账，
  不再用一个512次总预算令其他调用方失去采样机会。
- 对同一调用方/角色的**已捕获输入片段**，首次、片段变化和每64次重复保留快照。
  这不是全部外部输入相同的证明；被筛掉的调用没有完整状态或块轨迹。
  达到预算的调用明确标为 `sample_limit`，不能据此宣称覆盖整场战斗。
- 异常可读的Win32 ia32 CONTEXT用于FLAGS观察，复制到独立对象，不写原始上下文；
  异常回调返回false。完整SEH执行链仍不在当前采集范围。

`summary.json`区分全部观察到的调用、完整快照、缺失FLAGS、未返回调用、
截断、调用方预算耗尽和同步轨迹完整性。`original_diff_verified`始终为false；
最终差分必须另与OpenSWD3同输入逐字段、逐块比较。

## 打包和验证

运行 `analysis/tools/battle-actor-frame-oracle/build.cmd`。
输出是独立v2目录，绝不重建v1目录。
TEMP、Python缓存、PyInstaller缓存都在仓库 `build/tmp/runtime/`，禁用UPX。
整目录复制，包括exe和 `_internal/`。

`--self-test`只检查SDK、设备接口和打包源文件，不spawn或attach任何目标。
`agent_test.cjs`、`capture_test.py`使用模拟数据测试筛选、预算、边界和记录器。
`native_probe_test.py`及其 `--full-agent` 模式只生成并启动本项目自有32位测试PE，
验证真实原生ABI、七项FLAGS、四处调用和同步采集，不启动或修改原版。
独立测试通过不替代原版v2运行。

源码运行可在仓库根目录执行：

```bat
py -3 -B analysis\tools\battle-actor-frame-oracle\capture.py --game-dir .. --output build\vm\capture-v2
```

预算可用 `--max-samples-per-site` 调整，重复间隔用 `--repeat-interval` 调整。
采集锚点仍以LST为准：入口 `0x00479850`，四处CALL为
`0x004554F6 / 0x0045650F / 0x0045AA33 / 0x0045ACBF`。
FLAGS桥布局依据固定版本源码：
`https://github.com/frida/frida-gum/blob/16.5.1/gum/backend-x86/guminterceptor-x86.c`。
