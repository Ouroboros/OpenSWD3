# 战斗行动者目标动作就绪 `0x004751C0`

状态：`platform_adapted`。本页以完整`swd3.exe.lst`机器码与指令为行为真值；反编译、命名、测试及未关闭callee仅用于导航。

## 完整静态范围

权威主体为`0x004751C0..0x0047555D`，proc至endp共399行、284条实际指令、15个call、16个跳转、11个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。唯一caller是动作分派`0x004539B0`的case 33 callsite `0x0045407F`；caller以group-A行动者为this、group-B目标为第一个栈参数，并额外传入函数从不读取的`0x1791`。

十个唯一callee及次数为：动作更新`0x004321E0`一次、帧查询`0x004315D0`一次、采样播放`0x00485610`两次、声像`0x00485650`一次、软件绘制`0x004170E0`两次、目标坐标`0x004783B0`一次、粒子创建`0x004800F0`两处、粒子提交`0x004801A0`两处、目标阶段推进`0x0047FC40`两处、目标刷新`0x00478780`一次。坐标callee本轮已在原callsite直接组合，其余九个边界保持不变。

## 动作与frame前缀

函数先把行动者`+0x2A0C` u16零扩展写入唯一`+0x338`动作记录的action ID，置variant `0x30`与`+0x2AAC` latch，再直连已关闭动作更新器。更新返回零时保留此前写入并返回零。随后以记录`+0x4A/+0x4C`直连帧查询；空帧typed-stop严格位于返回token已写入`+0x254C`后的首次解引用点。

frame就绪后发布唯一共享source owner，复制记录draw X低word、mode flags及行动者`+0x3AE`到`+0x29B4/+0x26A0/+0x29AC`。`+0x2B08`精确等于一时以`test bl,al`选择分支：原bit0为一只执行`and al,0xFE`，因此仅清低bit并保留EAX高24位；原bit0为零则对完整EAX执行`or eax,1`。随后以frame width执行两个u16回绕镜像，辅助X为零时保持零。frame height分别发布无符号除三与右移二；三项运动默认为负六，仅`+0x2B00 == 1`时改为负一。

## 音频、绘制与事件门

首次采样ID来自动作记录`+0x58`低word，高word继承frame token；call-entry EAX为采样级别、EDX为frame height右移二。声像分支以signed行动者X减signed镜像X，计算会把EAX覆写为镜像X、EDX覆写为差值；负十六分支仅覆写ECX低word并保留采样callee返回ECX高word，正十六分支则以采样word覆写差值EDX低word。随后`+0x26A4 = (mode & 0x8000000F) | 0x0C`并清记录采样word。

两次绘制固定使用同一frame宽高。第一次坐标为`actor X - mirrored X`与`actor Y - height/3`，flags取`+0x26A4`；第二次Y改为`actor Y - record draw Y`，flags取`+0x26A0`。全部signed读取、32位减法与word回绕保持原样。记录`+0x5A`低byte的bit0/bit3都未置时，在两次绘制和采样清零之后立即返回零，不查询目标、不创建粒子。

## 双阶段粒子状态机

事件门开启后，先清`+0x630`效果记录variant并写action `0x1BF3`，再在`0x00475415`按X后Y顺序把canonical目标坐标写入既有`var_6/var_8`两个word局部槽；两个局部量进入任一粒子调用时均由`movsx`扩展，高位坐标不得零扩展。Y读取失败保留X写入和已提交效果记录，并阻断两组粒子、目标刷新、完成sample与双记录清理。`+0x267C`为phase，`+0x2F0C`为sequence：

- phase 0且sequence 0时，第一组九参数按`action,0,actor X,actor Y,target X,target Y,0x1C,0,-1`创建粒子，提交固定`0,0,0x0C`后sequence置一；
- 第一次推进EAX为一时清sequence、phase置一并刷新目标；EAX非一仍保留callee对typed行动者的副作用；
- phase 1且sequence 0时，第二组九参数按`action,0,target X,target Y,actor X,actor Y,0x20,7,-1`创建并提交；
- 第二次推进EAX为一时清sequence、phase置二并播放`0x114`；EAX非一仍保留callee副作用；
- phase最终为二时先清phase，再按物理顺序清`+0x630`与`+0x338`两个完整`0x98`记录，`rep stosd`语义使最终EAX为一、ECX为零。

callee可在EAX非一时直接发布phase。若第一次callee把phase改为一但未清sequence，第二粒子会被陈旧sequence抑制；第二callee再把phase改为二时仍会完成双记录清理，而sequence保持一。该非现代状态必须保留。

## Typed实现与caller回收

新增`advance_legacy_battle_target_ready`，复用`LegacyBattleGroupAActionExecutionState`内已经存在的唯一物理owner、共享frame/motion owner、真实动作更新器与frame provider。目标坐标直接复用Group-B lifecycle canonical owner；音频、软件绘制和五个待审战斗callee继续使用窄端口。目标阶段推进端口接收typed行动者引用，以保留callee对phase/sequence等字段的副作用。九参数粒子请求拥有显式第九参数承载，默认generic adapter也不会截断。

动作分派case 33不再调用整个`0x004751C0` opaque地址，而是直接消费group-A当前行动者owner、group-B目标token与unused常量。只有typed target-ready返回一后才清current actor并继续目标对象访问、属性概率及后续发布；typed-stop与返回零都阻止caller后缀。

## 验证与oracle

独立回归覆盖动作更新失败、帧typed-stop、共享owner typed-stop、零行动者首访问stop、完整双阶段、第一阶段未完成、正负声像、mirror高24位、负一/负六运动、事件门关闭、入口phase一/二、高位canonical目标坐标`movsx`、Y故障时X部分写入/寄存器/flags及粒子后缀阻断、两组九参数、两个默认adapter第九参数、callee直接发布phase、陈旧sequence、各pending call入口寄存器、双记录清理顺序与终端ECX。action-dispatch回归覆盖case 33直接typed组合、零目标后缀stop及旧opaque地址零调用。

最终定向测试`1/1`、AddressSanitizer `1/1`、Linux core `188/188`、Linux app `194/194`全部通过；八份最终日志均为零warning、零error、零sanitizer finding。工作包双生成逐字节一致，为`235/422 = 226 platform_adapted + 9 assembly_exact + 187 pending_audit`，SHA256为`1e48d0179a8d157c653d0d68acb3bfaa42156ab311a4a277d547760b9ba35043`。

动态差分登记为`blocked_runtime_oracle`：当前缺少原版group-A完整行动者、真实ACT/TSW frame、音频/软件绘制、目标坐标、粒子列表、两次`0x0047FC40`副作用、target refresh与caller寄存器的联合捕获后端。该阻塞不影响完整LST静态闭环、typed实现与Linux验证。
