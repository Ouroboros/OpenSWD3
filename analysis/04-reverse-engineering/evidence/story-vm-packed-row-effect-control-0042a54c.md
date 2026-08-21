# 剧情 VM framebuffer 区域效果控制 `0x0042A54C`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原程序陈旧局部动态值仍为`blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042A54C..0x0042A60C`，共享尾`0x00429F61..0x00429F76`

opcode：`84`

## 1. ID、查找与staged读取

handler先读`+2 u16 id`。id `>=256`只诊断并+6；不访问效果链、不读`+4`。合法id访问`dword_4BAB9C`，按链顺序寻找首个`(node.mode & 0xFF)==id`节点。

- 空链或所有ID miss：完全不读`+4`，静默+6；
- 首次ID命中：此时才读`+4 u16 operation`；
- 重复ID：只控制第一个节点。

四raw alias精确尾、空链、ID miss、operation截断与owner缺失测试锁定该顺序。现代固定全局head映射为nullable`runtime.packed_row_effects`，owner缺失在合法ID的原始list-head访问点typed-stop。

## 2. operation 0/1/2

命中节点后：

| operation | 原始行为 |
| ---: | --- |
| 0 | 丢弃旧高mode，仅保留低8位ID并OR `0x2000` |
| 1 | 丢弃旧高mode，仅保留低8位ID并OR `0x1000` |
| 2 | 从链中摘除首匹配节点，按row_offsets、row_lengths、节点顺序释放 |

所有合法操作完成后+6、common join发布normalized previous84并same-call继续。std::list erase/vector析构替代裸链与三次free，保持可见顺序和首匹配语义。

## 3. 非法operation与陈旧`var_44`

operation不为0/1/2时，原版没有诊断或默认值：0和1分支均不写`var_44`，随后`loc_42A5F9`把该陈旧栈局部OR进低8位ID并写回mode。其值可能来自同一次`sub_427920`调用中更早handler，也可能来自重用栈内容；单独从本handler和记录无法确定。

现代不把邻近分支值或零伪装成原始结果，返回`unsupported_packed_row_effect_operation`，不改节点、IP或previous。这是确定性typed适配；原程序具体陈旧值的动态差分仍受runtime oracle阻塞。

线性资产确实包含6条此类记录：

```text
TALK1.DAT 0x00053AD3  id2 op3
TALK2.DAT 0x0000FFB3  id2 op3
TALK2.DAT 0x0001AAC7  id2 op8
TALK3.DAT 0x000161EB  id2 op3
TALK3.DAT 0x00019341  id2 op8
TALK4.DAT 0x00018996  id2 op8
```

real CTest回放TALK1 op3记录并固定typed-stop，不宣称原版陈旧mode值已知。

## 4. 资产锁与测试

线性TALK目录含1879条物理记录/1879 probes：

```text
TALK1.DAT 485
TALK2.DAT 337
TALK3.DAT 396
TALK4.DAT 661
```

全部raw `0x0054`、长度6、id<256。operation分布：0为910条、1为963条、3为3条、8为3条；没有operation2真实记录。

real CTest还回放`TALK1.DAT@0x00006147`的op0和`@0x00009697`的op1，验证`0x2005/0x1001`写回、+6、previous与精确尾。synthetic覆盖operation2首匹配释放、重复ID、same-call、四alias和全部staged失败路径。剧情VM三项为3/3。

分类：`platform_adapted`。合法operation 0/1/2的查找、位写、释放、IP、previous与same-call保持；固定全局owner改为typed list，非法陈旧局部路径改为确定性失败。
