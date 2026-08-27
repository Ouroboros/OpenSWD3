# 战斗菜单分页前进 `0x00461A30`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整范围

权威LST主体为`0x00461A30..0x00461C00`，从proc到endp共210行、112条实际指令、3个call、17个跳转、12个局部标签、5个`retn`，没有外部`FUNCTION CHUNK`。

入口读取完整message后清pre-frame gate B，再依次减2、减2、减23，只接受消息2、4、27；其他值以`message-27`低32位返回并保留入口ECX/EDX。三个有效case各把sample mix装入EAX后播放一次既有选择样本。

## 2. 消息2列表分页前进

样本返回后只把CL替换为panel row limit A，并按i8作`limit < 7`判断。该signed byte早退保留样本EAX/EDX以及ECX高24位，既不置mouse action gate，也不修改selection或scroll；`0x80..0xFF`因此同样早退。

可分页时再把menu action装入EAX。menu action为0且list selection不等于7时只把selection写7，EAX仍为0，ECX仍保留样本高24位与limit低byte。其余路径把panel scroll A加7，EDX保留新page再加7的边界；若该边界signed大于i8符号扩展后的limit，则把page改为`limit-7`。page signed为负时只把共享scroll夹0并把list selection写sign-extended limit，EAX仍保留负page。所有非早退分页路径最后置mouse action gate。

## 3. 消息4装备网格分页前进

样本后以menu action和原grid selection判断是否先归一。menu action为0且grid不等于7时，selection写`min(7, u16 panel row limit C)`；随后加载current equipment索引和panel scroll B，先置mouse action gate，再把新grid与原scroll分别写入两份唯一equipment缓存。

其余路径把panel scroll B加7，并以signed `new page + 7`和zero-extended u16 limit比较；超界时page改为`limit-7`。page signed为负时EAX和共享scroll清0，grid及EDX改为limit。随后加载current equipment索引、置gate，并把EDX中的grid和EAX中的scroll依次写入缓存。正常返回EAX为page、ECX为equipment索引、EDX为grid。

## 4. 消息27网格分页前进

menu action为0且grid不等于7时复用消息4的归一和双缓存副作用，这是权威控制流的非对称共享路径，不能现代化消除。

其余路径把panel scroll B加7，EDX保留加页后的下一边界，EAX为u16 limit。边界signed超限时ECX改为`limit-7`并写回；ECX signed为负时共享scroll夹0且grid写limit，但返回ECX继续保留负值、EDX继续保留夹取前边界。最后置mouse action gate，不写equipment缓存。

## 5. typed-stop与caller回收

current equipment索引仅在两次真实数组store处检查。首项停止保留样本、selection、scroll、gate及完整EAX/ECX/EDX；若未来物理owner使第二项先越界，则第一项写前缀不回滚。

唯一caller为逐帧输入分派，原有两处调用分别位于interaction mode 4和record8三帧重复分支。两处均已直连本typed实现；稳定操作枚举原值保留reserved槽，不再发出opaque call。普通返回继续原鼠标后处理或后续输入记录，typed-stop阻断调用点后的鼠标发布与最终输入提交。

函数复用第110–113项同一message、pre-frame gate、menu action、mouse action gate、list/grid selection、panel scroll、row limit、current equipment和双equipment缓存owner。审计同时修正第113项样本call的预调用EAX：分页前进和后退都先装sample mix，再进入共享sample port。

## 6. 验证与动态差分

定向测试覆盖：默认寄存器；消息2的i8早退、CL局部写、128负byte、selection归一、普通页、上界夹取和u32绕回负页；消息4的归一共享副作用、普通缓存发布、短网格夹取和首store typed-stop；消息27的归一共享副作用、上界夹取和负ECX；interaction mode 4与record8两处caller直连及typed-stop传播；第113项sample mix预调用EAX。

当前缺少原版菜单/分页全局、样本后端、两处caller输入记录与EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
