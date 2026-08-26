# 战斗最终角色步进 `0x0045AA00`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整LST范围

权威函数为`0x0045AA00..0x0045ADEC`，从proc到endp完整465行、20个静态call站点、26个`loc_`标签，无外部FUNCTION CHUNK。14个唯一callee。

两个静态caller均已关闭：组A帧`0x00456680`和组B帧`0x004576A0`各一处。两个caller已删除本函数opaque token并直接组合typed最终角色步进。

## 2. 分支与对象token

第二参数完整dword等于1时进入组A；其他所有值进入组B。

```text
group A = 0x005029D0 + index * 0x2F34
group B = 0x00525508 + index * 0x2B28
```

两组token都按低32位回绕。组A和组B都先调用对象有效性callee，完整EAX不等于1时返回0。组A只有有效性返回1后才首次读取十槽完成flag，越界在此typed-stop。组B index全1在任何callee前直接返回0。

## 3. 组A完成计数与工作区

组A完成flag等于1时：

1. 打包计数第二byte按u8加1；
2. 与共享阶段dword高word做unsigned word比较；
3. 达阈值后，从`group_a_count * 32`对应位置开始，按每项32字节向低地址清零阈值项；
4. 只有全部清零成功后才清阶段高word并从group A数量减阈值；
5. 三个刷新callee始终在完成flag路径末尾调用，即使尚未达阈值。

工作区清零不预检完整范围。每个dword在原始首次写入点检查typed边界，停止时保留此前第二byte递增和已完成的清零写。

完成flag不等于1时，独立removed byte按u8加1。

## 4. 组A公共清理

函数随后按原顺序：

- 调用角色清理callee；
- 发布`index + 8`角色code；
- 清对应十槽值；
- 当前角色code命中时，先扫描完整组A，再扫描完整组B，对每个对象调用重置callee，最后发布七项共享门；
- selected code命中时发布全1并清选择门；
- 在当前group A数量内查找角色code，命中后固定左移至第九槽并把第十槽清零。

角色顺序表只在循环实际访问时typed-stop。两组批量callee扫描使用signed数量；非正数量不进入循环。

## 5. 组A终止与继续

剩余量按低32位计算：

```text
remaining = group_a_count - excluded_count - phase_high_word
```

removed byte以zero-extended dword与remaining做unsigned比较。达到或超过时固定清零126个dword工作区，发布终止共享值并返回1。

否则查询继续callee。完整EAX不等于1仍返回1；等于1时按顺序发布selected code加1、当前角色code、配置callee、pending、工作区偏移`2 + actor_index`的事件槽和辅助门，然后以callee可改写的角色code定位五dword记录并清零。事件槽与126 dword工作区保持同一物理别名，不能拆成独立数组；记录只在首次实际写入时typed-stop，且保留此前发布。

因此组A只有初始有效性失败返回0；有效对象的所有非typed-stop路径返回1。

## 6. 组B路径

有效对象按以下顺序执行：

1. 坐标callee返回两个低word，分别以u16回绕累加；
2. 描述符callee返回物理token，随后首次读取其偏移32的flag；零token在该实际访问点typed-stop，坐标副作用已保留；
3. flag bit5置位时延迟加20，否则加3，均在u16域；
4. 查询动作、以固定owner发布动作、发布actor index；
5. signed index位于`0..group_b_count`闭区间时，打包计数低byte加1；
6. `low_byte - third_byte`按低32位后做signed比较，达到group B数量时发布终止共享门；
7. reset word非零时查询固定组B首对象；返回0且`group_b_count - low_byte == 1`时，把数量置1、低byte和reset word清零，再调用三个刷新callee。

有效组B对象所有非typed-stop路径返回1。

## 7. 角色预处理共享别名

相邻`0x0045D490`关闭后，terminal、active、secondary、published、action execution、auxiliary、message、126 dword事件工作区和十组五dword角色记录直接复用本函数与动作分派的既有typed状态。预处理只新增此前未命名的source actor与双门字段，不复制角色状态或工作区。

全局重置按原物理写集合清对应标量及工作区槽`0..9`、`16..95`，保留工作区其他槽和五dword记录。逐帧协调器通过同一两份状态调用预处理。战斗调试快捷键进一步确认两项frame gate分别与逐帧选择mode及动作路径同址门共享，selection gate与逐帧active共享，排队角色与逐帧选择来源共享；C/W键和全局重置按原写序同步这些owner。十项角色顺序也与W键完整清零共用本状态。

## 8. caller回收

组A帧直接组合typed实现；子返回1时才清最终门和选择word，子typed-stop立即作为父状态返回，父函数没有后续副作用。父函数仍保留原始“子返回0也最终返回1”的行为。

组B帧在单条效果记录后直接组合typed实现；子完整返回值成为父返回值，只有1才清映射、最终槽、选择word和门。子typed-stop阻止这些清理。

两个caller源码均不再包含`0x0045AA00` token。

## 9. 测试与动态差分

定向测试覆盖：初始有效性失败、组A完成阈值和32字节逆向清零、三刷新、双组重置、角色顺序左移、removed unsigned终止、工作区typed-stop、配置后记录typed-stop、组B全1早退、坐标回绕、描述符bit5、动作发布、闭区间低byte递增、signed完成比较、组B重置、零描述符停点，以及组A/组B caller直连和组B父级typed-stop传播。

当前缺少原版两组角色对象、14类callee共享副作用、角色工作区、描述符对象和寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
