# `0x004763D0` 战斗组B行动十七逐帧演出

状态：`platform_adapted`、`blocked_runtime_oracle`

来源：`swd3.exe.lst`完整函数体、唯一caller `0x00455D60`、已关闭动作记录更新`0x004321E0`、帧查询`0x004315D0`、软件blitter `0x004170E0`及音频命令证据。完整LST是唯一行为真值；IDA名称、反编译输出、旧证据和测试只用于导航。

## 1. 完整主体、调用边与ABI

`0x004763D0..0x004765F8`从proc到endp共259行、158条实际指令、8个call、15个跳转、14个局部标签和3个返回点。函数没有外部`FUNCTION CHUNK`。

唯一调用边为：

```text
0x00456260  sub_455D60 -> sub_4763D0
```

caller只在调用前执行`mov ecx, esi`，因此入口只有组B source actor，没有栈参数，也不读取case的第二目标索引。caller在返回后把`EBX`设为一，并比较完整`EAX == 1`；只有相等才执行原三步行动收尾，返回零则回到公共逐帧路径。

直接callee目录为：

```text
0x004170E0  1 call  已关闭软件blitter
0x004315D0  1 call  已关闭帧查询
0x004321E0  1 call  已关闭动作记录更新
0x004785C0  1 call  待审坐标发布
0x00478600  1 call  待审坐标查询
0x00485610  2 call  已有窄音频播放命令
0x00485650  1 call  已有窄音频声像命令
```

已关闭callee直接使用typed接口；坐标和音频只保留上述窄端口，不重新引入整函数opaque边界。

## 2. 倒计时完成门

函数先按signed i32比较actor `+0x2668`倒计时与六：

- 倒计时大于六：进入逐帧演出。
- 倒计时小于等于六：从actor `+0x0468`开始精确执行`rep stosd`清零`0x26`个dword，即完整`0x98`字节动作记录，然后返回完整`EAX=1`。

完成路径的`ECX`由`rep stosd`耗尽为零；`EDX`从入口到返回完全没有被本函数写入，必须保留caller传入的陈旧完整值。倒计时只由内存比较读取，从未装入`EDX`。负数和`INT_MIN`同样走清零路径；不得把倒计时改成无符号，也不得只清动作记录中已知字段。

## 3. 动作记录更新与帧查询

逐帧路径先检查动作记录`+0x4C` word。它等于一时，以固定sample `0x10F`和共享音频handle调用一次播放命令；不等于一时不播放。

随后固定按以下顺序发布动作记录和actor状态：

1. actor `+0x2AAC` completion latch写一。
2. 动作记录`+0x00`写actor `+0x2A0C`的u16 profile值并零扩展。
3. 动作记录`+0x08`写固定variant `0x24`。
4. 动作记录`+0x90`先写零；actor `+0x2AD0`完整dword等于一时再写一。
5. 调用已关闭动作记录更新。

动作更新返回完整零时，函数立即返回完整`EAX=1`。更新callee内部可按既有合同重置动作记录字段；本函数不在返回前恢复这些字段。该早退不查询帧、不访问坐标、不递减倒计时。

更新非零后，把动作记录`+0x4A/+0x4C`两个u16键传给已关闭帧查询。查询callee最终只消费两个参数低word，因此caller在`mov ax`与`mov dx`中保留的高word不改变查询键。返回描述符token写入actor `+0x254C`；typed核心用`actor_token + 0x254C`作为非空描述符身份，并由`LegacyFramePiece`承载真实source、辅助色板及u16宽高。空描述符写零，但直到原版首次实际解引用位置才typed-stop。

帧查询后固定发布：

- 动作记录完整`+0x18`到actor `+0x26A0` render flags；
- 动作记录`+0x10`低word到actor `+0x29B4` target X offset；
- `EAX`重新载入完整倒计时；
- `ECX`为完整render flags；
- `EDX`仅低word被draw offset覆盖，高word保持帧查询后的未知值。

## 4. 倒计时十五音效与镜像位

倒计时严格等于十五时：

1. actor `+0x04C0` word写`0x2F`。
2. 播放sample `0x2F`。
3. 读取actor `+0x2B08`完整镜像mode。
4. mode等于一时，只把播放返回`ECX`低word改为`0x2F`并以声像`+16`调用；否则只把播放返回`EDX`低word改为`0x2F`并以声像`-16`调用。
5. 声像调用返回后把actor `+0x04C0`低word清零。

低word覆盖必须保留播放callee返回寄存器的陈旧高word；不能把sample参数重新零扩展。声像callee入口`EAX`是完整镜像mode，而不是播放返回值。

音效后对actor `+0x26A0`低bytebit0执行一次切换。镜像mode等于一时再切换一次，因此最终回到第一次切换前的完整flags；随后以动作记录`+0x10`完整dword判断是否进入宽度调整。该门不是低word判断：即使完整值非零而低word为零，仍必须解引用帧描述符。宽度调整只按u16执行：

```text
target_x_offset = (frame_width - draw_offset_low_word) mod 2^16
```

帧为空且镜像mode为一、完整draw offset非零时，typed-stop发生在首次`[frame+0x0C]`宽度访问。此前动作更新、帧查询、音效、flags切换和actor字段写入全部保留。

