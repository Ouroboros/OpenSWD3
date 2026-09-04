# 汇总装备和状态带来的角色属性加成 `0x0044AB00`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x0044AB00..0x0044AE61`，336行、2个物理callsite，无FUNCTION CHUNK。直接caller为49FF0、44A0D0、44A160、44A1D0；callee为44D6E0与477B40。

重建顺序严格如下：

1. 先把56字节second record清零。
2. 按mode从56字节表复制完整first base record，再把offset26/28两个bonus word清零。
3. 对当前mode的16个贡献记录逐项直接调用已关闭的44D6E0，second record前42字节映射为21个u16目标；贡献记录携带typed模板键、门限、模式、三项资源、六项战斗属性和两个附加值。
4. 把second offset4..1E的14个u16和offset26/28的2个u16，以u16回绕加到first；不回加primary、offset20..24、offset2A、level或modifier区。
5. 对贡献0..6，按byte回绕把offset9E..9A九个signed byte直接加到first offset2D..35。
6. 对贡献7..8，仅kind精确为33时以lookup key直接组合已关闭`0x00477B40`：先扫描固定根`0x004B8A00`，再读取并清理MON定义，以命中记录count作value、definition maximum作divisor。计算严格保持原两级整数除法：`factor=(-1000*count)/maximum/100`，每个非零source先按8位NEG取得magnitude，再加`factor*magnitude/10`的低字节。source=-128时NEG仍为-128，不修正为128。
7. 完成时固定返回9。

物理两个callsite在循环中最多实际执行18次：44D6E0固定16次，477B40最多2次。旧`accumulate_character_attributes_record`行为端口已删除；临时属性端口只隔离尚未关闭的476A80/4885A0资源边界。caller只按自身LST把整个44AB00计为一次调用。

停止边界：second不可用时在入口memset点停止；mode越界发生在second已清零后；first不可用发生在mode读取后；贡献不可用只在16次44D6E0和16个u16回加完成后的原直接读取点停止；477B40链、MON或输出访问typed-stop保留此前全部聚合与modifier副作用；maximum为0只在477B40成功返回、准备原idiv时停止。所有此前副作用均保留。

49FF0初始化、44A0D0前进、44A160后退及44A1D0显式回绕入口均删除opaque重建端口，直接调用typed helper。重建停止时向caller传播`rebuild_stopped`，不播放后续107提示音。

UT覆盖完整56字节base字段、16个owner顺序、16个u16回加、base bonus清零、前7组modifier、type33真实MON definition/固定根查询、正负及-128缩放、固定返回9；另覆盖477B40 typed-stop、maximum零、贡献不可用、mode越界在second清零后停止及second不可用保持原记录。既有模式切换与44A050循环UT已改走真实重建。

workpack双生成稳定为`165/227`，SHA256均为`c7826ea95008f54d3c5bf2a78d42be518aeb700a4df7d257029ec651aae59fdf`；下一单元`0x0044AE70`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
