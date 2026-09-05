# 战斗动作二十三双层逐帧演出 `0x004721F0`

状态：历史工作包214为`platform_adapted`。工作包282正在回收坐标callee并修正栈覆盖语义；历史门禁不验证这些改动，本轮发布门尚未完成。

权威LST主体为`0x004721F0..0x00472428`，proc至endp共261行、167条实际指令、7个call、10个跳转、9个局部标签、3个返回点，没有外部`FUNCTION CHUNK`。唯一真实caller为动作dispatch的case 23；隐藏`this`是当前group-A行动者，显式参数是group-B目标token。

函数把行动者`+0x338`识别为152字节`LegacyActionRecord`。原先分离的`force_gate`、`completion_gate`、`position_adjustment`与raw primary record实际是同一物理记录的`external_mode`、`field_8c`、`draw_offset_y`，本工作包迁移为唯一typed owner，并同步既有group-A动作执行和测试。

实现严格保留动作号取`+0x2A0C`、variant 43、special mode、动作更新、frame读取与共享源发布。完成位非零时在frame发布后清零整条记录并返回一；否则按`+0x2B08`和`field_1c`执行低位翻转、bit15 latch与宽度镜像。目标坐标现于`0x004722D7`直连共享`0x004783B0`叶函数，随后保留16位回绕和signed解释。

绘制路径发布frame高度三分之一与四分之一、三项`-6/-1`共享运动值，先用frame token高半与`field_58`低半播放sample，再按signed横坐标阈值320分别保留sample callee返回EAX或EDX高半设置左右声像。最终把原mode收敛为保留bit31和低四位再置bit2/bit3的首层flags，清零sample word，以`x-5`和包含完整32位`draw_offset_y`的非对称y算术绘制首层，再以signed相对坐标和原flags绘制第二层。

## 工作包282坐标与栈别名修正

入口`0x004721F0 push ecx`的4字节空间同时充当`var_4/var_2`，分别是X/Y的WORD输出。
它们不是两个预清零局部量；读取失败时未覆盖的字节必须保留入口ECX内容。
正常绘制尾`0x00472415 pop ecx`返回`Y << 16 | X`，不返回原ECX或最后绘制callee的ECX。
动作更新失败及缓存完成路径没有执行坐标写入，仍保留原入口ECX。
`coordinate_stack_word`仅报告该局部空间的到达内容，不建立第二份持久角色坐标。

`0x004722C8`令EAX为Y输出地址；ECX在call前重装为目标token。
镜像路径`0x004722A5..0x004722BC`先清EDX、读取帧宽WORD，再减原完整draw_offset_x；
例如帧宽32减`0x00010004`得到`0xFFFF001C`，不能提前截断。
不执行镜像时EDX来自`0x004315D0`返回值，依赖其资源查询/载入路径，不能沿用当前函数入口EDX。
两个局部token与frame EDX快照现由dispatch context显式传入；未捕获的默认零不构成原程序寄存器证据。
原版frame对象、资源路径返回EDX和动态栈地址联合捕获仍为`blocked_runtime_oracle`。

门、X或Y读取停止均保留帧与镜像前缀、已经写出的WORD及到达寄存器，阻断共享绘制参数、音频、绘制和尾部pop。
根dispatch把当前行动者ECX传入同一局部空间模型，并传播停止状态，不进入完成消息处理。
新增独立矩阵覆盖两条坐标分支、镜像两侧、三个读停点、正常ECX覆盖及根dispatch停止传播。
第十四轮core定向`1/1`通过。随后补充两个不查询坐标的出口，以及缓存完成出口保留frame EDX；第九轮ASan定向`1/1`通过，日志无编译或sanitizer诊断。本工作包完整门禁仍未运行。

## 历史工作包214验证记录

历史实现中已关闭动作更新、frame provider与主record owner均typed直连；当时目标坐标、sample和软件绘制保留窄port。case 23不再调用整个`0x004721F0`地址，而是消费typed返回值后继续原消息和物品路径。测试覆盖更新失败、frame/shared/render-toggle原访问点、缓存完成清零、宽度镜像、左右声像、陈旧高半寄存器、共享运动值、双层坐标、非对称y算术及旧地址零调用。定向测试与独立AddressSanitizer均通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`214/422 = 205 platform_adapted + 9 assembly_exact + 208 pending_audit`，SHA256为`84bd7142794065107646c5ae9bd5233694e1cc2eafac5fc9362b2fbd25ac9f04`。动态差分因原版行动者记录、目标坐标、frame、sample、绘制与caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
