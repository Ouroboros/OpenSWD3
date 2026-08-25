# 战斗持久动作槽偏移帧绘制 `0x00450400`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围与ABI

权威LST完整范围为`0x00450400..0x0045048F`，从`proc`到`endp`共66行，没有外部`FUNCTION CHUNK`。

cdecl三参数：

```text
arg0 = 目标锚点X
arg4 = 目标锚点Y
arg8 = 持久动作记录索引
```

唯一caller为`0x0045A225`：X来自战斗位置表，Y固定466，动作索引来自caller ESI。返回值未消费。

## 2. 0x98动作槽索引

入口以：

```text
ecx = index * 9
esi = index + ecx * 2
esi <<= 3
```

得到`index * 152`的低32位偏移，基址为`0x004FDD10`，因此owner是0x98字节`LegacyActionRecord`数组。

原固定数组没有边界检查。typed实现接收显式span；负数或超容量索引只在原首个`mov [eax],0x2392`写点报告`action_record_out_of_range`。在此之前没有owner写、动作更新、帧查询或绘制。

## 3. 有序覆盖与动作更新

选中槽后只按顺序覆盖：

1. `action_id=0x2392`，完整dword；
2. `base_variant=0`，完整dword；
3. 调用已关闭`0x004321E0`动作更新器。

函数不清零完整0x98槽，也不调用初始化器；其余旧字段是否保留由动作更新器的原状态机决定。更新返回0直接结束，两项覆盖及更新器已完成的失败前缀不回滚。

## 4. 动作产出帧与共享发布

更新成功后读取：

- `+0x4A field_4a`为资源号；
- `+0x4C field_4c`为帧索引。

LST以两个word读入AX/DX后传给`0x004315D0`。帧记录返回后先发布到`0x004FD78C`，再解引用`+0`发布源`0x004CD730`。

provider失败时空记录已发布、入口旧源保留，并在原`[eax]`读取点typed-stop。成功时typed state保存帧记录、源、索引和palette snapshot。

## 5. 坐标、flags与绘制

绘制参数：

```text
x = low32(arg0 - action.draw_offset_x)
y = low32(arg4 - action.draw_offset_y)
width  = frame.width  (u16零扩展)
height = frame.height (u16零扩展)
flags  = action.mode_flags (+0x18)
tail   = frame_record+4
```

两项减法按32位回绕。tail通过source palette与同一palette字节span表达raw/RLE routine的物理参数复用。

通用blitter正常`completed`、`clipped_out`或`opacity_disabled`返回后，公共后缀清目标高度、水平位移、纵向phase、opacity、RGB偏移与跳行状态，保留放大位。其他状态在原callee故障点typed-stop，不清入口共享状态。

## 6. 调用图与直接组合

callee只有：

- 动作更新`0x004321E0`一次；
- 帧查询`0x004315D0`一次；
- 软件blitter`0x004170E0`一次。

三者均已关闭。实现追加到战斗动作帧typed单元，直接使用`LegacyActionUpdater`和`LegacyFramePieceProvider`，不重复动作解释器，不引入callback边界。

## 7. 双向追溯

LST到C++：

- `0x00450400..0x0045040E`：0x98低32位步长和固定owner基址；
- `0x00450414..0x0045042F`：动作号/base覆盖、更新与零返回门；
- `0x00450431..0x0045044D`：资源/帧word、查询、帧记录与源发布；
- `0x00450453..0x00450485`：tail、mode flags、u16高宽和偏移坐标；
- `0x00450486..0x0045048F`：软件blitter、栈恢复与返回。

C++到LST：

- span索引对应唯一动作槽地址；
- 两项字段写顺序对应两条固定dword写；
- updater、provider和blitter各对应唯一callee；
- frame/source状态对应两项旧共享写；
- shared公共后缀来自已关闭blitter；
- typed-stop只位于原槽写、更新零返回、帧解引用或blitter故障点。

完整正向与反向追溯没有未解释基本块、算术、字段、callee、共享写或出口。

## 8. 验证与动态差分

定向测试覆盖：

- 索引1只更新第二个0x98槽，邻槽不变；
- 固定动作号2392和base variant 0传入真实动作更新器；
- 动作命令流产出资源、帧号和YX偏移；
- 指定坐标实际像素输出与mode flags路径；
- 正常公共后缀清理与放大位保留；
- 负索引只在首个owner写点停止，记录与callee均未触碰；
- 动作更新失败保留动作号/base两项覆盖并阻断帧查询。

battle聚合目标零warning构建及定向测试通过。

当前没有原版持久动作槽、帧记录、共享blitter状态和framebuffer联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整66行LST、三个callee直接组合与固定状态验证已经闭环。
