# 标准模式输入状态位组合helper `0x0043BA40`

状态：`platform_adapted`、`unit_tested`

## 1. LST物理范围与owner

权威范围为`swd3.exe.lst`的`0x0043BA40..0x0043BAA1`。函数无参数、无callee，读取四个dword owner并无条件重写一个共享状态word：

```text
first_gate   = dword_4B7DAC
first_state  = dword_4B7DA0
second_gate  = dword_4B7D9C
second_state = dword_4B7D90
output       = dword_4FB8AC
```

LST只有两个直接调用点：标准模式总入口`0x00439FD0`和商店侧入口`0x0044DBC0`各一次。两处都在下一次call或EAX写入前忽略本helper返回值；共享output才是后继业务使用的owner。

## 2. 第一组三态

入口先读取first gate、与1比较，并在消费比较flags前把output无条件清零；x86 `mov`不改变比较flags。

```text
output = 0
if first_gate == 1:
    if first_state == 1:
        output = 1
    else if signed(first_state) > 1:
        output = 2
```

first gate只接受精确值1。first state也按signed i32判断：1映射位值1，大于1映射位值2，0及负数保持0。

## 3. 第二组三态

第一组完成后，second gate只接受精确值1：

```text
if second_gate == 1:
    if second_state == 1:
        output.low_byte |= 4
    else if signed(second_state) > 1:
        output.low_byte |= 8
```

output在本函数入口已被清为0，且第一组只可能写0、1、2，因此原始`or al, 4/8`不会保留非零高24位。最终可观察output恰为以下9种值：

```text
0, 4, 8
1, 5, 9
2, 6, 10
```

两组互不覆盖：第二组只追加4或8，second gate失配或second state不大于0时保留第一组结果。

## 4. 路径相关EAX

虽然两个直接caller都不消费返回，modern仍保留原EAX：

- second gate不等于1：返回第一组最后留在EAX的值；first gate失配时是first gate，否则是first state。
- second gate等于1且second state小于等于0：返回second state。
- second state等于1或大于1：原函数重新装入output并执行`or al`，返回最终组合位值。

modern以`LegacyStandardModeInputStatusResult`同时承载组合flags和`legacy_return_value`，用四个typed i32参数替代裸全局，不改变比较顺序或signed规则。caller对四个输入owner的更新继续独立关闭，本单元不提前计入。

## 5. 验证

`special_modes.legacy_initial_menu`覆盖：

- 第一组0/1/大于1与第二组0/1/大于1的全部9种组合。
- first/second gate不等于1时不激活对应组。
- first/second state负值走signed非正路径。
- first state大于1时output为2但失配second gate下legacy EAX仍保留原first state。
- second state非正时保留第一组output但legacy EAX返回second state。

定向测试通过。workpack连续生成两轮均为`16/227`，SHA256均为`b2be6a6ee527034544e7fd9a7222c54db40dcb3a4117231651a4fa8c03307445`；只新增关闭`0x0043BA40`，`0x0043BB40`仍为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。用户要求本单元提交后暂停模块9，先修正世界运动插值拖拉感。
