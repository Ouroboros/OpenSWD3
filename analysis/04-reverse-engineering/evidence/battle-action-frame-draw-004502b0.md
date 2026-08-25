# 战斗动态动作帧组合绘制包装 `0x004502B0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与caller

权威LST完整范围为`0x004502B0..0x004503F7`，从`proc`到`endp`共148行，没有外部`FUNCTION CHUNK`。

cdecl四参数：

```text
arg0 = 动作base variant，函数只保留低16位
arg4 = 目标锚点X
arg8 = 目标锚点Y
argC = 附加复绘selector，只有完整dword等于1时启用
```

唯一caller为`0x0045A0DB`。caller从变体表取值后只写CX，ECX高16位可能保留旧值；本函数入口立即`AND ESI,0xFFFF`，因此现代必须显式截取低16位。caller传Y=460，X来自位置表，复绘selector来自前一查询的精确1门；返回EAX被直接覆盖。

## 2. callee与直接组合

静态callee：

- 动作记录更新`0x004321E0`一次；
- 帧记录查询`0x004315D0`一次；
- 四向描边包装`0x00417050`一次；
- 通用软件blitter`0x004170E0`两次。

四类callee均已关闭。typed实现直接接收`LegacyActionUpdater`、`LegacyFramePieceProvider`和软件绘制状态，不建立重复动作解释器或opaque绘制callback。battle目标公开链接既有asset-runtime库，依赖方向无反向环。

## 3. 0x98栈动作记录

函数申请0x9C字节局部区，从`var_98`开始以`rep stosd`清38个dword，恰为完整0x98字节动作记录。它不是调用选择性initializer；所有152字节先归零。随后：

```text
action_id    = 0x2390
base_variant = low16(arg0)，以完整dword写入
```

调用`0x004321E0`。返回0直接结束，不查询帧、不读取状态表、不绘制；更新器已经对局部动作记录完成的失败前缀保留。现代state保存完整动作记录和typed更新结果。

成功后使用动作记录字段：

- `+0x10/+0x14`：X/Y绘制偏移；
- `+0x18`：模式flags；
- `+0x4A`：帧资源号；
- `+0x4C`：帧索引。

LST以`mov edx,[base+0x4A]`和`mov ecx,[base+0x4C]`做重叠dword读取，但帧查询callee只消费各自低16位；typed实现直接读取两个u16字段，不把相邻字段高word伪造为业务值。

## 4. 帧查询与共享发布

动作更新成功后，以`field_4a`资源和`field_4c`索引查询帧。返回记录先发布到`0x004FD78C`，再解引用`+0`发布源`0x004CD730`。

provider失败时空记录已经发布，但源保持入口旧snapshot，函数在原`[eax]`读取点typed-stop。成功时帧记录、源、宽高和palette snapshot全部发布，之后才读取变体状态表。

## 5. 变体状态表与坐标

`0x00450319`以截断后的variant读取固定字节表`0x00524118[variant]`。现代使用显式span；容量不足只在这一真实读取点报告`outline_state_out_of_range`，保留已完成动作更新、帧查询和源发布。

绘制坐标为：

```text
draw_x = low32(arg4 - action.draw_offset_x)
draw_y = low32(arg8 - action.draw_offset_y)
```

两项减法按32位回绕，不以有符号溢出算术或更宽整数夹值。

## 6. 状态1的四向绿描边

状态字节精确等于1时，函数才执行描边：

1. 在`var_9C`写完整dword `0x07E007E0`；
2. 以帧宽高、校正后坐标、flags 0和该四字节颜色指针调用`0x00417050`；
3. 描边callee强制OR `0x24`，按`(+1,+1),(-1,-1),(-1,+1),(+1,-1)`绘制；
4. 返回后重新从共享帧记录读取当前帧。

palette/辅助物理尾参数在不同routine中复用。typed实现用对齐的两个`u16 0x07E0`同时表达indexed palette读取和RLE constant-fill辅助读取，不偷偷使用帧记录`+4`覆盖绿色槽。

已修正的描边callee逐遍传播通用blitter公共后缀；首遍正常完成后单次请求与RGB/跳行状态清零，后三遍看到清理后snapshot。首个typed-stop阻断剩余描边和本函数后续主绘制。

状态不是1时完全跳过颜色槽写和描边。

## 7. 主绘制

无论是否描边，随后都以：

```text
draw_x, draw_y, frame.width, frame.height, flags=0, frame_record+4
```

绘制当前帧一次。typed接口以帧source palette和同一palette字节span表达记录`+4`在raw/RLE像素routine中的双重用途。

正常`completed`、`clipped_out`或`opacity_disabled`经过通用公共后缀：清目标高度、水平位移、纵向phase、opacity、RGB偏移与跳行状态，保留放大位。其他状态在callee原故障边界停止，不检查复绘selector。

## 8. selector 1附加复绘

主绘制正常返回后，只有入口`argC`完整dword等于1才复绘。顺序固定：

1. 读取动作`mode_flags`；
2. 写green offset=-10；
3. 写blue offset=-10；
4. 显式写red offset=0；
5. flags=`mode_flags | 0x10`；
6. 用同一帧、同一宽高和同一校正坐标再次调用通用blitter。

不重新查询动作或帧。复绘正常返回后同样执行公共后缀；复绘typed-stop则保留已写入的`red=0, green=-10, blue=-10`和主绘制完成前缀。

## 9. 双向追溯

LST到C++：

- `0x004502B0..0x004502E8`：0x98清零、固定动作号、低16位variant与更新；
- `0x004502EB..0x00450313`：动作帧字段读取、帧查询、记录/source发布；
- `0x00450319..0x00450364`：状态表1门、绿色槽与四向描边；
- `0x00450367..0x0045038D`：帧记录`+4`、宽高、偏移坐标与主绘制；
- `0x00450392..0x0045039F`：完整dword selector 1门；
- `0x004503A1..0x004503E7`：模式flags、两项-10、一项0及附加复绘；
- `0x004503EC..0x004503F7`：栈与寄存器恢复、返回。

C++到LST：

- `LegacyActionRecord{}`对应38次dword清零，不调用选择性initializer；
- updater输入字段与唯一动作callee一致；
- frame provider输入来自`field_4a/field_4c`低word；
- outline span读取对应唯一固定表读取；
- outline、primary和overlay draw数量逐一对应三个静态绘制点；
- 两类公共后缀来自已关闭callee；
- 所有typed-stop只位于原更新失败、裸记录、表读取或软件blit故障边界。

完整正向与反向追溯没有未解释基本块、局部字段、全局写、callee、分支或出口。

## 10. 验证与动态差分

定向测试以真实`LegacyActionUpdater`命令流产出资源、帧号和YX偏移，覆盖：

- 入口高16位丢弃，更新器收到动作号`0x2390`与低16位variant；
- 动作产出的资源/0号帧查询与回绕坐标；
- 状态1四个绿色对角像素；
- 一次主绘制与selector 1附加复绘；
- 描边后、主绘制后和复绘后的共享公共后缀；
- 动作流加载失败在帧查询前停止；
- 短状态表只在真实变体读取点停止；
- 空帧源在主blit首次读取点停止并保留入口共享状态。

battle聚合目标零warning构建及定向测试通过。

当前没有原版动作记录、变体状态表、帧记录、共享blitter状态和framebuffer联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整148行LST、四类callee直接组合和固定状态验证已经闭环。
