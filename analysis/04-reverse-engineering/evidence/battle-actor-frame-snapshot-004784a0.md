# 战斗角色当前帧边界查询 `0x004784A0`

状态：`platform_adapted`、`unit_tested`、`caller_reclaimed:9/15`、`pending_remaining_callers`。typed leaf与`0x0045FC60`、`0x00460C40`、`0x00461240`共九个物理callsite已落地；目标刷新和选择标记的六个callsite按REVIEW 3继续保持待回收。

## 1. 完整LST范围与ABI

权威函数为`0x004784A0..0x0047859B`，跨度252字节，共68条实际指令、2个静态call、6个条件分支、1个无条件跳转和1个`retn 4`。没有范围外`FUNCTION CHUNK`、中段入口或额外返回点。

入口`ECX`为actor token；唯一栈参数是连续四个dword输出槽的首地址，callee清理4字节参数。函数分配并清零完整`0x98`字节临时`LegacyActionRecord`，保存并恢复ESI/EDI。两个callee依次为：

1. `0x004321E0`：已关闭的`LegacyActionUpdater`；
2. `0x004315D0`：已关闭的`LegacyFramePieceProvider`边界。

## 2. 前置门与临时动作记录

函数按以下顺序执行：

1. 清零完整临时动作记录；
2. 读取actor完整dword `+0x2AB8`；
3. 仅当其等于1时读取完整dword `+0x2AA0`，为0则提前返回；
4. 读取actor `+0x2A0C`低word，为0则提前返回；
5. 将该word零扩展后写临时记录`base_variant`；
6. 临时`draw_offset_x`先写`0x24`，actor `+0x02A8`完整dword非0时覆盖；`+0x2AB8 == 1`时再强制覆盖为`0x33`；
7. 调用`LegacyActionUpdater`，不根据其返回值或status增加现代早退。

动作记录的`action_id`保持0，因此现有typed updater按原路径返回1且不加载动作流。调用本身仍必须保留，不能折叠成常量或绕过已关闭callee。

## 3. 重叠dword读取与帧查询

updater返回后，函数先读取临时记录`+0x4C`起始dword，再读取`+0x4A`起始的unaligned dword。两次读取物理重叠；随后按先push `+0x4C` dword、再push `+0x4A` dword的顺序调用`0x004315D0`。frame provider只消费两个实参的低16位，但typed实现必须保留完整dword读取、重叠关系和调用顺序。

frame provider返回的token先保留在EAX。函数随后读取actor完整dword `+0x2B08`并与1比较，再把frame token写入actor `+0x254C`。因此mirror读取与CMP先于frame-token提交；写入失败不能伪造为已提交。

mirror完整值等于1时，函数读取frame `+0x0C`宽度word并计算`width - local_x`；其他值直接使用`local_x`。不能把非零值都当作mirror，也不能缓存frame token跨越后续可能别名写入。

## 4. 四项输出与寄存器

成功结果严格为：

```text
out[0] = sign_extend(word(actor + 0x0D66)) - adjusted_local_x
out[1] = sign_extend(word(actor + 0x0D68)) - local_y
out[2] = zero_extend(word(frame + 0x0C))
out[3] = zero_extend(word(frame + 0x0E))
```

四项都是完整dword写入，顺序固定为X、Y、width、height。输出首地址只在第一次位置减法后读取；不能预读参数、预清输出或把四次store合并成一次结构赋值。

X写入先于Y源读取；Y写入先于第一次从actor `+0x254C`重载frame token；width写入先于第二次重载。后续typed-stop必须保留已经提交的完整dword前缀。输出槽与actor frame-token字段别名时，后续重载必须观察刚写入的新值。

成功返回时：

- EAX为`0x0047855A`加载的输出首地址token；
- EDX为第二次从actor `+0x254C`重载的frame token；
- ECX为零扩展后的最终height；
- flags来自读取height前的`xor ecx,ecx`。

两条正常提前返回分别为：

- `+0x2AB8 == 1 && +0x2AA0 == 0`：EAX为0，ECX为1，EDX保持入口值，flags来自`test eax,eax`；
- `word(+0x2A0C) == 0`：EAX低word为0；经mode 1路径时高word保留`+0x2AA0`高word，其他mode从入口清零路径保持高word 0；ECX保留完整mode，EDX保持入口值，flags来自`test ax,ax`。

## 5. canonical owner

不得创建平行actor数组。typed view从既有owner组合字段：

- Group-A token：坐标和`+0x02A8`复用startup party坐标owner；`+0x2AB8`复用同一party的progress；`+0x2AA0`归入同一party的configuration；`+0x2A0C`、`+0x254C`和`+0x2B08`复用action dispatch的Group-A action-execution owner；
- Group-B token：坐标、`+0x02A8`、`+0x2A0C`、`+0x254C`和`+0x2B08`复用startup持有的lifecycle action-execution owner；`+0x2AA0/+0x2AB8`复用同一element的action-configuration owner。

调用方统一借用frame coordinator既有`LegacyActionUpdater`与`LegacyFramePieceProvider`。缺失owner、无效token或字段不可访问时，只在对应真实读取/写入点typed-stop，不提前做整体对象有效性判断。

