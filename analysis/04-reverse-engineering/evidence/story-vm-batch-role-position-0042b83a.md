# 剧情 VM 批量角色位置 `0x0042B83A`

状态：`platform_adapted`、有效运行域`assembly_exact`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042B83A..0x0042B8E5`

opcode：`116` / `OP_116_BATCH_SET_ROLE_POSITIONS`

## 1. 记录与循环

物理格式为：

```text
+0  u16 opcode
+2  u16 count
+4  count * { u16 selector, u16 tile_x, u16 tile_y }
```

入口把count零扩展为u32并冻结到EDI；count为0时直接进入尾部。正count循环严格使用该入口快照，不在每轮重读。

每项顺序为：

1. 读取selector；
2. selector为`0xFFF0`时替换为context source GUID，不写回脚本；
3. 调用`sub_40C0D0`，但完全忽略bool返回；
4. 先读取`tile_y`，再读取`tile_x`；
5. 两项都在16位寄存器中左移4，保留低16位；
6. 调用`sub_42DAF0(index,x,y,0,-1,-1,-1)`；
7. helper返回后才把resolved index与当前受控角色index比较；相等时对`dword_4A9920`置bit15；
8. 物理record cursor加6。

虽然记录布局是X后Y，机器访问顺序是Y后X。窗口有selector和X但缺Y时，故障发生在Y读取，helper、bit15、IP和previous都未提交。

## 2. lookup与位置owner

`sub_40C0D0`先把输出index写零。`0xFFFE`直接写受控角色index并返回一；普通GUID转入`sub_40C100`，miss时输出`0xFFFFFFFF`并返回零。

本handler不测试返回值，所以ordinary miss仍读取Y/X并把index `0xFFFFFFFF`交给`sub_42DAF0`。原版随即在helper首个角色记录访问处越界。现代实现只在该原危险点以`role_not_found`停止，不把missing角色静默消费，也不伪造位置更新。

合法index复用已闭环的`LegacyWorldStoryPathRuntime`：请求flags为0，三个action分量保持`-1`。正常“寻路无路径”由owner按机器合同直接移动并以completed返回；非completed状态对应typed runtime、索引、表面、空间链、路径容量或分配危险域，现代在对应helper失败处停止，不推进handler。

helper的历史整数返回不参与handler流控。受检span和持有型path owner替代固定角色表、72槽、裸空间链及节点池，属于平台适配；有效输入的角色副作用、调用顺序和完成流保持不变。

## 3. 位宽、长度与same-call

循环完成后，handler回到指令基址重新读取count：

```text
u16 IP += low16(4 + 6 * reread_count)
physical pointer += 4 + 6 * zero_extend(reread_count)
```

机器分别重读两次count；两次读取之间没有call或可观察写入，现代一次末尾重读等价承担两项计算。入口count仍只决定循环次数。

大count可令16位IP与完整物理指针分离；但在固定`0x8000`窗口中，此前逐record读取会先触发原裸越界，现代在同一访问阶段typed-stop。全部可完成记录的物理长度与低16位IP一致。

成功路径设置ESI=1，经common join发布normalized previous116且不service audio，同一次解释器调用继续取后继。count0与count1精确结束在`0x8000`时，先发布previous与IP，再由下一fetch返回窗口越界。

## 4. 资产锁与验证

线性TALK目录锁定30条物理记录/30 probes：

```text
TALK1  18
TALK2  12
TALK3   0
TALK4   0
```

共117个子记录；count分布为`1:2, 2:10, 3:4, 4:7, 5:4, 8:1, 9:1, 18:1`。55种selector范围`1..0x027F`，当前资产没有`0xFFF0/0xFFFE`；X范围`9..134`，Y范围`11..94`。

代表记录`TALK1.DAT@0x0001BF4F`为`116, count=1, {1,42,33}`，真实回放把目标写为`672,528`并same-call进入后继。最长记录`TALK2.DAT@0x0000F8BD`含18项、长度112；30条记录均满足`4+6*count`。

synthetic覆盖四raw alias、count0/2、`0xFFF0/0xFFFE`、受控bit15、16位坐标回绕、missing role、runtime缺失、count/selector/Y三阶段截断，以及count0/count1两种精确尾。Story VM synthetic、real及initial-session三项通过；Linux core 186/186及app 192/192完整门通过。未启动原版或OpenSWD3游戏EXE。
