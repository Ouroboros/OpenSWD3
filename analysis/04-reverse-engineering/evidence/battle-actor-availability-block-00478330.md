# 战斗角色可用性阻断值写入（`0x00478330`）

## 1. 范围与权威输入

- 函数：`sub_478330`。
- 权威输入：`swd3.exe_export_for_ai/swd3.exe.lst`。
- 完整主体：`0x00478330..0x0047833A`，从`proc near`到`endp`共10个物理行、3条实际指令、0个call、0个跳转、0个外部chunk、1个返回点。
- 静态交叉引用：24处物理call，分布于8个已关闭caller；无组B对象caller。
- 相关字段交叉引用：`actor+0x2AE4`除本函数写入外，另有`0x0047C670`查询读取和`0x0047D350`重置写零；本轮不扩大到这两个待审函数。

## 2. 完整指令语义

```asm
mov eax, [esp+4]
mov [ecx+2AE4h], eax
retn 4
```

函数先把栈上传入的完整32位参数装入EAX，再以ECX为角色对象基址，把该完整dword写入`actor+0x2AE4`，最后由callee清理4字节参数并返回。没有局部栈、隐藏分支、附加读取或隐式清零。

寄存器契约：

- EAX总是替换为完整参数值；
- ECX保持入口角色对象token；
- EDX完全不修改；
- 成功与写访问异常前缀都已完成EAX装载。

## 3. 参数与对象域

24处caller只传完整dword `0`或`1`：

- 写1：`0x004567EA`、`0x0045AC54`、`0x0045D51C`、`0x0045D5BB`、`0x0045F765`、`0x0046292B`、`0x004629F3`、`0x00462B02`、`0x00462ED5`、`0x00463108`、`0x00463335`、`0x00463A50`、`0x00463CF0`、`0x00464D04`、`0x0046AA6C`、`0x0046AC80`、`0x0046D221`；
- 写0：`0x00456BC0`、`0x00456C1C`、`0x00456CF4`、`0x00456D88`、`0x00456E69`、`0x0045AEEF`、`0x0045D647`。

所有ECX都由组A固定对象表按0xBCD stride形成。typed实现因此不建立新的角色对象数组，而把十个`+0x2AE4` dword挂在`LegacyBattleFinalActorStepState::group_a_availability_blocks`，由所有caller共享。

## 4. typed leaf与停止时序

`LegacyBattleActorAvailabilityBlockState`保存一个完整dword及写访问性；`set_legacy_battle_actor_availability_block()`显式返回EAX/ECX/EDX、写计数与状态。

执行顺序严格为：

1. EAX替换为参数；
2. 检查原对象写访问点；
3. 可写时写完整dword一次；
4. 不可写或owner缺失时在该访问点停止。

写停止保留入口owner值和EDX，EAX已是参数，写计数仍为0。caller必须在原call点立即返回，不得执行任何suffix；实现不增加nil继续、索引夹限、布尔规范化或替代写入。

## 5. 24处物理caller闭包

| caller | 物理call | 数量 | 参数 | typed集成 |
| --- | --- | ---: | --- | --- |
| `0x00456680`组A逐角色帧 | `0x004567EA`、`0x00456BC0`、`0x00456C1C`、`0x00456CF4`、`0x00456D88`、`0x00456E69` | 6 | `1,0,0,0,0,0` | 六处分支直接写共享owner；第一处保留末次terminal查询EDX，其余保留各前置callee残值 |
| `0x0045AA00`最终角色步进 | `0x0045AC54` | 1 | `1` | continuation路径在后续动作/workspace写之前直连 |
| `0x0045ADF0`战后处理 | `0x0045AEEF` | 1 | `0` | 第二个组A角色owner写零，停止阻断全部重置尾 |
| `0x0045D490`逐帧预处理 | `0x0045D51C`、`0x0045D5BB`、`0x0045D647` | 3 | `1,1,0` | 依次保留source actor、通知callee返回和secondary actor的EDX |
| `0x0045F2A0`输入分派 | `0x0045F765` | 1 | `1` | 撤退确认路径直连并传播停止 |
| `0x00462740`目标选择刷新 | `0x0046292B`、`0x004629F3`、`0x00462B02`、`0x00462ED5`、`0x00463108`、`0x00463335`、`0x00463A50`、`0x00463CF0` | 8 | 全部`1` | 八条消息/选择分支共用owner并保留各自EDX |
| `0x00464CC0`角色目标准备 | `0x00464D04` | 1 | `1` | 组A准备分支保留入口EDX，停止阻断组B完成查询 |
| `0x00469D20`脚本分派 | `0x0046AA6C`、`0x0046AC80`、`0x0046D221` | 3 | 全部`1` | opcode 9/23/58直连；高目标分支保留`actor*5-40` EDX及减8发布前缀 |

