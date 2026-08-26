# 战斗战后目标重排 `0x0045ADF0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整LST范围

权威函数为`0x0045ADF0..0x0045AF8E`，从proc到endp完整186行、11个静态call站点、7个`loc_`标签，无外部FUNCTION CHUNK。8个唯一callee。

唯一静态caller为已关闭组A帧`0x00456680`。caller已删除本函数opaque token，并在动作完成清理后直接组合typed战后目标重排。

## 2. 入口门与目标token

第二参数完整dword先与共享selected target低word的zero-extended值比较。只有完全相等才继续；不等时返回zero-extended selected值，不调用callee。因而sign-extended全1参数不等于zero-extended全1共享word。

相等时先按低32位形成组B目标token并调用一次重置：

```text
group B = 0x00525508 + target * 0x2B28
```

随后读取group A数量、把循环index置0。数量完整dword等于0时立即返回0；循环上界使用unsigned比较，不能把负数合理化为空循环。

## 3. 组A扫描

每轮按低32位形成组A token：

```text
group A = 0x005029D0 + index * 0x2F34
```

当前index等于第一参数source index时跳过本轮callee。其他角色查询目标，callee EAX低word先按i16扩展；bit15置位时直接跳过。非负值必须与selected target的zero-extended dword完全相等才进入组B扫描。

组A循环尾始终令index加1并按unsigned与启动时读取的group A数量比较。函数正常退出时返回最终index完整值。

## 4. 组B候选扫描

匹配角色读取signed group B数量；非正数量不进入候选循环。正数量从0扫描，并跳过selected target自身。

每个其他候选调用terminal查询：

- 完整EAX非0时重新读取共享group B数量，再以新的signed上界继续；
- 完整EAX为0时立即清当前组A动作、重置原selected对象、把当前候选index发布给组A对象，并把重排pending置1；本角色不再进入全局清理条件。

因此候选循环上界不是固定快照，callee修改group B数量会影响同一调用后续迭代。

## 5. 全局清理条件

没有找到非terminal候选时，比较：

```text
low_byte(packed_actor_counter) + 1 == observed_group_b_count
```

加法和比较均在完整低32位域。相等时按顺序：

1. 清当前组A动作；
2. 重置原selected组B对象；
3. 以参数0设置组A模式；
4. 以参数0配置组A对象；
5. 再次重置组A对象；
6. 固定清零十项角色顺序表；
7. 发布零target token；
8. 清secondary和queued角色code，把active code置全1；
9. 固定清零126 dword选择工作区。

十项角色顺序表就是组A帧actor queue与前一最终角色步进使用的同一物理数组，typed实现只保留一份共享存储，禁止拆分为多个状态副本。

## 6. caller回收

组A帧在子动作分派返回1、完成目标不为全1并执行既有target reset后，直接调用typed战后目标重排。子port call与迭代计数合并到父结果；若将来出现typed-stop，父函数在原调用点立即返回，阻止后续terminal与目标清理。

caller源码不再包含`0x0045ADF0` token。回归测试实际进入typed后缀并观察首个组B重置调用。

## 7. 测试与动态差分

定向测试覆盖：完整入口不等、匹配后group A零数量、source跳过、负目标跳过、首个非terminal候选重排、动态候选发布、packed完成全局清理、十项队列物理别名、126 dword固定清零，以及组A caller直连。

当前缺少原版两组角色对象、8类callee共享副作用、动态数量修改、十项角色顺序表、126 dword选择工作区和寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
