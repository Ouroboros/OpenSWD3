# 剧情 VM 文本布局参数 `0x0042B47E`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042B47E..0x0042B4B4`

opcode：`104`

## 1. 精确阶段顺序

机器先设置same-call标志并执行：

```text
text_control_flags &= 0xEFFFFFFF
text_layout_first  = sign_extend(s16(+2))
text_layout_second = sign_extend(s16(+4))
IP += 6
common join publishes previous104
```

bit28清除发生在任何operand读取之前。第一项读取后立即符号扩展并写入i32 owner，第二项随后才读取；两个值都不做范围、坐标或枚举检查。

旧modern case错误地一次性预检完整六字节，掩盖了两个原始unsafe访问点之前已提交的副作用；同时成功路径未发布previous104。本次改为分阶段typed-stop：

- 只剩opcode时，bit28已清，两个布局值保持旧值；
- 有`+2`但缺`+4`时，bit28和第一项已提交，第二项保持旧值；
- 完整记录成功时，两项、IP与previous均提交并same-call。

## 2. 精确尾与消费链

完整六字节记录起于窗口`0x7FFA`合法：机器先完成bit清除、两项写入、IP=`0x8000`和previous104，下一same-call fetch才失败。modern回归固定相同顺序。

后续对话创建在bit28清除时直接消费这对signed布局值，并在消息入队后把控制flags与布局pair恢复默认；既有dialog integration回归继续锁定该消费/复位链。

## 3. 资产锁与验证

线性TALK目录锁定125条物理记录/125 probes，全部raw `0x0068`、长度6，分布：

```text
TALK1/2/3/4 = 46/48/18/13
```

共有31种operand pair；第一项signed范围`-80..52`，第二项范围`-120..-20`。真实回放代表：

```text
TALK1.DAT@0x000054AD  0, -96
TALK2.DAT@0x0000CB89  0, -30
TALK3.DAT@0x00009DDF  20, -75
TALK4.DAT@0x00002273  20, -75
```

synthetic覆盖四个raw alias、i16最小/最大、bit28单独清除、第一/第二operand分阶段缺失、已提交副作用和精确尾。Story VM synthetic、real及initial-session三项通过。未启动原版或OpenSWD3游戏EXE。

分类：`platform_adapted`。有效记录的bit运算、signed扩展、写入顺序、推进、previous与same-call逐项一致；仅把两个原始越界读取收敛为各自访问点的typed状态，并保留此前副作用。
