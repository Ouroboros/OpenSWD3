# 战斗逐帧上下文提示绘制 `0x0045E660`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、数据表与调用图

权威LST完整主体为`0x0045E660..0x0045E7A3`，从`proc`到`endp`共169行、92条实际指令、6个call、11个跳转、10个局部标签，没有外部`FUNCTION CHUNK`。函数尾后另有3项直接跳转表和30字节间接switch表，均已纳入行为审计。

唯一caller是逐帧协调器`0x00453200`。六个call全部指向已关闭的动作帧偏移绘制`0x00450A80`；同一次函数调用最多执行其中一个callsite。

## 2. 逐帧计数与入口消息门快照

入口先读取完整消息门dword，再对逐帧计数执行u32加1并立即写回。后续门使用入口消息门快照，不重读callee或其他阶段可能改写后的值。

递增后的计数按signed i32与300比较：

- 小于300时继续处理提示；
- 大于等于300且入口消息门bit31为0时，立即返回递增后的完整计数；
- 大于等于300但入口消息门bit31为1时仍继续；
- `0xFFFFFFFF + 1`回绕为0，并因signed比较继续进入switch。

不饱和、不清零、不增加现代上限。

## 3. 30项switch的精确映射

函数先以u32执行`message - 1`，再对结果作unsigned `>29`检查。有效1..30通过原始30字节间接表分为三类：

- 通用鼠标提示：1、2、4、5、8、27、30；
- 特殊case 3：仅3；
- 默认类：6、7、9..26、28、29。

消息0、31及其他范围外值也进入默认类。现代实现使用同一固定映射，不按连续范围或推测业务枚举重写。

## 4. 四种绘制路径

### 4.1 通用鼠标提示

七个通用case固定调用动作`0x238E`，base variant为0，X/Y读取同一当前鼠标快照，offset selector为0。

### 4.2 特殊case 3

先要求共享pre-frame gate B完整等于1；不等于1时不调用callee，直接返回switch索引2。门通过后：

- 静态资源选择值等于0时使用动作`0x2393`；
- 任意非零值使用动作`0x238F`；
- base variant和offset selector均为0，坐标使用当前鼠标快照。

原始静态资源选择初值为1，typed状态同样以1初始化。

### 4.3 普通actor鼠标提示

默认类且入口消息门bit31为0时，动态读取当前active actor：

- active actor非0使用动作`0x238D`；
- active actor等于0使用动作`0x238C`。

两路都使用鼠标X/Y、base variant 0与offset selector 0。

### 4.4 消息actor提示

默认类且入口消息门bit31为1时固定使用动作`0x23A0`：

- base variant读取共享消息辅助值完整dword；
- X/Y分别从两项共享u16坐标作i16符号扩展；
- 启动镜像模式等于0时offset selector为0，任意非零时为1。

callee精确返回1后才把消息辅助值清0；返回其他值不清。callee typed-stop发生时也不执行清零尾。

## 5. 已关闭callee直连与返回值

六个callsite全部直接组合`draw_legacy_battle_offset_action_frame`，并通过唯一`LegacyBattleOffsetActionFrameDrawStatePort`复用其持久动作记录、帧记录、source和陈旧结果latch，不为本caller建立第二份物理状态。

动作更新返回0是原callee正常零返回，本函数继续以完整0作为返回值。帧record不可用或软件绘制typed-stop时，本函数保留callee入口写、动作更新、帧查询及已发生的绘制前缀，并向逐帧caller传播typed-stop。

所有绘制路径正常返回callee完整EAX。只有两个无callee出口例外：300帧门返回递增计数，case 3门返回switch索引2。

## 6. 单一typed owner与全局重置

本函数直接复用：

- 共享battle message state；
- 动作分派状态的入口消息门、消息辅助值和两项选择坐标；
- 最终角色状态的active actor与pre-frame gate B；
- 启动状态的镜像模式；
- 输入规范化阶段发布的鼠标X/Y快照；
- 逐帧协调器已有framebuffer、clip、动作更新器、帧provider和blitter共享状态。

仅新增本函数自有逐帧计数与静态case-3资源选择值。全局重置按原固定写集合清共享message、动作消息门、active actor、pre-frame gate B与启动镜像模式；消息辅助值、两项选择坐标、逐帧计数、静态资源选择值和偏移动作持久状态不在该写集合中，保持入口值。

## 7. caller回收

逐帧协调器原结果判定后的opaque stage已删除并直接组合本函数。结果判定正常完成后立即绘制上下文提示；提示callee typed-stop保留此前音乐、预帧、角色、surface、HUD、调试叠加和结果判定副作用，随后阻断颜色初始化、最终叠加、颜色累加、临时surface与截图。

旧枚举槽保留为reserved值，避免改变后续枚举数值；typed路径不再调用该opaque槽。

## 8. 验证与动态差分

定向测试覆盖signed 300帧门、u32回绕、bit31强制继续、七个通用case、case 3门与零/非零资源选择、全部23个表内默认case、范围外默认、active actor两路、i16坐标、镜像selector、callee精确1清辅助值、不可用帧typed-stop前缀、唯一偏移动作状态及逐帧caller阻断。全局重置测试覆盖共享owner清零与写集合外状态保留。

当前缺少原版逐帧计数、30项消息轨迹、输入鼠标、动作持久记录、帧record、framebuffer、callee寄存器和全部共享全局联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
