# 战斗角色显示种类查询（0x004786C0）

状态：`pending`。

## 权威范围

完整LST范围`004786C0..004786C7`，半开终点`004786C8`，8字节：

```text
004786C0 66 8B 81 70 2A 00 00  mov ax,[ecx+2A70h]
004786C7 C3                    retn
```

两条指令，无分支、调用、写入或外部chunk。读取完整WORD，仅替换AX；
高EAX、ECX、EDX和flags保持。唯一物理CALL为`00453A0E→00453A13`。
独立推导保存在`build/tmp/runtime/display-kind-direct-lst-analysis.md`。

## 实际字段与接口

查询借用实际`const u16*`，返回状态和`optional<u16> value`。
唯一调用直接借`startup.party[index].item_effect_application.display_kind`。
队员范围由父级原入口检查保证，party固定十项；没有额外索引检查或新停点。
原startup可空合同继续在原字段读取边界报告失败。
敌方已有字段`group_b_lifecycle[index].action_composition.display_kind`
也可直接借用，独立测试验证两类实际字段不共用缓存；生产调用域仍为队员。

删除本查询地址常量、token解析、owner/view、寄存器request/reply、
模拟栈、字段地址与访问/调用计数，不添加包装层。
调用方外层optional区分是否执行查询，内层optional区分是否读到WORD。
字段不可用或不可读时无值；RET不可访问时保留已读值。两者同时失败时
先报告字段读取失败。查询不写任何角色字段，也不把失败转换为计数零。

## 唯一调用的原始分支

1. `004539E6`读取动作种类；DI接AX，EDI经过FFFF掩码。
2. `004539F6`查询终态；完整EAX精确等于1时跳`00454538`并返回1。
3. `00453A08`测试EDI；非零跳`00453A33`，不执行显示种类查询。
4. 动作种类零且终态不等于1时，`00453A0E`查询显示WORD。
5. `00453A13`掩码EAX，`00453A18`保存栈局部；零值路径返回1。
6. 非零显示WORD跳`00453A42`，此时EDI仍为0；DEC得到FFFFFFFF，
   与23h比较后无符号JA到`00455CBB`默认尾。`00455CC2`清EAX，返回0。

**显示WORD非零不会成为新的动作编号。** 旧C++把显示值赋给动作编号后
执行case，旧测试也误要求“动作0、显示5进入case5”；两者与LST冲突，
本批改为原默认尾返回0，不执行case5、case99或扩展动作。
结果`action_code`保留真实动作种类0；显示值由查询结果独立记录。
局部栈槽没有宿主可观察消费者，不再复制成第二套动作状态。

字段/RET失败保留前置动作查询、终态调用及其实际副作用，并抑制正常尾。
父级仍暴露的失败返回EAX在调用点维护：字段失败保留terminal完整EAX，
RET失败仅用观察WORD替换其低16位。查询本身不再传递寄存器。
终态回调改写显示字段后，唯一查询必须读到当时实际值。

## 独立测试与验证

- WORD 0/1/00FF/0100/7FFF/8000/FFFF，邻居不变、实际字段分离与实时重读。
- 字段缺失、不可读、返回不可访问与失败先后顺序。
- 十种显示WORD乘四种非1终态值；显示零返回1，其余返回0，只有原终态调用。
- 显示1/5/63h/199h不可触发对应动作；0100和8000不能按低byte或符号误判。
- 终态精确1与动作种类非零跳过查询，查询访问故障此时不生效。
- terminal回调改写后读取；字段/RET失败分别保留完整前驱和部分AX。
- 缺少startup在原字段边界失败，不伪造正常返回1。

最终实际门禁均通过：

- core setup 1/1，9.74秒；actor-frame 1/1，24.40秒。
- ASan setup 1/1，15.47秒；actor-frame 1/1，25.31秒。
- SDL `openswd3`实际编译链接，未启动游戏。

均使用仓库`build.sh`或`build-asan.sh`，分别指定setup和actor-frame
测试目标及精确正则。setup注册查询本体；主动作及新增向量注册于
actor-frame程序。最初遗漏调用方门禁由发布检查发现，随后补跑并通过，
未用setup记录代替调用方验证。

日志前缀`build/tmp/runtime/display-kind-direct-`，后缀为`core.log`、
`callers-core.log`、`asan.log`、`callers-asan.log`和`sdl.log`。
core/ASan setup各保留一条既存outcome测试133行u16→u8窄化warning；
无新增warning、编译错误、测试失败或sanitizer报告。
六份源码测试身份记录于同前缀`validated-source.sha256`，前后校验一致。
发布检查见同前缀`publication-check.log`，覆盖原两指令、唯一调用、
DEC/无符号JA默认尾、测试实际注册、旧协议消除及inventory边界。
最后双向复核未留下本切片内的未解释差异；此结论不扩展到父级其他操作。

## 验收边界

保持`platform_adapted`。原版完整内存别名、异常页/SEH和联合动态捕获
仍未取得，差分状态仍为`blocked_runtime_oracle`。
父级动作种类、终态、其余操作编号、参数包、寄存器协议、计数及资源
仍待迁移；本查询不提升整个动作链、B11或WP316/318的验收状态。
