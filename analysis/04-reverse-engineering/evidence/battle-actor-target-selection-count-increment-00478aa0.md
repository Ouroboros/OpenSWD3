# 战斗角色目标选择计数递增（00478AA0）

状态：`pending`。已废弃GPT重建实现，本轮逆向尚未开始。

以下保留历史分析与证据；旧结论、完成记录和测试结果不作为本轮验收。
原版采集事实保留，所有解释和实现结论须重新核实。

## 历史资料（重新核实前不生效）


## 1. 原版范围与指令

权威为完整 `swd3.exe.lst` 的机器码和指令。叶范围
`00478AA0..00478AA7`，半开区间结束于 `00478AA8`：8字节、2条指令，
无callee、分支、外部chunk或中段入口。

```text
00478AA0  66 FF 81 76 2A 00 00  INC WORD PTR [ECX+2A76h]
00478AA7  C3                    RET
```

唯一字段操作为16位读、加1、写入；`FFFF -> 0000` 回绕，不饱和。
原版EAX/ECX/EDX保持，INC保留CF并改写其他算术flags；普通RET读取
返回地址并推进ESP四字节。这些是原指令事实，现代接口不再回传寄存器。

完整LST只有三个直接CALL：

```text
00456A1F -> 00456A24
00456CDD -> 00456CE2
00456D71 -> 00456D76
```

相邻数量读取 `00478AB0` 已单独迁移；`00478AE0` 的数量和启动门衰减
仍有待迁移协议。本批不扩大为组A、动作父链或整场战斗验收。

## 2. 实际字段与语义结果

`increment_legacy_battle_actor_target_selection_count` 直接接收实际
`compat::u16*`，返回访问状态、可选的实际旧值及可选的已写入值。
删除owner/view/resolver、callee地址常量、寄存器请求/结果、模拟栈、
flags回声、请求数组、轨迹、调用计数与转发执行器。

字段继续借用同一存储：

```text
Group-A  action.group_a_action_execution[index].target_selection_count
Group-B  startup.group_b_lifecycle[index].action_execution.target_selection_count
```

没有第二套计数数组或复制后写回。原相邻启动门和其他角色字段不动。
每次调用读取实时WORD；重复调用及外部后续写入均使用最新字段。

访问描述只有读、写、返回地址是否可访问。已有失败阶段保留：

- 读失败：没有旧值或提交值，实际字段不变。
- 写失败：保留实际旧值，没有提交值，实际字段不变。
- 返回失败：保留已写入的WORD及实际旧值，父调用不执行后缀。
- 正常返回：旧值和提交值都存在，写入为 `u16(old + 1)`。

空指针只表示当前宿主没有可借字段，在字段读处报告不可用；不会返回
默认零或成功。父结果的外层optional表示实际执行过；跳过则没有结果。

## 3. 三条调用链与真实前驱

### 3.1 扫描：00456A1F

`004569F9` 的terminal结果精确为1，或 `00456A09` 的映射非全1，均
跳过计数。其余路径先在 `00456A18` 清叠加绘制标记，再直接递增该轮
敌方实际WORD。正常返回后才执行 `00456A24` 的队员progress递增。

读、写或返回失败保留标记清零及此前轮次的计数/progress，阻断当前progress、
循环推进、后续角色及选择。后续迁移已删除清零操作的通用端口，
失败只提供状态，不再采纳端口任意EAX。
正常后续指令重新加载数量并改写算术flags，故不需要保留INC flags结果。

### 3.2 第一选择：00456CDD

选择来自 `4A754C`，现代字段为 `selected_actor_one_based`。先执行
`00456CBA` 的叠加标记清零，`00456CBF` 重读选择形成计数字段地址；递增
完成后 `00456CE2` 再重读选择、DEC，并在 `00456CEC` 发布目标。
随后 `00456CF4` 写可用性，`00456CFB` 执行最终处理，再清门及写两处UI门。

### 3.3 第二选择：00456D71

选择来自 `4A7554`，现代字段为 `selected_opponent_one_based`，用于
当前角色AI标记非零的分支。清标记在 `00456D4E`，选择重读在
`00456D53`，递增后重读在 `00456D76`，目标发布在 `00456D80`。
随后 `00456D88` 写可用性、清门并把第二选择写1；没有第一分支的最终处理。
旧实现和测试把两处后续目标CALL地址互换，本批按上述LST对应关系修正。

两条路径均按完整32位回绕地址借实际角色字段。清标记后重新读取选择，
再定位计数字段；不能截断one-based索引。清标记只写目标DWORD，
其内部不能回调修改选择。缺失宿主对象在首次标记写入处报告失败。

