# 战斗组B行动对象资料组合 `0x00476160`

状态：`platform_adapted`。完整LST、两处caller回收、固定mode 2 callee可达路径、共享owner、typed-stop、定向/ASan/Linux门禁和inventory双生成收敛后关闭。

## 1. 完整权威范围与ABI

权威LST主体为`0x00476160..0x004761C3`，从proc到endp共48行、32条实际指令、4个call、0个跳转、0个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。四个call依次是`0x00476DB0`、import `lstrcpyA`、`0x00476A80`与`0x00478710`；机械inventory只列出三个内部callee，因此完整call集合必须从LST本体恢复。

函数是带两个栈参数的thiscall：ECX为组Bactor，arg0是行动资料值，arg4是word输出指针，callee以`retn 8`清栈。第一callee接收actor内嵌`+0x10`资料地址和arg0；其返回寄存器随后按原指令继续线程。actor映射缺失只在第一callee完成后的首次`actor+0x60`读取点typed-stop，输出映射缺失只在读取word并形成EAX/ECX/EDX后的首次输出写入点typed-stop。

## 2. 资料、文字与profile组合

第一callee把164-byte行动资料加载到actor内嵌`+0x10`。函数随后读取该资料`+0x50`的u16并发布到arg4输出；不做符号扩展，也不预验值域。

文字源仍是actor内嵌资料首字节，目标是actor `+0x2630`的16-byte物理文字区。`lstrcpyA`按原调用点保留为窄import端口；正常返回后EAX按Win32合同固定为目标token，不采信端口的任意EAX。modern owner逐byte复制至首个NUL；若16-byte目标内没有NUL，则保留已写满的16-byte前缀，并在首次未拥有目标字节处typed-stop，阻断profile和mode后缀。

文字返回后以陈旧ECX高word配上资料`+0x3E`低word，调用第二loader把40-byte profile加载到actor `+0x0D90`。profile返回EAX只由`mov ax,[actor+0x0D9E]`覆盖低word，保留callee EAX高word；该低word再按u16加到actor `+0x29A4`首个派生word并保留16-bit回绕。

164-byte内嵌资料、16-byte文字、四个派生word、行动种类、显示种类和mode flags由`LegacyBattleGroupBActionCompositionState`唯一持有；`+0x0D90`继续复用行动配置40-byte profile owner，不建立平行副本。

## 3. 固定mode 2路径

末个call固定以参数2进入尚未整体关闭的`0x00478710`。本工作包只展开该callee的mode 2可达路径：显示种类写2、行动种类写0、EAX返回1，随后caller把mode flags或入`0x80`。该路径不依赖`0x00478710`其余mode，也不把整个待审函数错误计为已关闭。

`actor+0x2A87`的mode flags归组B生命周期composition owner。动作主分派既有target phase只在组B路径借用该owner，组A路径仍持有自身本地byte，因此同一组B物理状态不存在两个typed副本。

## 4. 两处caller回收

动作主分派`0x004539B0`的case 25在已发布choice cursor、choice commit和组Bstatus后，于原`0x00454EC1`调用点直接组合typed实现。成功后才清message gate、执行第二choice操作、或入choice低word、登记攻击顺序并清current actor。任一composition typed-stop保留上述选择/status前缀与callee已发生副作用，阻断message清理、第二choice、攻击顺序和成功尾。生产源码不再执行`0x00476160`整函数opaque token。

脚本分派`0x00469D20`的case 23先把三个signed操作数分别发布到`workspace.value_a/value_b/value_c`。组B候选扫描每次递增都同步更新`value_c`；选择目标槽与两个选择门发布后，于原`0x0046AEC6`调用点直接组合typed实现。arg0来自`value_a`，actor token按`0x00525508 + actor*0x2B28`形成，输出固定发布到共享message dword。成功后才登记类型2攻击顺序、执行frame并推进cursor 8；typed-stop保留操作数、候选扫描、目标槽与选择门前缀，阻断攻击顺序、frame和cursor后缀。reserved枚举槽保留旧数值稳定性，实际调用次数为零。

## 5. 测试与验证

纯函数测试覆盖actor/output真实故障点、三个callee typed-stop、文字边界前缀、`lstrcpyA`目标EAX、陈旧ECX高word、profile EAX高word、派生word回绕、固定mode 2及三callee ABI。动作case 25与脚本case 23集成测试分别覆盖成功路径、旧opaque零调用和文字copy停止时的前缀保留/后缀阻断。新增caller用例独立注册，避免扩大历史battle聚合巨型测试函数的ASan栈帧。

最终`./build-asan.sh`、`./build.sh core`和`./build.sh app`分别完成AddressSanitizer core `188/188`、Linux core `188/188`与Linux app `194/194`；零OpenSWD3源码warning、sanitizer finding或测试失败。inventory生成器连续双跑逐字节一致，关闭进度为`245/422 = 236 platform_adapted + 9 assembly_exact + 177 pending_audit`，SHA256为`487e433f3e903148309c0fed9d1b330af42c3d851b0e0c8c95f6e7e658b8db4c`。

当前缺少原版八个组B完整actor、内嵌164-byte资料、40-byte profile、三个callee、文字复制及两个caller寄存器联合捕获后端，因此`original_diff_verified`登记为`blocked_runtime_oracle`。
