# 战斗动作四百复合效果状态机 `0x00473C10`

状态：`platform_adapted`。完整LST、typed实现、唯一caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

权威LST主体为`0x00473C10..0x004745A4`，proc至endp共998行、604条实际指令、24个call、51个跳转、36个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。唯一caller位于动作dispatch的动作400分支；this为选中group-A行动者，两个栈参数为选中group-B目标token与固定零记录索引。函数严格只在完整收尾返回一时进入caller后续目标发布、效果分数、画面提交和延迟清理。

入口先按word检查行动者起始门，再按完整dword检查执行完成值是否精确为一；任一命中均无副作用返回零。通过后置完成latch，以profile word加1500和特殊variant配置`+0xAF0`主记录，并通过带记录、frame token、flags和坐标引用的窄port调用待审主更新。主记录field5A必须同时含低字节bit3与高字节bit0才启动冲出段。

首次冲出时按原`rep stosd`写集合清`+0xFCC`起第一条152字节工作记录，置初始化word，八个相隔十六字节的slot word写全一，两个dword清零、第三个写十二并清运动计数。主记录external mode置一；`+0x500`目标记录写同一action ID和variant加四十，并在field4C小于二时无现代上限地反复typed动作更新，任一次返回零即保留前缀返回。随后按主frame、`+0x29B8/+0x29BA`、`+0x26A0`与signed `+0x2668`执行flags加二十和加十六的双层绘制；镜像零每帧加十，非零每帧减十。目标记录field5A bit0先调用待审目标事件并破坏性清word。运动值按signed除150，余数非零立即返回；整除时清主field5A、两项计数、记录external mode，marker写全一并置阶段一。

阶段一中，目标记录完成位非零走反向双层绘制，仍使用每帧主更新发布的主frame和坐标；镜像零减十、非零加十，运动值到零时清完整阶段状态。未完成时重新配置目标记录并typed更新一次，查询frame、播放sample并清field58，保存flags与field76；镜像一翻转flags bit0并按frame宽度回绕偏移。`0x00473F5C`把canonical目标坐标写入既有`var_14`与stack `arg_4` dword槽的低word，保留两个高word，再分别按word减目标X偏移和draw Y。

第二条工作记录位于连续工作区`+0x1064`，首次访问同样清152字节、置八个全一slot与固定尾字段，再写状态word二、byte三和byte一。待审工作更新通过携带精确152字节span的窄port调用，随后重新查询目标frame并绘制。目标field5A bit3发布gate bit11、整记录清`+0x6C8`并破坏性清field5A。

gate bit11下，`+0x6C8`附记录action ID默认取行动者runtime word，但目标记录field24非零时覆盖；base variant写零。附记录未完成才typed更新、查询frame、保存并按镜像转换flags和field76，播放sample；声像严格由镜像模式固定选择正十六或负十六，并保留EAX/ECX/EDX陈旧高半。sample后又无条件翻转一次flags bit0并再次以frame宽度减当前偏移。只有双方field76/78任一非零时才按原word回绕公式组合附层坐标，否则坐标保持零。附记录field5A bit0清运动word、破坏性清field5A并调用待审目标事件。

公共尾先消费主记录field5A：bit3只发布gate bit15并清完整word；因此同word的bit0不会继续执行。bit0路径先调用目标事件，再清word、发布bit15、清运动word和`+0x630`效果记录，最后清共享motion word。gate bit15未置直接返回零。

gate bit15下，`+0x630`效果记录action ID取runtime word、external mode写零，并通过带记录、frame、flags与坐标引用的窄port调用待审效果更新。效果field5A bit0按原顺序清运动与field5A再调用目标事件。随后在原frame解引用点查询效果frame并发布共享frame token。效果未完成且gate bit14未置时，以signed `+0x29BC/+0x29BE`、当前flags、frame尺寸及资源`+0x04`绘制；bit14只抑制此层。

效果完成后先清主记录与目标记录field24；主记录完成位必须精确为一且冲出阶段必须为零才能收尾。收尾按原顺序整记录清`+0xAF0`、`+0x500`、`+0x468`、`+0x630`、`+0x6C8`，清`+0xFCC`起连续`0x4C0`字节工作区，四个目标索引写全一并清运动word。进度owner只在原`+0x2AB0`写点检查，故缺失时保留此前清理前缀。随后清进度、runtime gate、尾word，completion byte按八位回绕加一，清共享profile mode与阶段并返回一。

连续工作区使用按首次真实访问物化的`unique_ptr`唯一owner；逻辑零态不为所有行动者预占`0x4C0`栈空间，避免扩大既有大型测试栈，同时没有建立第二份物理状态。主、目标、转身、效果和附效果五条记录均各有唯一typed owner。已关闭动作更新、frame provider与目标坐标直接typed调用；尚未审计的主更新、工作更新、音频、绘制、目标事件与效果更新只保留窄port。坐标Y读取故障保留X低word写入与两槽高word，并抑制workspace/render后缀。唯一动作400 production caller已删除整函数opaque调用。

测试覆盖双入口门、第一工作记录初始化、正向十像素冲出、signed一百五十整除、同帧切换反向、目标记录更新、第二工作记录初始化与span身份、canonical坐标、caller dword参数/局部槽高word、Y故障时X部分写入及后缀阻断、sample、bit11附层、镜像两次flags变换、bit15目标事件、效果待完成绘制、资源传递、五记录与连续工作区清理、completion byte回绕、frame原访问点typed-stop、production动作400 caller及旧地址零调用。首次定向运行由AddressSanitizer定位为测试函数栈溢出；根因为连续工作区内嵌放大所有行动者状态，改为延迟唯一owner后定向测试与独立AddressSanitizer均通过且findings为零。Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`226/422 = 217 platform_adapted + 9 assembly_exact + 196 pending_audit`，SHA256为`9f51465d586ccff1e8351199c97c6b6345d3b9c4ec1ea20e9d05ceb0f8c5251d`。动态差分因原版五记录、连续工作区、两组frame、音频、多层绘制、目标事件、待审更新callee和唯一caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
