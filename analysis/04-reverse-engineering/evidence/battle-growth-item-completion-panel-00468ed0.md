# 战斗法宝完全成长提示框 `0x00468ED0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00468ED0..0x00468FEC`，从proc到endp共128行、84条带机器码和真实助记符的实际指令、9个静态call、2个跳转、1个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。9个callsite依次为`wsprintfA` 1次、`lstrlenA` 2次、固定矩形1次、九宫格1次、法宝提示查询1次、字体大小设置2次和文字绘制1次。

唯一静态caller位于已关闭消息阶段的消息113公共完成路径。原caller固定压入两个零参数，本函数返回后重新读取timer并按u32加一。

## 2. 局部缓冲、mode门与CP950格式

入口把live seed首byte写入64-byte局部缓冲，再从byte 1开始以15个dword、1个word和1个byte精确清零剩余63 byte；EAX和ECX归零，入口EDX保留。只有transition mode精确等于1才继续，其他值返回`EAX=0`、`ECX=0`和入口EDX。

继续路径以连续CP950格式`法寶%s已完全成長!!`包装角色升级owner中的24-byte共享成长标题。格式token为`0x004A7AD4`，共享标题token为`0x0053C154`。共享标题在24 byte内缺NUL时，保留固定前缀和已复制的24 byte后在首次源越界访问typed-stop；不读取相邻输入文字工作区。格式reply达到64 byte时保留完整64-byte写前缀，随后NUL目标访问typed-stop。正常格式后第一次`lstrlenA`调用保留格式callee返回寄存器，仅把EDX改为局部缓冲token。

长度按原`cdq/sub/sar`链执行带符号除二，即向零截断。半长度记为`half`，随后以u32回绕计算：

```text
panel_base_width = half * 17 + 32
rectangle_width = panel_base_width + 12
```

## 3. 动态矩形与九宫格

矩形固定从`196,208`开始，宽度为`half*17+44`，高度为live transition stage加8，颜色参数为`0,4,4`且mode为0。矩形返回后，函数重新读取live stage到ECX，并只以共享胜利面板动作记录的`field_4a`替换矩形返回EDX低word，保留高16位作为九宫格资源高word。

九宫格范围为：

```text
left   = 200
right  = panel_base_width + 200
top    = 212
bottom = live stage + 212
```

opacity step为0，flags固定`0x80000008`。矩形和九宫格直接复用现有typed framebuffer、raster、pixel conversion、frame provider、共享blit效果和jitter owner；画面安全停止保留此前格式和矩形前缀。

## 4. 查询、字体与单行文字

九宫格正常返回后，EAX改为局部缓冲token并执行第二次`lstrlenA`；ECX/EDX继承九宫格返回。随后固定查询`212,244,3`，返回值不精确等于1时直接返回查询callee的EAX/ECX/EDX，不修改字体也不绘文字。

查询精确1时，以共享字体对象token把字体大小先设为17。该callee正常返回后，EDX改为共享framebuffer token、ECX重新装载字体token，并在`216,218`以颜色`0xFFC0`、字体参数16绘制完整局部文字。文字callee返回后再次装载字体token，把大小恢复为16；最终函数返回该恢复callee的完整EAX/ECX/EDX。

格式、两次长度、查询、两次字体设置和文字绘制通过独立窄typed端口保留七处不同的预调用寄存器布局。主帧适配使用显式格式文字和显式测量长度发布位，合法零长度不会与“未提供reply”混淆。

## 5. owner、caller回收与验证

本函数不新增持久state。24-byte标题继续由`LegacyBattleLevelAdvancementState`唯一持有；transition mode/stage、胜利面板资源、framebuffer和字体对象均复用既有owner。64-byte文字仅是本次调用局部状态，物理地址以`compat::u32`请求token表示，不转换为宿主指针。

消息113现已在法宝成长结果角色选择、sample、组A完成查询和`8,1`分配链之后直连本实现。结果选择会把共享定义scratch的标题复制到本函数消费的同一24-byte owner；正常返回后才u32递增timer。结果选择或本函数typed-stop均阻断后续timer。入口actor已存在时仍直接进入本函数，不执行前述选角链。旧选角槽与旧阶段113槽均保留枚举数值并改名为reserved，生产代码零调用。

定向测试覆盖局部seed、mode精确1门、CP950完整字节、24-byte标题源边界、64-byte格式边界、两次长度、signed half、动态矩形/九宫格尺寸和资源高word、查询非1、字体17→16、七处generic寄存器布局、九宫格stop、消息113直连/旧槽零调用/timer后置/子stop传播及主帧五类服务映射。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码构建零warning；app仅有既有ALSA开发库CMake提示。

当前缺少原版共享标题与transition联合状态、真实framebuffer/边框资源、法宝提示查询callee、字体对象、动态局部栈地址、`wsprintfA/lstrlenA`返回及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
