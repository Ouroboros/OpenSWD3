# 战斗菜单选择后退 `0x00460C40`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整范围与间接表

权威LST主体为`0x00460C40..0x004611F6`，从proc到endp共699行、410条实际指令、18个call、43个跳转、38个局部/默认标签、10个`retn`，没有外部`FUNCTION CHUNK`。

函数后的只读控制数据也已计入审计：`0x004611F8..0x0046121C`为十项跳转目标，`0x00461220..0x0046123D`为30-byte间接索引。入口先以u32回绕计算`message-1`并清pre-frame gate B；结果unsigned大于29时直接返回。范围内只有消息1、2、3、4、5、7、8、27、30进入有效case，其余消息映射到默认返回。范围内默认case把ECX装为9；消息0或大于30在装表前保留入口ECX。

18个callsite完整归类为：7次既有样本播放、2次组B候选查询、3次角色原点准备、4次角色选择配置和2次组A候选查询。四类角色callee继续复用战斗输入typed端口；没有遗漏callsite。

## 2. 消息1权限后退

完整selection signed大于8时只把selection写1，保留入口后计算出的EAX并立即返回，不播放样本、不置mouse action gate。

其余值先播放样本，再把selection减1并在该值上首次读取物理权限前缀`0x00524413`。权限owner严格由一个前置byte和后续两个dword的八个little-endian byte组成，共九byte；负值回绕或大于8只在真实读取时typed-stop，保留样本和减量store。

首byte为0时，EDX只覆盖低16位为startup extra，保留callee返回高16位；ECX保留物理前缀token。循环每次先减EAX，signed小于1时改为`zero_extend(extra)+5`，再读取该权限byte。循环不增加modern上限；首个非零byte发布为selection并置mouse action gate。

## 3. 消息2、4、5、7、8、27、30

消息2先减一基列表selection。仍signed不小于1时走共享“样本后置gate”路径。下溢时把selection写1；只有panel scroll A signed大于0才先播放样本，然后无条件减scroll。结果signed非负时置gate返回；负值还原scroll 0、置gate并保留减量EAX。

消息4先减网格selection。下溢时根据row-limit word是否非零写1或0，再减panel scroll B并把负值夹0。随后无条件播放样本，按原顺序装载current equipment、grid selection、scroll，先置mouse action gate，再写当前equipment的选择缓存，最后写启动owner中的四项scroll缓存。两个数组只在各自真实store typed-stop；第一数组已写时第二数组停止不回滚。

消息5无条件播放样本并减组B行selection；signed小于1时存2，但返回EAX仍是减量值。

消息7先清两项transition值，再播放样本并减alternate selection；signed小于1时从live alternate limit回绕。返回EAX仍保留减量值，ECX在回绕路径保留live limit。

消息8先把row-limit byte写入ECX低byte。零值直接返回；非零时减narrow selection，signed下溢以i8符号扩展row limit写回，再走共享样本与mouse action gate。

消息27与消息2相似，但操作网格selection和panel scroll B。selection仍signed不小于1时播放样本；下溢分支不播放样本，只按row-limit word写1/0、减scroll并夹0，最后置gate。

消息30把网格selection减1，signed小于1时回绕到10，再走共享样本与gate。

## 4. 消息3组B后退

消息3先读取目标阻断dword；完整值精确为1时立即返回。否则播放样本，再按`active*5-40`的u32回绕索引读取启动模式表，越界只在该真实访问typed-stop。

模式表为0时使用组B路径：

1. target cursor先减1；signed小于1时回绕到live组B数量。
2. 以cursor直接读取八项组Border，再发布target actor。order只在真实读取typed-stop。
3. target actor在首次候选thiscall前验证八对象物理域；完整callee EAX等于1时重新从live cursor后退，不增加循环上限。
4. 首个callee EAX不等于1的对象先调用原点准备。
5. 从0开始以i32 signed比较live组B数量，对每个物理对象配置mode 0；数量在每次callee后重读，首次对象8调用typed-stop。
6. 对selected对象配置mode 1，然后置mouse action gate和target selection gate；ECX改为`target+1`并写共享action kind。

