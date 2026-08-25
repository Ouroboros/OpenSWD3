# 模式1按整页前进记录选择 `0x0044E0C0`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0044E0C0..0x0044E1A0`，105行，无外部FUNCTION CHUNK。caller为`0x0044DBC0`和`0x0044F920`。仅level2有行为，其他level返回`level-2`且无副作用。

## 2. 页内与分页分流

入口先按32位计算`visible_count-1`：

- 若当前cursor不等于该末项索引，立即先发布cursor=0；visible_count至少1时再发布末项索引，随后返回。此路径不移动window、不解析文本、不播放声音、不置帧标志，也不刷新候选属性。空页返回值仍为-1，但发布cursor为0。
- 仅当cursor已等于末项索引，才把window按32位加13并先发布。若新window按i32不小于total，立即减13恢复；即使未翻页，仍继续刷新当前选择。
- 若新window按i32小于total，从完整工作区head按signed新window推进可见head，调用F7D0重计最多13条，再把cursor按i32夹到`new_visible_count-1`。

## 3. 刷新链

已在页尾的两条路径共用：以`window+cursor`的32位结果执行B9C0语义索引、解析共享文字、播放样本`0x00BF`、对帧标志低字节OR `0x30`，随后以D6B0低16目标索引并调用FAF0发布四名角色差值。B9C0 signed计数与D6B0低16目标不能合并。

## 4. typed-stop与验证

可见head推进、B9C0选择、共享文本、D6B0及FAF0只在原读取/调用点停止。可见head短链停止时保留已经发布的`window+13`和null head。

UT覆盖：页内直接跳末项且零helper；空页发布cursor0但返回-1；15条链从0翻到13、重计2条并把cursor从12夹到1；到达total时恢复window但继续完整刷新；短链保留window13和null head后停止。

workpack双生成稳定为`207/227`，SHA256为`5a2f9741787c6093e6d8986c967088b66ec13a8ac623e91ca2224c3ab1859982`。`0x0044DBC0`继续等待callee闭环；下一单元为`0x0044E1B0`。
