# 剧情 VM 自定义 ANI 相位等待 `0x0042AD75`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`external_dependency_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

handler入口：`0x0042AD75..0x0042AD8D`

完成路径：`0x0042D182..0x0042D193`

opcode：`99`

## 1. signed严格阈值协议

机器先读取signed dword `dword_4B7AC8`，再零扩展读取`u16(+2)` threshold，随后执行signed比较：

- `phase <= threshold`：IP不推进，`ESI`保持0；进入common join发布previous99并yield；
- `phase > threshold`：物理脚本指针和u16 IP各`+4`，`ESI=1`；进入common join发布previous99并在同一VM调用继续。

比较是严格`>`而不是`>=`，threshold保持完整u16零扩展。phase为`-13..0`的ANI reveal启动区因此小于或等于所有u16 threshold。handler不读取active extent、file、flags或错误状态；activity未启动/已结束时phase为0，threshold非负仍按原版等待，modern不得增加active检查。

原访问顺序是phase owner读取先于threshold读取。modern port先查询typed phase，再在原operand访问点检查四字节记录；threshold缺失时返回`operand_out_of_range`，不推进IP、不发布previous，但phase查询已经发生。

## 2. typed owner与SDL接线

原裸全局由既有`LegacyAniActivityState::phase`承载：

- ANI start按flags写入1或`-13`；
- reveal/playback/ending逐帧按原状态机递增；
- close/finalize写回0。

SDL Story VM port直接返回该live i32字段，不复制、不归一化、不夹限，也不引入nullable owner或额外失败分支。world-frame ANI stage已由opcode97包接入actual activity update，因此phase可真实推进并使opcode99完成。

## 3. 窗口边界与资产锁

完成记录起于`0x7FFC`时，handler先提交IP=`0x8000`和previous99，再由same-call下一fetch返回`instruction_out_of_range`；此前副作用不回滚。等待路径在同一位置保持IP=`0x7FFC`并yield。synthetic覆盖四个raw alias、等值等待、大于完成、signed `-13`、完整u16 `0xFFFF/0x10000`边界、threshold缺失访问顺序和精确尾。

线性TALK目录锁定139条物理记录/139 probes，全部raw `0x0063`、长度4，分布：

```text
TALK1/2/3/4 = 0/44/64/31
```

threshold范围1..350，共111个不同值。真实回放代表：

```text
TALK2.DAT@0x0000D3AE  threshold 1
TALK2.DAT@0x00017089  threshold 350
TALK3.DAT@0x0000898A  threshold 30
TALK4.DAT@0x0001484C  threshold 11
```

每条先以phase等于threshold验证原地等待，再以phase大一验证精确尾完成。Story VM synthetic、real及initial-session三项共同通过；依赖门覆盖`LegacyAniActivity`的start/update/finalize phase状态机。未启动原版或OpenSWD3游戏EXE。

分类：`platform_adapted`。signed比较、严格阈值、读取顺序、IP、previous及yield/same-call协议保持；Win32裸全局映射为已集成typed activity live owner。
