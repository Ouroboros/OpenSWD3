# 战斗动作帧偏移与模式位切换绘制 `0x00450A80`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与调用图

权威LST完整范围为`0x00450A80..0x00450B31`，从`proc`到`endp`共91行，没有外部`FUNCTION CHUNK`。

cdecl五参数依次为动作号、base variant、目标X、目标Y和偏移模式selector。十个caller分布在`0x0045E6B8`、`0x0045E6ED`、`0x0045E70B`、`0x0045E743`、`0x0045E77E`、`0x0045E79B`、`0x004654A9`、`0x004654E3`、`0x00465A4A`和`0x00465A84`；前六个上下文提示callsite已由其caller直接组合typed实现，并通过唯一state port复用本函数持久状态。静态callsite都传selector 0，但调用路径可依据原栈值传入1，完整函数保留selector精确等于1的分支。`0x0045E743`是唯一立即比较返回值1的callsite。

callee依次为动作更新器`0x004321E0`、帧查询`0x004315D0`和软件blitter`0x004170E0`各一次，三者均已关闭并由typed接口直连。

## 2. 单个持久动作记录与更新门

函数操作`0x004FD6D0`起始的单个0x98字节持久动作记录，不清零整个记录。入口只依次覆盖：

```text
record.action_id = arg0
record.base_variant = arg4
```

随后调用动作更新器。返回0时立即返回EAX=0；入口两项写和更新器失败前缀不回滚，帧查询、source发布、偏移计算、绘制和结果latch读取均不发生。

现代state持有同一个持续记录，复用`LegacyActionUpdater`，不误建局部记录或动作槽数组。

## 3. 更新后EDX陈旧高字与帧查询

成功更新器固定返回EAX=1。因此：

- `mov ax,[record+0x4A]`生成零扩展资源号；
- `mov dx,[record+0x4C]`只覆盖EDX低16位，帧号高16位保留更新器成功出口EDX。

现代包装显式接收`action_update_edx_snapshot`：

```text
resource_id = record.field_4a
frame_index = (edx_snapshot & 0xFFFF0000) | record.field_4c
```

不得把帧号擅自零扩展。帧查询失败时，函数在随后首次帧record解引用点typed-stop；它没有发布帧record全局，也没有发布source。

定向测试以EDX snapshot `0xABCD1234`锁定查询帧号`0xABCD0000`。

## 4. selector与X修正

帧查询后，函数读取动作X偏移和完整mode flags到局部寄存器。

selector只有完整dword等于1时进入特殊分支：

1. 本次调用的flags bit0翻转；原动作记录`mode_flags`不改；
2. `dx = low16(frame.width - low16(action.draw_offset_x))`；
3. X修正量为`sign_extend_i16(dx)`。

selector不等于1时不改本次flags，X修正量为`sign_extend_i16(low16(action.draw_offset_x))`。动作X偏移高16位在两条路径都不参与最终坐标。

最终坐标严格按低32位回绕：

```text
draw_x = low32(arg8 - signed_x_correction)
draw_y = low32(argC - action.draw_offset_y)
```

Y偏移使用完整32位。特殊路径中帧宽小于X偏移时，u16差可被解释为负数，从而使目标X增加；测试以宽1、偏移2锁定X从30变31。

## 5. source、固定tail与公共后缀

坐标准备后，函数从帧record `+0`读取并发布source到旧共享槽。软件绘制使用：

- 记录u16宽高；
- 上节校正后的X/Y；
- 原mode flags或只在本次翻转bit0后的flags；
- 第六物理tail固定0。

固定空tail由typed调用清空source palette和request auxiliary表达；indexed8帧在完整源首word与几何检查通过后于首次palette读取点得到`palette_out_of_bounds`。

`completed`、`clipped_out`或`opacity_disabled`经过通用blitter公共后缀，清目标高度、水平位移、纵向phase、opacity、RGB和跳行，保留放大位。其他typed-stop未到公共后缀，不清入口共享状态，也不读取最终结果latch。

## 6. 陈旧结果latch与返回值

软件blitter正常返回后，包装不使用callee EAX，而读取独立全局`0x004FD75C`，比较完整32位是否精确等于1，再以`setz`返回0或1。

权威LST中该全局没有直接写点，属于外部或间接维护的陈旧共享latch。typed state显式保存`result_latch`，并只在正常公共后缀之后置`result_latch_read`和计算返回值：

```text
return_value = result_latch == 1 ? 1 : 0
```

不得把modern blitter成功状态直接伪造为返回1。

## 7. 双向追溯

- `0x00450A80..0x00450AA4`：两项持久动作记录写、更新器及零返回门；
- `0x00450AA5..0x00450AB6`：EDX陈旧高字帧号、EAX零高字资源号及帧查询；
- `0x00450ABB..0x00450AE6`：selector精确1、flags bit0翻转和u16宽减偏移；
- `0x00450AE9..0x00450B18`：source发布、记录宽高、完整Y偏移、signed低字X修正和固定tail；
- `0x00450B19..0x00450B31`：软件绘制、陈旧latch精确1门及0/1返回。

C++到LST反向追溯覆盖唯一持久记录、两个入口写、更新失败前缀、两个寄存器高字合同、selector两路算术、source发布、六个blitter物理参数、公共后缀和最终返回；没有未解释基本块、callee、共享读写或出口。

## 8. 验证与动态差分

定向测试覆盖：

- 普通路径按X偏移低字signed解释、Y偏移完整32位及入口坐标绘制；
- base variant完整32位进入动作更新器；
- selector 1只翻转本次flags且不改持久记录；
- 宽1减偏移2形成`-1`修正并使X增加1；
- EDX高16位进入帧号，在首次帧解引用点停止；
- 更新失败保留入口记录写并阻断帧查询；
- 正常公共后缀后，latch 1返回1、其他值返回0；
- indexed固定空tail typed-stop保留入口共享状态且不读取latch。

battle聚合目标零warning构建及定向测试通过。

当前没有原版动作更新后EDX、持久动作记录、帧record、共享blitter状态、结果latch和framebuffer联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整91行LST、十个caller和三个关闭callee的固定状态闭环已经完成。
