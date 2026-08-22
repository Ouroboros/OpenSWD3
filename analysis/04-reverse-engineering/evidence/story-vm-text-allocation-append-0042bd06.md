# 剧情 VM 文本分配链追加 `0x0042BD06`

状态：`assembly_exact`、`unit_tested`、`asset_absence_verified`、`platform_adapted`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042BD06..0x0042BDBB`

opcode：125

## 1. 进程期链owner

`dword_4C99FC`是`unk_4C99F8+4`的链首。全LST只有两处业务xref：

- `sub_40AFF0:0x0040B0FF..0x0040B169`尾插准备后的dialog文本副本。
- opcode125 `0x0042BD06..0x0042BDBB`尾插脚本内的opaque文本。

两处都从链首沿node `+4`扫描到第一个null next，再追加8字节节点。节点`+0`指向独立文本分配，`+4`为next。没有读取节点文本、摘链或释放该全局链的第三个xref，因此它是进程期只追加分配链，不是dialog显示链。

现代VM state以`list<array<u8,0x100>>`承接opcode125节点并跨`initialize_legacy_world_story_vm`保留。既有dialog message由RAII text owner承接`sub_40AFF0`的实际显示文本，不再复制无消费者的第二份进程泄漏链；节点与buffer两次unchecked malloc合并为单一RAII节点分配。这是所有权平台适配，不改变opcode125合法域的追加顺序或文本字节。

## 2. 机器顺序

handler在读取任何脚本字节前完成：

```text
扫描到现有链尾
malloc(8-byte node)
链尾.next = node
malloc(0x100-byte buffer)
node.text = buffer
node.next = null
buffer[0..255] = 0
```

随后从指令`+2`开始，以每个位置的unaligned u16查找首个字节对`25 51`（`%Q`）。若当前位置不是terminator，就先推进输入和copy count，再把刚越过的单字节写到buffer。terminator本身不由copy循环复制。

找到terminator后，机器顺序为：

```text
IP = u16(terminator_offset + 2)
physical script pointer = terminator + 2
buffer[count + 0] = '%'
buffer[count + 1] = 'Q'
buffer[count + 2] = 0
previous = 125
audio service
跨帧让出
```

因此物理长度是`4 + byte_count_before_%Q`。空文本长度4合法。记录完成后不same-call执行后继。

## 3. 原始不安全点与typed stop

原版固定分配256字节，但copy与`%Q\0`追加均不检查长度：

- 复制第257个普通字节时首次越界，IP尚未提交。
- 254字节文本找到terminator后，IP先提交；随后`%`和`Q`成功写入索引254/255，NUL在索引256越界。
- 255/256字节文本分别在suffix第二/第一字节越界。
- 缺少terminator会继续跨窗口读并最终越界。

现代实现只在原script u16读、buffer byte写和分配点增加typed-stop。失败保留已追加且已零填充的节点、此前copy、已提交IP及已写suffix；不发布previous，不service audio。合法文本、terminator、IP和yield行为不变，故分类为`platform_adapted`。

## 4. 资产锁与验证

完整线性TALK目录58,782条记录中，opcode125为0条物理记录、0个entry probe，因此使用`asset_absence_verified`，不伪造真实回放。

全文件非入口字样统计：

```text
raw word  TALK1  TALK2  TALK3  TALK4  total
0x007D       16     29      5     11     61
0x407D        0      0      0      0      0
0x807D        0      0      0      0      0
0xC07D        2      0      1      0      3
```

synthetic覆盖四raw alias、空文本、顺序多节点、初始化后链保持、正常audio-yield与下一调用后继、缺terminator部分copy、第257字节copy越界、254字节suffix部分提交，以及窗口`0x7FFA`开始的完整精确尾。

Story VM synthetic、real及initial-session三项通过。Linux core 186/186与app 192/192完整门通过。未启动原版或OpenSWD3游戏EXE。
