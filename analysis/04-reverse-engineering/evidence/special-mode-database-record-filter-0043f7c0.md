# 标准模式数据库record筛选 `0x0043F7C0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。范围`0x0043F7C0..0x0043F85B`，97行；caller为E3D0三处、F0D0一处、F940一处，无内部sub callee。

函数先执行一次返回值完全未使用的`lstrcmpA(record+0xC, 4A6D10)`；typed端记录文本已由有界owner承载，因此不复制该无输出平台读。随后：

1. category仅允许0..9或15..19。
2. flags严格与`0x0FFF7FFF`后再匹配，不是按bit包含。
3. page0仅接受`1/2/4/8/10h/1000h`。
4. page1仅接受`100h/200h/400h`。
5. page2仅接受`800h`；其他page一律false。

新增纯typed谓词。Forward node补充filter flags/category owner；F0D0逐节点直接调用，不再使用select port。E3D0三份inline/runtime record直接从`+2C/+5E`调用，F7C0不再被错误建模为可改写text ID的resolver。F940尚未关闭，caller接入留待其工作包。

UT覆盖mask清bit15、category两段边界、三page全部组和非法flag/category；F0D0及E3D0旧fake resolver事件按真实不改ID语义更新。

定向测试通过。workpack双生成稳定为`63/227`，SHA256均为`f246813ecc2afd0d632f51e48fcbc22130bdfac8150c36db8d772fff71ddcbc3`；下一单元`0x0043F880`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
