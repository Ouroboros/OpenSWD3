# 战斗菜单分页后退 `0x00461900`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整范围

权威LST主体为`0x00461900..0x00461A28`，从proc到endp共148行、81条实际指令、3个call、12个跳转、8个局部标签、6个`retn`，没有外部`FUNCTION CHUNK`。

入口把完整message依次减2、减2、减23，只接受消息2、4、27；其他值在清pre-frame gate B后直接以`message-27`低32位返回，ECX/EDX保持入口值。三个有效case各恰好播放一次既有选择样本。

## 2. 消息2列表分页后退

样本后把menu action读入ECX，并把EAX强制为1。menu action为0且list selection不等于1时，只把selection写1并返回；panel scroll、mouse action gate均不变，ECX仍为0，EDX保留样本callee结果。

其余路径把panel scroll A减7并先写回完整u32。结果signed非负时直接置mouse action gate；signed负值时再把共享scroll写0，但ECX刻意保留负的减法结果。EAX保持1，EDX保持样本callee结果。

## 3. 消息27网格分页后退

消息27与消息2同样先播放样本、装menu action并把EAX置1。menu action为0且grid selection不等于1时只把grid写1并返回，不修改scroll或gate。

其余路径把panel scroll B减7并先写回；signed负值时把共享scroll夹0，但返回ECX仍保留负结果。随后置mouse action gate并返回，不写equipment缓存。

## 4. 消息4装备网格分页后退

样本后先比较menu action，但无论分支都把原grid selection装入ECX。menu action为0且grid不等于1时只把grid写1并返回；因此ECX返回原grid值，而不是menu action。

其余路径以EDX计算panel scroll B减7并先写回；signed负值时把scroll写0，再从共享scroll重新装入EDX，所以后续和返回看到的是夹值后的0。严格按原顺序置mouse action gate、加载current equipment selection到EAX、写equipment grid selection缓存、写启动owner的equipment scroll缓存。

current equipment索引只在两次真实store处typed-stop。第一项越界时保留样本、scroll/clamp、gate及EAX/ECX/EDX；若第一写成功而第二写停止，第一写不回滚。正常返回EAX为current equipment索引、ECX为grid、EDX为夹值后的scroll。

## 5. caller回收与共享owner

唯一caller是逐帧输入分派，原有两处调用分别位于interaction mode 3和record7三帧重复分支。两处均已直连本typed实现；稳定操作枚举原值保留reserved槽，不再发出opaque call。

普通返回的完整EAX/ECX/EDX继续进入原鼠标后处理或下一输入记录。typed-stop保留样本、selection、scroll、gate和数组写前缀，并阻断调用点后的鼠标发布、mode 4和最终输入提交。

函数直接复用第110–112项同一message、pre-frame gate、menu action、mouse action gate、list/grid selection、panel scroll、current equipment及双equipment缓存owner，不新增第二份物理状态。全局reset映射不变，因为这些物理地址都已由既有owner按权威写集合维护。

## 6. 验证与动态差分

定向测试覆盖：默认消息寄存器；消息2先归一selection、正scroll和负scroll的陈旧ECX；消息27先归一grid与负scroll；消息4先归一grid、正常双缓存写、负scroll EDX重载和首store typed-stop；interaction mode 3与record7两处caller直连；typed-stop阻断后续mode 4。

当前缺少原版菜单/分页全局、样本后端、两处caller输入记录与EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
