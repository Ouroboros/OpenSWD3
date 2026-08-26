# 战斗单条效果记录帧 `0x004599B0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整LST范围

权威函数为`0x004599B0..0x00459BE9`，完整269行、10个静态call站点、14个`loc_`标签，无外部FUNCTION CHUNK。9个唯一callee。

唯一caller是已关闭组B战斗帧`0x004576A0`中的`0x00458244`。本工作包关闭后，caller删除`0x004599B0` opaque token，直接组合typed实现并合并嵌套port call计数；嵌套typed-stop映射为caller的effect-record stop。

入口读取actor token、source value和8槽slot index。主/备用record继续使用固定152字节布局：

```text
primary:   0x005202A8 + slot * 0x98
alternate: 0x004FE600 + slot * 0x98
```

slot越界只在首次主status/complete访问typed-stop。

## 2. signed status前缀

函数先读取主status word。按i16为负时，严格先把battle gate清零，再把唯一共享战斗消息/阶段写1。该dword与startup、动作和预帧路径共用`LegacyBattleSharedPhaseStatePort`。该前缀即使主record complete已等于1也执行；随后成功尾会清整个主record。

## 3. 主record初始化失败

主complete为0时写source、zero和global mode snapshot，再初始化主record。

初始化完整EAX为0时：

1. 清同槽备用152字节record；
2. alternate active清零；
3. 返回1。

主record此前source/zero/mode写保留，不清主record，不查询resource。

## 4. resource owner首读

初始化成功后按两个u16 key查询resource owner。与前两项不同，本函数在sample之前立即执行`[owner]`读取：

- owner token为0时在首次owner value访问typed-stop；
- 不发布current resource；
- 不查询坐标；
- 不播放sample。

有效owner发布其内部value token、u16宽高和data token。global flip mode等于1时翻转render flags完整bit0，并把base offset改为`u16(width)-record base`。

## 5. 坐标选择

先以actor token查询两项offset。两个低word必须同时非零才查询base coordinates并做完整u32相加；任一为0时丢弃两项offset，改为直接查询actor coordinates。

随后：

- Y只把低word减record base-Y；
- X按完整u32减base offset。

sample参数先取调整后X完整dword，再只覆盖低word为record pan。因此sample参数高word来自调整后坐标，不来自resource或callee寄存器。

## 6. 左右sample pan

play sample后，以`base_offset + signed16(X)`按低32位回绕并以i32比较320：

- 大于等于320：pan参数保留play callee EDX高word，只覆盖DX为record pan，level为16；
- 小于320：pan参数保留play callee ECX高word，只覆盖CX为record pan，level为-16。

set-pan之后主record pan清零。两条路径都不根据play返回值早退。

## 7. 绘制和释放顺序

resource render参数固定为：

1. signed16 X；
2. signed16 Y；
3. owner u16 width；
4. owner u16 height；
5. 本地render flags；
6. owner data token。

owner内部value非0时先释放value；为0时跳过该call。随后无条件把owner内部value槽写0，再释放owner。typed状态以`released_owner_value_clears`记录该真实写时机；共享current resource token继续保留释放前value，不随owner内部槽清零。

## 8. 完成尾

绘制/释放结束或主complete入口已非0后，重新读取主complete：

- 完整值不等于1：返回0，保留主record；
- 完整值等于1：清整个主152字节record并返回1。

成功尾不清备用record、不写alternate active；只有初始化失败路径清备用状态。

## 9. caller回收

组B帧原pending-effect分支只在pending ID非全1时调用本函数。modern caller现持有完整`LegacyBattleSingleEffectFrameState`：

- actor token使用当前组B物理token；
- source value使用原共享pending参数；
- slot使用当前组B index；
- 子函数返回1才把pending ID写全1；
- 返回0保持pending ID；
- 子函数typed-stop立即传播，不执行后续最终actor step；
- 子函数所有9类callee通过adapter进入caller既有typed端口，计数累加一次，不再发布已关闭函数token。

调用端测试把子record预置complete=1，证明同调用清主record、清pending ID，且port中不存在`0x004599B0`调用。

## 10. 测试与动态差分

定向测试覆盖：

- slot首访问typed-stop；
- signed status在complete record清零前发布；
- 初始化失败清备用record与active但保留主前缀；
- owner零token在任何坐标/sample前停；
- global flip、offset AND门、fallback坐标、signed X/Y和data token；
- 左侧sample使用坐标高word、pan使用play ECX高word；
- 右侧pan使用play EDX高word；
- value非0时value→owner释放，value为0时只释放owner；
- owner内部value清零计数；
- caller直连、嵌套状态与pending/final公共尾。

当前缺少原版主/备用record、9类callee共享副作用、resource owner内部槽、actor坐标、sample manager、framebuffer和寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
