# 剧情 VM 头像动作键改写 `0x0042A673`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042A673..0x0042A6CA`，共享尾`0x0042A2AC..0x0042A2C1`

opcode：`86`

## 1. 节点与operand

原版从固定`dword_4BA6E0`取得首个0xB4头像动作节点，并同时以`unk_4BA630`作为哨兵游标。每轮两个游标均沿`+0xB0`推进；业务比较/写入只使用live节点：

```text
node +0x00  action_id dword
node +0x08  base_variant dword
node +0xB0  next pointer

script +2  old action id u16 -> zero-extended dword
script +4  old variant u16 -> zero-extended dword
script +6  new action id u16 -> zero-extended dword
script +8  new variant u16 -> zero-extended dword
```

modern `LegacyRoleHeadActionList`保持线性live节点顺序；typed list owner替代固定裸全局/哨兵地址。

## 2. 精确访问顺序

LST顺序不是整条10-byte记录预读：

1. 先访问全局list head；空链直接进入`+10`共享尾，不读任何operand；
2. 非空链才读`old action id`；每个节点以完整32位`action_id`和零扩展u16比较；
3. 只有ID命中才读`old variant`，并以完整32位`base_variant`比较；
4. 只有首个ID/variant exact match才读`new action id`并立即写node `+0x00`；
5. 写完new ID后才读`new variant`并写node `+0x08`；
6. 首次exact match后停止，不改后续duplicate。

因此现代bounded failure保留原unsafe点：

- 空链或ID全miss即使后续operand不存在，也静默消费10 bytes；
- ID命中但`+4`缺失，在任何写入前typed-stop；
- exact match但`+6`缺失，在任何写入前typed-stop；
- `+6`存在而`+8`缺失时，new action ID已经写入，variant仍保留旧值，然后typed-stop；
- typed owner缺失在原list-head访问点停止，且不读operand。

## 3. 推进与common join

命中、variant miss、ID miss和空链都进入`loc_42A2B0`：script pointer和16位IP加10，`ESI=1`，再到`loc_42B0AE`发布normalized previous86并回解释循环，同一次`step`继续执行下一条指令。合法记录恰好结束在`0x8000`时，改写、IP和previous先完成，下一次fetch才报告`instruction_out_of_range`。

## 4. 资产锁

线性TALK目录含34条物理记录/34 probes：

```text
TALK1.DAT 4
TALK2.DAT 22
TALK3.DAT 5
TALK4.DAT 3
```

全部raw `0x0056`、长度10。资产只观察到两个action ID：`10001` 24条、`10002` 10条；34条均保持action ID不变而改写variant，旧variant覆盖`0..41`，新variant覆盖`0..54`。这是资产观察，不缩小handler支持new action ID变化的机器语义。

real CTest独立回放：

- `TALK1.DAT@0x0001CC18`：`10002/18 -> 10002/24`；
- `TALK4.DAT@0x0002BA21`：`10001/22 -> 10001/54`。

两条均置于精确窗口尾，在同ID variant-miss、首个exact、duplicate三节点链上验证只改首个exact，随后IP=`0x8000`、previous86完成并由下一fetch停止。

synthetic覆盖四raw alias、new action ID实际变化、same-call下一指令、完整32位node key比较、空链、ID miss、三段operand截断、new-ID已写的部分副作用和typed owner缺失。剧情VM三项为3/3。

分类：`platform_adapted`。合法owner域的节点顺序、32位比较、u16零扩展、首匹配、staged访问、部分写入、IP、previous和same-call均按汇编保持；仅固定裸全局/哨兵与越界访问由typed list owner和确定性失败替代。
