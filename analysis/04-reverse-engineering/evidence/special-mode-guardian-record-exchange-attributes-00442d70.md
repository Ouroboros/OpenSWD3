# 护驾记录交换属性调整 `0x00442D70`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00442D70..0x00442E3F`，88行，无FUNCTION CHUNK；唯一caller为41160，现已直接回收。唯一callee 44D6E0继续由已存在的typed名称合并端口隔离。

## 精确顺序

输入arg0为将装备的新记录，arg4为当前slot旧记录。

1. 将共享scratch前`0x38`字节清零。
2. 读取旧记录`text_index`；若非`FFDC`，以记录`+0x0C`名称调用44D6E0合并。
3. 以`party_selector.low16 * 0x38`定位party record。
4. 将scratch `+0x0A/+0x0C/+0x0E`三个u16依次从party record `+0x1A/+0x1C/+0x1E`扣除。
5. 再次将scratch前`0x38`字节清零。
6. 读取新记录`text_index`；若非`FFDC`，合并其名称。
7. 将同三个scratch u16依次加回party record三项。
8. 返回`party_index * 0x70`，即LST末尾`(party_index * 0x38) * 2`的EAX。

三个加减均保留x86 word回绕。`FFDC`只跳过对应合并；由于scratch已经清零，该侧三项贡献为0。

## typed边界

- old record裸pointer为null：第一次scratch清零后，在原`[arg_4+4]`读取点停止。
- 旧名称合并失败：保留第一次清零，尚未扣除party三项。
- party low16越界：在原party第一项读取/写入前停止，保留旧名称合并后的scratch。
- 新名称合并失败：旧三项已完整扣除、第二次scratch清零已完成，不执行加回。

41160先通过已存在的A40同源party-record resolver读取当前slot旧记录，再直接调用D70 helper；随后才进入剩余0xB0存储交换typed边界。resolver不可用在原party表读取点停止。caller helper计数因此增加resolver与D70两项，D70不再隐藏在storage exchange端口内。

modern state新增四party的三个u16交换属性总量，只表达原party record `+1A/+1C/+1E`，不复制整张裸表。

UT以old贡献`5/7/9`、new贡献`2/3/4`验证`100/200/300 -> 97/196/295`及返回`0x70`；覆盖old null、party越界、old/new merge停止、双FFDC，并在41160正常交换路径验证同一差值及resolver→D70→storage exchange顺序。定向测试通过。

workpack双生成稳定为`95/227`，SHA256均为`847a94e44959f73b54235f020b8d4897874a82108b8b045b6a60a6ee38ec664b`；下一单元`0x00442E40`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
