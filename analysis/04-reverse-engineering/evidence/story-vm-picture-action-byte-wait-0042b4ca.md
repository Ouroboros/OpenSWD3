# 剧情 VM 图片动作首节点字节等待 `0x0042B4CA`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`asset_absence_verified`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042B4CA..0x0042B50A`

opcodes：`106`（主图片动作链）、`154`（副图片动作链）

## 1. 共享入口与访问顺序

机器先比较normalized opcode是否为154，再从`+2`读取完整u16 threshold。随后选择父对象：106使用`unk_4B7BD0`，154使用`unk_4B88C8`。两者的`+0xA0`均为图片动作链头：

- 链头为空：不读取节点，直接完成；
- 链头非空：读取首节点`+0x49`的u8，零扩展后与threshold作无符号比较。

`LegacyPictureActionNode::action`位于`+0x08`，`LegacyActionRecord::packed_ap_state`位于action `+0x40`，因此节点`+0x49`精确对应packed word高字节。modern从selected `std::list`首节点读取`packed_ap_state >> 8`，没有把相邻字段或后续节点纳入比较。

## 2. 等待与完成合同

```text
selected head != null && u8(head+0x49) <= u16 threshold
    -> IP不推进，ESI=0，common previous=normalized opcode，yield

selected head == null || u8(head+0x49) > u16 threshold
    -> IP += 4，ESI=1，common previous=normalized opcode，same-call
```

比较严格要求byte大于threshold。链持续非空时，threshold `>=255`永远不能完成。等待路每次都重读当前首节点及其高字节；不缓存previous值，也不读取另一条链。

modern复用已集成`LegacyPictureActionLists` owner。缺少runtime binding时，在threshold读取之后、原父对象访问点typed-stop为`runtime_unavailable`；空链仍按机器合同直接完成。

完整四字节记录起于窗口`0x7FFC`合法：完成路先提交IP=`0x8000`和previous106/154，下一same-call fetch再返回`instruction_out_of_range`。

## 3. 资产锁与验证

opcode106在线性TALK目录中有60条物理记录/63 probes，全部raw `0x006A`、长度4，分布：

```text
TALK1/2/3/4 = 21/8/14/17
```

threshold共25种，范围`2..110`。真实回放代表：

```text
TALK1.DAT@0x0001C0C9  threshold 9
TALK2.DAT@0x000101E7  threshold 3
TALK3.DAT@0x00016EF6  threshold 25
TALK4.DAT@0x0000258A  threshold 110
```

opcode154无任何线性TALK记录，使用`asset_absence_verified`，不把raw word扫描结果冒充资产入口。

synthetic覆盖两个变体各四个raw alias、主/副链选择、相等等待、严格大于完成、空selected链、threshold 256、operand先于runtime owner、缺runtime typed-stop和精确尾；opcode106四库真实记录另作回放。Story VM synthetic、real及initial-session三项通过。未启动原版或OpenSWD3游戏EXE。

分类：`platform_adapted`。有效链的选择、首节点偏移、u8/u16位宽、比较、推进、previous和same-call/yield逐项一致；仅以typed list owner替代裸父对象/链指针，并在缺binding时收敛原访问点。
