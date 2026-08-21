# 剧情 VM 姓名记录装载 `0x0042B287`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042B287..0x0042B3AB`；字符串helper `0x0040BAA0..0x0040BB10`；公共退出 `0x0042D182..0x0042D193`

共享opcode：`91`、`162`

## 1. 两个入口的索引来源

opcode91先读取`+2 u16 record index`；值`0xFFF0`时替换为当前TALK context `+0x24`的u16 source GUID。索引0合法，不执行额外检查。

opcode162读取`+2 u16 variable selector`：

- 只接受11或12；
- 从64项`dword_4ACBD0`脚本变量表取得完整u32 record index；
- selector非法或变量值0时，原版只经debug `wsprintfA/nullsub_1`，随后固定消费4字节、发布previous162并same-call继续，姓名和MAPS均不访问；
- 非零值进入与opcode91相同的copy块。

modern复用`LegacyWorldStoryVmState::script_variables`，没有把动态u32索引截成u16。四个TALK资产中的opcode162 operand均为11；selector12、非法值和zero路径由synthetic锁定。

## 2. MAPS相对目录与32位回绕

共享copy块执行：

```text
base = dword_4C9A10
relative_table = u32(base + 0x20)
entry = relative_table + record_index * 4       // u32 wrap
record = u32(base + entry)
source = base + record
copy 32 bytes to word_4CF6B8
```

modern对`record_index * 4 + table_offset`显式保持u32回绕，再转为bounded offset。opcode162以`record_index=FFFFFFFF`、`table_offset=44`回绕到entry40的synthetic通过。目录项或记录越界在原始unsafe访问点typed-stop；记录copy尚未发生时speaker buffer保持。

## 3. `%Q`终止与姓名替换

32-byte copy完成后，原版从`word_4CF6B8`逐字节执行unaligned u16扫描，首个`25 51`（`%Q`）的`%`改为NUL；扫描无边界。modern只在32-byte buffer内扫描；缺terminator以`name_terminator_not_found`明确停止，但保留完整32-byte copy，不推进IP、不发布previous。

随后原版调用`sub_40BAA0`两次：

1. 若speaker以`C1 C9 AF 53`开头，以当前first-name C string替换；
2. 若speaker以`A9 67 A5 69`开头，以当前second-name C string替换。

匹配时helper按固定32-byte范围执行`memmove(dest + replacement_len, dest + 4, 32 - replacement_len)`，再复制replacement，返回值被忽略。modern在合法`replacement_len>=4`域保持完整32-byte搬移，包括NUL后的bytes；当前默认姓名都是4 bytes。更短姓名会使原版memmove读取speaker buffer尾后相邻全局，modern使用bounded C-string fallback，不伪造相邻裸内存。

成功路径固定IP+4、发布normalized previous91/162并same-call继续。完整记录恰好结束在`0x8000`时，copy、替换、IP和previous先完成，下一fetch再返回越界。

## 4. 资产锁与测试

线性TALK目录合计1184条物理记录/1203 probes：

```text
opcode91: 1180 records / 1199 probes
  TALK1/2/3/4 = 388/239/295/258
  raw 0x005B，operand 1..7023；另有33条0xFFF0当前source selector
opcode162: 4 records / 4 probes
  TALK1/2/3/4 = 3/1/0/0
  raw 0x00A2，四条operand均为11
```

real CTest在初始化resource DB前预读TALK记录，避免Windows共享模式影响独立资产锁：

- `TALK1.DAT@0x000364EB`：opcode91显式record index782；
- `TALK1.DAT@0x000364C9`：opcode162 selector11，fixture变量11设为782。

两条均置于精确窗口尾并使用真实MAPS payload，最终得到完全相同的32-byte speaker结果；IP=`0x8000`、previous和next-fetch边界通过。

synthetic覆盖91/162四raw alias、FFF0→source且record0合法、selector11/12、非法selector、zero dynamic index、u32目录回绕、目录越界、copy后缺terminator、first/second prefix替换、staged publication和精确尾。Linux Story VM三项为3/3。

分类：`platform_adapted`。合法域的入口分流、索引位宽、目录回绕、32-byte copy、首个terminator、固定buffer替换、IP、previous和same-call保持；无界MAPS/terminator访问、短replacement跨全局读取及debug-only格式化由bounded/typed行为替代。
