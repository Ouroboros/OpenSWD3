# 剧情 VM 批量角色步进 `0x0042B63C`

状态：`platform_adapted`、有效运行域`assembly_exact`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042B63C..0x0042B6A0`

opcode：`109`，C++语义常量`OP_109_STEP_ROLES`

## 1. 变长记录与循环

物理记录为：

```text
+0  u16 opcode
+2  u16 count
+4  count个u16角色selector
```

handler先零扩展读取count，再从`+4`开始逐项处理。每项严格按以下顺序执行：

1. 读取当前selector；
2. 调用`sub_40C0D0(selector, &role_index)`；
3. 无论命中与否，物理游标前进2字节；
4. lookup失败时静默处理下一项；
5. lookup成功时调用`sub_42E280(role_index)`，忽略其`0/1/2`返回值。

handler不把`0xFFF0`替换为当前Talk来源；它仍是普通GUID字面量。`0xFFFE`继续由`sub_40C0D0`按受控角色selector处理。

现代实现复用已闭环的`query_legacy_world_story_path`。该typed owner保留未找到type-2槽、准备下一步和已到达三类副作用；运行时、槽、cursor、direction或surface损坏在原危险点明确停止，因此整体分类为`platform_adapted`。

## 2. 长度、previous与让出

循环结束后，机器在`0x0042B688`重新读取原记录count，再分别计算：

```text
物理游标 += 4 + 2 * count
u16 IP   += low16(4 + 2 * count)
```

现代实现保留count重读和u16 IP加法。count为0合法。固定`0x8000`窗口内最大完整记录是count `0x3FFE`，完成后IP为`0x8000`；count `0x3FFF`在最后一个selector首次越界点停止，不提前整表预检。

每次fetch前`0x00427B59`清零ESI。opcode109不改ESI，因此成功后经`0x0042B0AE`发布previous109，再执行一次audio service并让出；不会同帧继续下一条。

selector或helper中途失败时，IP、previous和audio保持未提交；此前已完成角色helper副作用不回滚。

## 3. 资产锁

线性TALK目录锁定67条物理记录和67个entry probe：

```text
TALK1.DAT  53
TALK2.DAT  14
TALK3.DAT   0
TALK4.DAT   0
```

全部为raw `0x006D`，长度均满足`4 + 2 * count`。count分布为：

```text
1:2, 2:47, 3:4, 4:8, 5:4, 9:1, 18:1
```

共187个selector引用、55种值，范围`0x0001..0x027F`；资产中没有`0xFFF0/0xFFFE`。真实回放覆盖`TALK1.DAT@0x0001BF5B`的count1记录、`TALK2.DAT@0x0000F92D`的count18记录，以及opcode17跳转后同调用执行opcode109的真实链。

## 4. 验证

synthetic覆盖四raw alias、helper返回`0/1/2`、missing静默、`FFF0`字面lookup、`FFFE`受控lookup、后项helper失败保留前项副作用、count/selector分阶段截断、count0、精确尾、完整窗口与首个窗口外selector。

Story VM synthetic、real及initial-session三项通过；Linux core 186/186、app 192/192通过。未启动原版或OpenSWD3游戏EXE。
