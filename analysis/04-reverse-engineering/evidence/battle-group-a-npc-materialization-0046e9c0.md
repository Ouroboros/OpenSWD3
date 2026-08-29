# 战斗组A护援角色资料物化 `0x0046E9C0`

状态：`platform_adapted`。完整LST、typed资料物化、startup两处caller、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围与ABI

权威LST主体为`0x0046E9C0..0x0046EBA9`，从proc到endp共196行、169条实际指令、4个call、1个条件跳转、1个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。函数是thiscall并由callee弹出两个dword参数。

this是组A角色；参数0是`0x0053AF70 + index * 0x20`的32-byte护援源记录，参数1是首组A角色当前`+4`返回的56-byte调整源记录。正常尾返回ECX为角色基础记录token、EDX为新0xA4资料token；EAX高word保留角色基础记录token高word，低word来自资料`+0x60`。

## 2. 分配、清零和资料callee

函数以固定0xA4调用旧分配边界，先把返回token写入角色`+0x0C`，再以41个dword清零该资料记录。这个顺序与上一项不同：缺少角色typed owner时在资料清零前停止；分配token为零但角色owner存在时先发布零token，再在第一次清零写停止。

资料清零后首次读取护援源`+0x14`角色号，依次调用资料加载和资料内动态文字释放边界。callee完成后，源记录8个dword分别复制到角色`+0x0D50`和`+0x0D70`，源`+0x1C`另写角色`+0x2AA0`，角色号另写`+0x2A0C`。零角色号以窗口token、固定`Npc`文字token、flags零、固定源文件token和行号`0x14E`调用公共诊断，随后继续。

## 3. 调整源、signed域和低位写回

资料u16 `+0x24`按i16解释为主系数。函数从调整源读取六项基值，并以magic常量`0x66666667`执行signed向零除10，保持原低位写回：

- 调整源u16 `+0x0A`先按i16参与乘法，再把除10结果加回原低16位，写角色基础记录`+0x0A`；
- 调整源u16 `+0x26,+0x28,+0x14`按无符号基值乘主系数，分别写角色`+0x26,+0x28,+0x14`；
- 调整源byte `+0x2C`按无符号基值乘主系数，只以`add dl,cl`累加低byte并写角色`+0x2C`；
- 资料u16 `+0x2C`按i16解释为次系数，调整源u16 `+0x16`按无符号基值参与乘法，结果写角色`+0x16`。

所有word和byte结果只保留原目标宽度，不夹值、不饱和。调整源token为零时，typed-stop放在资料主系数已读之后、第一次调整源`+0x0A`访问处；此前资料callee、双份护援源复制、尾值、角色号和可选诊断均保留。

## 4. 尾部资料投影与返回寄存器

角色基础记录在主调整完成后接收资料byte `+0x90`零扩展word到`+0x1E`，再完成次系数`+0x16`调整。资料`+0x92..+0x9A`九个byte按两个dword加一个byte的原布局写到基础记录`+0x2D..+0x35`；角色`+0x2A93`镜像基础记录`+0x1E`低byte。基础记录`+0x0A`随后复制到`+4`，角色`+4`改写为基础记录token，资料u16 `+0x60`写角色`+0xF2`。

基础记录token为零时，typed-stop放在第一次调整结果`+0x0A`写处；第一项结果已计算，但基础记录保持未写。

## 5. shared owner与两处caller回收

startup两处caller分别位于随机护援分支和顺序护援分支。两者都先从首组A角色查询live `+4` token，再把新角色placement、该token和对应角色this传入本函数。现在两个callsite统一经`add_supplemental_actor`直连typed物化器，旧配置槽改为reserved且生产零调用；typed-stop阻断角色激活、镜像mode、party count、使用标记和后续护援。

首角色`+4`的owner具有原始动态别名：普通启动中它指向四份startup配置源之一；若第一个stale护援占据角色0，物化尾会把它改为角色0基础记录token，第二个护援随即读取该角色基础记录。typed caller按live token在配置源数组和首角色基础记录间解析，不复制第二份物理调整源。

## 6. 验证状态

纯函数测试覆盖负主系数、signed `+0x0A`、三项无符号word、无符号byte、独立次系数、低宽度回绕、全部尾部投影和EAX/ECX/EDX；同时覆盖NPC固定诊断及角色owner、零分配、源记录、调整源和基础记录五类停止顺序。startup回归覆盖普通双caller、旧槽零调用、配置源token解析、stale角色0别名从配置源切换为基础记录、无限重试路径，以及零调整源在资料和placement发布后停止。

验证结果：定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning，app仅有既有ALSA提示。inventory连续双生成逐字节一致，稳定为`176/422 = 167 platform_adapted + 9 assembly_exact + 246 pending_audit`，SHA256为`3f337f7a288106bd5968dc273869d2e89f36673fb52b82ab134b68ae98df41e1`。原版组A对象、0xA4资料、mon.dat加载/动态文字释放、动态首角色调整源和caller寄存器联合捕获后端缺失，动态差分登记为`blocked_runtime_oracle`。
