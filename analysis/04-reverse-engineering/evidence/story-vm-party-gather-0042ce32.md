# 剧情 VM 队伍向玩家集合 `0x0042CE32`

状态：`platform_adapted`、有效运行域`assembly_exact`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042CE32..0x0042CF7B`

opcode：177 / `OP_177_GATHER_PARTY_AT_PLAYER`

## 1. raw bit15两相协议

handler无operand，但直接测试raw opcode word的bit15。解释器分派仍以`raw & 0x3FFF`得到normalized opcode177，所以四种raw alias分为：

```text
00B1 / 40B1  -> setup相位
80B1 / C0B1  -> poll相位
```

setup把当前raw word OR `0x8000`，因此00B1变80B1、40B1变C0B1；IP保持不变。poll只有完成时才把raw word AND `0x7FFF`并把IP推进2；等待时raw和IP均保持。bit14始终保留。

所有正常出口都不设置same-call carry：common join发布normalized previous177，service audio一次并yield。即使poll完成并推进2，也不在同一次VM调用读取后继。setup从不检查完成条件，所以单人队伍也必定先setup-yield，再由下一次poll完成。

## 2. setup相位写序

setup严格执行：

1. 原地设置脚本raw bit15；
2. 对共享`dword_4A9920`完整u32 owner OR `0x00008000`；
3. 把受控角色`+0x48`，即`action.base_variant`，写零；
4. 用受控角色`world_y`填充`dword_4B7998`的32个dword；
5. 用受控角色`world_x`填充`dword_4B7A18`的32个dword；
6. 只扫描角色索引`1..count-1`；对flags bit7置位的party角色，OR flags bit26并把角色`+0x88`，即`action.wait_override`，写`0x8000`；
7. previous、audio一次、yield，IP不动。

两组历史复用实际`LegacyWorldPlayerPostFrameState::world_y_history/world_x_history`；SDL把Story VM runtime直接绑定到`world_frame_state_.player_post_frame`。现代required owner可空时，在原第一次历史写入点返回`runtime_unavailable`；此前脚本bit15、dialog bit15和受控角色base variant清零保持，次要角色循环尚未发生，不回滚。

## 3. poll相位与完成门

poll不访问dialog counter、受控角色base variant或玩家历史。它扫描角色索引`1..count-1`，只处理flags bit7置位且X、Y分别等于当前受控角色X、Y的记录。每个匹配者按顺序：

```text
matching_count += 1
role.flags &= 0xFBFFFFFF   // clear bit26
role.action.wait_override = 0
```

扫描完成后读取`dword_4BABA0`。该global由世界角色转入/移出路径维护，现代对应实际`live_party_role_count` owner。完成条件是精确相等：

```text
matching_count + 1 == live_party_role_count
```

左侧加一计入受控玩家。相等时清脚本raw bit15并推进2；不等时保留raw bit15与原IP。两路随后都previous、audio一次并yield。

缺少live party count owner时，modern在原global读取点typed-stop；此前所有匹配角色的bit26和wait override已经逐项清除并保持，脚本raw/IP/previous/audio尚未改变。两个nullable owner及typed failure代替原裸global必达域，因此分类为`platform_adapted`；owner存在的合法业务域逐项`assembly_exact`。

## 4. 资产锁与验证

线性TALK目录锁定29条物理记录/29 probes，全部raw `0x00B1`、固定长度2：

```text
TALK1/2/3/4 = 0/0/1/28
```

代表记录：

```text
TALK3.DAT@0x000066FE
TALK4.DAT@0x00009842
```

两条均在`IP=0x7FFE`回放setup→poll：第一次把raw改为80B1并在原IP audio-yield；单人队伍的第二次poll清回00B1、推进到`0x8000`并再次audio-yield。全文件双字节候选为00B1 63处、C0B1 21处、40B1/80B1各0处；除上述29条00B1外均非线性入口。

synthetic覆盖四raw alias、setup的self/dialog/base/history/party写序、非party跳过、已置bit26幂等、poll匹配与X/Y不匹配、history在poll不读取、等待精确不等、完成精确相等、bit14保持、单角色与精确窗口尾、setup缺history和poll缺party count的已提交副作用。player-history、party transfer依赖与Story VM synthetic/real/initial-session共5/5通过，SDL app编译通过；Linux core 186/186、app 192/192通过。未启动原版或OpenSWD3游戏EXE。
