# 剧情 VM 随机目标重载 `0x0042A6CB`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042A6CB..0x0042A722`；RNG `0x00439020..0x004390E0`；重载helper `0x0042E430..0x0042E47B`

opcode：`87`

## 1. 目标表扫描

handler从`ip+2`开始按unaligned little-endian u32扫描，终止值固定为`0xFF00FF00`。它只累计sentinel前的目标数；扫描游标最终停在sentinel，但随机选择仍从原script base的`+2 + index*4`读取目标。

原版扫描没有窗口边界。modern逐dword检查；sentinel缺失返回`operand_out_of_range`，不调用RNG、不重载窗口、不修改IP/previous。

## 2. `sub_439070`随机算法

`sub_439070(count)`先执行unsigned：

```text
acceptance_limit = (0xFFFF / count) * count
```

每次尝试严格消耗两个secondary RNG raw值：

1. 调`sub_439020`推进一次147/103-word XOR状态并丢弃结果；
2. 内联同一XOR步骤取得candidate；
3. candidate >= acceptance_limit则重新尝试；
4. 接受后返回`candidate % count`。

modern复用已经assembly-exact测试的`LegacySecondaryRng::next_bounded`，保持0xFFFF阈值、two-raw-per-attempt、拒绝次数、250-word index回绕和u32状态。

count为0时原版在任何RNG状态访问前执行`DIV 0`。modern以明确`random_target_divide_by_zero` typed-stop隔离CPU fault，不伪造index/目标；RNG index、IP、previous和load/audio副作用均保持未发生。

## 3. 选中目标与窗口重载

非空表选中target后调用`sub_42E430(global_talk_file, target)`：

1. `_AIL_serve()`；
2. 当前TALK context data offset写target、instruction offset清0；
3. 文件游标seek到`target + 0x200`；
4. 读取0x8000-byte窗口；
5. handler把script pointer替换为新窗口base；
6. `ESI=1`进入common join，发布normalized previous87并在同一次解释调用继续新窗口首指令。

modern复用`load_same_file_story_window`，保持audio、当前file number、target publication、IP0、0x8000窗口和same-call。typed I/O失败时保留已经发生的RNG、audio、target/IP0与previous87，标记window未加载并返回`load_failed`。

RNG typed owner只在完整非空表扫描后访问；owner缺失不提前覆盖sentinel缺失或空表DIV语义。

## 4. 资产锁与测试

线性TALK目录含5条物理记录/5 probes：

```text
TALK1.DAT 3
TALK2.DAT 1
TALK3.DAT 0
TALK4.DAT 1
```

全部raw `0x0057`。四条长度18、各含3个target；一条长度30、含6个target。没有空表资产记录。

固定seed `0x12345678`时首轮丢弃raw `0xA606`，candidate为`0xE086`：

- `TALK1.DAT@0x00028B22`三目标表选index1，target `0x00028995`；
- `TALK4.DAT@0x0002FE09`六目标表选index4，target `0x0002FEB3`。

两条real记录均置于精确窗口尾，验证表解析、RNG index推进2、当前file number、audio、target/IP0 publication、previous87与same-call进入新窗口。

synthetic覆盖四raw alias、精确尾、三目标选择、空表DIV typed-stop、缺sentinel、RNG owner缺失和load失败已提交副作用。secondary RNG模块既有UT另锁定raw stream、two-raw bounded序列、rejection与index wrap。剧情VM三项为3/3。

分类：`platform_adapted`。合法域的表扫描、secondary RNG调用序列、目标读取、audio、窗口替换、previous与same-call保持；无界扫描、CPU DIV fault、裸全局RNG/file owner和unchecked I/O由bounded/typed失败隔离。
