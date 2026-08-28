# 战斗HUD帧 `0x00459D10`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整LST范围

权威函数为`0x00459D10..0x0045A97C`，从proc到endp完整1363行、51个静态call站点、62个`loc_`标签，无外部FUNCTION CHUNK。24个唯一callee。

三个静态caller均已关闭：画面转场`0x004527E0`内两处、逐帧协调器`0x00453200`内一处。三个call前都额外push一个word高位陈旧参数，但本函数没有参数且以普通`retn`返回；该栈项由caller较后的统一cleanup消费，函数行为不读取它。typed直连不把该假参数引入HUD接口。

## 2. 固定字体前缀与返回域

每次调用无条件先对固定字体token执行两次配置：第一次参数0，第二次参数`0xFFFE`。角色数非正、底部条关闭时，函数返回第二次字体callee完整EAX。

角色数为正时，两轮循环的循环尾会把完整角色数重载到EAX；底部条关闭时最终返回该值。底部条开启时最终返回已关闭文字面板内部文字callee完整EAX。

## 3. 顶部队伍条

第一轮按共享signed角色数扫描固定组A对象，物理基址与步长继续为`0x005029D0`和`0x2F34`。超过十槽只在首次actor active访问typed-stop。

每个active完整值等于1的角色：

1. side mode完整值等于1时X为10，否则450；首行Y为10，只有active角色令Y加28；
2. 查询actor数据后直连已关闭文字面板，以170×20背景和显式`X+5,Y+2`文字坐标绘制姓名；
3. 查询primary current/max，以x87扩展精度计算`current/max*56`，通过已闭合向零qword转换取低dword；
4. 在`X+100,Y+8`绘制显式宽度条；
5. status低16非0时查询颜色并在`X+100,Y+12`绘制三高颜色条；
6. 当前物理index等于`actor_count-1`且top pulse为负时，pulse在u8域加3，低7位大于28则整byte清0，再绘制170×28脉冲框。

x87除零、无穷、NaN或qword越界产生整数不定值，低dword固定为0，不增加modern错误分支。

## 4. 第二轮过滤与选中脉冲

第二轮同样按signed角色数扫描十槽。两个固定skip dword任一等于1，或excluded callee完整EAX等于1时，直接跳到循环尾。

剩余角色先读当前display-order，再以映射值索引三套X表；每项越界在对应首次访问typed-stop。

`selected_actor_code-8`按低32位回绕后等于当前index时：

- 用value-X减4、固定Y394绘制124×76面板；frame和三项颜色都取selected pulse低7位；
- cadence加1，只有完整值等于3才清0并推进pulse；
- 负pulse先u8减1，低7位到0时整byte清0；
- 非负pulse再u8加1，大于8时置bit7；
- 因而8推进为`0x89`，负端回到0后同调用推进为1。

## 5. 角色名、动作框与重复查询

blocked状态不缓存，机器码在不同用途前重复查询：

1. 第一次为0时查询status低16；actor status mode为0才绘制固定宽度条；
2. 第二次为1时以颜色`0xF000`绘制名字；
3. actor status mode为1时无条件以`0xFFFF`绘制名字；选中角色再按计数模4以`0xF3F0/0xFFFF`叠绘并递增计数；
4. 第三次决定动作框末参数；
5. 固定actor值等于56时还会第四次查询，只有为0才绘制index frame。

名字token固定为`0x0049E148 + mapped*16`。动作框使用value-X与Y460，随后状态发布使用bar-X与Y462。

## 6. 固定actor值平滑

每槽保存display和target。

- 相等时先读取actor内部value指针；零token在此首次解引用typed-stop。target更新为真实值，但绘制仍消费更新前target。
- 不等时先绘制当前display，再平滑；向上差不超过10加1，否则加3；向下差不超过10减2，否则减5。
- 不等路径到真实值比较前才首次解引用actor value token，因此typed-stop保留此前绘制和平滑副作用。
- 真实值等于56且第四次blocked查询为0时绘制indexed frame。

## 7. primary数值组

primary查询返回signed i32 current/max，并以x87计算后落为float ratio。

真实current变化时：

- delta按旧snapshot减current低32位回绕并原样保存；
- 计算step时错误地测试bit27而不是符号位；置位则对本地delta按位取反；
- 之后做signed除10并加1，保留原始BUG。

保存delta小于0时加step，穿过0才清零；大于0时减step，穿过0清零，并且仅该正delta路径在current小于等于0时同步清current、delta和snapshot。

固定底图使用资源`0x2350`。display等于target时，float ratio通过x87向零转换更新target，但绘制仍消费旧target；不等时向上近端加1/远端加3，向下近端减1/远端减3。

current大于10时，严格大于`max/3`选`0x2354`，否则`0x2355`；小于等于10选`0x2356`。十进制值为保存delta加本地current，全部低32位回绕。

## 8. secondary与tertiary组

两组查询都把current/max低word按i16扩展，再计算float ratio。snapshot、delta、bit27 step和delta收敛规则与primary相同。

secondary：

- 固定底图`0x235F`，Y436；数字缓存Y439；十进制资源固定`0x2354`，Y429；
- display向下近端减1、远端减3。

tertiary：

- 固定底图`0x241F`，Y453；数字缓存Y456；十进制资源固定`0x2354`，Y446；
- display向下近端减1、远端减3。

四个静态`0x00489654`调用已由相同x87向零qword规则直接闭合，不再保留端口token；其余23类唯一callee由统一HUD typed端口发布。

## 9. 底部条

footer mode完整值等于1时：

```text
delta = signed(68 - position) / 3
position += delta
```

减法和加法按低32位回绕，除法向零。共享footer delta发布后，直连已关闭文字面板，以`position-58,354,70,24`绘制背景、以双零坐标触发`left+2,top+4`相对文字定位，并返回文字callee完整EAX。magic除法后的符号修正位、原始分子和delta分别重建为helper入口EAX/ECX/EDX。

## 10. caller回收

- 画面转场首次建帧后直连一次；mode 0第二次建帧后再直连一次，分别保留两个HUD结果。
- 逐帧协调器在固定框和可选角色面板之后直连一次；HUD内部port call计入父port call总数。
- 任一HUD typed-stop立即映射caller专用HUD stop，阻止surface操作或后续渲染阶段。
- HUD顶部与footer两个旧文字面板边界均已直连typed helper；共享动作记录复用胜利结算唯一owner，旧地址仅保留reserved常量且生产零调用。helper展开的动作更新、矩形、九宫格和文字四次服务调用计入父HUD及主帧port总数。
- 转场旧`publish_status_word`伪边界和逐帧旧`post_render_stage_0`伪边界均删除；三个caller源码不再包含本函数token。

## 11. 测试与动态差分

定向测试覆盖：空HUD返回、十槽边界、顶部左右位置、x87宽度、status颜色、top pulse、选中pulse、三次blocked与名字叠绘、actor value typed-stop时机、三组不同平滑、bit27 BUG、零分母x87低dword、footer signed步进、文字面板两种坐标分支与旧槽零调用，以及转场两调用点、逐帧直连和父级typed-stop传播。

当前缺少原版组A十对象、端口callee共享副作用、字体/文本、framebuffer、三组实时数值、映射表、actor内部value指针、x87控制字和寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
