# 战斗三通道颜色初始化 `0x0045D3E0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整LST范围与ABI

权威函数为`0x0045D3E0..0x0045D48E`，从proc到endp完整69行、53条实际指令、3个静态call、0个分支、0个局部标签，无外部FUNCTION CHUNK。

ABI是七个dword参数的cdecl，依次为三项current signed整数、三项target signed整数和signed countdown。函数不建立普通栈帧，以plain `retn`返回，由caller清理28字节参数。

三个静态call均为已关闭`0x00489654`。该callee把ST(0)向零转换为signed qword并弹栈，EAX返回低dword、EDX返回高dword。modern直接组合该语义，不保留opaque地址边界。

## 2. 固定状态发布顺序

入口按以下顺序写唯一`LegacyFrameColorTransitionState`：

1. 先把第七参数原始dword写signed countdown；
2. 红、绿、蓝current各以`fild dword`转为x87整数，再以`fstp dword`发布float；
3. 红、绿、蓝target执行相同转换并发布float；
4. 再次`fild`原始countdown并让该扩展精度值持续留在x87栈中，供三次除法共享。

整数转float的舍入发生在后续current向零转换之前。因此`INT_MAX`先成为float `2147483648.0`，再转qword得到低dword `0x80000000`；不能直接用原始整数参数代替。

## 3. 三step计算

红、绿、蓝依次执行：

```text
rounded_current = trunc_to_qword(current_float)
delta = target_argument - low_dword(rounded_current)  // 32位回绕
step = x87_fild(delta) / retained_x87_countdown
```

每次delta临时覆盖caller栈上的第七参数槽，但全局countdown和x87 denominator已先保存，不受覆盖影响。`fdiv st,st(1)`使用signed整数的x87扩展精度除法，最终`fstp dword`只在发布step时舍入为float。

countdown为0时保留masked x87结果：正delta为正无穷，负delta为负无穷，零delta为位形`0xFFC00000`的indefinite NaN。负countdown照常参与有符号除法，不增加现代保护或夹值。

## 4. 尾寄存器与x87栈

第三次蓝通道转换之后：

- EAX保留蓝current向零qword的低dword；
- EDX保留同一qword的高dword；
- ECX保留蓝target减EAX后的32位回绕delta；
- 最后一条`fstp st`弹出持续保留的countdown，使x87栈恢复入口深度。

单体效果caller把返回EDX继续作为后续完成门的陈旧来源；群体效果caller恢复完整EAX/ECX/EDX寄存器快照。typed结果显式发布三者，不把函数误建模成纯void。

## 5. 七个caller与共享门

七个caller中三处已关闭并立即直连：

- 战斗逐帧协调器固定传入`24,24,24,0,0,0,8`，仅在共享countdown不大于0且共享初始化门不等于1时执行，返回后把门写0；同帧随后直接执行已关闭颜色累加；
- 单体效果帧和群体效果帧都把record七个u16先按i16符号扩展，再依次作为七参数；初始化后清status bit `0x0400`并把同一共享门写1；
- 单体效果保留蓝转换EDX，群体效果保留全部三项尾寄存器。

剩余四个caller`0x0046F8C0`、`0x00473010`、`0x004745B0`、`0x004758A0`尚未关闭，继续由各自后续工作包独立审计，本项不提前计数。

初始化门原先分别存于逐帧状态和效果共享状态，实际都对应同一物理dword。现与九float及countdown一起通过`LegacyBattleColorAccumulationStatePort`虚共享，删除两个可分叉副本。全局重置不写该门，因此只清十项颜色值，门保持入口值。

## 6. 验证与动态差分

定向测试覆盖固定逐帧参数、signed参数发布、float舍入后再取整、32位delta回绕、正负与零countdown、正负无穷和indefinite NaN位形、EAX/ECX/EDX尾值、三处已关闭caller直连、i16符号扩展、共享门0/1发布，以及全局重置保持门值。

当前缺少原版九float、signed countdown、七个caller记录、共享门和寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
