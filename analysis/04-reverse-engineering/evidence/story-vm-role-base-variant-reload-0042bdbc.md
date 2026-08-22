# 剧情 VM 角色基础变体条件重载 `0x0042BDBC`

状态：`assembly_exact`、`unit_tested`、`real_asset_tested`、`asset_absence_verified`、`platform_adapted`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042BDBC..0x0042BE89`

opcodes：126、127

## 1. 记录与角色来源

两条指令共享固定10字节布局：

```text
+0  u16 opcode
+2  u16 role selector
+4  u16 expected base variant
+6  u32 same-file target，只有taken分支读取
```

handler先读selector。字面`0xFFF0`替换为当前Talk来源GUID，其他值原样交给`sub_40C0D0`；因此`0xFFFE`继续使用helper-native受控角色规则。lookup命中时直接借用运行角色。

lookup失败时，机器在读取`+4`前执行：

```text
malloc(0xD8)
sub_40D560(temporary_role, selector)
```

`sub_40D560`从mutable MAPS role source物化GUID、动作三字段、坐标、Talk、Path及flags。handler忽略其返回值，只保存`temporary_role+0x48`，即完整u32 `action.base_variant`。临时角色在分支或顺序路径完成后、进入common previous join前释放。

若MAPS source也缺失，`sub_40D560`返回`0xFFFFFFFF`且不写临时buffer，原handler仍读取未初始化heap `+0x48`。现代实现不伪造陈旧值，在该原危险读点返回`role_not_found`；临时分配由RAII释放。unchecked malloc、缺少现代MAPS binding及损坏source owner同样只在原访问阶段typed-stop，故整体分类为`platform_adapted`。

## 2. 两个互反谓词

机器先保存完整u32 current base variant，再零扩展读取`u16(+4)`并比较。不是低16位比较：例如current `0x00010008`与operand `8`不相等。

```text
opcode126: current == expected 时taken
opcode127: current != expected 时taken
```

内部实现以`setz`产生相等位，再对`opcode == 127`的布尔值执行XOR，因此两条谓词严格互反。

not-taken路径完全不读`+6`target，只把物理脚本指针与u16 IP增加10，随后发布previous126/127并same-call继续。窗口只提供到`+4`compare word时，该路径仍提交IP；下一fetch才受检失败。

## 3. taken窗口重载

taken路径才读取完整unaligned `u32(+6)`并调用`sub_42E430`：

```text
service audio
context talk-data offset = target
IP = 0
在当前TALK文件绝对读取target+0x200处的0x8000字节窗口，不预清旧窗口
```

helper返回后，missing-live临时角色先释放；common join再发布normalized previous并以`ESI=1`同调用从新窗口offset0继续。handler本身不yield。

现代复用既有`load_same_file_story_window`。I/O失败不会伪装成功：保留audio、context target、IP0、loader调用、临时owner释放和previous发布，再返回`load_failed`并标记窗口未载入。valid I/O域顺序与机器一致。

## 4. 资产与验证

线性TALK目录锁定opcode126共236条物理记录/236 probes：

```text
TALK1  97
TALK2  77
TALK3  62
TALK4   0
```

全部raw `0x007E`、长度10。230种selector范围`0x0117..0xEA55`，无`FFF0/FFFE`；expected只有0和8，分别5条与231条；98种target范围`0x000079D7..0x00038D03`，全部`target+0x200`位于所属TALK文件内。

opcode127在线性目录中为0条记录/0 probes，以`asset_absence_verified`和共享handler synthetic锁定。全文件`0x007F`字样为28处，高位alias字样为0，均不提升为资产记录。opcode126的高位字样只有`0xC07E`共20处，亦非线性入口。

真实回放使用`TALK1.DAT@0x00007B8C`：

```text
7E 00 46 EA 08 00 D7 79 00 00
```

运行角色GUID `0xEA46`的base variant为8，因此opcode126重载data offset `0x79D7`；目标窗口同调用执行opcode1026后遇到`FFFF`，按既有terminal合同结束Talk源。synthetic另覆盖两opcode四raw alias、互反taken/not-taken、完整u32比较、FFF0/FFFE、live与MAPS临时物化、缺database/source、selector/compare/target分阶段截断、not-taken未读target尾及loader failure顺序。

Story VM synthetic、real及initial-session三项通过。Linux core完整门186/186、app完整门192/192通过。未启动原版或OpenSWD3游戏EXE。
