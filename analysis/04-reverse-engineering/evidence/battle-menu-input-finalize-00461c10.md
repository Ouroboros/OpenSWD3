# 战斗菜单输入最终提交 `0x00461C10`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整范围与分派表

权威LST主体为`0x00461C10..0x00462085`，从proc到endp共504行、258条实际指令、7个静态call、20个跳转、25个局部/默认标签、8个`retn`，没有外部`FUNCTION CHUNK`。4个active group-A reset callsite属于`sub_4750C0`，selected/group-B/group-A三类对象重置callsite属于`sub_47C660`。

函数后的`0x00462088`十项跳表和`0x004620B0`三十byte间接索引也已审计。message 1、2、3、4、5、7、8、27、30各有独立目标；6、9..26、28、29以及u32越界message进入默认路径。入口固定EAX=1、EBX=0，先清pre-frame gate B并置mouse action gate。

## 2. selected group-B特殊清理

selected actor cleanup gate为1时不进入message跳表。函数先把message和mouse gate清0，再以one-based selected group-B code构造对象token；预调用EAX保留`code*0x159`，ECX为`0x005229E0+code*0x2B28`，EDX保留入口值。code 1..8映射八个物理group-B对象；code 0在一过前对象call typed-stop。

callee返回后EDX清0，随后严格清两份启动缓存、三项selection cache、cleanup gate和16-bit状态，并把selection actor code写全1。普通返回EAX/ECX保留callee结果，EDX为0。

## 3. message固定路径

message 1关闭菜单并清固定启动缓存、runtime gate、mouse gate、动画帧和phase，返回EAX=0。message 2按原交错顺序清五项workspace、发布message/list/target gate、清四项selection cache并置phase 5，再以active code减8映射group-A对象；其预调用EAX为`index*0xBCD`、ECX为对象token、EDX为原active code，callee后才清动画帧。

message 4、5、8、27、30都重置active group-A对象，但保留两套原始寄存器构造：message 4/27令EDX为`index*0xBCD`；message 5/8/30保留进入公共块前的EDX。各路径严格保留原message、grid/list、scroll、workspace、target gate、fallback action kind、cache和phase写集合。message 7不调用actor，关闭message后发布alternate limit 2、selection 1和action kind 1。默认路径只清两个动画帧。

## 4. message 3完整角色重置

selected option为全1时发布message 1、phase 5并清runtime gate；否则清两项transition并发布message 7。随后发布target action，清selected actor reset gate，先重置active group-A对象。

live group-B count按i32判断；正数时从group-B物理首对象开始无现代上限逐项调用，每次callee后重新加载live count。count 9在第九次真实对象call停止并保留前八次结果。随后固定重置十个group-A对象，并在每次callee返回后逐byte清十项target marker；EAX/ECX/EDX跨callee完整传播。

角色循环后按action kind 2、3、27、30、4分别发布message 2、4、27、30、8；其他值清动画帧并把fallback action kind写回。匹配路径清target/selection缓存，以active code映射既有`0x004FE5D4`十项缓存，再按每角色五dword物理布局依次清攻击缓存第3项和第1项。返回EAX为角色攻击缓存byte偏移、ECX为active code、EDX保留最后callee结果。

## 5. typed-stop、共享owner与caller回收

selected group-B、active group-A、live group-B、十项marker、十项selection缓存和五dword攻击缓存均只在首次真实call或store处typed-stop。所有停止保留此前message、gate、workspace、角色call、marker和缓存写前缀，不执行原调用点后的动画或caller尾清理。

新增状态全部并入既有input dispatch、frame input、startup reset、final actor和actor metric唯一owner，没有复制物理全局。全局reset同步补齐4A754C、4A7564及本函数使用的已映射selection状态，并把action kind恢复为权威reset值1；不在全局reset原写集合内的workspace、fallback和cache C保持入口值。

唯一caller为逐帧输入分派record0三帧重复路径。旧`commit_final`操作槽保留稳定reserved值，caller现直连typed实现；普通返回后才清两个尾值，typed-stop阻断这两项尾清理。

## 6. 验证与动态差分

定向测试覆盖：selected group-B正常与code-zero停止；message 1/2及active一过前停止；message 3完整两组角色循环、匹配/回退action、group-B count 9前缀；message 4/5/7/8/27/30与默认跳表；caller普通与停止传播；全局reset新增owner同步。

当前缺少原版两组角色对象、两类角色callee、message/selection全局、record0 caller与EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
