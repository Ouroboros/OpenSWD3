# 战斗角色列表状态处理 `0x00470380`

状态：`pending`。

本批状态：`pending`。

状态处理及唯一菜单调用方的协议移除已通过定向验证。
源码提交`bc6b14e7`已推送，远端main与该提交一致；21份被测文件与提交内容一致。
所属`CHAIN-e0f6819f24dc`整链仍未验收，不勾选。原版联合动态差分仍为
`pending`；完整SDL战斗帧接线和真实交互未验收。

## 删除范围与实际借用

删除`LegacyBattleActorListStateRequest`、`LegacyBattleActorListStateCallReply`、
`LegacyBattleActorListStatePort`及结果中的寄存器、索引回包和诊断计数。
结果只保留状态与`available`。匹配序号是算法所需局部变量，不是调用计数。
删除菜单`validate_primary_action`操作编号及参数包调用，改为真实函数调用。

主游标借用`action.group_a_action_execution`，列表和配置借用同一角色的
`party.actor_list/configuration`。消息使用实际文本队列与共享消息头；音频使用
`LegacySampleManager`。音频引用沿coordinator、input/message、entry、refresh
传递，所有调用方和测试同时更新；不新增状态转发Adapter。

## 权威范围与逐块追溯

LST主体`00470380..004705B7`，proc至endp共284行；唯一caller为`00462C2D`。
完整函数及caller已重新逐块核对，映射如下：

- `470380..4703B1`：category 0/1/2映射10/C/1001，其余保持原mask。
- `4703B1..4703BF`：occurrence零先返回；非零才发布主current=next。
- `4703C4..4703F8`：先推进并发布current，再检查零及节点；mode&5、
  category&mask、无符号type 1B..1E共同筛选，第N项命中停止。
- `4703FA..47043E`：bit15清selected并写低15位primary；bit14清selected并
  写低14位secondary。未命中位不清另一阈值。
- `47043E..47045F`：bit11最后用低11位覆盖primary，再发布实际资源链头。
- `470464..470483`：先推进并发布资源current，再匹配resource id。
- `4704B6..4704EF`：比较资源+8/+A两个signed WORD，任一大于零则发布selected
  并成功；+6不参与。本分支不执行第二次主链提交。
- `4704F2..470521`：第二次主链提交；非零primary与配置记录+6的signed WORD
  比较，required零扩展，容量足够立即成功。
- `470521..470552`：head为空才入队主容量提示，返回后读取混音并播放140。
- `470555..47057D`：消息及音频返回后重读secondary、配置记录+8及head；
  容量足够立即成功。
- `470485..4704B1`、`4704C4..4704DB`、`47057D..4705AE`：资源或次容量
  不足时执行同一消息及音频顺序；已有head抑制两者。
- `4705B1..4705B7`：没有可用条件时返回false。

消息入队参数为`118/A/14/文字/40000002`。资源、主容量、次容量文字分别为
`4A7CD0/4A7CC0/4A7CB0`。调用实际`enqueue_legacy_battle_text_message`后
读取共享混音，再调用`play_legacy_sample`；原版未检查音频返回值，保持该顺序。
普通容量分别从配置DWORD[1]高WORD、DWORD[2]低WORD读取并符号扩展，
不再使用Request中的容量快照。

## 调用方及失败前缀

`462BD7..462C2D`保留输入门、候选门、非零候选及角色条件。
`462C32..462C73`不可用立即退出；可用后重读角色、候选和类别，再执行后续
4705C0资料应用。该资料应用协议留在后续批次，未用本批替代。

宿主owner/node缺失在对应实际访问处保留既有typed-stop方式，游标和阈值的
已发生写入不回滚。文本子函数分配或测量失败向上传播并保留其失败前缀，
不继续播放音频；这不是原版异常等价或新完成状态。

文本分配内部协议、资料应用、资源生产及完整SDL主循环仍pending。
本批新增音频借用的编译通过，不代表已在真实游戏中到达该路径。

## 本批验证

- core `battle.legacy_battle_setup`：1/1，11.93秒。
- ASan同一定向测试：1/1，19.65秒。
- Linux SDL app构建成功；未运行app测试或游戏。
- 21份源码、测试及构建配置SHA256身份核验通过，`git diff --check`通过。
- 已删除状态类型和`validate_primary_action`在include/src/tests中零命中。
- 独立向量覆盖分类、mode、type上下界、序号、组合阈值、signed容量、资源
  +8/+A正负零、+6无关、消息抑制、失败前缀与callback后重读。
- 合成SND经实际加载器和sample manager验证：消息发布先于sound 140，消息
  callback后的混音5产生音量58，居中pan63，播放一次；音频callback修改的
  次容量及阈值在返回后可见。这是UT资源验证，不是真实声卡或原版动态差分。

日志和身份清单：`build/tmp/runtime/actor-list-state-final-{core,asan,sdl,identities}.log`
及`actor-list-state-source.sha256`。独立分析与双向映射保存在同目录
`actor-list-state-lst-analysis.md`和`actor-list-state-bidirectional-review.md`。

首次运行暴露输入测试栈溢出，SND索引已改为夹具堆所有权；新增音频向量的
短载荷被实际加载器拒绝，修正为48字节后以上门禁重新通过。

历史资源链头发布`3eda365b`见[链头证据](battle-actor-resource-list-commit-00470900.md)。
历史全量测试及旧workpack计数不作为本批验证；B10仍保持0/422、316待审计。