合计`6+1+1+3+1+8+1+3=24`。旧`pending_478330`、`configure_group_a_actor`、`prepare_group_a_actor`及同义包装边界均已删除；生产源码对`0x00478330`只保留leaf文档注释，不再通过端口模拟该地址。

## 6. caller寄存器与前缀

caller在构造ECX时会改写EAX，但leaf第一条指令立即把参数完整覆盖到EAX，因此返回EAX只可能是0或1。ECX按组A索引精确映射：索引0为`0x005029D0`，索引1为`0x00505904`，其余按0xBCD stride递增。

EDX不由leaf修改，必须由caller显式传入：

- 组A AI首路径使用最后一次terminal查询返回EDX；
- pre-frame三处依次保留source actor、组A通知callee返回及secondary actor；input撤退路径使用`(actor_code-8)*0xBCD`，actor-target-preparation保留函数入口EDX；
- 脚本opcode 9/23/58在目标码大于7时先以`actor_code*5-40`覆盖EDX，否则保留入口EDX；
- 其余caller保留原调用点前最近一次真实写入或callee返回的EDX。

各层结果结构都携带leaf结果和调用计数，typed-stop逐层向上传播；测试同时断言到达前缀和禁止suffix。

## 7. owner与生命周期

组A十个角色对象由既有battle startup/final-actor组合拥有。`LegacyBattleFinalActorStepState`是跨组A帧、输入、目标选择、角色准备和脚本分派共享的唯一状态；`LegacyBattleActorAvailabilityBlockState`只投影对象内`+0x2AE4`，不复制对象token、角色记录或其他字段。

全局重置及相邻`0x0047D350`仍按其各自LST写集合处理。本工作包只回收24处已证明caller，不把查询或重置函数提前标成已关闭。

## 8. 测试覆盖

独立leaf测试覆盖写0、写1、任意完整dword、ECX/EDX保持以及不可写owner的EAX先装载与零写停止。caller测试覆盖八个caller类的正常共享owner路径，并覆盖组A帧、最终角色、战后处理、pre-frame、输入、目标选择、角色准备和脚本分派的写停止传播；关键分支额外验证actor code 7的对象地址EAX回绕、pre-frame三类EDX来源、script高目标EDX覆盖及所有suffix抑制。

## 9. 验证门禁

最终定向门经`OPENSWD3_CTEST=build/workpack278/directed-ctest.sh ./build.sh core`运行，独立leaf与战斗聚合共`2/2`通过；Linux core`199/199`、AddressSanitizer`199/199`、Linux app`205/205`及随后连续十轮完整core`199/199`全部通过。所有最终stdout日志零源码warning、sanitizer finding与runtime error，四份stderr日志均为空；全部触碰C++文件通过clang-format Werror，`git diff --check`通过。

inventory生成器连续双跑逐字节一致，闭包为`278/422 = 268 platform_adapted + 10 assembly_exact + 144 pending_audit`，下一项为`audit_order=279 / 0x00478340`；SHA256为`aae441118631359298b010f4aa01755551f3851c0942b406f228807cfa0e16d9`。系统TMP快照中的项目相关候选均确认为Pi/context-mode运行时，不存在需迁移的repo-owned条目；未启动原版或OpenSWD3游戏程序。

## 10. 动态差分状态

原版组A完整对象、24处caller联合输入状态、异常写访问及EAX/ECX/EDX动态捕获后端当前不可用，因此`original_diff_verified`登记为`blocked_runtime_oracle`。完整LST静态审计、24处物理xref闭包、typed owner复用和现代侧定向/全量门禁不依赖该oracle。
