# 战斗帧鼠标输入与目标解析 `0x0045FC60`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整范围

权威LST主体为`0x0045FC60..0x00460BF7`，从proc到endp共1917行、1165条实际指令、31个call、189个跳转、118个局部标签、27个`retn`，没有外部`FUNCTION CHUNK`。

31个callsite完整归类为：1次已关闭热点命中查询、6次既有样本播放窄边界、3次已关闭TSW命令流像素命中、1次选项角色资格查询、6次角色选择配置、1次组B候选查询、3次角色原点准备、3次角色surface解析、3次角色镜像查询、1次组B模式查询和3次组A候选查询。前三类直接复用已关闭typed语义；其余八类尚未关闭callee统一归单一帧输入typed端口。

函数没有参数。唯一caller是战斗逐帧协调器。入口EAX/ECX/EDX只在若干早退路径保留；函数按路径返回0或1，完整ECX/EDX会直接成为相邻逐帧输入分派的入口寄存器。

## 2. 鼠标入口门与热点owner

入口先保存当前鼠标X/Y。只有当前坐标同时等于两项战斗前帧坐标，且pre-frame gate B为0时立即返回EAX 0，ECX/EDX保持caller入口值。

坐标改变时先把pre-frame gate B写1、独立pointer activity gate写0；同坐标但gate B非零则保留原gate。该门与右侧按钮/目标命中使用的mouse action gate是两个物理全局，不能折叠。两项战斗前帧坐标随后无条件更新为当前值。

原`0x004C8BEC`是选择热点链头，不建立第二份标量owner。现代直接以热点vector非空作为同一条件，调用已关闭严格开区间命中查询；命中时把零基索引和非零选择owner token发布到输入共享状态，miss发布零token。查询后按原时点重读live鼠标。

消息值大于30时在switch范围检查处返回0并保留此前ECX。0到30先按权威31-byte间接表装载ECX；只有case 0、1、2、3、4、5、8、27、30进入有效分支，其余值统一默认返回0。

## 3. case 0、1与30

case 0先以组A排除低byte和组B数量作unsigned比较，再要求Y严格位于`0x182..0x1E0`。组Acount按i32 signed正数循环，依次读取启动期紧凑party source映射和八项横向offset；X严格位于`offset-0x18..offset+0x74`时发布`index+8`低word并返回1。无命中或非正count把selected option写全1并返回0。映射与offset只在首次真实访问typed-stop。

case 1要求active actor非零。面板矩形严格使用live origin：X为`origin+0x0A..origin+0x76`，Y为`origin+0x28..origin+0x88`；按`0x36`列宽和`0x18`行高计算八项索引。索引必须signed小于`startup extra+5`，permission byte非零。索引4以上还以`active-8`组A对象token和物理选项role id调用资格callee，并在callee后动态重读permission。有效项变为一基selection，变化时播放样本`0x2E`，再按extra+5夹上界并发布双gate。

case 1无效矩形先清mouse action gate与selected option，再按case 0同一party owner遍历全部成员；每个命中都覆盖selected option，不提前退出，最后仍返回0。

case 30按三列、每列五行严格矩形计算`5*column+row+1`，变化时播放样本，先发布live值再以signed比较夹到10。完全miss清mouse action gate和selected option全1。

## 4. case 2、4、5、8与27

case 2先把equipment hover写全1；Y严格位于`0x82..0xA0`时按`0x2A`列宽扫描，再发布hover并置pre-frame gate B。主列表要求X严格位于`0xE0..0x180`，以`0x14`行高计算一基行；`panel scroll+row`与BDF2低byte的i8符号扩展作signed比较。有效变化播放样本并发布列表选择、interaction 0与双gate。

case 4与27使用同一十项hover/列表框架，但横向上界分别为`0x19C`和`0x194`；一基行加scroll后与BDF4的u16零扩展作signed比较。列表miss先清mouse action gate；record15 held按i32 signed大于等于1时立即返回。否则清menu action和三项临时选择，只有BDF4 unsigned大于7才处理右侧四个严格按钮。

case 2、4、27的右侧按钮分别保留不同X/Y边界。命中依次发布interaction 1、2、3、4；完全miss严格按原路径清interaction、menu action与mouse action gate。

case 5在X`0xC4..0x178`内按`0x16`行高选择两行；case 8在X`0xE0..0x199`内按`0x18`行高选择七行，并把一基行与BDF3低byte的i8符号扩展作signed比较。变化时均播放样本，成功发布双gate，miss只清mouse action gate。

所有矩形使用unsigned严格开区间；边界点不命中。加法、减法和选择值均保留u32回绕。

