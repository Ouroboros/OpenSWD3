# 战斗角色状态选择坐标查询 `0x004783B0`

状态：`platform_adapted`、`unit_tested`。REVIEW 1至4已完成本体与全部28个已有modern caller物理callsite的typed组合；7个尚无modern caller实现的外部workpack站点继续由其各自工作包隔离，不保留本函数opaque生产边界。

## 1. 完整范围与调用关系

权威LST函数为`0x004783B0..0x004783F7`，有效指令从`0x004783B0`到`0x004783F5`，共16条指令、72字节。函数只有一个内部条件分支、两个`retn 8`，没有callee、外部`FUNCTION CHUNK`、间接入口或跳入中段。

LST静态交叉引用记录24个物理caller、35个callsite。REVIEW 1关闭`0x0045B0E0`内部两个callsite；REVIEW 2关闭`0x00469D20`脚本分派六处；REVIEW 3关闭四个效果caller八处；REVIEW 4关闭动作、特殊动作和目标就绪十一caller十二处，共28个已有modern caller物理站点。剩余7处位于尚未实现的外部workpack caller，没有modern生产opaque调用可保留，继续由对应caller workpack承担。

## 2. 精确语义与ABI

入口`ECX`是角色对象token，两个栈参数依次是两个16位输出位置；`retn 8`由callee清理两个参数。函数首先执行：

```text
cmp word ptr [ecx+0x26D8],0
jz selector_zero
```

selector非零时按以下顺序执行：读取第一个输出指针、读取`[actor+0x0D86]`、写第一个word、读取`[actor+0x0D88]`、读取第二个输出指针、写第二个word。完成时`EDX=arg_0`、`ECX=arg_4`，`AX`为第二个坐标；`EAX`高16位保留入口值。

selector为零时按以下顺序执行：读取第一个输出指针、读取`[actor+0x0D66]`、写第一个word、读取第二个输出指针、读取`[actor+0x0D68]`、写第二个word。完成时`EAX=arg_0`、`EDX=arg_4`，只有`CX`被第二个坐标替换，`ECX`高16位保留角色token高16位。

两条路径的后续`mov`均不改标志，因此返回标志保持最初word `cmp`的结果。实现记录CMP的`ZF/SF/CF/OF/PF/AF`及AF有效性，不把selector压缩成只剩布尔分支；selector读取失败时则逐位保留caller提供的入口标志与AF有效性。

## 3. 访问顺序、别名与typed-stop

modern入口为`query_legacy_battle_actor_coordinates`。每个原始内存访问都有独立typed-stop：selector读取、第一个栈参数读取、第一个源word读取、第一个输出word写入、第二个源word读取或第二个栈参数读取（顺序随分支保持LST）、第二个输出word写入。selector读取失败时`cmp`尚未执行，结果保留request入口flags；其余停止保留已完成CMP的flags、此前寄存器替换和第一个输出写入，不执行原程序尚未到达的后缀。

实现不预读两个源值，也不预取两个输出指针。测试覆盖源字段与输出位置别名，以及两个输出指针相同的情况；第二次源读取必须观察第一次输出写入形成的新值，第二次输出写入最终覆盖第一次输出。不存在快照、回滚、默认坐标、夹值或失败后继续。

## 4. owner复用

组A动作执行owner `LegacyBattleGroupAActionExecutionState`和启动owner `LegacyBattlePartyStartupRecord`复用同一轻量坐标状态基类；同时存在两种view时优先借用由平台装载和startup布局实际写入的`LegacyBattlePartyStartupRecord`，缺少startup owner时才回退到动作执行view。组B由既有`LegacyBattleActorGroupBElementState::action_execution`解析。组B的32-byte行动记录以`+0x16/+0x18`持有X/Y；配置函数按原两次`rep movsd`顺序把它复制到actor `+0x0D50/+0x0D70`时，同步同一element的primary与alternate坐标视图，因此启动与对手动作materialization不建立第二套坐标owner。

固定token解析严格使用LST基址和步长：

- 组A：`0x005029D0 + index * 0x2F34`；
- 组B：`0x00525508 + index * 0x2B28`。

非精确对齐token、越界索引或缺失owner均在selector读取位置停止，不映射到相邻角色。

## 5. 双向追溯

LST到C++：

- `[ecx+0x26D8]`对应状态selector；
- selector零路径的`+0x0D66/+0x0D68`对应primary坐标；
- selector非零路径的`+0x0D86/+0x0D88`对应alternate坐标；
- 两次word store对应按参数顺序提交的两个输出；
- 分支专有的partial-register写入和最初CMP标志均在结果对象中保留。

C++到LST：helper没有额外callee、分配、日志、验证、坐标变换、浮点运算或高word输出写入；每个状态字段、访问顺序和寄存器结果都有唯一指令依据。

## 6. 验证边界

定向聚合目标覆盖两条完成路径、CMP标志及AF有效性、selector停止的入口flags与未定义AF标记，以及每个后续可失败访问的EAX/ECX/EDX残值、第一次写入后的部分提交、源/输出别名、相同输出指针、固定token解析、Group-B记录复制别名和28个已关闭caller物理站点的真实owner集成。REVIEW 4额外覆盖caller既有word/dword/参数槽的低16位别名、高word保留、两次查询共享槽、Y读取失败时X部分写入及各caller后缀阻断。生产`src/`静态扫描无`0x004783B0` opaque调用；兼容枚举只保留reserved地址槽，测试中的地址匹配只用于断言调用次数为零。

原版异常页和全部caller寄存器/SEH联合动态捕获尚无运行时oracle；该限制按`blocked_runtime_oracle`登记，不以静态结果冒充动态差分。
