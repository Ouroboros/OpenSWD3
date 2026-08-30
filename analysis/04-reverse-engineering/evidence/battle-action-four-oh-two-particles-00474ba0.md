# 战斗动作四百零二八向粒子序列 `0x00474BA0`

状态：`platform_adapted`。完整LST、typed实现、唯一caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

权威LST主体为`0x00474BA0..0x00474E57`，proc至endp共294行、187条实际指令、10个call、14个跳转、10个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。唯一caller位于动作dispatch的特殊动作402分支；caller先把frame effect三色系数写负十二，并置primary suppression与alternate surface mode，再以选中group-A行动者为this、选中group-B目标token为唯一参数调用本函数。严格只在返回一后继续通用目标发布、效果分数、画面提交与延迟清理。

函数没有起始门。入口把两个坐标局部dword清零，以profile word加1500和特殊variant配置`+0xAF0`主记录，置完成latch，并通过带记录、frame token、flags与坐标引用的窄port调用待审主更新。

主记录field5A bit1处理可选转身记录：field24非零时发布runtime gate bit14，并把field24与field28复制到`+0x468`记录action ID和base variant；field5A bit9存在时把主记录external mode写一。随后只清bit1，并破坏性清field24与field28。gate bit14存在时以转身记录和主field78调用待审逐帧更新；只有返回一才整word清主field5A、清external mode并清gate bit14。

主field5A低字节bit0或bit3任一置位时，函数发布runtime gate bit15并整word清field5A。随后先调用目标坐标callee写两个dword局部；行动者`+0xD94` bit1置位时在坐标callee之后把两个完整dword清零。待审坐标更新callee通过两个dword引用接收并可原地修改。事件末尾把当前`+0x2F24` word零扩展写入`+0x2F0C` dword，再令原word按十六位回绕加一。

gate bit15未置直接返回零。置位时先令`+0x2958` word回绕加一，再把其按signed i16除三并比较余数是否精确为一；负值保持x86向零截断余数。只有余数一且`+0x2A80` unsigned word小于八才生成粒子。生成前再次独立执行目标坐标与可选归零，再调用同一坐标更新callee。

方向索引取`spawn_count & 7`。权威只读表X为`0,0,-120,120,-70,70,-70,70`，Y为`-100,100,0,0,-50,-50,50,50`；两表值按signed word加到行动者position X/Y并保持32位回绕。粒子callee有九个栈参数，依次为行动者runtime word、当前sequence dword、源X、源Y、目标X、目标Y减四十、固定一、固定五十二、固定零。实现使用专用九参数请求，避免把第九参数丢入既有八槽opaque请求；spawn count以引用交给待审callee承接其this副作用。

粒子callee后固定调用待审提交callee参数零、零、十二，再播放sample六十二。无论本帧是否生成粒子，gate bit15路径最后都以目标token和当前sequence dword调用待审完成callee，并携带主记录引用承接完成位副作用。返回非一直接返回零；返回一先整dword清runtime gate，再检查主记录field8C是否精确为一。未完成时保留计数与记录返回零。

双完成门满足时，按原顺序清tick word、sequence dword、sequence count word，整记录清`+0x630`效果记录和`+0xAF0`主记录，并返回一；spawn count不在本函数清理。实现只新增`+0xD94`坐标抑制、`+0x2A80`spawn count、`+0x2F0C`sequence dword与`+0x2F24`sequence word唯一owner。主更新、转身更新、坐标查询、坐标更新、九参数粒子、粒子提交、sample与完成更新保留窄port。特殊动作402唯一production caller已删除整函数opaque调用。

测试覆盖field24/28转身转移、bit9 external mode、bit14完成门、bit0/bit3事件门、事件与生成两次独立坐标查询、坐标callee后抑制归零、sequence发布与word回绕、signed三帧节拍、spawn count小于八门、八向第一组偏移、九个参数全部顺序、粒子提交、sample、完成callee非一保留、双完成门、双记录清理，以及production动作402的frame effect前缀与旧地址零调用。定向测试与独立AddressSanitizer均通过且findings为零；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`229/422 = 220 platform_adapted + 9 assembly_exact + 193 pending_audit`，SHA256为`89762014c855c7bfb77ca3bbfe5719f90ae3b4233fb8b328f3d794cfaf3f7bdc`。动态差分因原版主/转身/效果记录、坐标更新、八向粒子、sample、完成callee和唯一caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
