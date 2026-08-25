# 模式1按整页后退记录选择 `0x0044E1B0`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0044E1B0..0x0044E25B`，83行，无外部FUNCTION CHUNK。caller为`0x0044DBC0`和`0x0044F920`。仅level2有行为，其他level返回`level-2`且无副作用。

## 2. 页内与分页分流

- 当前cursor非0时，只把cursor写0并立即返回。不移动window、不解析文本、不置帧标志，也不刷新候选属性。
- cursor为0时，把window按32位减13并先发布。
- 若结果符号位为1，先把cursor和window都写0，再以完整工作区head作为可见head。
- 若结果非负，从完整工作区head按signed新window推进可见head。
- 两条分页路径都调用F7D0重计最多13条可见记录。

## 3. 刷新链及方向差异

以`window+cursor`的32位结果执行B9C0语义索引、解析共享文字、对帧标志低字节OR `0x30`，随后以D6B0低16目标索引并调用FAF0发布四名角色差值。

本函数没有`sub_485610`调用，不播放样本`0x00BF`。这与E0C0分页前进及DF00/DFF0单步移动不同，不能补出方向对称声音。

## 4. typed-stop与验证

可见head推进、B9C0选择、共享文本、D6B0及FAF0只在原读取/调用点停止。短链停止时保留已经发布的`window-13`和null可见head。

UT覆盖：非零cursor只归零且零helper；从window13退到0、重计13条并刷新文字/帧/属性且无样本；window5减13后把window和cursor归零并刷新首页；短链保留window2和null head后停止。

workpack双生成稳定为`208/227`，SHA256为`63413dd4ecc3258817e98a488e4f0599adb339479d0bb7fbbd538069c3d03ab0`。`0x0044DBC0`继续等待callee闭环；下一单元为`0x0044E260`。
