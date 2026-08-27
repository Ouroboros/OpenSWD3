# 战斗全帧暗化步进 `0x0045BD10`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整LST范围

权威函数为`0x0045BD10..0x0045BD8B`，从proc到endp完整58行、3个静态call站点、1个条件跳转与1个局部标签，无外部FUNCTION CHUNK。

两个静态caller都位于已关闭的`0x0045E580`，调用点为`0x0045E5BE`与`0x0045E638`。两个caller现均直接组合本typed实现，并将完整EAX与1比较；只有精确返回1才进入各自结果整理路径。子typed-stop分别保留组A或组B结果latch与三颜色偏移前缀，再阻断其后副作用。

## 2. 共享颜色状态

入口读取共享channel delta，按原始顺序发布到红、绿、蓝三个完整dword偏移。三项偏移复用既有`LegacyBlitEffectState`，不建立第二份战斗专用副本；像素地址复用唯一owned `LegacyFramebuffer`。

三个颜色callee分别是已关闭的渲染函数：

1. `0x00420560`对完整`0x4B000`个u16像素执行红通道signed偏移；
2. `0x00420600`按两个u16 lane一次的packed运算执行绿通道signed偏移；
3. `0x004206F0`按两个u16 lane一次的packed运算执行蓝通道signed偏移。

完整`0x4B000`像素等于640×480画面。红通道保留末像素dword读取，因此typed实现使用同一framebuffer的只读guard；guard不计入画面、不被写回。三个callee均直接调用已关闭typed helper，不保留地址token或opaque端口。

原LST在第一个callee后重新读取绿偏移与framebuffer地址，在第二个callee后重新读取蓝偏移与framebuffer地址。三个已关闭callee均不改写这些共享全局，因此typed直连继续读取同一共享字段与同一framebuffer，不缓存独立副本。

## 3. delta推进与返回

三个颜色callee全部返回后，函数重新读取共享channel delta，按低32位模运算减2并立即写回。随后按signed dword将结果与-32比较：

- 结果严格大于-32时保留减后值并返回0；
- 结果小于或等于-32时把共享delta清零并返回1。

比较针对减后的完整dword，不是低byte或低word。`0x80000000 - 2`按低32位回绕为`0x7FFFFFFE`，随后走signed大于-32路径并返回0。

若typed framebuffer不能提供第一次完整红通道访问，停点位于三项偏移发布之后、绿蓝访问与delta推进之前；后续通道同理在各自真实调用点停下。

## 4. 测试与动态差分

定向测试覆盖固定三callee顺序、完整`0x4B000`画面首尾、红通道guard只读、三项共享偏移、-2通道效果、减后-31继续、减后-32清零返回1、`INT_MIN`低32位回绕、固定画面不足时首个红通道typed-stop及停点前副作用。

定向`1/1`、独立AddressSanitizer`1/1`、Linux core`188/188`与Linux app`194/194`全部通过。

当前缺少原版完整framebuffer、三项颜色偏移、共享delta、像素mask与caller状态机联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
