# 战斗线段光栅坐标单步推进 `0x00434350`

状态：`assembly_exact`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、callee与caller

权威LST函数范围为`0x00434350..0x00434410`，入口`proc`至`endp`连续，没有外部`FUNCTION CHUNK`，也没有callee。

工作包记录九个直接caller，分布在战斗动作、效果与对象服务循环中。九处均把同一类32字节线段记录指针压栈；调用后立即读取其他对象或记录字段，没有一处消费EAX。typed入口仍返回原0/1终点结果。动画沿线横向命中`0x0045D810`关闭后，其中一处caller已删除旧token并在无现代上限的循环中直接组合本函数；其余八处等待所属函数关闭。

IDA把函数标为`stdcall`。虽然若干caller在ECX装入战斗全局对象，完整函数体从未读取ECX，唯一输入是栈上的记录指针。现代实现因此是只接收记录引用的自由函数，不伪造对象所有权。

## 2. 记录布局

按LST读取与写入确定32字节记录：

- `+0x00`：起点X；
- `+0x04`：起点Y；
- `+0x08`：终点X；
- `+0x0C`：终点Y；
- `+0x10`：当前X；
- `+0x14`：当前Y；
- `+0x18`：Y主轴时使用的X误差；
- `+0x1C`：X主轴时使用的Y误差。

起终点只用于每次重新计算全程差值；当前坐标与两个误差会原地变化。现代`LegacyBattleLineRaster`按同序字段表达该布局。

## 3. 差值、符号与取绝对值 `0x00434357..0x00434382`

函数按32位回绕计算：

- `horizontal_distance = end_x - start_x`；
- `vertical_distance = end_y - start_y`。

X/Y步进符号初始均为`+1`。对应差值为负时：

1. 步进符号改为`-1`；
2. 以x86 `neg`取反差值。

所有减法和取反只保留32位。`INT_MIN`取反后仍是`INT_MIN`，继续作为负距离进入后续signed比较；现代实现不把它修正为更宽绝对值。

## 4. 退化轴分支 `0x00434382..0x00434391`

优先测试水平距离：

- 若水平距离为零，直接`current_y += vertical_step`；
- 否则才测试垂直距离；
- 若垂直距离为零，直接`current_x += horizontal_step`。

这保留零长度线段的原BUG：起点等于终点时两距离都为零，但函数仍先走水平距离为零分支，把当前Y加一，随后终点检查返回零。现代实现不得提前报告已经到达。

## 5. Y主轴分支 `0x0043438F..0x004343B7`

当两个距离都非零且`horizontal_distance < vertical_distance`时：

1. `x_error += horizontal_distance`，32位回绕；
2. 以signed算术右移计算`vertical_distance >> 1`；
3. 只有`x_error`严格大于该半值时：
   - `x_error -= vertical_distance`，32位回绕；
   - `current_x += horizontal_step`，32位回绕；
4. 每次都`current_y += vertical_step`。

阈值是严格`>`，等于半值时不推进次轴。现代实现显式模拟算术右移，包含`INT_MIN`残留导致负距离时的符号扩展。

固定`(0,0) -> (2,5)`、误差零的五步状态为：

1. `(0,1), x_error=2`；
2. `(1,2), x_error=-1`；
3. `(1,3), x_error=1`；
4. `(2,4), x_error=-2`；
5. `(2,5), x_error=0`并返回1。

## 6. 等距对角分支 `0x004343B7..0x004343CB`

当水平与垂直距离相等时，函数按顺序：

1. `current_x += horizontal_step`；
2. `current_y += vertical_step`。

两个误差字段均不读取、不修改。正负方向组合只由前面的两个step snapshot决定。

## 7. X主轴分支 `0x004343CB..0x004343F1`

当`horizontal_distance > vertical_distance`时：

1. `y_error += vertical_distance`，32位回绕；
2. 以signed算术右移计算`horizontal_distance >> 1`；
3. 只有`y_error`严格大于该半值时：
   - `y_error -= horizontal_distance`，32位回绕；
   - `current_y += vertical_step`，32位回绕；
4. 每次都`current_x += horizontal_step`。

固定`(0,0) -> (5,2)`、误差零的五步状态为：

1. `(1,0), y_error=2`；
2. `(2,1), y_error=-1`；
3. `(3,1), y_error=1`；
4. `(4,2), y_error=-2`；
5. `(5,2), y_error=0`并返回1。

## 8. 终点返回 `0x004343F1..0x00434410`

所有更新分支汇合后，函数：

1. 比较当前X与终点X；
2. 不等立即返回EAX零；
3. 相等再比较当前Y与终点Y；
4. 两者都相等才返回EAX一。

比较发生在本次坐标更新之后。现代bool返回在ABI上对应0/1，不改变九个caller当前忽略EAX的事实。

## 9. 双向追溯

LST到C++：

- `0x00434357..0x00434382`映射为回绕差值、符号snapshot及回绕取反；
- `0x00434382..0x00434391`映射为水平零优先的退化轴分支；
- `0x0043438F..0x004343B7`映射为Y主轴X误差与坐标推进；
- `0x004343B7..0x004343CB`映射为等距双轴推进；
- `0x004343CB..0x004343F1`映射为X主轴Y误差与坐标推进；
- `0x004343F1..0x00434410`映射为更新后双坐标终点判定。

C++到LST：

- 每个记录字段均对应明确位移；
- `wrapping_add/subtract/negate`只表达x86 32位算术；
- 显式算术右移只表达两条`sar`；
- 所有signed分支对应`jge/jnz/jle`；
- 零长度异常推进对应原水平零分支；
- 没有新增循环、夹值、宽整数、资源访问或callee。

完整正向与反向追溯未发现未解释基本块、遗漏写入或额外状态。

## 10. 测试

定向测试覆盖：

- 正负垂直轴和水平轴；
- 正负混合的等距对角线，且误差保持不变；
- 完整Y主轴与X主轴五步序列；
- 严格半误差阈值的相等不推进；
- 更新后才返回终点；
- 零长度线段Y加一原BUG；
- 当前坐标从`INT_MAX`到`INT_MIN`的回绕并命中终点；
- `INT_MIN`差值取反后仍为负的异常分支；
- 误差从`INT_MAX`加法回绕后参与signed阈值比较。

本函数不读取物理游戏资产；九个caller传入运行时线段记录。固定状态覆盖全部LST分支和异常32位域。

定向battle聚合测试通过。动画沿线横向命中caller进一步覆盖固定次数循环、水平命中和垂直线只比较X的调用方行为。

## 11. 动态差分

当前没有可用原版战斗线段记录逐步捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。该阻塞不改变无callee完整LST、全部分支、回绕域、typed实现和固定状态已经闭环的结论。
