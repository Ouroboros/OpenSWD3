# 标准模式数据库交互提交 `0x0043E3D0`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与边界

唯一行为真值为`swd3.exe.lst`。函数范围`0x0043E3D0..0x0043E741`，425行；DA30有四个direct call点，B480另以callback地址绑定。

已关闭callee B980/B9A0/BC90/BCC0/E770直接复用typed实现。F1E0、F7C0、FDE0、4404D0、4405C0、44D2D0、sample、物品查询和release继续保留最小可观察port边界，不提前计数。DA30命中E3D0时直接调用本helper并传播phase4 typed-stop。

新增/确认typed owner：

- FCA88/FCBA0：first/second runtime `0xB0` record。
- FCBA8/FCC58：first/second inline `0xB0` record，word+4由F7C0更新。
- FCC54/FCD04：first/second heap token。
- FC320：phase4 reset owner。
- 4AB7BC：phase1 comparison value，使用低8位。
- FB970/FB978：primary action ID/base variant。

## 2. phase 1

严格顺序为：

1. 查询物品`0x1BB0`；存在时立即直接调用已关闭E770并返回。
2. F1E0重建两个inline record；EAX为0时写phase5并返回0。
3. 调用4404D0后重新读取phase并加1。
4. primary action写`0x232A/0x39`，toggle和runtime flags清0。
5. 查询物品`0x1BA9`；缺失写toggle1。
6. 若物品存在，first runtime `u16(+0x60) - comparison.low8 > 9`时OR flags bit0并写toggle1。
7. second runtime同一差值大于9时无条件OR flags bit1。
8. sample `0x2E`返回EAX。

UT覆盖正常prepare、first/second差值分流、F1E0失败转phase5及物品1BB0直接E770。

## 3. phase 2/3

phase2仅两类组合拒绝并sample `0x8C`：toggle0且flags bit0置位，或toggle1且flags bit1置位。其他组合写phase3、countdown为`-40`，依次FDE0和sample `0x2E`。

phase3先调用4405C0。若更新后countdown小于`-35`，写countdown35及primary action `0x232A/0x46`；随后总是sample `0x2E`。UT锁定拒绝、未知toggle转移和`-36`阈值。

## 4. phase 4

1. 写FC320为0。
2. 对first inline调用F7C0；word+4非FFDC时按F7C0 EAX是否1选择shared/alternate destination，以`text,-1,0`调用44D2D0；随后条件release FCC54 token、token清0、word+4写FFDC。
3. 对second inline执行同一流程并处理FCD04 token。
4. toggle0选择first runtime，否则second runtime；F7C0后word+4非FFDC时以`text,1,2`调用44D2D0，并保留返回节点word+6自增语义于materialize边界。
5. 直接执行B980→B9A0→BC90→BCC0，重建完整count、current head、16界count、selected text和shared buffer。
6. 成功后写countdown0、toggle0、phase1及primary action `0x232A/0x3B`。

B9A0短链只在原空指针读取点typed-stop；BCC0各typed-stop同样保留此前materialize/release副作用且不发布最终复位。UT覆盖三记录分别shared/alternate/shared、两个token释放、missing index复位、FFDC内建文本、12次helper顺序与B9A0 typed-stop；DA30明确传播`database_commit_stopped`。

## 5. 其他phase与EAX

phase5和phase10写phase1，同时保留switch index EAX 4/9。phase6–9及越界phase不写owner并返回`phase-1`。phase4成功返回BCC0文本formatter EAX；FFDC内建文本为2。

## 6. 验证

定向测试通过。workpack双生成稳定为`55/227`，SHA256均为`9c4abc1d3061dbb831d8b66fb43355da754fb4f0c9b0732c7c779a52006b54b6`；下一单元为`0x0043E770`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
