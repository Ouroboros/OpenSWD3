# 按控制位解析特殊模式打包值 `0x0044FF90`

状态：`assembly_exact`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0044FF90..0x0044FFB7`，28行，无callee、无外部FUNCTION CHUNK。三个caller都在待审计的`0x0044F800`。

输入为完整u32，按顺序覆盖返回：

1. 默认值为`input << 16`的低32位，即输入低16移到高16。
2. 输入bit8置位时改为`1`。
3. 输入bit9置位时改为`input >> 24`，覆盖bit8结果。
4. 输入bit10置位时改为`0x800`，覆盖此前所有结果。

判断使用`CH`低三位，与u32 bit8/9/10完全等价。多个控制位同时存在时优先级严格为bit10 > bit9 > bit8 > 默认。

## 2. 验证

UT覆盖无控制位、仅bit8、仅bit9、bit8+bit9和bit8+bit9+bit10，验证32位移位截断及逐级覆盖。

workpack双生成稳定为`196/227`，SHA256为`1dcfc415aa074ae00fef71aa11c274af641a48b67bfea6e5ff302e03f7a00aca`。当前主线仍为`0x0044DBC0`依赖闭环；本helper关闭后，下一依赖叶子为`0x0044F770`。
