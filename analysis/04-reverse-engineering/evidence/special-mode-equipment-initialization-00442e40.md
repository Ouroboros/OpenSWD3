# 装备物品模式初始化 `0x00442E40`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00442E40..0x00442F08`，90行，无FUNCTION CHUNK。无direct call caller；444FC0把该地址绑定到模式初始化callback表，待其独立关闭时回收。直接callee B9C0、B9E0、444E80、444F60与444FB0已关闭并由typed helper直接复用；487C10仅以固定`0x28` workspace allocation端口表达。

## 严格顺序

1. 若party selector low16为5，只清low16为0，保留high16。
2. 写文本资源word `0x2A`、selected party action 0、mode enabled 1、list kind 0。
3. 直接调用444E80记录列表初始化；callee重建record head并把list offset/local selection清0，后续必须重读。
4. 直接调用444F60：动作数写3，再按完整selector派生两项ID并按非零存在性递增。
5. active party count先清0，再按四个party record首word是否不等于`FFFF`计数。
6. 以调用后的`list_offset + local_selection`执行已关闭B9C0索引；null记录在B9E0原`[record+4]`读取前typed-stop。
7. 以记录text index直接调用已关闭B9E0。`FFDC`在空MAPS上写CP950“無”三字节；普通文本按B9E0自身边界停止。
8. 成功后才清两个render owner，写viewport extent `0x1E0`。
9. 申请固定`0x28` workspace；返回0也照原样发布，不擅自改成分配失败分支。
10. 直接调用无参数444FB0取得常量3；随后清final owner、发布动作数、写global mode `0x45`，并返回该EAX。

## typed边界

444E80按party池、filter表与缺省节点分配的typed边界停止；B9E0失败保留party count和已提交共享文本，不写后续render/workspace字段。444FB0停止时workspace token已发布，final zero、动作数和global mode保持调用前值。

modern state只表达E40实际读写的模式字段、四个party marker、typed forward head、128字节共享文本和workspace token，不映射裸全局地址。

UT令444E80从party池抽取二节点列表并清selection0、输入state携带四槽marker，再由444F60发布动作数，验证party `ABCD:0005 -> ABCD:0000`、active count2、FFDC“無”、0x28 allocation、最终3与global mode45。另覆盖记录池/缺省分配与B9E0停止，并直接验证444FB0恒定返回3。定向测试通过。

workpack双生成稳定为`96/227`，SHA256均为`0a0021e19a076bd684ef5063f2f9207fa8f84db8850c1fe18b9d0047fc25828c`；下一单元`0x00442F10`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
