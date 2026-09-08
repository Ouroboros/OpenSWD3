# 战斗角色坐标发布与记录复制 `0x004785C0`

状态：`typed_implemented`、`unit_tested`、`caller_reclaimed:4/19`。Workpack 287完整关闭仍需后续REVIEW回收其余15个caller，inventory继续保持`pending_audit`。

## 1. 完整LST边界

权威函数为`0x004785C0..0x004785F1`，共50字节、13条实际指令、无callee、无条件分支或无条件跳转、无外部FUNCTION CHUNK，末尾为`retn 8`。

指令顺序固定为：

1. 从`[esp+4]`读取X参数低word到AX；
2. 从`[esp+8]`读取Y参数低word到DX；
3. 依次`push esi`、`push edi`；
4. 依次写actor `+0x0D66`与`+0x0D68`；
5. 令`ESI=actor+0x0D50`、`EDI=actor+0x0D70`、`ECX=8`；
6. 在DF=0生产合同下执行八次正向`movsd`；
7. 依次恢复EDI、ESI并弹出两个参数。

复制源`actor+0x0D50..+0x0D6F`包含刚写入的X/Y，因此不能把坐标写和记录复制折叠为无序结构赋值。目标范围固定为`actor+0x0D70..+0x0D8F`。

## 2. typed leaf合同

`publish_legacy_battle_actor_coordinates(...)`直接接收canonical actor坐标view。参数读取只替换EAX与EDX的低word；坐标写入同样只提交两个u16。leaf不修改算术flags，正常返回时ECX为0，ESI与EDI恢复入口完整值。

八次复制按每个dword的source read、destination write、ESI/EDI加4、ECX减1顺序执行。destination write fault不提交当前dword，也不推进寄存器，但保留当前source read和此前全部复制前缀。

显式typed-stop覆盖：两个参数读取、两次栈保存、X/Y写入、每个source dword读取、每个destination dword写入及两次栈恢复。后续fault保留此前坐标、复制前缀、栈动作和当时EAX/ECX/EDX/ESI/EDI；flags始终保持入口值。

## 3. 唯一记录owner

`LegacyBattleActorCoordinatesState`现唯一保存连续的两个0x20-byte逻辑记录：

- source基类记录从`LegacyBattleActorCoordinateSourceRecord::prefix`开始，`identity_word`、`position_x`、`position_y`分别位于记录内`+0x14/+0x16/+0x18`；
- destination基类记录从`LegacyBattleActorCoordinateDestinationRecord::prefix`开始，`alternate_position_x`与`alternate_position_y`分别位于记录内`+0x16/+0x18`；
- 编译期断言锁定两个独立记录类型的0x20尺寸及上述字段偏移；canonical state按source后destination的声明顺序直接继承这两个记录。

`LegacyBattleGroupBActionConfigurationState`已删除重复的`source_record/copied_record`存储。Group-B action configuration把相同的0x20-byte输入按原16个dword复制到`action_execution`中的canonical source与destination；Group-B action execution也从同一`action_execution.position_x/position_y`读取，不再观察平行数组。

## 4. REVIEW 1 caller回收

效果步进`0x0045BD90`的四个物理setter callsite已回收：`0x0045BE56`、`0x0045BEB6`、`0x0045BF77`、`0x0045BFD7`。生产源码继续通过窄port调用`0x00478600` getter，但不再向port发送`0x004785C0`。

每个actor仍先执行getter，再把共享actor delta按完整u32加到arg0，然后直接组合typed publication：

- Group-A使用完整arg0作为EAX、actor token作为ECX、scratch作为EDX、actor token作为ESI、signed index bits作为EDI；
- Group-B同样构造EAX/ECX/ESI/EDI，但EDX保留getter返回值；
- 两组均把完整ADD的`CF/PF/AF/ZF/SF/OF`作为leaf入口flags；
- publication成功后才提交成功迭代并重载动态group count；
- publication typed-stop公开精确leaf结果，并抑制当前迭代提交、剩余actor、后续group、completion latch和父caller清理后缀。

单体效果帧与群体效果帧保存最后一次完整`LegacyBattleEffectShiftResult`，分别区分Group-A/Group-B容量typed-stop与坐标publication typed-stop。父caller由startup传入的canonical owner解析真实Group-A/Group-B actor，不创建平行数组。

当前caller回收计数为`caller_reclaimed:4/19`。Workpack 287仍保持`pending_audit`，inventory不在REVIEW 1提前关闭。

## 5. 测试与动态差分

新增leaf测试覆盖正常X/Y写入与八dword复制、刚写坐标的源可见性、AX/DX低word替换、正常ECX/ESI/EDI、flags保持、参数与push停止、X/Y写部分提交、source/destination中段fault、当前source read保留、复制前缀及两次pop停止。

效果步进测试覆盖Group-A后Group-B、getter唯一opaque调用、固定token与步长、typed publication真实owner写入、完整ADD flags、Group-B EDX高word、动态count重载、容量停止、低32位环绕、publication fault后缀抑制及生产零`0x004785C0` port调用。单体和群体父caller测试覆盖精确publication结果与清理抑制；Group-B配置测试覆盖source/destination唯一owner初始化。

REVIEW 1最终门禁为战斗定向`1/1`、Linux core `199/199`、ASan/UBSan `199/199`和Linux app `205/205`；四份日志均无源码warning、失败或runtime sanitizer诊断，stderr为空。

原版动态差分仍为`blocked_runtime_oracle`：缺少完整Group-A/Group-B actor、异常栈与内存页、DF/寄存器/SEH及十九处caller的联合捕获后端。该阻塞不削弱当前静态LST边界、typed实现或单元测试结论。
