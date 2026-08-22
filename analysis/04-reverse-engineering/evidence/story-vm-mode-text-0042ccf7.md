# 剧情 VM 模式文本槽 `0x0042CCF7`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042CCF7..0x0042CDEC`

opcode：

- 170 / `OP_170_CLEAR_MODE17_TEXT`
- 171 / `OP_171_SET_MODE17_TEXT`
- 172 / `OP_172_CLEAR_MODE18_TEXT`
- 173 / `OP_173_SET_MODE18_TEXT`

## 1. 两个nullable 52字节owner

handler用normalized opcode选择两个独立全局指针和剧情位：

```text
opcode 170/171 -> dword_4B751C -> mode 17 text -> story flag 77
opcode 172/173 -> dword_4B74F4 -> mode 18 text -> story flag 78
```

两个指针各自为空或指向`malloc(0x34)`的52字节块。生命周期交叉引用确认：

- `sub_40E0B0:0x0040E46F/0x0040E475`初始化为null；
- `sub_4070A0:0x00408047..0x00408083`先零填104字节保存块，再把两个非空槽各复制52字节；
- 同函数`0x00408B18..0x00408B63`按保存块的两个52字节区域恢复，首字节为零时保持null；
- `sub_4070A0:0x00408302..0x00408326`关闭旧owner并置null；
- special-mode消费者`sub_4442B0:0x00444BB7..0x00444C0B`和`sub_447100:0x00447ECD..0x00447F16`分别在mode17/18读取，null时临时分配并复制固定默认文本。

现代`LegacyWorldStoryVmState::mode_texts[2]`以`allocated + array<u8, 0x34>`承接nullable owner。初始化只清`allocated`，不可访问的backing bytes可保持旧值，对应原指针置null后旧分配不可见。B9/B11后续消费者和持久化接入可直接复用该owner，不在本工作包伪造UI或存档路径。

## 2. 偶数opcode清槽

170和172不读取operand，严格执行：

```text
IP += 2
free(selected slot)        // null也调用，CRT free(null)直接返回
selected slot = null
clear story flag 77/78
publish normalized previous
service audio once
 yield
```

handler不写ESI，跳`loc_42B0AA -> loc_42B0AE`后normal carry为零，因此进入`_AIL_serve`并yield，不same-call后继。记录位于`IP=0x7FFE`时，owner/flag/IP/previous/audio/yield均先完成，不进行下一fetch。

## 3. 奇数opcode变长文本

171和173从指令`+2`开始，在每个字节位置执行unaligned u16比较，寻找首个`25 51`（`%Q`）。marker不复制到最终owner，物理长度为：

```text
4 + byte_count_before_%Q
```

找到marker后，机器顺序为：

```text
IP = u16(marker_offset + 2)
free(old slot) when non-null
new slot = malloc(52)
new slot[0..51] = 0
lstrcpyA(new slot, shared scratch)
set story flag 77/78
publish normalized previous
service audio once
 yield
```

parser在共享`FileName` scratch中保留`%Q`前全部bytes并追加NUL。embedded NUL不停止parser，但`lstrcpyA`只复制首个NUL前的prefix；后续scratch bytes不进入52字节slot。空文本合法，产生allocated且全零的slot并置位。

## 4. 原始危险点与typed-stop

原版scanner可跨`0x8000`窗口读取，共享scratch无当前handler局部边界；现代缺marker或最后一个u16不完整时以`mode_text_terminator_not_found`停止，且不提交IP、不释放旧owner、不改flag/previous、不audio。

原版`lstrcpyA`不检查52字节目标。首个NUL前长度51以内合法；长度52时先成功写入52个bytes，再在目标索引52写NUL越界。现代在同一危险点以`mode_text_out_of_range`停止，保留此前已提交的IP、旧owner释放、新owner存在、52字节零填/已复制内容和旧flag值；不发布previous、不audio。若`%Q`前总长度超过52但embedded NUL更早，`lstrcpyA`仍安全，现代同样只复制prefix并成功。

裸malloc/free、共享scratch和无界Win32复制由固定typed owner承接，因此分类为`platform_adapted`；合法文本、nullable状态、初始化、flag、IP和yield行为不变。

## 5. 真实TALK资产

完整线性目录锁定21条物理记录/21 probes，全部基础raw：

```text
opcode 170  9
opcode 171  3
opcode 172  5
opcode 173  4

TALK1.DAT   8
TALK2.DAT   4
TALK3.DAT   5
TALK4.DAT   4
```

奇数记录文本长度为10、26、34、36，全部小于52字节且marker为`25 51`。代表回放：

```text
TALK1.DAT@0x00038FA3  opcode170  length 2
TALK1.DAT@0x000233D7  opcode171  text length 34
TALK4.DAT@0x0000510C  opcode172  length 2
TALK3.DAT@0x00010AAE  opcode173  text length 10
```

四条分别验证两个槽的clear/set、对应flag、IP、previous、audio一次与yield。

## 6. 验证

synthetic覆盖四opcode全部四raw alias、两槽/两flag隔离、空文本、embedded NUL及其后长suffix、旧owner替换、初始化清nullable状态、缺terminator、52字节NUL越界危险点、previous/audio/yield顺序、后继留到下一调用，以及固定/变长精确窗口尾。

真实资产测试逐条锁定21条record的raw、长度、marker、opcode/file分布，并回放四种变体。既有opcode134真实测试改用显式OP1025停止点，同时仍断言真实后继是171，避免把已实现的独立变长handler当旧unsupported边界。Story VM synthetic、real及initial-session三项通过。Linux core完整门186/186、app完整门192/192通过。未启动原版或OpenSWD3游戏EXE。
