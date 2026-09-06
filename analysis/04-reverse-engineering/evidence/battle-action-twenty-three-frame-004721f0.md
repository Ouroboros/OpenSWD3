# 战斗动作二十三双层逐帧演出 `0x004721F0`

状态：`platform_adapted`。完整LST、typed实现、caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

权威LST主体为`0x004721F0..0x00472428`，proc至endp共261行、167条实际指令、7个call、10个跳转、9个局部标签、3个返回点，没有外部`FUNCTION CHUNK`。唯一真实caller为动作dispatch的case 23；隐藏`this`是当前group-A行动者，显式参数是group-B目标token。

函数把行动者`+0x338`识别为152字节`LegacyActionRecord`。原先分离的`force_gate`、`completion_gate`、`position_adjustment`与raw primary record实际是同一物理记录的`external_mode`、`field_8c`、`draw_offset_y`，本工作包迁移为唯一typed owner，并同步既有group-A动作执行和测试。

实现严格保留动作号取`+0x2A0C`、variant 43、special mode、动作更新、frame读取与共享源发布。完成位非零时在frame发布后清零整条记录并返回一；否则按`+0x2B08`和`field_1c`执行低位翻转、bit15 latch与宽度镜像。`0x004722D7`按X后Y顺序把目标canonical坐标写入既有`var_4/var_2`两个word槽，随后保留16位回绕和signed解释；typed-stop保留已到达的record/frame/mirror前缀并抑制sample、绘制和清理。

绘制路径发布frame高度三分之一与四分之一、三项`-6/-1`共享运动值，先用frame token高半与`field_58`低半播放sample，再按signed横坐标阈值320分别保留sample callee返回EAX或EDX高半设置左右声像。最终把原mode收敛为保留bit31和低四位再置bit2/bit3的首层flags，清零sample word，以`x-5`和包含完整32位`draw_offset_y`的非对称y算术绘制首层，再以signed相对坐标和原flags绘制第二层。

已关闭动作更新、frame provider、主record owner与目标坐标均typed直连；sample和软件绘制保留窄port。case 23不再调用整个`0x004721F0`地址，而是消费typed返回值后继续原消息和物品路径。测试覆盖更新失败、frame/shared/render-toggle原访问点、缓存完成清零、宽度镜像、左右声像、陈旧高半寄存器、共享运动值、双层坐标、非对称y算术及旧地址零调用。定向测试与独立AddressSanitizer均通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`214/422 = 205 platform_adapted + 9 assembly_exact + 208 pending_audit`，SHA256为`84bd7142794065107646c5ae9bd5233694e1cc2eafac5fc9362b2fbd25ac9f04`。动态差分因原版行动者记录、目标坐标、frame、sample、绘制与caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
