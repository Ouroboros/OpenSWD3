# 战斗角色绘制偏移查询 `0x00478400`

状态：`platform_adapted`、`unit_tested`。REVIEW 1已完成typed本体和三个效果callsite，REVIEW 2已回收选择框的三个callsite；两个动作callsite继续由REVIEW 3隔离，因此工作包inventory仍为`pending_audit`。

## 1. 完整范围与调用关系

权威LST函数为`0x00478400..0x00478464`，有效指令从`0x00478400`到`0x00478462`，共25条指令、101字节。函数有三个条件分支、一个`retn 8`，没有callee、外部`FUNCTION CHUNK`、间接入口或跳入中段。

LST静态交叉引用记录6个caller、8个物理callsite：

- `0x004582B0`、`0x00458DE0`、`0x004599B0`各一处；
- `0x00464270`三处；
- `0x004717F0`、`0x00471AD0`各一处。

REVIEW 1关闭前三个效果callsite，REVIEW 2关闭`0x00464270`内三处选择框callsite。`0x004717F0`与`0x00471AD0`两个动作callsite仍由REVIEW 3处理；在八处全部回收前不更新`battle-function-workpack.tsv`关闭状态。

## 2. 精确ABI与基础写入

入口`ECX`是角色对象token，两个栈参数依次是X、Y的16位输出位置；`retn 8`由callee清理两个参数。

基础路径严格按以下顺序访问：

1. `EAX=[esp+4]`读取X输出指针；
2. `DX=[actor+0x0316]`读取基础X；
3. 保存ESI；
4. `[EAX]=DX`写X低16位；
5. `EDX=[esp+0x0C]`读取Y输出指针；
6. `SI=[actor+0x0318]`读取基础Y；
7. `[EDX]=SI`写Y低16位。

两项结果只做word写入，不清输出所在dword的高16位。第一次输出写入发生在第二个参数读取和Y源读取之前；输出位置与后续actor源字段别名时，后续读取必须观察已提交的新word。

## 3. 特殊坐标重复写入

基础X/Y写入后，函数对`byte ptr [actor+0x2A87]`执行`TEST 2`。bit1置位时：

1. 读取`[actor+0x0B66]`到SI并再次写X；
2. 读取`[actor+0x0B68]`到SI并再次写Y。

因此启用覆盖时共有四次有序word store。任一覆盖读取或写入失败只保留此前store，不回滚基础值或已写覆盖X。Group-A动作owner把`+0x2A87`映射为现有`action_override_flags`的高byte；Group-B把它映射为既有action-composition mode flags，不复制状态。

覆盖分支结束后读取`dword ptr [actor+0x2B08]`到EDX，恢复入口ESI，再执行`CMP EDX,1`。非1路径在此返回；最终flags来自该完整dword CMP，ESI已经恢复。

## 4. 镜像X

镜像mode完整值等于1时，函数再次读取基础X到DX并执行`TEST DX,DX`。基础X为0时直接返回，保留覆盖后的输出值，最终flags来自16位TEST且AF标记为未定义。

基础X非0时：

1. `ECX=[actor+0x2548]`读取render source token；
2. `CX=[ECX+0x0C]`读取source宽度，只替换token的低16位；
3. `SUB CX,DX`执行16位减法；
4. 最后一次写X低16位。

镜像计算使用再次读取的基础X，不使用`+0x0B66`覆盖X。返回ECX保留source token高16位并以镜像结果替换低16位；EDX高16位来自mirror-mode完整dword，低16位被基础X替换。最终flags完整记录16位SUB的CF/PF/AF/ZF/SF/OF。

source token为0时不伪造宽度：在真实`[ECX+0x0C]`访问点typed-stop。最后X写失败时，结果对象可观察已计算值和SUB flags，但caller输出仍保持前一笔基础或覆盖X。

## 5. 寄存器、flags与typed-stop

modern入口为`query_legacy_battle_actor_render_offsets`。每个真实参数读取、actor读取和输出word写入都有独立typed-stop。结果对象记录：

- 返回EAX/ECX/EDX/ESI；
- X/Y最终计算值；
- 参数读取、actor读取和物理输出store计数；
- 是否执行覆盖与镜像；
- caller入口flags或最后实际TEST/CMP/SUB flags。

ESI在基础Y和覆盖X/Y读取期间只替换低16位；只有执行到原`POP ESI`后才恢复入口完整值。发生在恢复之前的typed-stop保留局部SI残值，发生在恢复之后的typed-stop保留入口ESI。函数不预读source、不预取两个输出指针，不做默认值、夹值、回滚或失败后继续。

