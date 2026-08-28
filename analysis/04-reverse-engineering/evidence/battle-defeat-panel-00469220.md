# 战斗失败提示面板 `0x00469220`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00469220..0x0046933B`，从proc到endp共119行、68条带机器码和真实助记符的实际指令、9个静态call、1个跳转、1个返回标签和1个返回点，没有外部`FUNCTION CHUNK`。9个callsite依次为动作记录更新、矩形效果、首层九宫格、标题文字、次层九宫格、面板查询、字体17、详情文字和字体16。

唯一静态caller位于已关闭消息阶段的消息103普通分支。battle调试位未置位时先调用本函数，正常返回后才递增timer并执行signed 150目标选择门；调试位路径完全跳过本函数。

## 2. 固定双层面板与ECX资源链

入口不建立局部文字缓冲，也不修改入口寄存器，先把共享动作记录写为动作`0x233B`、variant零并更新。随后把live stage加40写入EAX，矩形使用`x=196`、`y=176`、宽184、高EAX、RGB `0,4,4`和mode零。

矩形返回ECX只替换低word为共享动作资源，保留高16位后绘制`(200,180)..(376,196)`首层九宫格。标题使用CP950“戰鬥失敗”，在`260,180`以颜色`0xFFC0`和字体参数16绘制。

标题文字返回后重新读取live stage并加212写入EAX；标题返回ECX再次只替换资源低word并保留高16位，绘制`(200,212)..(376, live stage + 212)`次层九宫格。矩形或首层九宫格typed-stop阻断标题；次层九宫格typed-stop保留标题前缀并阻断查询和字体。三者均不执行现代补偿或字体恢复。

## 3. 查询、详情与返回

次层九宫格正常返回后固定查询`212,244,3`。返回EAX不等于1时立即返回查询callee的完整EAX/ECX/EDX，不修改字体。

只有查询精确等于1时，函数以同一字体对象设置字号17，再在`254,216`以颜色`0xFFC0`和字体参数16绘制CP950“隊伍全滅!!”，最后把字体恢复为16。字体17返回EAX继续进入详情文字call；详情返回EAX/EDX与重新装载的字体ECX进入恢复call。正常查询命中路径最终返回字体16 call的完整EAX/ECX/EDX。

## 4. owner、caller回收与验证

本函数不新增持久state。共享动作记录、transition stage、framebuffer、字体、矩形/九宫格资源均复用既有owner；两段CP950文字为只读常量。物理地址仅作为`compat::u32` token，不转换为宿主指针。

消息103普通路径已直连本实现；面板typed-stop保留入口cache写和面板内部画面前缀，并阻断timer、目标选择及主帧后续stage。调试bit路径仍不调用面板。旧消息103面板槽保留枚举数值并改名为reserved，生产代码零调用。主帧适配分别映射标题、查询、字体和详情四类服务，并逐项保留请求参数、文字载荷和EAX/ECX/EDX回复。

定向测试覆盖入口动作寄存器、live stage矩形与次层底边、两次ECX资源高字链、CP950“戰鬥失敗”与“隊伍全滅!!”、标题/详情坐标、查询精确1两侧、字体17→16、首层与次层九宫格stop、消息103调试旁路/普通直连/旧槽零调用/面板先于timer/子stop传播及主帧四服务映射。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码构建零warning；app仅有既有ALSA开发库CMake提示。

当前缺少原版真实framebuffer/字体/边框资源、面板查询callee、动作/矩形/九宫格/文字返回和EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
