# 护驾属性cache刷新编排 `0x004429B0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x004429B0..0x00442A36`，67行，无FUNCTION CHUNK。

callers共十一项：40630、407F0、40B20、40C20、40D20、40E10、40F00、40FB0、41060、41160、41590。全部已关闭owner现均直接调用typed helper；原`prepare_guardian_attribute_cache`及`invoke_guardian_selection/input(refresh_attribute_cache)`端口调用均已移除。结果中的last_target只作为原调用位置trace，不触发opaque callback。

## 七步顺序

1. `442AA0(0, cache+0x000)`。
2. `442AA0(1, cache+0x050)`。
3. `442AA0(2, cache+0x0A0)`。
4. `442AA0(3, cache+0x0F0)`。
5. `seed=442A40()`。
6. `442B10(u16 party_selector, guardian_slot, cache+0x140, seed)`。
7. `return 442CA0(seed, cache+0x140)`。

modern state固定持有`0x190`字节cache；四party destination以offset表达，不暴露分配pointer。四个尚待独立关闭的callee由强类型端口表达：party population、seed preparation、selected combination和summary finalization。seed、party low16、guardian slot与`0x140`destination均按原顺序传递。

每个callee边界可独立返回typed-stop。429B0只在对应调用返回时停止，保留此前party/seed/combine副作用；最终EAX来自442CA0。parent owner把整次429B0计为自身一个helper/callback，内部结果保留七步计数。

## caller回收

- 40630在第三次allocation成功和cache清零后直接刷新；失败则在选中记录文本及viewport常量前返回`attribute_cache_stopped`。
- 407F0列表行路径直接刷新，失败不再调用旧input callback。
- 40B20/C20/D20/E10、40F00/FB0、41060、41160、41590经共享selection适配器直接刷新；失败不执行其后的sample或后续生命周期步骤。
- 原selection/input target枚举值仅用于result trace，测试port target列表不再出现refresh callback。

UT覆盖成功七步的`0/0x50/0xA0/0xF0/0x140`参数、party low16、slot、seed及最终返回；分别覆盖party第三项、seed、combine、finalize停止点的3/5/6/7次已发生调用；并覆盖40630、selection owner与407F0的停止传播。定向测试通过。

workpack双生成稳定为`89/227`，SHA256均为`04f0f157ad2cefc3577b4b353e558a526662418829a4420d81a642b8034381d5`；下一单元`0x00442A40`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