## 5. case 3组B目标扫描

case 3先检查目标阻断dword、目标抑制byte和阻断word。随后按`active*5-40`的u32回绕索引首次读取启动reset物理表；越界在该访问typed-stop，入口鼠标副作用已经可见。

表值为0时选择组B路径：

1. 以i32 signed动态组Bcount正向遍历，每个对象调用配置mode 0；callee后重新读取count，不增加modern上限。
2. 从`count-1`按i32 signed逆向扫描对象token。
3. 候选查询完整EAX不等于1时，依次准备两项原点、解析surface；只有旧surface数据指针非零才把目标动作可用写1并进入像素扫描。
4. 外层与内层都按`0,2,4,6`。每个点先动态查询镜像；普通点为`mouse+(inner,outer)`，镜像X为`width+2*origin_x-mouse_x-inner`。
5. 每点直接调用已关闭TSW命令流像素命中；短源只在该helper真实读取点typed-stop。零命中当点清mouse action gate，非零命中立即发布`candidate+1`、selected target、配置mode 1和双gate。
6. selection等于6时再查组B模式；完整EAX为0把目标动作可用清零，但不撤销目标发布。

无候选返回0并保留此前角色配置和像素查询副作用。

## 6. case 3组A目标扫描

启动表非零时先按i32 signed动态组Acount对全部组A对象配置mode 0，再对live selected target对象补一次mode 0。随后计算：

```text
remaining = group_a_count
remaining -= startup final_subtract_word
remaining -= startup supplemental_count_word
```

三步均为u32回绕。

remaining unsigned不小于4时，从`remaining-1`按i32 signed逆向读取最终角色owner的十项actor order。每个actor index先真实读取组A对象`+0x2B00/+0x2B04`共享完成槽；任一精确等于1跳过，否则查询组A候选。order或完成槽只在实际访问typed-stop。

有效候选执行8×8逐点扫描。若候选正是`active-8`且selection不等于2或3，可见像素不能发布，但原程序仍继续剩余点；modern不提前退出，完整保留最多64次镜像与像素call。扫描后清live selected target marker；鼠标Y位于闭区间`0x16A..0x1B6`且X位于actor offset闭区间`offset..offset+0x7C`时，可绕过像素发布该actor并置marker。order耗尽直接返回0。

remaining小于4时改按`group_a_count-1`直接逆向扫描组A对象。可见目标同样服从当前actor排除；成功发布后把marker前四byte作为一个物理dword清零。扫描耗尽后先清live selected marker，再在同一Y闭区间正向扫描紧凑party source：完成槽按紧凑索引访问，X offset与marker按source索引访问，发布actor code为紧凑索引加1。

marker、source、offset、actor order和组A完成槽分别只在首次原始访问typed-stop，已完成的角色配置、候选callee、像素查询及前缀写不回滚。

## 7. caller回收与全局重置

唯一逐帧caller删除最后一个前置opaque stage并直连本typed实现。音乐查询/提交留下的完整EAX/ECX/EDX进入本函数；普通返回的ECX/EDX直接进入相邻逐帧输入分派。typed-stop保留音乐与本函数前缀，阻断输入分派、角色预处理、metric、surface和全部后续帧。

状态复用：当前鼠标来自输入归一化owner；party source与offset来自启动owner；permission、extra、启动模式表和两个减数来自启动/reset owner；组A数量与组B数量来自metric owner；active、published、actor order与组A完成槽来自最终角色owner；selection、interaction、mouse action、selected option、热点token与样本混音来自输入分派owner；热点链只保留vector owner。

新增状态只承载此前未命名的战斗前帧鼠标、独立pointer activity、菜单几何/行选择、阻断值、目标索引、十byte marker及边界。全局重置只同步权威LST实际写入的前帧鼠标、列表初值、当前equipment、scroll、origin和三项阻断值；未在reset写集合中的hover、行限制、边界、marker和其他选择保持入口值。

## 8. 验证与动态差分

定向测试覆盖：同鼠标早退寄存器；热点首命中；case 0 party映射；case 1 permission与样本；case 2按钮；case 4 signed负held；case 5/8行高与signed byte；case 27独有按钮边界；case 30网格；组B逆向像素命中和selection 6模式；组A直接命中及首marker dword清零；同active不可选时完整64点调用；actor order、启动模式和图像短源typed-stop；全局重置别名；逐帧caller阻断。

当前缺少原版鼠标/菜单全局、八类未关闭角色callee、两组角色对象、surface记录、TSW命令流、热点链、样本及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
