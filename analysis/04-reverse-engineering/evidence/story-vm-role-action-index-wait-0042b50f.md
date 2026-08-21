# 剧情 VM 角色动作下标阈值等待 `0x0042B50F`

## 1. 机器访问顺序

opcode107是独立二级表入口，物理长度固定为6字节。`0x0042B50F`先读取`+2`的u16角色selector；仅当值为`0xFFF0`时，替换为当前TALK context `+0x24`的source GUID。随后立即调用`sub_40C0D0`完成角色lookup，lookup结束后才读取`+4`的u16 threshold。

lookup成功时，机器从角色记录`+0x40`读取action的`packed_ap_state`：

- 低字节：动作项数量/合法threshold上限；
- 高字节：一基当前动作项下标。

现代实现先分阶段取得selector并lookup，再分阶段取得threshold，最后才读取typed `LegacyWorldRoleRecord::action.packed_ap_state`，保持`selector → lookup → threshold → action`顺序。`0xFFFE`仍由`sub_40C0D0`对应的共享lookup helper解析为controlled role。

## 2. 无符号边界与控制流

机器在`0x0042B55A`按u16比较低字节上限和threshold：

```text
threshold > low_byte:
    调用空诊断函数，IP += 6，ESI = 1，同帧继续

threshold <= low_byte && high_byte < threshold:
    IP不变，ESI = 0，让出

threshold <= low_byte && high_byte >= threshold:
    IP += 6，ESI = 1，同帧继续
```

因此threshold等于低字节上限仍合法；threshold为0时，高字节不可能小于它，必然完成。lookup失败在`0x0042B5BB`读取threshold供诊断，然后同样消费6字节并继续。

两条诊断调用均落到`nullsub_1`；LST在`0x0044A240`把它锁为单字节collapsed null function，因此现代实现不伪造日志或副作用。

所有路径最终经过common join `0x0042B0AE`，先发布有效opcode107到previous，再由ESI决定让出或继续。旧C++业务比较基本正确，但whole-record预检把threshold读取提前到lookup之前，并在等待与消费路径均漏发previous；本轮已修复。

## 3. Typed映射与精确尾

原始角色数组的`role + 0x40`正好映射到`LegacyWorldRoleRecord::action`，其内部`packed_ap_state`偏移也由static assertion锁在`+0x40`。有效角色域内不需要平台替代；缺角色、非法threshold和正常完成均按机器静默消费。

synthetic覆盖：

- 四个raw alias；
- `0xFFF0` current-source与helper-native `0xFFFE` controlled-role selector；
- threshold等于上限时等待；
- 高字节等于threshold时完成；
- threshold大于低字节上限；
- lookup失败；
- selector缺失与lookup后的threshold缺失；
- 完整6字节恰好结束于`0x8000`时先消费、发布previous，再由下一次fetch失败。

## 4. 资产锁与验证

opcode107在线性TALK目录中共有294条物理记录/296 probes，全部raw `0x006B`、长度6，分布：

```text
TALK1/2/3/4 = 64/27/36/167
```

35种selector、20种threshold（范围`1..50`），合计67种`{selector, threshold}`组合。两条TALK3记录各被两个入口probe命中，其余记录各一个probe。四库代表回放：

```text
TALK1.DAT@0x00005445  selector 0x0001  threshold 5
TALK2.DAT@0x000074CC  selector 0x0001  threshold 5
TALK3.DAT@0x00002B20  selector 0x000B  threshold 9
TALK4.DAT@0x00001987  selector 0x0011  threshold 8
```

四条真实记录均放置在`0x7FFA`精确尾，以typed角色action高/低字节同时等于threshold驱动完成；结果先推进至`0x8000`并发布previous107，下一fetch才返回越界。Story VM synthetic、real及initial-session三项通过。未启动原版或OpenSWD3游戏EXE。

分类：`assembly_exact`。
