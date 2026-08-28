# 战斗目标选择进入 `0x004620D0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x004620D0..0x0046231F`，从proc到endp共269行、157条实际指令、7个静态call、23个跳转、12个局部标签、7个`retn`，没有外部`FUNCTION CHUNK`。callee为既有队列完成查询、sample command、角色选择配置、两类目标扫描及三项角色动作查询边界；动作模式刷新与选择状态刷新callee均已关闭并直连typed实现。

导航caller共有十三个静态callsite：已关闭逐帧输入分派十处，已关闭消息阶段分派三处。逐帧输入分派的八个直接数字键callsite在typed循环中合并为两条业务分支，record2、record15和base confirm三处仍各自保留原调用时机；消息100/103共用signed 150阈值、消息102独立signed 150阈值、消息104使用signed大于20阈值，三处均直连本实现并传播子stop。

## 2. 入口门与消息短路

函数不读取caller压入的参数。entry word非零时保持入口EAX/ECX/EDX直接返回；随后依次读取input gate，精确等于1时返回；outcome darkening gate精确等于1时返回；message gate高bit置位时返回。

通过前三门后只以target suppression byte替换EAX低byte并把message装入ECX。suppression为0时，函数以group-B count减packed actor counter低byte；signed结果不大于1、retreat target非全1且message不是99时清message并返回，EAX为zero-extended packed byte，EDX保留差值。

message 110读取独立transition high word。u16值小于30时写29；恰好30时写100；大于30继续普通路径。两条提前返回都保留写前EAX低word，不把新常数投影为返回值。

## 3. dialog、ready与active actor

现代dialog list非空对应原head pointer非零：函数发布one-shot interaction state 1并以canonical非零token 1返回；不把主机节点地址转换为兼容指针。dialog为空时EAX为0。

target-ready gate不等于1、当前queued角色code为0，或message非零且逐帧option cache为全1时，函数以当前寄存器直连已关闭选择状态刷新并直接返回；刷新typed-stop保留此前入口副作用并传播给逐帧输入。这里queued角色严格复用`0x0053BD54`既有owner；逐帧option cache严格对应`0x004A7644`。

进入目标选择时先把option cache写全1，再以`queued_code-8`构造group-A token和预调用EAX=`index*0xBCD`。物理索引只允许0..9；一过前/后一过尾在首次队列查询call typed-stop。队列完成查询完整EAX等于1时直接返回。

## 4. sample与角色配置

队列未完成时，函数把sample mix装入ECX，以编号45调用既有sample command；EAX/ECX/EDX完整采用callee结果。随后重新读取queued code，把它装入EDX，发布mouse gate、target gate与phase 5，再重新计算group-A token和EAX=`index*0xBCD`，以两个固定output token调用角色选择配置；callee写回的两项word同步到输入分派内唯一选择角色原点owner，供已关闭选择帧直接读取。第二次实际对象call独立执行typed-stop，因此sample或外部callee若改变queued code，停止点仍保持此前声音和三项gate写。

配置返回后，group-B index word为全1，或signed group-A index不等于`queued_code-8`时，函数发布message 1、action kind 1并直连已关闭动作模式刷新。两个index word物理相邻但独立；group-A index不与逐帧option cache合并。刷新typed-stop保留此前sample、gate和两项角色原点发布并立即返回。

## 5. 五项目标扫描

两个index匹配时，函数先发布message 7、alternate limit 2。随后以signed group-B index构造对象token：

- 第一轮固定三次，预调用EAX=`index*0x159`，参数为scan 0..2、固定文本token和transition output token；
- 第二轮固定两次，预调用EAX=`index*0x565`、EDX=`index*0x159`，参数为scan 0..1及同一文本token。

每次callee完整EAX等于1时alternate limit加1。每轮都重新读取group-B index并在首次真实对象call检查0..7；index 8在第一轮call前停止，保留sample、角色配置、message 7和limit 2。五次完成后才清transition output dword并返回最后callee寄存器。

## 6. 物理owner纠正与caller回收

本轮交叉核对`0x0045AA00`、`0x0045FC60`及菜单相邻函数，消除了既有错误别名：

- 菜单当前角色统一复用final-actor queued owner；真正active owner仍是另一物理dword；
- group-B发布code统一复用final-actor published owner；
- group-B index、group-A index与逐帧option cache保持三个独立u16；
- target-ready复用actor-frame shared owner；outcome门复用outcome state；message高bit复用action state；group-B count和packed低byte复用既有metric/action owner；one-shot interaction复用世界player-control owner。

逐帧输入分派旧`commit_selection`槽保留稳定reserved值，五个typed调用点覆盖原十个静态callsite。普通返回寄存器继续原caller路径；active/group-B typed-stop立即阻断caller随后option清理或帧阶段。未关闭AI/action caller三处不提前改写。

## 7. 验证与动态差分

定向测试覆盖：四个入口门、group-B差值清message、message 110两种边界、dialog非空、ready刷新、active query完成、group-A一过前停止、sample/configure/动作刷新普通与typed-stop、五项扫描与可见数、group-B index 8前缀停止、逐帧输入普通直连和typed-stop传播，以及物理owner/reset交叉回归。

当前缺少原版两组角色对象、动作刷新内三个角色查询及其余未关闭callee共享副作用、动态dialog head token、三处AI caller输入、两处输出word、已关闭刷新函数的动态状态/callee轨迹及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