生产实现位于`legacy_battle_actor_frame_snapshot.hpp/.cpp`。action updater与frame provider的现代接口不携带全部寄存器/flags，因此调用方另行提供两段callee的原ABI残值；provider EAX token按原时点提交到actor `+0x254C`，不再伪造成actor字段地址。typed request分别控制`+0x4C/+0x4A`两次重叠local dword读取、输出指针、四次输出写、两次frame-token重载和frame宽高访问；已提交输出通过32位地址token与actor frame-token字段发生别名。

## 6. 15个物理callsite

LST记录5个caller、15个物理callsite：

- `0x0045FC60`：`0x004605D2`、`0x004607D9`、`0x00460A03`；三处共用同一组四dword局部槽，分别服务Group-B命中测试和两条Group-A命中路径；成功后继续`0x00478620`表面解析；
- `0x00460C40`：`0x00460E27`、`0x00460F63`、`0x00460FEA`；菜单选择后退的Group-B、Group-A大列表和Group-A小列表路径；输出未消费，但动作更新、frame lookup和actor frame-token提交可观察；
- `0x00461240`：`0x00461469`、`0x004615A4`、`0x0046162C`；与后退路径对称的菜单选择前进三处；
- `0x00462740`：`0x00462E1A`、`0x00463623`；两条目标选择刷新路径共用局部输出块，成功后分别设置输入门，第二处再设置message 3并prime输入；
- `0x00464270`：`0x00464840`、`0x00464905`、`0x00464A72`、`0x00464B1D`；Group-B遍历标记、Group-A遍历标记、当前Group-B目标和当前Group-A目标四处，实际消费原点与宽高绘制选择标记。

frame input原`prepare_actor_origin`以及菜单前进/后退两个原点准备槽均已保留枚举ordinal并改名为reserved，生产调用数为0。尚待REVIEW 3回收的generic槽只有target runtime的`build_selection_snapshot`和selection frame的`build_actor_snapshot`；完成后同样必须保留ordinal、改名reserved并实现生产零调用。

## 7. REVIEW计划

REVIEW 1已实现typed leaf并回收`0x0045FC60`三处命中测试。它建立owner resolver、动作更新/frame查询组合、四dword共享局部块、显式provider EAX frame token提交、mirror、完整输出、寄存器/flags和逐访问typed-stop；frame coordinator把既有action、updater和provider注入frame-input路径。旧frame-input原点准备槽保留为reserved，生产零调用。

REVIEW 2已回收`0x00460C40`与`0x00461240`六处菜单选择路径。两函数通过input dispatch直接复用startup/action canonical actor owner、action updater和frame provider；三类路径各保留原actor token算术和入口EAX/ECX/EDX，把四项结果写入各caller共享局部块。即使菜单后缀不消费结果，动作更新、frame lookup和actor frame-token提交仍执行。正常早退保留局部块原值；typed-stop保留已提交前缀与返回寄存器/flags，并抑制选择配置、gate及输入确认后缀。旧两个菜单原点准备槽已改为reserved且生产零调用。

REVIEW 3回收`0x00462740`两处和`0x00464270`四处，关闭工作包。目标刷新保留两条success suffix差异；选择帧保留四处snapshot消费、中心/偏移计算、reset与render-offset调用顺序及caller寄存器/flags。最终同步五个caller证据、模块文档、生成器、inventory和主PLAN，并执行完整发布门禁。

## 8. 测试与动态差分点

REVIEW 1 typed leaf测试已覆盖：两条早退、mode非1、默认/actor/强制anchor、mirror完整值1与其他值、负位置符号扩展、四项dword结果、updater和provider调用、`+0x4C/+0x4A`重叠dword参数及两处独立local读取停点、显式frame token提交、成功寄存器/flags、每类真实读取/写入停点、X/Y/width部分提交、两次frame token重载及X/width输出与frame-token别名。

frame-input caller测试已覆盖三个物理路径：Group-B逆序命中、Group-A大列表actor-order命中与Group-A小列表直接命中；并覆盖共享局部块早退保留、reserved槽零调用、frame provider失败、输出X写typed-stop、候选查询前缀保留及surface/像素/发布后缀抑制。

菜单caller测试已覆盖前进/后退各自的Group-B、Group-A大列表与Group-A小列表六个物理路径；固定snapshot入口actor token与EAX/ECX/EDX、完整输出、updater/provider调用和frame-token提交，并覆盖两条正常早退、provider typed-stop、Y/width输出typed-stop、逐dword部分提交、reserved槽零调用、选择配置与gate后缀抑制。input dispatch测试另固定snapshot request/result透传及确认后缀抑制。

REVIEW 3仍须补齐其余六处目标/选择标记生产路径、两条目标刷新后缀、中心坐标、signed render offset和当前目标的Group-A/Group-B不同寄存器形状。

当前缺少原版完整Group-A/Group-B actor对象、动作资源、frame provider、异常内存页以及15处物理callsite联合寄存器/SEH捕获后端。原版动态差分预登记为`blocked_runtime_oracle`；该限制不阻止modern typed实现，也不能由静态测试冒充`original_diff_verified`。