## 6. owner复用

固定token解析复用角色坐标查询的既有owner边界：

- Group-A：`0x005029D0 + index * 0x2F34`，startup record优先；缺少startup时回退既有Group-A action-execution state；
- Group-B：`0x00525508 + index * 0x2B28`，使用startup持有的Group-B lifecycle action-execution state，并从同一element的action-composition读取mode flags。

Group-A startup record新增的是同一角色对象的绘制偏移状态，不建立第二套角色数组。Group-A action与Group-B lifecycle复用已有基础坐标、special action record、mirror mode和render source字段。非精确对齐token、越界索引或缺失owner在首次实际actor字段读取处停止。

## 7. REVIEW 1 caller回收

三个效果caller已删除`0x00478400` generic port调用并直接组合typed结果：

- `0x004582B0`：位于resource与argument-mode前缀之后、sample之前；X/Y任一低word非0时继续原base-coordinate组合；typed-stop保留resource发布和此前模式计算，抑制sample、finalize、render、release及公共尾；
- `0x00458DE0`：位于resource sample、pan清零和owner发布之后；typed-stop保留这些既有前缀，抑制argument-mode读取、animation、render、release及公共尾；caller把原`ADD ESP,0x10`的入口flags作为可注入状态传给leaf；
- `0x004599B0`：位于resource发布和flip计算之后、sample之前；typed-stop抑制base-coordinate fallback、sample、render、release和完成尾；入口flags按非flip的`CMP global_flip,1`或flip路径的32位SUB精确构造。

三处caller继续把局部坐标建模为原有dword槽，typed leaf只覆盖低16位；已有高word不被主动清除。三个caller均在typed-stop后立即返回，不通过opaque port伪造尚未到达的副作用。

## 8. REVIEW 2 caller回收

选择框`0x00464270`已删除三处generic角色原点查询并直接组合typed结果，两个输出固定复用输入分派owner中的`0x0053BF4A/0x0053BF4E`共享word：

- 遍历Group-B标记：完成查询、快照和模式1重置后查询绘制偏移；入口EAX/EDX与flags来自重置callee真实返回。双偏移为0时继续使用快照中心，任一非0时分别按i16偏移加快照原点；typed-stop保留此前前缀，阻断输出读取、动作绘制、循环递增及余下suffix；
- 当前Group-B目标：重置和快照后，以`EAX=0x565*index`、`EDX=0x159*index`及`24*index-index`最终32位SUB flags进入typed leaf；typed-stop阻断one-based actor code发布、动作6可用性查询、prepared动作帧及余下message 3 suffix；
- 当前Group-A目标：快照后，以`EAX=0x3EF*code`、快照callee残留EDX及`0x3F0*code-code`最终32位SUB flags进入typed leaf；成功后才执行模式1重置并把共享Y word加10，typed-stop阻断这些后缀和prepared动作帧。

当前目标路径在Group-A/B分流前严格按Y后X顺序清零两个共享word。三处查询都复用Group-A startup/action fallback与Group-B lifecycle/action-composition canonical owner；旧call枚举及frame-coordinator转发枚举保持原数值但改名为reserved，生产路径保持零调用。

## 9. 测试与动态差分

定向聚合测试覆盖：

- 基础、覆盖、非镜像、镜像与基础X为0路径；
- source token高word、16位SUB结果及全部flags；
- Group-A startup/action fallback与Group-B lifecycle owner；
- `action_override_flags`高byte别名；
- 两个输出相同、输出与后续基础/覆盖源字段别名；
- 16个真实可失败访问点的精确读取/store计数与部分提交；
- source token为0的真实宽度解引用停止；
- 三个效果caller的非零偏移、零偏移既有坐标回退、caller入口flags、X后Y部分写入及后缀抑制；
- 选择框Group-B遍历标记的signed偏移与重置callee flags，当前Group-B的canonical覆盖/镜像、`0x159` EDX系数、X后Y部分提交和动作6后缀抑制，当前Group-A的one-based token、快照EDX、SUB flags、Y加10及reset后缀抑制；
- 生产三个效果实现和选择框实现不再调用对应opaque `0x00478400`边界。

当前缺少原版完整Group-A/Group-B对象、render source异常内存页、八处caller联合寄存器与SEH捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。该限制不以静态实现或modern单元测试冒充动态差分。
