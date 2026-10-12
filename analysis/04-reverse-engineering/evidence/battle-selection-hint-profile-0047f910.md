# 敌人信息框的实际参数读取 `0x0047F910`

状态：限定读取切片已迁移、`unit_tested`、`fixed_state_tested`。
实现提交：`634a082a`。完整信息框与其他父协议仍为迁移`pending`。

## 1. 权威范围与读取时点

独立LST分析先于当前C++审计。父函数为`00466950..00466BFE`，
从proc到endp共312个物理行、18个静态call和两个retn，没有外部函数块。
唯一物理caller为选择帧`00464BD9`；两个cdecl参数为提示框原点。
本处`00466AC8`属于所选敌人信息框，先前迁移记录中的“摘要读取”归属错误。

叶`0047F910..0047F917`只有`mov ax,[ecx+2A0Ch]`和`retn`。
它读取无符号WORD，不产生业务成功码，不修改ECX、EDX和FLAGS，保留EAX高WORD。
后继`00466AD3`的固定数量查询只消费该键低WORD，故不重建查询寄存器回复。

初始`004669AE/B4/BC`按signed判断选择在`1..敌人数`，
名称访问使用`005229E0 + code*2B28`，code1对应实际敌人0的`00525508`。
count9仍可让code9进入既有名称访问typed-stop；不增加入口八槽夹值。
参数读取之前依次完成名称、动作233B、背景矩形、图形、字体20、名称文字和字体16。
`00466AAA`在此之后重新读取共享选择；不得缓存初始槽或参数。

## 2. 实际字段、共享选择与数量状态

`LegacyBattleSelectionHintFrameBindings::published_actor_code`借用
唯一caller的`final_actor.published_actor_code`。名称快照仍保存在原结果中，
参数、后继数值pair和进度分别在`00466AAA`、`00466AF0`和`00466B7D`重读共享选择。

参数来自startup实际生命周期数组的`action_execution.profile_value`。
既有actor snapshot与startup reset双向映射均确认该u16对应`+2A0C`；
没有平行参数表、默认敌人或回显副本。记录定位消费原DWORD地址算术，
回绕后的实际地址映射到八槽对象；不能只按当前code字面值再次作入口上下界判断。
例如`20000001/80000001/E0000001`的DWORD地址均回绕到敌人0。

缺失共享存储、无法映射的地址或既有`profile_value_read_accessible=false`，
仅在该真实读取点复用`group_b_actor_typed_stop`。不增查询ordinal、新成功码或新停止状态。
这些失败保留已绘名称及恢复字体的完整前缀，阻断数量查询与信息后缀。

实际u16键直接查询`LegacyBattleFixedObjectState`拥有的根和动态节点。
命中返回数量WORD，缺键返回0；节点键或数量访问失败保留前缀并阻断后缀。
保留无符号门槛10和15；名称镜像仍要求完整值1，生命文字与渐变仍判断非零。
原父诊断由数量、实际匹配/停止节点与DX低WORD替换推导，不回显已删除源查询。

删除HintFrame与FrameCoordinator两处源查询枚举、转发case、调用计数和全部模拟回复。
没有把该查询改名为reserved或加入包装层。
其他名称/字体/文字、数值pair、颜色和完整父寄存器协议仍待迁移。
镜像等其他共享字段的读时点继续属于完整父复核范围。

## 3. 汇编推导的实际状态测试

新增93次帧执行：

- 56次：8槽乘7个WORD边界，含0、1、001C、7FFF、8000、BEEF、FFFF；邻居不可读且保持，真实根命中，不改变数量或角色存储。
- 18次：数量0/9/10/14/15/FFFF乘根、实际拥有节点与缺键，检验无符号门槛及后缀。
- 2次：名称绘制或字体恢复前继改写实际选择与参数，保留原名称，数量查询使用新槽新WORD。
- 6次：完整值入口门、零/负选择、signed敌人数边界，在无参数存储时仍按原门返回。
- 2次：真实节点键/数量访问不足，保留名称前缀与节点所有权。
- 5次：字体恢复后存储缺失、参数访问失败及0/9/FFFFFFFF无法映射选择，阻断全部数量后缀。
- 3次：DWORD选择地址回绕别名，读取实际敌人0，保留数量叶只改DX的高WORD诊断。
- 1次：字体恢复后选择7、数值pair前继再改选择3，原名称和已捕获数量保持，后续进度读取新槽。

原矩形和图形失败夹具改为无参数存储，证明此前失败不预读参数。
唯一选择帧caller夹具以实际敌人7的参数替代源查询回复。

## 4. 验证与失败记录

首轮`proc_db6a`的core仅三条回绕诊断断言失败：
固定数量叶只改DX，测试遗漏345乘选择值保留的EDX高WORD。
修正期待为2000BEEF/8000BEEF/E000BEEF；生产读取行为未因此改动。

最终`proc_32d9`成功：

- `./build.sh core --build-target openswd3_battle_legacy_battle_setup_tests --test --test-regex '^battle\.legacy_battle_setup$'`：1/1，9.97秒。
- `./build-asan.sh --build-target openswd3_battle_legacy_battle_setup_tests --test --test-regex '^battle\.legacy_battle_setup$'`：1/1，16.75秒。
- `./build.sh app --build-target openswd3`：实际重新链接SDL程序；未运行游戏或app UT。

五文件SHA-256一致，完整已审查与staged差异逐字节一致。
core/ASan有未修改的outcome-resolution及script-dispatch测试中既有转换warning；
本批文件无新增warning，SDL日志无warning/error，无sanitizer或运行期错误。
日志、身份和完整正反向记录位于`build/tmp/runtime/hint-enemy-profile-*`。

## 5. 剩余范围

六个物理47F910站点现均改读实际字段；生产与测试已无该编号及源查询模拟。
它们的脚本、行动、最终角色和信息框父链仍有其他通用调用协议。
定向UT与生产绑定静态检查不证明完整SDL战斗链、原版差分或全游戏验收；不标`assembly_exact`。
完整清单、相关父项与范围穷尽性保持未验收，不能按叶站点消失判定整体完成。
