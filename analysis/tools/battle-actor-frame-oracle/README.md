# Workpack 316：原版战斗角色逐帧联合采集

本工具仅用于 `sub_479850` 与四处物理父 CALL 的原版差分输入。
它不写入游戏业务状态；Frida 会插桩代码并改变运行时序，不是无侵入观察。
异常回调返回 false，将异常交还原版；不能据此证明完整 SEH 分支已采集。
原版执行必须由用户在自己的 Windows/VM 环境发起；构建、自检不启动原版。

## 一键运行

将整个 `build/vm/battle-actor-frame-oracle/` 便携目录复制到原版游戏目录的
子目录下；确认原版没有运行，双击 `battle-actor-frame-oracle.exe`。
工具先校验父目录的 `swd32.exe` SHA-256 为
`4c4c226876fd2f3169bfe62c58ede86bba59e0036b7cef4cfaf7d49475c03f2a`，
以 Frida spawn 在恢复执行前安装入口、块轨迹及异常采集。
**不要在本工具运行时再手动启动第二份原版。**

进入一场能触发战斗角色动作的战斗，覆盖需要比较的角色、动作和异常场景，
然后正常退出原版。日志输出到游戏目录的：

```text
battle-actor-frame-oracle-output/run-YYYYMMDD-HHMMSS-PID/
  run.json
  events.jsonl
  summary.json
  actor-NNNNNN-enter.bin
  actor-NNNNNN-leave.bin
  stack-NNNNNN-enter.bin
  stack-NNNNNN-leave.bin
  frame-header-NNNNNN-enter.bin
  list-head-word-NNNNNN-enter.bin
```

`events.jsonl` 记录四处父 CALL 的返回地址分类、入口和正常返回的
通用寄存器、所列全局值和角色原始 0x2B00 字节快照，
以及目标函数内执行的块序列。块数据逐批落盘，正常返回后的延迟批次也保留。
另外按可读范围保存 ESP 起128字节、`actor+0x2548` 指向的16字节帧头、
`actor+0x2584` 指向节点的首dword；空指针或不可读范围明确记录，
不填充虚构数据。上述片段不代表全部嵌套记录或完整异常栈。
异常事件附带故障地址、角色和栈快照；没有正常 leave 的调用不能冒充正常返回。

**FLAGS 不是保证可用的数据。** 仅当 Frida context 实际提供 `eflags` 时记录；
否则 `registers.flags_available=false`，`summary.json` 列出缺失编号。
本工具没有补造 FLAGS，也没有捕获完整 SEH 执行链，
当前诊断材料不能直接作为 316 最终联合差分验收。
最多采集 512 次入口、每次 16384 个目标块；`dropped_total>0`
或快照不完整的调用均标记 incomplete。块缓冲已完整排空尚未经原版验证，
`trace_drain_completion_verified=false`，不能宣称完整块轨迹。

请将**整个新生成的 run 目录**原样复制到仓库 `build/vm/` 内或打包交回，
不要只发截图或摘录。若提示 SHA 不匹配、就绪失败、
没有采集到 `entry`/`blocks` 或出现 `capture-error`，保留整个输出和控制台文字；
此时不能标记 `original_diff_verified`。首次运行需要用户验证便携工具
的实际 Frida 挂钩与游戏操作覆盖，静态打包自检不替代原版运行。

源码模式（需要 Windows Python 与 `frida==16.5.1`）：

```bat
py -3 -B analysis\tools\battle-actor-frame-oracle\capture.py --game-dir E:\Game\swd3 --output E:\Game\swd3\battle-actor-frame-oracle-output\manual-01
```

## 打包与离线验证

执行 `analysis/tools/battle-actor-frame-oracle/build.cmd` 打包。
脚本将 TEMP、Python字节码和 PyInstaller缓存全部放在仓库 `build/tmp/runtime/`，
禁用 UPX。输出目录内包含 exe、`_internal/` 和本说明，必须整目录复制。
打包后执行 `battle-actor-frame-oracle.exe --self-test` 只检查 Frida device
及打包 agent，不会 spawn、attach 或启动游戏。

`agent_test.cjs` 和 `capture_test.py` 只使用模拟回调和合成字节验证
事件解析、延迟批次、快照大小、重复文件保护和缺失证据标记。
它们不是 Frida 对真实原版的挂钩验证，不升级 `original_diff_verified`。

`--self-test` 只验证解释器、Frida device 与 agent 文件，不启动游戏。
源码校验字节以 `swd3.exe_export_for_ai/swd3.exe.lst` 为准：
四处 CALL 为 `0x004554F6 / 0x0045650F / 0x0045AA33 / 0x0045ACBF`，
入口为 `0x00479850`。已对本地 `swd32.exe` 与 `swd3.exe`
这五处目标及当前释放链片段逐段核对机器码一致；不据此推断
其他 `.text` 字节完全相同。当前工具只提供动态输入材料，
最终差分还必须和 OpenSWD3 同输入逐字段比较、逐块复审。
