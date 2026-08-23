# 标准模式entry alias重建 `0x0043CC00`

状态：`assembly_exact`、`unit_tested`

## 1. LST范围与caller

唯一行为真值为`swd3.exe.lst`。函数物理范围为`0x0043CC00..0x0043CC1C`，24行，无callee、无自身外置chunk。六个direct caller为：

- `0x0043C520`：`0x0043C54D`。
- `0x0043C590`：`0x0043C5BD`。
- `0x0043C3C0`外置chunk `0x0043C600`：`0x0043C62D`。
- `0x0043C670`：`0x0043C69D`。
- `0x00446420`外置chunk `0x0043C6E0`：`0x0043C71B`。
- `0x0043C760`：`0x0043C799`。

已关闭的C520/C590/C3C0/C670/C760直接复用typed helper；未关闭`0x00446420`及其chunk留待该owner独立审计。

## 2. 参数与精确语义

三个参数依次为signed window offset、entry base指针、entry alias owner指针。函数：

1. 把entry base写入alias owner。
2. 若signed offset小于等于0，立即返回。
3. 若offset大于0，每轮读取alias值、加4、offset减1、写回alias，直到offset为0。
4. EAX从入口起始终保持alias owner指针，返回的是owner地址，不是最终entry地址。

modern runtime以entry base为typed index0，因此最终alias等价为：

```text
window_offset <= 0 ? 0 : window_offset
```

结果以`i32* legacy_alias_owner_pointer`表达原EAX owner指针。正`INT_MAX`直接得到typed index `INT_MAX`；本函数从不解引用最终alias，不能在这里提前钳制或报告越界。真正的负/超界alias隔离仍在后续已关闭`0x0043CBD0`原解引用点。

## 3. caller回接

原`rebuild_entry_alias`port已从共享接口和测试fake完全移除。C520/C590/C670/C760及C3C0 page-advance在window/cursor更新后直接调用本helper，再由CBD0读取alias。

这也修正负window offset的旧fake行为：原函数对任何非正offset都保持entry base，而不是把负offset直接发布为alias。

## 4. 验证

`special_modes.legacy_initial_menu`覆盖window offset `-7/0/7/INT_MAX`：

- 非正值均写alias0。
- 正值写同值typed index，不作边界钳制。
- 四种情况均返回alias owner本身。
- 所有既有caller定向测试继续覆盖CC00→CBD0→selected/consume/sample顺序和CBD0边界传播。

定向测试通过。workpack连续生成两轮均为`38/227`，SHA256均为`fd99444cb9fe7102c3218207b6361d26c7b0a1933bd04a6bcf5d68fe72ef4a5c`；只新增关闭`0x0043CC00`，`0x0043CC20`仍为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
