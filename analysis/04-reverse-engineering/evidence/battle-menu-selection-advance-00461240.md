# 战斗菜单选择前进 `0x00461240`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整范围与间接表

权威LST主体为`0x00461240..0x004618A8`，从proc到endp共785行、460条实际指令、19个call、47个跳转、44个局部/默认标签、14个`retn`，没有外部`FUNCTION CHUNK`。

函数后的只读控制数据也已计入审计：`0x004618AC..0x004618D0`为十项跳转目标，`0x004618D4..0x004618F1`为30-byte间接索引。入口以u32回绕计算`message-1`并清pre-frame gate B；消息0或大于30在装表前返回，范围内默认消息把ECX装为9。有效消息与后退函数相同：1、2、3、4、5、7、8、27、30。

19个callsite完整归类为：8次既有样本播放、2次组B候选查询、3次角色原点准备、4次角色选择配置和2次组A候选查询。四类角色callee继续复用战斗输入typed端口。

## 2. 消息1权限前进

完整selection signed大于8时只把selection写1并返回，不播放样本、不置mouse action gate。

其余值先播放样本，再把selection加1并首次读取九byte物理权限owner。索引9或u32回绕只在真实permission访问typed-stop，保留样本与增量store。

首byte为0时，ECX先以zero-extended startup extra计算，EDX完整写为`extra+5`，ECX再保留物理权限前缀token。循环每次先加EAX；EAX signed大于EDX时回绕到1，再读取permission。循环不设modern上限；首个非零byte发布selection并置mouse action gate。这里与后退函数只覆盖EDX低16位的行为明确不同。

## 3. 消息2、4与27的前进边界

消息2先把列表selection加1。结果signed大于7时先把selection夹7、panel scroll A加1，再以i8符号扩展row limit比较`scroll+7`：

- signed不大于limit时播放样本并置gate；
- signed大于limit时把scroll写为`limit-7`，不播放样本，返回EAX保留该差值。

结果仍不大于7时，直接与i8符号扩展limit比较。超过limit只把selection夹到limit并置gate，不播放样本，EAX仍保留增量值；未超过才走样本路径。负row limit和减7回绕不被现代夹值改写。

消息4对grid selection执行同型逻辑，但limit是u16零扩展，scroll为panel scroll B。边界允许时播放样本，边界夹值时不播放。汇合后严格按原顺序：加载current equipment/grid/scroll，写equipment选择缓存，写启动owner的scroll缓存，最后才置mouse action gate。任一数组真实store typed-stop时，尾gate尚未写；第一数组成功、第二数组停止时第一写保留。

消息27复用消息4的grid/scroll推进和无样本夹值，但不写equipment数组。每条非样本返回路径直接置gate；样本路径在callee后置gate。

## 4. 消息5、7、8与30

消息5先播放样本，再把组B行selection加1；signed大于2时存1，但返回EAX保留增量值。

消息7先清两项transition值并播放样本，再把alternate selection加1；signed大于live limit时存1。EAX保留增量值，ECX保留live limit，不置mouse action gate。

消息8先把row-limit byte写入ECX低byte；零值直接返回。非零时把limit作i8符号扩展到EDX，再把narrow selection加1；结果signed大于EDX时存1，随后无条件走共享样本与gate。

消息30把grid selection加1；signed大于10时存1，再走共享样本与gate。

## 5. 消息3组B正向选择

消息3先检查目标阻断完整dword，精确为1时立即返回；否则播放样本并按`active*5-40`的u32回绕索引读取启动模式表。

模式表为0时使用组B路径：target cursor加1，与live组Bcount作signed比较，超过时回绕1；随后以cursor直接读取八项组Border。组B候选callee完整EAX等于1时重新从live cursor前进，不增加循环上限。

组B查询、原点和selected配置前严格恢复机器乘法落点：EAX为`index*0x565`、EDX为`index*0x159`、ECX为物理对象token。首个可用对象先调用原点准备，再把live组Bcount装入EAX，从0作i32 signed循环；每轮以递增物理token配置mode 0，callee后重装live count。count 9会在第九次真实配置thiscall停止，保留前八次配置，并返回EAX live count、ECX一过尾token、EDX前一callee值。最后对selected对象配置mode 1。普通完成置mouse action gate、target selection gate，并把`target+1`发布为action kind与返回ECX。order和对象域仅在首次真实访问typed-stop。

## 6. 消息3组A正向选择

启动模式非零时，remaining仍按u32回绕由组Acount减两个u16减数得到；计算过程保留EAX/ECX/EDX原寄存器落点。

remaining unsigned不小于4时，target cursor加1并与remaining作signed比较，超过时回绕1。以cursor读取actor order code，再按机器地址`0x0050259C+code*0x2F34`读取两项完成槽。typed owner只覆盖code 1..10并映射到物理角色0..9；code 0在首次完成槽读取停止。候选查询严格使用`0x004FFA9C+code*0x2F34`。拒绝后必须从共享target cursor重新装载EDX，再按live count和两个减数重算remaining；这保证callee破坏EDX后仍按原cursor推进。order首次越界typed-stop时保留原remaining EAX、supplemental ECX和cursor EDX。

remaining小于4时，每次循环都从共享action kind重新装载ECX，再加1并与live组Acount作signed比较，超过时回绕1。完成槽和候选按同一一基code映射；候选callee拒绝后不能使用callee返回ECX继续加一。小组原点准备使用`0x004FFA9C+code*0x2F34`，映射物理角色`code-1`。

大组首个可用code的原点准备保留另一条机器公式`0x005029D0+code*0x2F34`；code 10只在该真实一过尾thiscall停止，且返回EAX=`code*0x3EF`、EDX=`code*0xBCD`和ECX物理token。普通返回后清target actor，把EAX自增并把同一值写action kind，故首个reset callee也看到自增后的EAX。两条路径随后固定从`0x005029D0`开始对十个组A对象配置mode 0并逐byte清marker，再按`0x004FFA9C+action_kind*0x2F34`配置selected对象；一基code 10精确映射第十个物理对象，不是停止点。只有0或大于10才在真实selected thiscall隔离；普通完成再置两个选择gate。

## 7. caller回收与共享owner

唯一caller是逐帧输入分派，三处原调用分别位于interaction mode 2、record6重复前进和record5右向分支。三处均已直连typed实现；稳定枚举原值保留reserved槽，不再发出opaque call。

调用前完整EAX/ECX/EDX进入本函数；普通返回继续进入原确认/方向callee。typed-stop保留样本、cursor、角色callee、marker和数组写前缀，并阻断调用点后续输入。状态完全复用第110/111项建立的输入、帧输入、启动、最终角色和metric唯一owner，不新增第二份物理数组。

第111项组A大小两条循环同时补强寄存器恢复：callee拒绝后从共享cursor重新装载，而不是误用callee返回ECX/EDX。定向测试固定前进与后退两侧的连续拒绝轨迹。

## 8. 验证与动态差分

定向测试覆盖：消息0；权限上界与typed-stop；消息2 signed byte边界、有样本推进和无样本夹值；消息4写后置gate与equipment typed-stop；消息5/7/8/27/30；组B回绕、预调用乘法寄存器和第九次配置停止；组A大组回绕、live拒绝推进、actor order寄存器停止、候选/原点/配置预调用寄存器、code10原点一过尾停止、固定十对象重置和一基selected code10映射第十对象；组A小组callee拒绝后cursor重装；输入分派三处caller直连和typed-stop阻断；后退函数对应cursor与对象寄存器回归。

当前缺少原版权限相邻内存、两组角色对象、四类角色callee、样本后端、菜单/目标全局及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
