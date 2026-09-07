# 战斗角色基准坐标查询 `0x00478470`

状态：`platform_adapted`。typed leaf、8个现代物理callsite、caller证据、发布矩阵与生成器关闭映射均已完成。

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

## 5. REVIEW 1–3 caller回收

三个已关闭效果caller各删除1处opaque `0x00478470`调用，并在原callsite直接组合typed leaf：

- `0x004582B0 / 0x004586E5`：绘制偏移X/Y任一低word非零时查询argument actor基准坐标，再分别按完整dword加偏移。入口EAX/EDX为X/Y输出token，flags来自实际到达路径最后一次`CMP offset,0`。typed-stop保留resource、argument mode、render flags、width与绘制偏移前缀，抑制sample、finalize、render、release和公共尾。
- `0x00458DE0 / 0x00458EDB`：绘制偏移X/Y两个低word都非零时查询argument actor。入口EAX为Y输出token，EDX继承绘制偏移leaf残值，flags来自`CMP offset_y,0`；成功后按完整u32分别加X/Y偏移。typed-stop保留此前sample、pan清零、owner发布与绘制偏移前缀。
- `0x004599B0 / 0x00459AEF`：绘制偏移X/Y两个低word都非零时查询入口Group-B actor，两个基准输出使用独立零初始化dword local，再与偏移做完整u32相加。任一offset为0的`0x004783B0` fallback保持不变。typed-stop保留初始化、owner发布与flip前缀，抑制sample、render、release与完成尾。

REVIEW 2又回收脚本case 59的两个物理站点：

- `0x00469D20 / 0x0046A3E0`：Group-A先在`0x0046A3AF`查询当前坐标，再以`actor-8`形成`EAX=1007*index`和actor ECX；EDX保持前一坐标查询的真实残值，flags来自最后一次`1008*index-index`。基准X写共享`position_x`，基准Y写共享`pair_y`。
- `0x00469D20 / 0x0046A477`：Group-B先在`0x0046A448`查询当前坐标，再重新形成`EAX=actor`、actor ECX和`EDX=1381*actor`，flags来自`24*actor-actor`；输出使用与Group-A相同的非对称共享槽。

脚本格式化继续读取前一查询写入的`pair_x`和基准查询覆盖的`pair_y`，动态命令仍按X后Y写`+0x1E/+0x20`。任一base-coordinate typed-stop保留分配及当前坐标前缀，并抑制角色清理、文字格式化、finalize、坐标发布和message写入。脚本生产路径已删除`pending_478470`调用，枚举数值仅以reserved名称留给适配层。

REVIEW 3最后回收目标阶段与两个动作caller：

- `0x004710D0 / 0x004710F6`：目标资源查询后无条件查询显式Group-B目标的基准坐标。X输出复用原目标参数dword槽，只覆盖低16位并保留目标token高word；Y写独立零初始化local。入口EAX为Y输出指针token，EDX与flags来自`0x00478620`资源回复。任何base-coordinate typed-stop都保留资源token与X部分提交，并在`0x58`字节演出记录清零前停止。
- `0x004717F0 / 0x0047191D`：动作十三仅在绘制偏移X/Y低word均非零时查询基准坐标，X/Y依次写原`var_8/var_C` dword local；mixed-zero继续走已关闭`0x004783B0`查询。基准查询保留X/Y输出指针token高word、目标ECX、入口CMP flags和X后Y部分提交，成功后再按原顺序叠加偏移与动作记录扣减。
- `0x00471AD0 / 0x00471BB1`：动作十四同样只在双非零路径查询，但X/Y原槽顺序为`var_C/var_8`；EAX高word保留前一绘制偏移查询装载的X输出指针token，ECX/EDX分别保留基准Y/X输出指针token。成功后继续反向raster，typed-stop抑制raster、sample、render、runtime gate推进与完成清理。

7个已关闭caller中的8个现代物理callsite均已删除generic `0x00478470`调用；生产源码唯一保留的地址值是`reserved_actor_base_coordinates = 0x00478470U`，调用数为零。`0x00484020 / 0x00484046`属于`audit_order=419`，当前没有现代生产实现，待其自身工作包关闭时必须直接复用本typed接口。

## 6. 测试与验证

定向测试覆盖：

- 成功路径的X/Y回绕、EAX高word、ECX/EDX残值和最终Y SUB的CF/PF/AF/ZF/SF/OF；
- 相同输出指针与输出X/actor Y源别名；
- X源、首输出指针、X减数、X写入、Y源、Y减数、次输出指针和Y写入八个逐访问typed-stop；
- X写入后的四类Y侧故障保留已提交前缀；
- startup Group-A与Group-B lifecycle canonical owner解析；
- 三个效果caller的成功组合、无opaque port、caller入口寄存器/flags、部分提交及sample/render/release等后缀抑制；
- 脚本Group-A/Group-B两条动态文字路径的非对称共享槽、各自入口寄存器/flags、零reserved调用、X后Y部分提交与清理/格式化/finalize后缀抑制；
- 目标阶段的资源回复寄存器/flags、目标参数槽低word别名、Y local、记录清零前typed-stop及X部分提交；
- 动作十三/十四的双非零基准查询门、mixed-zero canonical fallback、两套局部槽顺序、不同EAX高word来源、CMP flags、Y故障部分提交及完整后缀抑制。

REVIEW 1在格式化后的最新代码上通过：

- 定向`battle.legacy_battle_setup`：`1/1`；
- Linux core：`199/199`；
- AddressSanitizer：`199/199`；
- Linux app：`205/205`；
- 最终四份验证stderr均为空；
- changed-range格式化、新文件全量clang-format和`git diff --check`通过。

REVIEW 2在最新代码上再次通过定向`1/1`、Linux core `199/199`、AddressSanitizer `199/199`和Linux app `205/205`，最终四份stderr为空；changed-range格式与`git diff --check`通过。

REVIEW 3在changed-range格式后的最终代码上通过定向`1/1`、Linux core `199/199`、AddressSanitizer `199/199`和Linux app `205/205`，四份正式stderr为空；随后连续10轮完整core均为`199/199`且stderr为空。inventory由生成器连续双跑逐字节一致，关闭为`284/422 = 274 platform_adapted + 10 assembly_exact + 138 pending_audit`，SHA-256为`e945ecc6cf0c24204114f7eff9006d5d8ddbf4d0822403e40a426e459af38601`。系统TMP迁移dry-run为selected `0`、unselected suspicious `0`，14项全部识别为受管runtime。

未启动原版游戏或OpenSWD3应用程序。当前缺少原版完整Group-A/Group-B actor对象、异常内存页以及9处物理callsite联合寄存器/SEH捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。
