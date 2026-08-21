# 剧情 VM overlay action 双链等待 `0x0042B70C`

状态：`platform_adapted`、有效运行域`assembly_exact`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042B70C..0x0042B722`

opcode：`112` / `OP_112_WAIT_PACKED_ROW_AND_ROLE_HEAD_ACTIONS`

## 1. 短路谓词

handler按固定顺序读取两条全局链头：

```text
1. dword_4BAB9C  packed-row framebuffer效果链
2. dword_4BA6E0  角色头顶action链
```

第一条链非空时立即进入common join，绝不读取第二条链。只有第一条链为空，才读取第二条链。任一相关链非空时IP不推进。

`dword_4AD3E8`世界移动action链不在谓词中。即使该链非空，只要上述两条链都为空，opcode112仍完成。

## 2. 完成、previous与让出

两条相关链都为空时，机器在`0x0042D1C4..0x0042D1D0`把物理指针与u16 IP各推进2字节，再进入common join。两条等待路径直接进入同一join且保持IP不变。

所有路径的`ESI`都为0。common join先把normalized opcode112发布到`dword_4CF6D8`，随后执行一次`_AIL_serve`并返回1。因此：

```text
任一相关链非空  IP不变  previous112  audio一次  yield
两条相关链都空  IP+2    previous112  audio一次  yield
```

完成路径不会在同一次解释器调用中取后继指令。这是与普通same-call等待完成路径不同的固定合同。

## 3. typed owner适配

SDL生产runtime把`packed_row_effects`绑定到实际`world_frame_effects_.packed_rows`，把`role_head_actions`绑定到实际`world_role_head_actions_`。现代实现按机器访问顺序检查这两个typed owner：

- packed-row owner缺失时，在第一次固定全局链头访问点停止；
- packed-row链非空时，不要求role-head owner存在；
- packed-row链为空而role-head owner缺失时，才在第二次链头访问点停止。

owner缺失不推进IP、不发布previous且不service audio。该失败域只隔离原固定全局/裸链所有权，因此整体分类为`platform_adapted`；有效绑定域的短路、位宽、顺序与副作用为`assembly_exact`。

## 4. 资产锁

线性TALK目录锁定9条物理记录/9 probes，全部为raw `0x0070`、固定长度2：

```text
TALK1.DAT 4
TALK2.DAT 3
TALK3.DAT 0
TALK4.DAT 2
```

后继opcode分布为：26一条、52一条、25一条、67一条、88两条、120两条、96一条。代表记录`TALK1.DAT@0x000307D8`后继为opcode26。

真实回放先保持role-head链非空，确认opcode112原地yield；随后清空该链，确认第二次调用只把IP推进到2并再次yield。整个过程保留一条非空moving-action链，证明它不参与谓词。

## 5. 验证

synthetic覆盖四raw alias、packed-row首链短路、仅role-head非空、两链都空、非空moving-action忽略、两个typed owner失败点、audio回调观察到previous/IP的顺序，以及等待/完成两类精确窗口尾。

Story VM synthetic、real及initial-session三项通过。Linux core 186/186、app 192/192通过。未启动原版或OpenSWD3游戏EXE。