## 5. 坐标副作用、绘制参数与正常收尾

宽度调整后调用坐标查询。原版用两个栈局部地址接收X/Y；typed窄端口以稳定slot token `0/1`表示这两个输出位置，并以actor token作为`ECX`。查询返回的坐标按完整u32回绕调整：

- 镜像mode等于一：X加`0x19`。
- 其他mode：X减`0x19`。

调整后的X和原Y立即传给坐标发布callee。即使帧描述符为空，查询与发布副作用也必须先发生；随后才在`mov ecx, [frame]`首次source读取点typed-stop。

帧有效时，`[frame+0]`对应的source身份发布到共享`0x004CD730` owner；typed核心沿用稳定描述符token，同时实际绘制读取同一`LegacyFramePiece::source`。绘制六个物理参数按原顺序为：

```text
x = sign_extend(actor_position_x) - sign_extend(target_x_offset)
y = sign_extend(actor_position_y) - action_draw_offset_y
width = zero_extend(frame_width)
height = zero_extend(frame_height)
flags = actor_render_flags
auxiliary = frame_palette_or_auxiliary_bytes
```

X/Y减法按u32回绕后解释为i32。宽高只零扩展u16。第六参数是描述符`+0x04`的palette/辅助指针，不是opacity。软件blitter正常完成、裁空或opacity禁用都执行既有正常epilogue：清共享target height、横向位移、纵向phase、opacity、RGB偏移和每三行跳过门。其他主机侧blitter失败在返回点typed-stop，不伪造倒计时推进。

正常绘制完成后才把actor倒计时按32位回绕减一，并返回完整`EAX=0`。blitter后的`ECX/EDX`保持未知；函数不现代化为确定值。

## 6. typed owner与故障点

本函数不建立平行状态：

- actor动作记录、profile、倒计时、frame token、位置、镜像mode、render flags、target offset、sample word和latch复用`LegacyBattleGroupAActionExecutionState`。
- 组B actor物理槽继续由`LegacyBattleStartupState::group_b_lifecycle`唯一拥有。
- 共享frame source复用`LegacyBattleGroupAActionExecutionSharedState`。
- 动作更新、帧查询和软件blitter复用已关闭typed实现。
- raster、framebuffer、共享blitter request/effect和row jitter均借用既有渲染owner。

新增typed-stop仅位于原版实际访问或平台调用边：

- actor缺失：入口首次`[actor+0x2668]`读取。
- 帧缺失：镜像宽度首次读取，或坐标发布后的source首次读取。
- 共享owner缺失：有效frame source向共享owner发布的位置。
- blitter失败：typed软件blitter返回的位置。

所有停止点保留此前动作、音效、坐标、寄存器knownness及共享副作用；不回滚状态，也不执行原版尚未到达的倒计时递减。

## 7. 唯一caller回收

对手行动dispatcher的case 17直接调用typed实现。它只从已在入口动作查询处验证的组B source索引取得actor，不新增target索引访问。旧`0x004763D0`整函数opaque token已删除。

caller严格比较typed结果完整`return_eax`：

- `EAX=0`：保留原公共逐帧返回，不执行case 17收尾。
- `EAX=1`：依次执行原三项mode收尾，再推进overlay、处理计数及后续公共状态。
- typed-stop：保留helper此前副作用并阻断上述收尾与后续case后缀。

旧地址在生产调用和caller测试中保持零调用；音频与坐标只通过四个窄callee token适配。

## 8. 测试与动态oracle缺口

专门UT覆盖actor首访问、signed倒计时完成门、精确`0x98`清零、动作更新失败早退、首个sample、倒计时十五sample与陈旧高word声像参数、帧查询键、镜像双bit切换、完整draw-offset门、u16宽度回绕、两处不同frame故障点、坐标副作用顺序、索引色板辅助参数、16位实际像素写入、共享owner stop、blitter路径和倒计时递减。caller UT覆盖逐帧返回零、typed-stop传播、返回一收尾及旧整函数地址零调用。

最终验证：战斗聚合定向测试、完整core AddressSanitizer `188/188`、Linux core `188/188`、Linux app `194/194`全部通过；三份最终日志零OpenSWD3源码warning、零sanitizer finding。

原版组B actor、动作记录流、帧描述符、音频返回寄存器、坐标callee、软件blitter共享状态及唯一caller后缀的联合捕获后端仍缺失，因此动态差分为`blocked_runtime_oracle`。所需最小回放记录为：

```text
actor_index
actor_token
entry_eax
entry_ecx
entry_edx
turn_countdown_before
profile_value
special_mode
mirror_mode
action_record_before
action_update_return_registers
frame_query_arguments
frame_descriptor_fields
sample_call_arguments_and_returns
pan_call_arguments_and_returns
coordinate_query_outputs
coordinate_publish_arguments
framebuffer_before_hash
framebuffer_after_hash
blitter_shared_state_before
blitter_shared_state_after
turn_countdown_after
return_eax
return_ecx
return_edx
caller_suffix_calls
```

该阻塞不影响完整静态闭环、typed owner收敛和Linux验证。
