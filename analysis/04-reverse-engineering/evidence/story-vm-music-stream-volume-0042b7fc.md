# 剧情 VM 音乐stream音量 `0x0042B7FC`

状态：`platform_adapted`、有效运行域`assembly_exact`、`unit_tested`、`asset_absence_verified`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042B7FC..0x0042B839`

opcode：`115` / `OP_115_SET_MUSIC_STREAM_VOLUME`

## 1. 记录与夹取

记录固定为4字节：

```text
+0  u16 opcode
+2  u16 level
```

入口先清EAX，再执行`mov ax,[current+2]`，因此level严格零扩展为`0..65535`。随后用signed `cmp eax,11`：

- `level > 11`时把参数改成11；
- `level <= 11`时进入`test eax,eax / jge`；
- 该分支上的`level < 0`路径对零扩展数据不可达。

因此`0x8000/0xFFFF`等高位置位值不是负数，都会取上限11。handler不写全局mix level或任何场景音乐请求字段，只向wrapper提交一次夹取后的档位。

## 2. `sub_485850`与stream manager

wrapper在32位中执行`level << 7`，再用`0x2E8BA2E9`的有符号高半乘法、算术右移和符号修正实现向零除11。handler传入域为`0..11`，所以结果为`floor(level*128/11)`。

wrapper固定调用`sub_4866C0(manager,100,scaled)`。原manager：

1. 未初始化时返回0；
2. 找不到stream 100时返回`-1`；
3. 找到时更新节点fixed volume、调用`AIL_set_stream_volume`，再读取并返回`AIL_stream_volume`。

handler不测试EAX；上述成功、缺stream及backend结果都不能改变VM流转。

现代窄port复用`audio_video::set_legacy_stream_volume`和实际`LegacyStreamManager`，并把返回值明确丢弃。typed manager和宿主audio backend替代Miles对象，属于平台适配；stream 100、缩放、固定音量状态和返回忽略合同保持不变。既有stream command测试固定level 5缩放为58及实际manager/backend写入。

## 3. IP、previous与让出

两条夹取分支调用wrapper后都跳到共享4字节尾：

```text
0042C7E6  reload current physical pointer
0042C7EA  physical pointer += 4
0042C7ED  u16 IP += 4
0042C7F6  jump common join
```

handler不写ESI，正常zero-carry路径在common join发布normalized previous115，调用一次`_AIL_serve`并返回一。因此消费后总是让出，不同调用继续后继。

operand缺失时停在`[current+2]`首次读取，不调用wrapper，不推进IP，不发布previous，也不service audio。完整记录位于窗口末尾时，先完成volume调用、IP=`0x8000`、previous115和audio维护，再返回yield。

全局common-join继承carry属于P3独立runtime path；本handler自身没有设置continuation carry。

## 4. 资产锁与验证

线性TALK目录中opcode115为0条物理记录、0个entry probe，登记为`asset_absence_verified`。四种raw双字节字样统计为：

```text
raw 0073  TALK1/2/3/4 = 70/32/4/1
raw 4073  TALK1/2/3/4 = 1/0/0/0
raw 8073  TALK1/2/3/4 = 0/0/0/0
raw C073  TALK1/2/3/4 = 1/0/0/0
```

这些109处字节候选均未被线性目录或entry probe证明为指令入口，不能冒充真实记录回放。

synthetic覆盖四raw alias、level `0/10/11/12/0x7FFF/0xFFFF`、不可达负夹分支、高位零扩展、volume→IP/previous→audio顺序、VM音乐状态保持、operand截断和完整精确尾。Story VM synthetic、real及initial-session三项通过；Linux core 186/186、app 192/192完整门通过。未启动原版或OpenSWD3游戏EXE。