selected配置callee的EAX/EDX保留为函数返回；ECX返回一基action kind。

## 5. 消息3组A后退

模式表非零时先按u32回绕计算：

```text
remaining = group_a_count
remaining -= excluded_group_a_count_u16
remaining -= startup_supplemental_count_u16
```

remaining unsigned不小于4时，target cursor先减1；signed小于1时回绕到remaining。以cursor直接读取十项actor order code，再按机器地址`0x0050259C+code*0x2F34`读取两项完成槽。typed owner只覆盖code 1..10并映射到物理角色0..9；code 0在首次完成槽读取停止。完成槽任一精确为1或以`0x004FFA9C+code*0x2F34`调用组A候选后EAX精确为1时继续后退。callee拒绝后先从共享target cursor重装ECX，回绕时重新读取live count与两个减数，不能误用callee返回ECX。order和完成槽分别只在首次实际访问typed-stop，循环不设现代上限。

首个可用code调用原点准备时保留另一条原始地址公式`0x005029D0+code*0x2F34`，因此code 10只在该真实一过尾thiscall停止，并保留查询前缀及预调用EAX/ECX/EDX乘法结果。普通返回后把target actor清0，并把action kind写为`code+1`；首个十对象reset call看到的EAX也是已经自增后的action kind。

remaining小于4时每轮都先从共享action kind重装ECX再减1，signed小于1回绕到live组A数量；完成槽与候选查询仍按一基code映射，原点准备则使用`0x004FFA9C+code*0x2F34`，正好映射物理角色`code-1`。此路径不写target actor，原值保持。

两条路径汇合后固定从`0x005029D0`开始对全部十个组A对象配置mode 0，每次callee之后清对应marker byte。随后按机器公式`0x004FFA9C+action_kind*0x2F34`配置selected对象；一基值10精确映射第十个物理对象，不是停止点。只有0或大于10才在这次真实selected thiscall隔离。普通完成再置mouse action gate和target selection gate。四类对象call前的乘法中间值和物理token均作为端口入口寄存器固定，不用逻辑索引替代。

## 6. caller回收、共享owner与重置

唯一caller是逐帧输入分派，原有三处调用分别位于interaction mode 1、record4重复后退和record3左向分支。三处现均直连本typed实现；保留调用前EAX/ECX/EDX，本函数普通返回完整寄存器继续进入原相邻callee。typed-stop保留样本、cursor、callee、marker和数组写前缀，并阻断该调用点之后的确认、方向提交和本帧其余输入。

状态复用：message、action kind、selection、mouse action gate和样本混音来自输入分派owner；active、actor order、完成槽和excluded count来自最终角色owner；两组数量和组Border来自metric owner；启动模式、权限dword、extra和equipment scroll来自启动owner；列表、网格、narrow selection、scroll、row limit、target cursor/actor/gate、marker、transition和equipment选择缓存来自相邻帧输入owner。

相邻第110项同时修正一个由本LST交叉验证出的物理别名：入口坐标改变清的是独立pointer activity gate，不是右侧按钮与目标命中使用的mouse action gate。两者现保持独立typed存储。

全局reset只同步其权威写集合中的target cursor、alternate limit/selection、target actor和equipment缓存首项；pointer activity、target selection gate、transition值及其余equipment缓存不在reset写集合中，保持入口值。权限前置byte同样不被启动/全局reset擅自清零。

## 7. 验证与动态差分

定向测试覆盖：消息0寄存器；权限前缀wrap与EDX半字；权限typed-stop；列表scroll；equipment双数组写序与typed-stop；消息5/7/8/27/30；组B逆向选择；组A小组一基cursor；组A actor order typed-stop；组A大组order、候选/原点/配置预调用寄存器、code10原点一过尾停止、固定十对象重置、marker清零及一基selected code10映射第十对象；输入分派三处caller直连与typed-stop阻断；pointer activity/mouse action物理分离；全局reset别名。

当前缺少原版权限相邻内存、两组角色对象、四类角色callee、样本后端、菜单/目标全局及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
