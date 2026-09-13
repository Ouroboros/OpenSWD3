# 战斗角色首条动作记录清零 `0x004786F0`

## 1. 完整LST边界与ABI

权威函数为`0x004786F0..0x00478703`，共20字节、8条实际指令、0个callee、0个分支和1个普通`retn`，没有外部`FUNCTION CHUNK`或中段入口。完整指令顺序为：

```text
mov edx,ecx
push edi
mov ecx,26h
xor eax,eax
lea edi,[edx+2A0h]
rep stosd
pop edi
retn
```

入口为thiscall：`ECX`是actor token。`MOV EDX,ECX`先保存actor token；`PUSH EDI`保存caller的EDI；随后`ECX=0x26`、`EAX=0`、`EDI=actor+0x02A0`。`REP STOSD`执行38次独立dword零写入，共`0x98`字节。完成后ECX为零，EDI已按DF推进38个dword；`POP EDI`恢复caller值，普通RET从栈读取返回地址并令ESP再增加4。

## 2. canonical owner与物理范围

actor从`+0x02A0`起存在18条连续`0x98`字节动作记录。函数只处理第0条，即`LegacyBattleActorActionRecordSlots::frame_source_action_record`；不会清零第1至17条。Group-A继续由`LegacyBattleActionDispatchState::group_a_action_execution[]`持有唯一owner，没有新增平行actor数组、第二套动作记录或token槽。

DF=0时写入地址依次为`actor+0x02A0..actor+0x0334`，完成后的EDI为`actor+0x0338`。DF=1时写入地址依次为`actor+0x02A0..actor+0x020C`，完成后的EDI为`actor+0x0208`。typed view的反向字节窗口只是显式描述actor前部物理内存，不拥有或复制业务状态；未提供该物理窗口时，DF=1在首个`STOSD`处停止。

## 3. flags、访问顺序与部分提交

`MOV`、`LEA`、`PUSH`、`POP`、`STOSD`和`RET`不修改算术flags。唯一flags写入来自`XOR EAX,EAX`：CF=0、PF=1、ZF=1、SF=0、OF=0，AF未定义。DF独立保留入口值并决定每次EDI加4或减4；函数本体没有`CLD`或`STD`。

每次`STOSD`按物理顺序独立执行：先验证当前destination写访问，成功后只提交当前dword，再推进EDI并递减ECX。任一写入fault保留此前已清零的前缀或反向后缀；当前dword不写入，EDI仍指向当前destination，ECX仍包含当前及剩余次数，EIP停在`0x00478700`。实现不使用结构赋值、数组fill或隐藏事务回滚。

栈访问也保持逐指令停止点：`PUSH EDI`写fault停在`0x004786F2`且不修改ESP；38次写入完成后`POP EDI`读fault停在`0x00478702`，保留推进后的EDI与`entry_esp-4`；POP成功后RET读fault停在`0x00478703`，EDI已恢复且ESP回到callee入口值。普通RET成功后EIP为caller返回地址，ESP为callee入口值加4。

## 4. 唯一物理caller

全程序唯一直接CALL为`sub_4539B0` case 15中的`0x0045529A`，真实返回地址为`0x0045529F`。caller从`word ptr dword_53BF24`取得Group-A索引，mask到低16位后用两次SHL、两次SUB与两次LEA形成：

- `EAX=index*0xBCD`；
- `ECX=0x005029D0+index*0x2F34`；
- EDX继承前序`0x0047DAB0`返回；
- EDI是switch已递减的动作值14；
- flags来自最后一次`SUB(index*0x3F0,index)`，后续两条LEA不修改flags。

caller现直接组合typed实现，返回地址固定为`0x0045529F`。leaf typed-stop保留case 15此前已提交的计数、索引、选中与模式调用，以及所有成功的局部清零写入；同时阻断`0x0047D870`、全局模式写入、召唤构造、动画和case 15全部后缀。生产路径对`0x004786F0`为零opaque调用。

## 5. 测试覆盖

leaf测试覆盖Group-A canonical owner解析、38次正向写入、38个正向逐写fault、DF=1完整反向写入、38个反向逐写fault、缺失物理窗口、PUSH EDI写fault、POP EDI读fault、RET地址读fault、完整栈内容、ESP/EIP、EAX/ECX/EDX/EDI、XOR flags与AF未定义表达。

action-dispatch测试覆盖真实Group-A actor算术、`EAX=index*0xBCD`、EDX继承槽、EDI=14、末次SUB flags、真实返回地址、canonical第0动作记录清零、第四次写fault的三次部分提交、PUSH fault前缀、后缀抑制以及raw地址零调用。

Workpack 297最终定向`1/1`、AddressSanitizer/UBSan `199/199`、Linux core `199/199`、Linux app `205/205`及连续十轮core均通过。正式发布日志stderr为空；changed-range格式检查、inventory双次逐字节稳定生成与TMP分类审计通过。

## 6. 动态oracle缺口

原版动态差分登记为`blocked_runtime_oracle`：当前缺少完整Group-A actor前部物理内存、异常destination/栈页、DF、前序callee残值以及唯一caller联合寄存器、flags与SEH捕获后端。该缺口不影响完整LST、静态地址序列、typed部分提交、canonical owner或固定状态测试结论。
