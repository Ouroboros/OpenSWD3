# 战斗角色ready查询 `0x0045A980`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整LST范围

权威函数为`0x0045A980..0x0045A9F4`，从proc到endp完整64行、1个静态call站点、4个`loc_`标签，无外部FUNCTION CHUNK。唯一callee为`0x0047C1F0`。

两个静态callsite都位于已关闭待执行动作提交`0x0045EB40`，现已直接组合本typed实现并删除旧地址边界。caller在两次对象callee之间分别传入live顺序索引、固定组selector与前一callee EDX。

## 2. 组选择

函数接收actor index和actor group。只有group完整dword等于1时选择组A；其他所有值都选择组B。

物理token按低32位回绕：

```text
group A = 0x005029D0 + index * 0x2F34
group B = 0x00525508 + index * 0x2B28
```

函数自身只做token地址运算，不解引用角色对象，因此任意index均不在本函数typed-stop；例如全1 index继续按u32回绕后调用callee。

## 3. 全局分支与陈旧寄存器

入口先读取共享global mode。global mode为0和非0的组A分支最终ECX token完全相同，但机器码使用不同中间寄存器，导致callee入口陈旧EAX不同：

- global mode为0：`EAX = index * 0xBCD`；
- global mode非0：`EAX = index * 0x3EF`。

两条组A路径都不改EDX，因此callee入口EDX保留caller快照。

组B路径固定产生：

- `EAX = index * 0x565`；
- `EDX = index * 0x159`。

以上乘法全部保留低32位回绕。typed端口请求同时发布actor token、陈旧EAX和陈旧EDX，不能把两条组A分支合理化合并后丢弃寄存器差异。

## 4. 返回

唯一callee完整EAX严格等于1时原样返回1；任何其他值，包括全1，都先清EAX并返回0。不布尔化为非零真值。ECX与EDX不被归一化尾修改，typed结果完整保留callee返回，供已关闭caller的组B陈旧EDX路径继续使用。

## 5. 测试与动态差分

定向测试覆盖：

- 组A global-zero的`0xBCD`中间值；
- 组A global-nonzero的`0x3EF`中间值；
- 组B任意非1 selector及`0x565/0x159`中间值；
- callee返回1、2和全1的严格归一化，以及ECX/EDX完整保留；
- 全1 actor index的token和陈旧EAX低32位回绕；
- 待执行动作caller两处直接组合及ready成功/失败后的不同EDX传递。

当前缺少原版组A/B对象和ready callee共享副作用及寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
