# 剧情 VM 角色 Talk 脚本写入 `0x0042B3B0`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`external_dependency_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042B3B0..0x0042B436`

opcode：`100`

## 1. staged operand与selector

机器在任何selector替换或角色lookup之前依次读取：

```text
+2 u16 role selector
+4 u16 Talk script number
```

因此缺少`+4`时不会执行lookup、live角色写入或MAPS fallback。handler随后只把字面`FFF0`替换成当前`LegacyWorldTalkContext::source_guid`；`FFFE`保持原值并由共享lookup helper解析为受控角色。普通GUID lookup继续跳过带原skip bit的角色并采用首个合法匹配。

Talk script number不做范围或sentinel验证；`0000`和`FFFF`都按原word写入。

## 2. live与MAPS双路径

lookup命中时，机器只把Talk script number写到live role `+0x1E`，即`LegacyWorldRoleRecord::talk_script_id`。

lookup失败时，机器调用`sub_40D460`，11参数精确等价为：

```text
guid            = resolved selector
action/base/delta/tile_x/tile_y = FFFF
talk_script_id  = instruction +4
path_data_id    = FFFF
flags_or_mask   = 0000
flags_and_mask  = FFFF
logical_map_id  = FFFF
```

modern复用已集成`LegacyMapsRolePatchRequest`和SDL world-session MAPS database port。`OR 0`与`AND FFFF`按机器真实调用保留，虽然两者对现有flags数值均为恒等操作。source GUID不存在或当前没有world-session owner时，typed平台层保持无写入结果，不伪造live角色。

两条路径都固定IP`+6`，进入common join发布previous100，并以`ESI=1`在同一VM调用继续。

## 3. 边界、资产锁与验证

synthetic覆盖四个raw alias、live角色写入、`FFF0`当前source、`FFFE`受控角色、`FFFF` Talk值、missing-role完整MAPS patch、双operand staged失败和精确窗口尾。完整记录起于`0x7FFA`时，live/MAPS副作用、IP=`0x8000`和previous100先提交，下一same-call fetch再返回`instruction_out_of_range`。

线性TALK目录锁定192条物理记录/192 probes，全部raw `0x0064`、长度6，分布：

```text
TALK1/2/3/4 = 49/14/47/82
```

资产含112种selector、68种Talk值；Talk范围0..6909。当前线性记录没有`FFF0`或`FFFE`，两者由synthetic独立锁定。真实回放代表：

```text
TALK1.DAT@0x00043399  selector 0x0019  Talk 0
TALK2.DAT@0x00013B3F  selector 0x0068  Talk 0
TALK3.DAT@0x00002E44  selector 0x000E  Talk 0
TALK4.DAT@0x0000488B  selector 0x0252  Talk 6421
```

四条真实记录均在missing-role路径提交Talk-only MAPS patch并完成精确尾。Story VM synthetic、real及initial-session三项通过；`legacy_world_map_business`、`legacy_maps_world_database`与real database依赖3/3通过。未启动原版或OpenSWD3游戏EXE。

分类：`platform_adapted`。operand顺序、selector语义、live字段、fallback参数、推进、previous和same-call均保持；Win32裸role-source全局映射为typed MAPS database owner。
