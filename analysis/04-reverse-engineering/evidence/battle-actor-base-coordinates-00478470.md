# 战斗角色基准坐标查询 `0x00478470`

状态：typed leaf `assembly_exact`、REVIEW 1 caller `caller_reclaimed`、工作包暂为`pending_audit`。

## 1. 完整LST范围

权威函数为`0x00478470..0x0047849C`，共45字节、9条实际指令、0个call、0个条件分支和1个`retn 8`，没有外部FUNCTION CHUNK。

ABI为ECX传actor token，栈参数依次为X与Y输出指针，callee清理8字节参数。返回值不是独立坐标dword；EAX只由两次`mov ax`与两次`sub ax`覆盖低16位，ECX/EDX保留最后加载的Y/X输出指针token。

## 2. 指令顺序与结果

函数严格执行以下访问序列：

1. 读取`word(actor+0x0D66)`到AX；
2. 从首个栈参数读取X输出指针到EDX；
3. 从AX减`word(actor+0x29B2)`；
4. 把AX写入X输出槽低16位；
5. 读取`word(actor+0x0D68)`到AX；
6. 从AX减`word(actor+0x02B4)`；
7. 从第二个栈参数读取Y输出指针到ECX；
8. 把AX写入Y输出槽低16位。

因此结果为：

```text
X = word(position_x) - word(source_y_offset)
Y = word(position_y) - low16(target_phase_y_adjustment)
```

两项运算均按16位回绕。成功返回时EAX高word保持入口值、AX为Y结果，EDX为X输出token，ECX为Y输出token，flags来自最终Y侧16位SUB。

## 3. 故障、别名与部分提交

实现只在原八个真实访问点typed-stop：四次actor字段读取、两次栈输出指针读取和两次word写入。无效actor token通过canonical resolver产生空view，并在第一次X源读取处停止，不通过opaque port伪造越界结果。

X写入发生在全部Y侧访问之前。Y源、Y减数、Y输出指针或Y写入故障时，已提交X低word保留，caller后缀不得继续。每次停止都保留当时真实EAX/ECX/EDX和最近一次已执行SUB flags；栈指针读取故障不提前覆盖对应目的寄存器。

输出槽仍由caller建模为原有dword local，leaf只覆盖低16位。相同X/Y输出指针保持X后Y覆盖顺序；输出X与actor后续Y源别名时，X写入会影响随后Y源读取，不缓存或合理化为独立值。

## 4. canonical owner

`source_y_offset`与`target_phase_y_adjustment`归入既有`LegacyBattleActorCoordinatesState`，与`position_x/position_y`共用同一个actor coordinate view。Group-A按现有优先级解析startup party或action execution状态；Group-B解析startup持有的lifecycle action-execution状态。不新增平行角色数组，也不建立独立基准坐标dword。

## 5. REVIEW 1 caller回收

三个已关闭效果caller各删除1处opaque `0x00478470`调用，并在原callsite直接组合typed leaf：

- `0x004582B0 / 0x004586E5`：绘制偏移X/Y任一低word非零时查询argument actor基准坐标，再分别按完整dword加偏移。入口EAX/EDX为X/Y输出token，flags来自实际到达路径最后一次`CMP offset,0`。typed-stop保留resource、argument mode、render flags、width与绘制偏移前缀，抑制sample、finalize、render、release和公共尾。
- `0x00458DE0 / 0x00458EDB`：绘制偏移X/Y两个低word都非零时查询argument actor。入口EAX为Y输出token，EDX继承绘制偏移leaf残值，flags来自`CMP offset_y,0`；成功后按完整u32分别加X/Y偏移。typed-stop保留此前sample、pan清零、owner发布与绘制偏移前缀。
- `0x004599B0 / 0x00459AEF`：绘制偏移X/Y两个低word都非零时查询入口Group-B actor，两个基准输出使用独立零初始化dword local，再与偏移做完整u32相加。任一offset为0的`0x004783B0` fallback保持不变。typed-stop保留初始化、owner发布与flip前缀，抑制sample、render、release与完成尾。

三处生产文件均无`0x00478470`常量或generic base-coordinate port调用；工作包其余5个现代callsite留给REVIEW 2和REVIEW 3。`0x00484020 / 0x00484046`属于`audit_order=419`，当前没有现代生产实现，待其自身工作包关闭时必须直接复用本typed接口。

## 6. 测试与验证

定向测试覆盖：

- 成功路径的X/Y回绕、EAX高word、ECX/EDX残值和最终Y SUB的CF/PF/AF/ZF/SF/OF；
- 相同输出指针与输出X/actor Y源别名；
- X源、首输出指针、X减数、X写入、Y源、Y减数、次输出指针和Y写入八个逐访问typed-stop；
- X写入后的四类Y侧故障保留已提交前缀；
- startup Group-A与Group-B lifecycle canonical owner解析；
- 三个效果caller的成功组合、无opaque port、caller入口寄存器/flags、部分提交及sample/render/release等后缀抑制。

REVIEW 1在格式化后的最新代码上通过：

- 定向`battle.legacy_battle_setup`：`1/1`；
- Linux core：`199/199`；
- AddressSanitizer：`199/199`；
- Linux app：`205/205`；
- 最终四份验证stderr均为空；
- changed-range格式化、新文件全量clang-format和`git diff --check`通过。

未启动原版游戏或OpenSWD3应用程序。当前缺少原版完整Group-A/Group-B actor对象、异常内存页以及9处物理callsite联合寄存器/SEH捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。inventory在REVIEW 1后仍保持`283/422 = 273 platform_adapted + 10 assembly_exact + 139 pending_audit`；只有REVIEW 3回收8个现代callsite并登记延期caller后才关闭row 284。
