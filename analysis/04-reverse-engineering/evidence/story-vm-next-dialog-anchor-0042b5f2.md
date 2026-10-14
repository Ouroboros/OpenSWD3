# 剧情 VM 下一对话框锚点 `0x0042B5F2`

## 1. 分阶段写入

opcode108是独立二级表入口，物理长度固定为6字节。机器顺序不可合并成whole-record预检：

1. `0x0042B5F2`读取`+2`的u16 X；
2. `0x0042B5F6`立即写入`dword_4A135C`低word；
3. `0x0042B5FC`读取`+4`的u16 Y；
4. `0x0042B604`立即写入同一dword高word；
5. 两个word都写入后才分别执行边界替换。

现代typed owner是现有`LegacyWorldStoryVmState::dialog_anchor_left/top`。selector缺失时两字段均不变；Y缺失时保留已经提交的原始X，且尚不执行X的边界替换。

## 2. 无符号边界与控制流

比较使用u16无符号语义：

```text
X <= 639  -> 保留X
X >  639  -> X = 16
Y <= 479  -> 保留Y
Y >  479  -> Y = 16
```

这是独立替换，不是把坐标夹到右/下边界。`0xFFFF`等负数的u16编码也会替换为16。成功后IP固定加6，置ESI=1，经`0x0042B0AE`发布previous108并同帧继续。

synthetic覆盖四个raw alias、`639/640`和`479/480`边界、`0xFFFF`、独立Y替换、两阶段截断、完整记录恰好结束于`0x8000`，以及previous和same-call。

## 3. 下游消费合同

共享dialog handler在`0x00427C2D`检查低word是否为`0x8000` sentinel；非sentinel时把低/高word分别加到下一条dialog的基准位置。dialog成功排队后，现代共享handler把`dialog_anchor_left/top`恢复为`0x8000/0x8000`，保持one-shot语义。

测试把opcode108与真实共享dialog opcode2串联在同一次step中，锁定108同帧继续、下一dialog消费并重置typed anchor owner。

## 4. 资产锁与验证

线性TALK目录中没有任何opcode108物理记录或入口probe，因此使用`asset_absence_verified`；不把raw字扫描或旧候选图当成真实资产入口。

Story VM synthetic、real及initial-session三项通过。未启动原版或OpenSWD3游戏EXE。

分类：`assembly_exact`。
