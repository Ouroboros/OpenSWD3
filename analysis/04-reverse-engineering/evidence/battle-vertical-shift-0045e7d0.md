# 战斗逐帧纵向位移提交 `0x0045E7D0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与调用图

权威LST完整主体为`0x0045E7D0..0x0045E9B3`，从`proc`到`endp`共227行、144条实际指令、4个call、14个跳转、13个局部标签，没有外部`FUNCTION CHUNK`。

唯一caller是逐帧协调器`0x00453200`。两个直接call都指向已关闭显示surface选择器`0x00437DF0`；其余两个是返回surface的虚表`+0x14`矩形Blt。两次Blt均以等待标志`0x01000000`、固定source surface token和空效果指针执行，返回值不参与后续控制流。

## 2. 16项signed偏移表与三次动态读取

函数以共享phase完整u32作为无符号索引，读取固定16字节signed表：

```text
4,3,4,3,4,3,2,3,2,3,2,1,2,1,2,0
```

同一次调用共有三次物理读取：

1. 第一次Blt前构造首组矩形；
2. 第一次Blt后重读phase，取得framebuffer清零行数；
3. 清零后再次读取同一live phase，构造第二组矩形。

第一Blt端口可改写phase，后两次读取必须观察新值。索引16及以上只在对应实际表读取点typed-stop；此前已发生的surface提交和清零前缀不回滚。原逻辑不做现代夹值或取模。

## 3. 第一组640×480矩形

入口对phase执行原signed余2序列，即先与`0x80000001`，负值再按机器码修正；结果只用于奇偶分支，不改共享phase。

令当前signed表值为`offset`：

- source矩形固定为`{0,0,640,480-offset}`；
- 偶phase destination为`{0,0,640,480-offset}`；
- 奇phase destination为`{0,offset,640,480}`。

随后以固定owner和selector解析primary surface；零token在原`mov edx,[eax]`解引用点typed-stop。非零时提交第一组完整destination/source矩形、source token、等待标志和两个零尾。

## 4. 硬编码1280字节行清零与第二组矩形

第一Blt后重读phase与signed offset，按`offset × 1280`计算清零字节数。原程序从共享framebuffer首地址依次执行dword清零，再处理余数字节；固定表使字节数始终被4整除。typed实现按同样dword粒度检查owned framebuffer：不足一个完整dword时在该store前停止，保留此前Blt与已经完成的清零dword。

清零后再次读取live phase和offset：

- source矩形固定为`{0,0,640,offset}`；
- 偶phase destination为`{0,480-offset,640,480}`；
- 奇phase destination为`{0,0,640,offset}`。

第二次重新解析primary surface，零token保留第一Blt和清零前缀后typed-stop；非零时以完整矩形和同一等待标志提交第二次Blt。两次Blt不能合并为一次整surface复制或shader近似。

## 5. live节拍推进与尾返回

第二Blt后才动态读取节拍计数和上限：

1. 计数u32加1并立即写回；
2. 递增值与live上限按signed `<=`比较；
3. 未超过时返回live phase完整u32；
4. 超过时先把计数清0，再读取live battle mode。

battle mode的`0x100`置位时，phase写`(signed)(phase+1) % 2`的原位形；未置位时phase按u32加1。新phase精确等于10时，先保留EAX=10，再把逐帧完成门和phase清0，因此函数仍返回10。其他路径返回推进后的完整phase。

第二Blt端口可改写phase、计数、上限和battle mode；尾部全部按原call后顺序动态重读。

## 6. 单一typed owner与全局重置

本函数直接复用：

- 逐帧协调器的完成门；
- 调试快捷键状态的battle mode；
- 唯一owned framebuffer；
- 显示surface owner、primary source token与矩形surface操作端口。

仅新增phase、节拍计数和节拍上限的唯一`LegacyBattleVerticalShiftStatePort`。全局重置按原写集合清phase与节拍上限；节拍计数不在写集合中，保持入口值。16项偏移表为固定只读typed常量，不建立可变副本。

## 7. caller回收

逐帧协调器原条件为：完成门任意非零且battle mode `0x100`清零时执行普通整surface提交；完成门为0或该mode位已置时调用本函数。旧实现错误收窄为完成门精确等于1，并把本函数保留为opaque stage；现已恢复任意非零比较并直接组合typed纵向位移。

旧枚举槽保留为reserved值，避免改变后续枚举数值。子typed-stop保留颜色累加及此前全部帧副作用，随后阻断截图写入与截图请求清除。

## 8. 验证与动态差分

定向测试覆盖全部16项表值、偶/奇首尾矩形、两个surface token与等待标志、1280字节行清零、phase动态重读、第一/第二表越界、两次surface零token、dword清零边界、live节拍/上限/mode重读、0/1切换、phase 10返回后清门、全局重置别名，以及逐帧普通提交、mode强制位移和typed-stop阻断截图。

当前缺少原版phase/节拍轨迹、DirectDraw矩形Blt、surface对象、共享framebuffer地址、模式位与寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
