# 战斗背景资源初始化 `0x00451940`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`fixed_state_tested`。

## 1. 完整LST范围

权威LST函数为`0x00451940..0x00451A15`，共108行，无外部FUNCTION CHUNK。五个栈参数按原物理顺序解释为：背景物理资源号、初始动作号、缓存字段B4、缓存字段B8、旋转除数。

## 2. 路径与旧资源释放

入口先把全局数据目录复制到临时文件名，再拼接`all_map2.tsw`。modern用`data_root / "all_map2.tsw"`隔离ANSI字符串与宿主路径分隔符；不把旧固定缓冲当主机地址。

之后严格执行：

1. 对固定旋转缓存owner调用已关闭`0x00451730`；
2. 若旧背景图像token非零，则调用旧释放入口；
3. 无条件把背景图像槽清零；
4. 才尝试打开与读取新资源。

modern复用`release_legacy_battle_action_rotation_cache`，然后清空typed背景图像。测试以一组嵌套image/owner token和旧图像验证释放回调顺序为image→owner→新资源load，且loader观察到旧图像已空。

## 3. TSW加载与失败出口

LST以固定文件名、入口资源号、variant零和背景图像槽调用`0x00433320`。打开或读取失败时EAX为零，函数立即`retn`：

- 不执行命令流转换；
- 不执行除法或图像旋转；
- 不初始化动作旋转缓存；
- 不写三个完成word；
- 保留此前旋转缓存与旧背景释放副作用。

modern默认适配器直接复用`LegacyTswArchive`读取`all_map2.tsw`的one-based物理槽和variant零；可注入load port只用于失败与顺序测试。失败返回`image_load_failed`与EAX零。

## 4. 命令流转换与signed除法

加载成功后，LST把背景图像槽地址传给已关闭`0x00401C70`，允许indexed8流经外置palette重建为direct16，也允许word流原地转换。modern复用`convert_legacy_image_command_stream`与当前`LegacyPixelConversionState`，成功后发布转换后的typed字节。

转换完成后才读取第五参数并执行signed `idiv`：

```text
shift = trunc_toward_zero(640 / rotation_divisor)
```

除数不夹值、不取绝对值。零除数在该原始指令点返回`rotation_division_by_zero` typed-stop，保留已完成的释放、加载和转换，不写完成word。

## 5. 背景图像水平循环旋转

商作为shift，以固定mode 3和固定几何owner调用已关闭`0x00433F70`。modern直接调用`rotate_legacy_battle_literal_image(..., pixels_right, shift)`：

- 非正shift、magic不符、首行flag门和非法mode属于callee原正常早退，caller继续；
- 首次真实图像或临时缓冲越界属于typed-stop，caller不伪造公共后缀；
- valid路径保留首行bit15清除、literal布局、低32位回绕和原copy顺序。

## 6. 可选三帧动作旋转缓存

图像旋转返回后，LST先检查战斗记录共享word非零，再只检查第二参数低16位非零。只有两者均真才调用已关闭`0x00451420`。

传参按原push顺序映射为：

- 固定几何owner；
- 第三参数→`field_b4`；
- 第四参数→`field_b8`；
- 第二参数完整32位→initial action，但callee只存低16位；
- 第五参数bit pattern→rotation divisor。

modern直接调用`initialize_legacy_battle_action_rotation_cache`。callee的普通动作更新停止仍回到caller公共后缀；帧索引越界、除零、非法image、旋转typed-stop或完整循环重复则阻断后缀。测试以`0xABCD1234`证明门看低word且callee存`0x1234`，并以`0x12340000`证明高word非零不能打开该门。

## 7. 公共后缀与返回

除资源加载失败或typed-stop外，所有路径进入公共后缀：

1. EAX强制为`0xFFFFFFFF`；
2. 恢复ESI；
3. 按地址从高到低依次把`0xFFFF`写入`0x004A7636`、`0x004A7634`、`0x004A7632`；
4. 返回完整EAX。

modern completion words按低地址升序建模，但显式写序为索引`2→1→0`，结果记录该顺序。加载失败和零除typed-stop测试证明此前值保持不变。

## 8. 双向追溯

- `0x00451940..0x0045195F`：路径复制与固定文件名拼接；
- `0x00451960..0x00451985`：旋转缓存释放、旧背景释放与图像槽清零；
- `0x00451986..0x004519A1`：固定variant零TSW加载及唯一零返回；
- `0x004519A3..0x004519C2`：命令流转换、读取除数与signed `idiv`；
- `0x004519C3..0x004519D0`：固定mode 3背景图像循环右移；
- `0x004519D1..0x004519FE`：双word门与可选三帧动作旋转缓存；
- `0x004519FF..0x00451A15`：EAX全1、ESI恢复和三个word逆地址发布。

C++到LST反向追溯覆盖108行完整函数、五参数、四个closed callee、唯一失败出口和公共后缀。

## 9. 验证与动态差分

定向测试覆盖释放/加载/转换/旋转/动作缓存顺序、old image释放、固定variant零、load失败前缀、signed零除点、模式3一像素循环、双门、完整动作号低word与三个word逆序发布。

真实资产测试读取`all_map2.tsw`物理槽1，完成真实palette/命令流转换和640×400图像shift 160循环右移；该CTest加入`legacy_real_assets`全局锁。普通定向与独立ASan定向均`1/1`通过。

当前没有原版路径缓冲、TSW loader、共享图像token、记录门、动作缓存与framebuffer联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整LST、typed实现、synthetic与真实资产状态已闭环。
