# 标准模式数据库输入分派 `0x0043DA30`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与owner

唯一行为真值为`swd3.exe.lst`。函数范围`0x0043DA30..0x0043DD1F`，365行；由B480 callback表间接绑定，没有direct call caller。直接callee中C090、DD20、DDF0、DED0、DFA0、E080和E170已关闭；E310/E3D0/E770与外部物品查询继续独立审计。

DA30读写D530共享owner：

- dword `FCD20`是交互phase，D530写1；不得与D530/D880写u16 2/1的独立`word_4FC900`生命周期phase合并。
- `FCAD8`是B980完整forward count；`FCB98`是BC90写出的16界count。
- `FCBA4/FCAB8/FCADC/FCAD4`分别是列表选择、方向选择、页选择与hover。
- `FCA8C/FCAAC`是交互toggle与runtime input flags。
- `FC4F0/F4/F8/FC`是两组动态strict-X边界。

未关闭callee用地址枚举port表达；已关闭DD20/DDF0/DFA0/DED0直接回接typed helper。port同时取得state与input引用，使DA30能保留callee后重读全局X、button和toggle的顺序。C090直接回接typed availability helper；物品`0x1BA9`查询保留最小port。

## 2. phase 1

入口无条件清hover。按钮仅取低2位用于主矩形：

- `x=51..72,y=9..115`：页=`floor((y-8)/18)+1`，即1..6，调用E080并返回。
- `x=81..463,y=3..187`：索引=`floor((x-80)/24)`；以signed比较对BC90的16界count，越界直接返回count；有效时列表选择=`index+1`并调用DDF0。
- `x=71..336,y=255..392`：方向1并调用E310。
- `x=71..336,y=466..602`：方向0并调用E310。
- `x=338..357,y=413..451`：先发布hover=1；有低2位按钮才调用E3D0。

完整forward count signed大于16时调用C090查询availability record15。可用且`y=199..213`时依次检查：

1. `x=77..89`调用DDF0。
2. 重读X；`455..467`调用DD20。
3. 重读X；落入第一组动态strict边界调用DFA0。
4. 再重读X；落入第二组动态strict边界调用DED0并返回。

原函数在前三个检查后重读X。闭环DD20/DDF0/DFA0/DED0均不改鼠标X，UT不通过伪port注入不存在的副作用，而以重叠strict动态边界分别锁定`x=80`的`DDF0→DFA0→DED0`与`x=460`的`DD20→DFA0→DED0`；X保持原值，三个callee均为直接typed helper。availability span不足16项时只在原C090读取点typed-stop。其余路径最后重读button，低位`0x0C`调用E770。

## 3. phase 2

上面板矩形为`x=41..414,y=13..291`。物品`0x1BA9`存在、runtime flag bit0清且button bit0置位时先调用E080。闭环E080成功路径会再次查询物品并清toggle，因此随后独立检查button bit1时真实路径进入E3D0；旧的double-E080高层假设不可达。toggle1仅使E080先sample107，toggle0则跳过sample。

下面板矩形为`x=41..414,y=333..611`。runtime flag bit1清且button bit0置位时先调用E170。闭环E170成功路径必把toggle写1，因此随后button bit1置位时真实路径进入E3D0；旧的double-E170高层假设不可达。toggle1跳过sample，其他值先sample107。

callee后会重读X/button/toggle，不缓存为高层事件。未命中时低位`0x0C`调用E770。

## 4. phases 3–5与返回

phase3/4在button低4位非零时调用E3D0；phase5同条件调用E770；其他phase不调用。typed结果记录callback次数、最后地址target及最后callee EAX；无callee路径保留控制流最后装入EAX的owner值。

## 5. 验证

定向UT覆盖页1..6上界、24像素索引及signed count边界、strict hover、C090 typed-stop、phase1两条三callee直连/重读路径、phase2 E080/E170真实toggle耦合、物品查询ID、phase3/4/5和未知phase。

`special_modes.legacy_initial_menu`定向测试通过。workpack双生成稳定为`46/227`，SHA256均为`53c46bdf47c9719b2ee102b907d48d820ec10bbf52731ebca6de7dfb53d9c184`；下一单元为`0x0043DD20`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
