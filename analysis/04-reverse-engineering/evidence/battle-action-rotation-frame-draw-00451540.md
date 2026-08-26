# 战斗旋转缓存帧绘制 `0x00451540`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与调用图

权威LST完整范围为`0x00451540..0x004515D7`，从`proc`到`endp`共70行，没有外部`FUNCTION CHUNK`。ABI为thiscall且无栈参数，ECX指向`0x00451420`同一扩展动作状态；唯一caller位于`0x00453580`。

callee只有动作更新`0x004321E0`和通用blitter `0x004170E0`各一次。typed实现复用前一工作包的`LegacyBattleActionRotationCacheState`与动作更新端口，直接消费已建立的三个owner/frame缓存，不创建平行record模型。

## 2. 零动作门

入口先读取扩展状态`+0xC0`的u16动作号。若等于0：

- 不写动作record；
- 不调用更新器；
- 不访问缓存；
- 不改共享水平位移；
- 返回EAX 0。

测试以非零入口水平位移证明该门不执行任何清理。

## 3. 动作更新返回值被忽略

动作号非零时，函数把它零扩展写到`action_id`，把`base_variant`清零，再调用动作更新。

与多个相邻绘制helper不同，本函数不检查更新后EAX。更新返回完整0仍继续使用更新器写入的record字段。测试用EAX 0完成正常缓存绘制，锁定该差异。

## 4. 三owner缓存索引与source发布

更新后先以`xor eax,eax; mov ax,[record+0x4C]`得到零扩展u16帧索引，再访问`+0x9C+index*4`。

现代只允许索引0、1、2；其他值在首次owner槽访问处typed-stop，不发布source、不写共享位移。有效槽owner为0时，在原`mov edx,[ecx]`解引用点以`cached_owner_invalid`停止；动作更新前缀保留。

owner有效时，从对应typed frame record发布source。初始化工作包已把查询返回的owner token和frame record同时存入同一槽，因此caller/callee边界现已直接闭合。

## 5. 共享水平位移

source发布后，函数把扩展状态`+0xBC`完整dword写入共享`0x004CD724`。现有blitter映射证明该全局为`horizontal_resample_displacement`，modern以bit pattern解释为i32写入共享request。

这个写入发生在owner首次解引用之后、第二次owner/frame record读取之前。owner失败不改位移。

## 6. 坐标、flags与固定空tail

绘制参数为：

```text
X = low32(field_b4 - action.draw_offset_x)
Y = low32(field_b8 - action.draw_offset_y)
width  = cached frame u16 width
height = cached frame u16 height
flags  = action.mode_flags
tail   = 0
```

减法保留低32位回绕。frame索引在读取flags前再次从record u16获得；typed实现使用同一更新后snapshot，因为中间无可变callee。

第六物理参数固定为0，不使用缓存frame palette。modern同时清空call-local source palette和request auxiliary。indexed缓存因此在首次palette读取处typed-stop，保留source发布与共享水平位移，不执行公共后缀、不返回`field_8c`。

## 7. 公共后缀、显式清零与返回

accepted blit才执行通用公共后缀：清target height、水平位移、纵向phase、opacity、RGB和跳行，保留放大位。随后原函数再次显式把`0x004CD724`写0；typed实现保留这次幂等清零。

最后读取并返回动作record `field_8c`完整dword。该返回发生在blitter和显式位移清零之后；任何typed-stop都不得提前发布。

测试锁定更新EAX为0仍绘制、偏移坐标、实际像素、公共后缀、显式水平位移0及`0xCAFEBABE`完整返回。

## 8. 双向追溯

- `0x00451540..0x00451552`：u16存储动作号零门与EAX 0返回；
- `0x00451553..0x00451567`：动作号/base variant写入与忽略返回的更新调用；
- `0x00451567..0x00451580`：零扩展帧索引、首次owner解引用与source发布；
- `0x00451580..0x004515A6`：共享水平位移、第二次owner/frame record和flags/宽高；
- `0x004515A6..0x004515BE`：偏移坐标、固定空tail与blitter调用；
- `0x004515C3..0x004515D7`：显式水平位移清零和`field_8c`返回。

C++到LST反向追溯覆盖70行全部基本块、两个callee、两次owner访问、共享状态时机及两个出口。

## 9. 验证与动态差分

在同一动作旋转缓存测试目标中新增覆盖：

- 存储动作号0立即返回且共享位移不变；
- 更新EAX 0仍使用更新record绘制；
- u16索引3在首次三owner访问停止；
- 有效索引但owner 0在source发布前停止；
- 固定空tail使indexed缓存保留位移并typed-stop；
- 正常偏移坐标、实际像素、公共后缀、显式位移清零和完整`field_8c`返回。

battle聚合目标零warning构建及定向测试通过。

当前没有原版扩展动作状态、更新后record、三owner/frame record、共享水平位移/blitter状态和framebuffer联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整70行LST与唯一caller已完成固定状态闭环。
