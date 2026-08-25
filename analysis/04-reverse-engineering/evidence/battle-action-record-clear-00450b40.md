# 战斗动作记录完整清零 `0x00450B40`

状态：`assembly_exact`、`unit_tested`、`fixed_state_tested`。

## 1. 范围与调用图

权威LST完整范围为`0x00450B40..0x00450B50`，从`proc`到`endp`共12行，没有外部`FUNCTION CHUNK`。

函数无参数，以plain `retn`返回，没有callee。两个callsite均位于`0x004539B0`函数内，调用地址为`0x004543BE`和`0x00454423`。

第一个caller在另一个callee返回后清零动作记录，再继续后续状态分派；第二个caller在音频命令返回后清零记录，随后立即以LEA覆盖EAX。两个caller都不消费本函数返回值。

## 2. 精确写入范围

完整指令序列为：

```text
push edi
ecx = 0x26
eax = 0
edi = 0x004FD6D0
rep stosd
pop edi
retn
```

`0x26 * 4 = 0x98`，写入范围严格为`0x004FD6D0..0x004FD767`，恰好覆盖一个完整`LegacyActionRecord`。没有前缀字段保留、选择性initializer、额外尾字节、相邻记录或共享状态写。

modern helper接收该记录的typed引用，以`std::memset`写零`kLegacyActionRecordSize`即152字节。`LegacyActionRecord`已有`sizeof == 0x98`静态断言，因此typed范围与原dword范围逐字节一致。

## 3. 寄存器与返回合同

原函数保存并恢复EDI。`rep stosd`完成后：

- EAX保持0；
- ECX递减至0；
- EDI在循环中前进152字节，但由`pop edi`恢复；
- EBX、EBP和ESI从未修改。

C++公开返回`compat::u32 0`保留EAX合同。ECX属于易失寄存器且两个caller均不观察；EDI及其他callee-saved寄存器由平台ABI和普通C++调用边界保存，不引入额外业务状态。

原代码依赖进程正常执行路径中的DF清零约定；函数自身不执行`cld`。modern连续正向清零与该有效域一致，不为异常DF伪造反向写。

## 4. 双向追溯

- `0x00450B40`：保存EDI；
- `0x00450B41..0x00450B46`：设置38个dword与零源；
- `0x00450B48..0x00450B4D`：发布固定记录地址并执行完整清零；
- `0x00450B4F..0x00450B50`：恢复EDI并plain返回。

C++到LST反向追溯只有固定152字节零写和EAX零返回两项可观察语义；没有遗漏参数、分支、callee、循环退出、错误域或共享访问。

## 5. 验证

定向测试在记录的全部152字节写入非零变化模式，调用后逐字节验证全零；记录前7字节与后9字节canary分别保持`0xA5`和`0x5A`，同时验证返回值0。

该函数是确定性的无输入叶函数，完整LST和逐字节边界测试已覆盖全部可观察状态，不依赖原版动态oracle。