```text
EAX = n * 1381
EDX = n * 345
address = 005229E0 + n * 2B28
CF = SUB32(n * 24, n).carry
```

计数失败沿用上述EAX并阻断目标发布/清门。正常后继DEC覆盖INC的非CF
算术flags，CF来自真实地址SUB；未迁移的目标选择仍取得实际EAX/EDX及
重读选择的DEC结果。叶本身不再提供CPU式回复。

地址回绕后可借任一实际角色字段：`20000001`别名敌方0，
`16DED16F`别名队员2；不能仅以原one-based数值超过8为由提前停止。
不能映射的宿主对象现于前驱标记写入处报告不可用；这不证明原版地址必然fault。
完整原版静态内存、其他字段别名及父链其余协议仍属待闭合范围。

## 4. 独立向量与双向复核

LST推导先记录于本批独立分析，再核对C++及测试。正向映射覆盖唯一WORD
RMW、返回边界及三个调用顺序；反向核对实际字段、实时选择、回绕地址、
三类失败前缀与后缀抑制。成功计数不再依赖请求序号或端口回复。

实际字段向量：

- WORD `0/1/2/1234/7FFF/8000/FFFE/FFFF` 与独立加1结果。
- 邻接 `ABCD/5678`、重复 `FFFF -> 0 -> 1`、实时后续写入。
- 队员2/敌方3及其他索引哨兵，两个相邻启动门保持。
- 读失败、写失败、返回失败和没有字段；optional区分观察与提交。

调用向量：

- 两条选择分支各覆盖三类失败，phase/EAX及旧目标保留、后缀抑制。
- 索引1及8借首/尾敌方；`7FFF -> 8000` 后DEC覆盖INC的符号/溢出。
- 索引0/9及缺少敌方存储，在实际标记写入处失败，不进入计数操作。
- 前置队列查询把选择9改3、计数7改FFFF；随后清敌方2标记、递增并发布目标2。
- `20000001`借敌方0，保持高EAX `A0000000`、EDX `20000159` 及CF。
- `16DED16F`借队员2，返回失败保留0和EAX `5FFFCBCB`，敌方0不动。
- 扫描三类失败保留phase、阻断progress/后续选择；第二轮写失败保留首轮。
- terminal精确1与已映射对象跳过，计数不动且没有伪造成功结果。

## 5. 验证与限制

下述为计数迁移批次的历史验证。后续实际标记写入及调用测试已修改，
最新结果见[叠加绘制清零](battle-group-a-overlay-clear.md)。

计数迁移补充边界后的最终命令：

```text
./build.sh core --build-target openswd3_battle_legacy_battle_setup_tests --test --test-regex '^battle\.legacy_battle_setup$'
./build.sh core --build-target openswd3_battle_actor_frame_316_tests --test --test-regex '^battle\.actor_frame_316$'
./build-asan.sh --build-target openswd3_battle_legacy_battle_setup_tests --test --test-regex '^battle\.legacy_battle_setup$'
./build-asan.sh --build-target openswd3_battle_actor_frame_316_tests --test --test-regex '^battle\.actor_frame_316$'
./build.sh app --build-target openswd3
```

core setup 12.70秒、角色帧31.79秒；ASan setup 19.03秒、角色帧32.79秒，
每项均1/1通过。SDL最终增量构建成功；此前本批实际链接所用四份生产
源码身份与最终相同。没有启动游戏。

六份源码/测试身份与最终门禁匹配。五份最终日志没有warning、错误或
sanitizer记录；此前两个setup编译仅报告既存的outcome-resolution
测试133行窄化警告。边界补充前的日志不用于最终测试验收。

日志及身份在仓库 `build/tmp/runtime/` 下的
`target-count-increment-direct-{core,callers-core,asan,callers-asan,sdl}.log`、
`target-count-increment-direct-validated-source.sha256` 和本批发布检查日志。

旧Workpack 307的199/199、205/205、十轮core及inventory SHA均为历史，
不支持当前源码验收。当前B10仍为315/422，316/317/318保持pending。

C++实际字段及访问描述不能制造原CPU字段页、RET栈页、完整flags或SEH。
分类保持 `platform_adapted`；联合原版动态差分仍为
`blocked_runtime_oracle`，没有升为 `assembly_exact` 或
`original_diff_verified`。SDL链接不证明整条角色帧或游戏运行；组A、
动作及资源父链仍需逐项迁移，整体INVOKE清理尚未完成。
