# B3 `input_time_rng` 工作包

状态：实施中

当前单元：B3.6 鼠标采样、坐标夹取、rebase 和灵敏度合同

## 1. 范围与非范围

B3 负责设备采样边界、旧输入快照与归一化记录、文字输入编辑、帧时钟和等待规则，以及两套随机序列。世界、菜单、剧情和战斗如何解释输入或随机结果属于各自模块；B3 只保证原始字段、调用顺序和确定性流。

当前机器目录共有 26 个 `input_time_rng` 函数：

- 输入归一化：`0x004050E0`、`0x004053C0`。
- 帧间隔：`0x0040DD20`、`0x0040DD30`。
- 默认绑定：`0x00424390`。
- DirectInput/鼠标包装：`0x00436FB0`、`0x004372B0`、`0x004372D0`、`0x004372E0`、`0x00437300`、`0x00437310`、`0x00437430`、`0x004374C0`、`0x004374E0`。
- DBCS/IME 编辑驱动：`0x00438B50`、`0x00438DC0`、`0x00438DD0`、`0x00438DE0`、`0x00438E10`、`0x00438E50`、`0x00438F60`。底层 `LegacyDbcsTextBuffer` 的物理对象与字节边界由 B2 `resource_io` 交付，B3 只借用并执行输入编辑。
- 第二套 RNG：`0x00438FA0`、`0x00439020`、`0x00439070`、`0x004390F0`、`0x00439110`。

CRT-compatible `0x00489B10/0x00489B20` 属于外部 CRT 代码，但它的种子和输出流是 B3 必须提供的兼容合同。正时长 `Sleep` 的具体业务调用点随 owner 模块实现；B3 固定阻塞语义和可注入等待端口，不集中搬运所有业务函数。

## 2. 状态所有权与接口

B3 拥有：

- 256 字节 DIK 域键盘快照、鼠标绝对轴基准、灵敏度和逻辑坐标映射。
- 20 条、每条 16 字节的归一化记录，以及左键抑制、鼠标静止计数和当前/前帧输入快照。
- 0x80 字节稀疏按键绑定兼容块；B2 只按明确文件顺序序列化其 16 个低字节。
- `previous/current` 32 位毫秒样本和帧间隔门槛。
- CRT-compatible seed 及第二套 250-word RNG 状态。

SDL3 只在平台层产生键盘、鼠标、时钟和文字输入样本。兼容核心不读取 SDL repeat，不让宿主刷新率决定逻辑推进次数，也不使用 `std::uniform_int_distribution` 替代原随机流。

## 3. 自动化验证策略

本模块不依赖人工“玩游戏判断”：

- 输入：向核心注入固定的逐帧 256-byte 快照和鼠标样本，逐帧比较 20 条记录、坐标、静止计数及合成按键副作用。
- 时间：注入包含相等、阈值边界、长跳变和 `u32` 回绕的 tick 序列，比较接受/拒绝帧、共享时间快照与等待谓词。
- RNG：固定 seed，比较完整状态哈希、逐次返回值、上界参数和累计调用序号；游戏回放保存 RNG 调用轨迹，避免“最终状态碰巧相同”掩盖顺序漂移。
- 集成：以后把固定输入与 tick 回放送入无界面主循环，按帧保存关键状态哈希；可用原程序捕获后端时再做同轨迹差分。

设备失败时旧快照或未初始化鼠标局部值属于旧平台风险。为新系统可启动而做的确定性隔离必须标记 `platform_adapted`，不能改变成功路径、重复节奏或游戏逻辑。

## 4. 已有证据

- [`input-entry-abi.md`](../evidence/input-entry-abi.md)：帧归一化、DirectInput 初始化和键盘读取 ABI。
- [`input-normalization-and-repeat-semantics.md`](../evidence/input-normalization-and-repeat-semantics.md)：20 条记录、鼠标、连按、repeat、raw query 与合成写入。
- [`frame-clock-and-wait-semantics.md`](../evidence/frame-clock-and-wait-semantics.md)：帧门槛、时间快照、等待、Sleep 和双种子顺序。
- [`legacy-secondary-rng-00438fa0.md`](../evidence/legacy-secondary-rng-00438fa0.md)：第二套 RNG 的初始化、xor 流与拒绝采样。
- [`legacy-crt-rng-00489b10.md`](../evidence/legacy-crt-rng-00489b10.md)：CRT-compatible RNG、两次独立时间采样和真实 SDL3 播种接线。

## 5. 当前执行顺序

1. `[x]` 建立 26 项函数范围、核心状态所有权、平台边界和确定性测试策略。
2. `[x]` B3.1：实现并验证 `0x00438FA0..0x00439119` 第二套 RNG；初始化状态、raw 流、拒绝采样和调用次数向量通过，Windows LLVM `core`/`app` 为 35/35 CTest。
3. `[x]` B3.2：恢复 CRT-compatible seed/rand 输出合同和两次独立 `time(NULL)` 注入顺序；SDL3 空播种端口已替换为真实两套 RNG，Windows LLVM `core`/`app` 为 36/36 CTest。
4. `[x]` B3.3：复用并下沉既有帧门控，补齐 `0x0040DD20/0x0040DD30` 恒返回一合同；阈值等号、拒绝快照、`u32` 回绕、零间隔和不补帧已通过独立 UT，Windows LLVM `core`/`app` 为 37/37 CTest。
5. `[x]` B3.4：实现 0x80 字节稀疏默认绑定块和单条 16 字节输入记录状态机；物理间隙、字段偏移、八类转移、严格 `>150`、回绕和全零首按异常均有固定 UT，Windows LLVM `core`/`app` 为 38/38 CTest。
6. `[x]` B3.5：实现 256 字节 DIK 快照、raw query、synthetic write、首键扫描和 SDL scancode 显式适配；默认绑定与扩展键映射有固定 UT，Windows LLVM `core` 为 38/38、`app` 为 39/39 CTest。
7. `[>]` B3.6：实现鼠标采样、坐标夹取、rebase 和灵敏度合同。
8. `[ ]` B3.7：组合单帧输入归一化，逐记录验证完整状态快照。
9. `[ ]` B3.8：实现 DBCS/IME 编辑驱动并接入 UTF-16 边界前的旧字节缓冲。

每项达到既定测试门后立即实现，不等待 B3 全部细节逆向完成。原程序动态 oracle 缺失统一登记为 `blocked_runtime_oracle`，不阻止已静态闭环且确定性测试通过的单元继续移交。
