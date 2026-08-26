# 战斗强度衰减效果帧 `0x00459BF0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整LST范围

权威函数为`0x00459BF0..0x00459D04`，从proc到endp完整130行、4个静态call站点、4个`loc_`标签，无外部FUNCTION CHUNK。4个唯一callee为角色坐标查询、效果record初始化、resource owner查询和resource绘制。

唯一caller是已关闭八槽效果记录帧协调器`0x004582B0`中的`0x00458D49`。该caller关闭时曾暂留单一opaque调用；本工作包将其删除，直接组合typed子状态。

## 2. 入口先后顺序

四个cdecl参数依次为actor token、source value、secondary value和slot index。

入口严格按以下顺序：

1. source完整dword为0时立即返回1；
2. 此时不读取slot，不读取强度byte，不调用任何callee；
3. source非0后才首次按slot读取强度byte；
4. slot越界在该首次真实读取typed-stop；
5. 强度按i8小于等于-32时只清该byte并返回1。

source-zero和阈值返回都保留caller进入时的EDX；唯一caller在call前最后把secondary value装入EDX，因此typed结果显式返回该完整dword供caller后缀继续使用。

## 3. 坐标与record初始化

强度大于-32时先调用actor坐标查询。原始调用把arg-C地址作为第一个输出指针、arg-4地址作为第二个输出指针，因此第一输出是X，第二输出是Y。

坐标callee返回后才写当前槽152字节record：

- `+0x00`写原始source；
- `+0x08`写secondary value；
- `+0x90`先清0，global mode完整值等于1时再写1。

record物理token为`0x00524980 + slot * 0x98`。初始化callee完整EAX为0时直接返回该EAX，不查询resource、不发布强度、不绘制；返回EDX保留初始化callee完整EDX。

## 4. lookup陈旧高word

初始化成功后，两项lookup参数不是普通u16：

- 参数一保留初始化callee EAX高word，只把AX替换为record `+0x4A` key；
- 参数二保留初始化callee EDX高word，只把DX替换为record `+0x4C` key。

resource查询返回owner token。owner为0时，原程序随后的`[owner]`访问故障；typed实现只在该首次owner内部value访问点停止，保留坐标查询、record写、初始化和lookup全部副作用，并发布lookup callee最终EDX。

## 5. 共享强度与绘制

有效owner先把其内部value token发布到共享current resource。随后把当前强度byte按i8有符号扩展为i32，并以同一值依次发布三项共享渲染强度。

resource绘制参数固定为：

1. `signed16(X) - record +0x10完整dword`，低32位回绕；
2. `signed16(Y) - record +0x14完整dword`，低32位回绕；
3. owner `+0x0C` u16宽；
4. owner `+0x0E` u16高；
5. record `+0x18`完整render flags；
6. owner `+0x04` data token。

本函数不释放owner，也不释放owner内部value。render返回后，强度只在u8域减4并回写，随后EAX清零返回0；返回EDX保留render callee最终完整EDX。

## 6. caller回收

八槽效果协调器只在pending step完整值等于1时进入子函数，并传入：

- 既有argument object token作为actor token；
- 主record resource key token作为source；
- 主record resource auxiliary value作为secondary；
- 当前slot。

直连后的规则：

- 子port call计数累加到父结果；
- 子最终EDX替代父函数后续final-gate陈旧EDX；
- 子返回1才清主status并清pending step；
- 子返回0保留pending；
- slot或owner typed-stop立即映射到父对应typed-stop，不执行final gate；
- 端口中不再出现`0x00459BF0`函数token。

caller回归以source为0、secondary含非零高word，证明子函数零调用返回1且final gate继续消费secondary高word。

## 7. 测试与动态差分

定向测试覆盖：

- source-zero先于非法slot并保留secondary EDX；
- source非零时slot首次访问typed-stop；
- i8阈值-32清byte且不写record；
- 坐标先于record初始化；
- 初始化失败完整EAX/EDX；
- 双lookup参数分别保留初始化EAX/EDX高word；
- owner首次解引用typed-stop；
- signed16坐标、完整dword offset、u16宽高、render flags和data token；
- 三项signed强度发布、u8减4、render EDX返回和零resource释放；
- 唯一caller直连及final gate陈旧高word。

当前缺少原版八槽强度record、强度byte数组、4类callee共享副作用、resource owner、framebuffer和寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
