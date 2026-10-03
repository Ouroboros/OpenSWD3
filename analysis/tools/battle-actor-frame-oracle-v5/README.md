# Workpack316：原版“絕招”列表节点与角色动作来源只读采集 v5

本版继承v4的四处`sub_479850`物理调用、父入口/出口、采样父内三处子调用和真实CPU/FLAGS采集（默认不启用Stalker），额外独立观察列表来源（新增探针不修改原版业务数据）：

- 只在`sub_470180`由窄列表帧`0x466684`调用时，按原版`actor+0x2EC0`游标的起始链表头最多读取128个物理节点，每节点原样保存`0x60`字节。记录完整性、循环、越界或不可读；相同角色/头只读取一次。该窄列表的标题引用`byte_4A7648`，原字节`B5 B4 A9 DB`按Big5为“絕招”。打开列表即可观察节点，不需要猜测哪个选项会触发动作7。
- `sub_4705C0`的每次入口/返回，及**返回后游标**`actor+0x2EC0`所指的`0x60`字节与角色`+0x2F18`数值，分别记账。退出游标**可能不是被选节点**；没有挂钩内部`0x470706`，不能据此断言原版执行了该读指令或该节点导致动作7。
- `sub_478710`仅在实际参数等于7时记录调用及返回后角色`+0x2A6C`的16位值；子函数异常未返回时只留下入站事件。它不覆盖全部直接字段写入、也不证明所有动作7来源。

采集不修改业务数据、存档或CPU寄存器；Frida插桩仍可能改变时序。节点、profile和setter事件**与父函数CPU轨迹分别记账**；未形成同次序列的事件不得拼成联合复放。v1/v2/v3归档和既有v4两次原版输出不得覆盖或删除。

## 准备与执行（仅用户在需要人工操作的专门TG及聊天同步后运行）

运行`analysis/tools/battle-actor-frame-oracle-v5/build.cmd`可生成仓库内的`build/vm/battle-actor-frame-oracle-v5/`独立便携目录；构建/自检不启动原版。工具验证游戏目录父层的原版`swd32.exe` SHA-256为`4c4c226876fd2f3169bfe62c58ede86bba59e0036b7cef4cfaf7d49475c03f2a`。

把**整个**v5便携目录复制到实际原版游戏目录内的独立子目录，确认原版已退出，双击`battle-actor-frame-oracle-v5.exe`。工具验证EXE并spawn原版，无须另外启动一份。加载可进入战斗的存档后，打开战斗菜单的“絕招”列表，待列表显示后正常退出游戏；无需盲选/释放技能。保留完整的`battle-actor-frame-oracle-v5-output/run-.../`目录并原样回传，包括`run.json`、`events.jsonl`、`summary.json`和全部`.bin`。若界面没有“絕招”、工具失败或进程异常，亦应保留完整输出及错误文字，不猜测替代菜单。

`list-node-NNNNNN-NNN.bin`和`profile-exit-cursor-NNNNNN.bin`保存原始`0x60`字节，事件含SHA-256和`+0x4C`的低16位值。`list-scan`只有`complete=true`且记录数吻合才证明该次链表头已完整遍历；未匹配、未返回或节点不可读不补零。`summary.json`分别列出列表完整/不完整ID、值为7的原始节点、profile入/出及退出游标快照与action7调用/返回；退出码0只表示捕获到有效父样本或至少一条完整且非空的窄列表扫描，不等于已发现动作7、原版四caller覆盖或送达。

## 静态边界和自测

LST锁定新钩子的`0x470180`首字节`83 EC 28`、`0x4705C0`首字节`51 53 8B`和`0x478710`首字节`8B 44 24 04`，不匹配即拒绝安装新钩子。尝试在自有PE的内部读取指令挂钩会造成访问异常，故没有部署该钩子；实际执行仍须以原版只读回传为准。依旧要求Frida16.5.1、Windows ia32、基址`0x400000`及匹配目标哈希。

在仓库根目录分别运行受管`node analysis/tools/battle-actor-frame-oracle-v5/agent_test.cjs`、`python3 -B analysis/tools/battle-actor-frame-oracle-v5/capture_test.py`；Windows自有PE探针`native_probe_test.py --full-agent`验证原v4桥/父子CPU路径并禁用新增钩子；再以`native_probe_test.py --full-agent --list-probe`启用**自有**节点与profile退出路径桩，验证三个新钩子、源节点和记录器。JS合成、Windows自有PE和记录器测试均不冒充原版节点、玩家菜单或原版CPU数据。`--self-test`检查SDK/设备及包内源文件，不spawn原版。
