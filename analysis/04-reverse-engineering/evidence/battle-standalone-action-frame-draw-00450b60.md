# 战斗独立动作记录帧定点绘制 `0x00450B60`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与调用图

权威LST完整范围为`0x00450B60..0x00450BCA`，从`proc`到`endp`共51行，没有外部`FUNCTION CHUNK`。

cdecl三参数依次为动作号、目标X、目标Y。唯一caller为`0x0045342C`，传固定动作号`0x2391`和同一索引记录中的两项坐标；返回值不被caller消费。

callee依次为动作更新器`0x004321E0`、帧查询`0x004315D0`和软件blitter`0x004170E0`各一次，均已关闭并由typed接口直接组合。

## 2. 独立持久动作记录

函数操作`0x004FDC70`起始的独立0x98字节动作记录，与`0x004FD6D0`记录及索引动作槽数组不是同一owner。

入口只按顺序覆盖：

```text
record.action_id = arg0
record.base_variant = 0
```

不清空其他字段。动作更新器返回0时立即返回，EAX保持0；两项入口写和更新器失败前缀保留，帧查询与绘制均不发生。

modern使用专用`LegacyBattleStandaloneActionFrameDrawState::action_record`持续保存该记录，不与相邻已关闭包装共享错误owner。

## 3. ECX/EDX双陈旧高字

更新成功后LST依次执行：

```text
mov cx, [record+0x4C]
mov dx, [record+0x4A]
push ecx
push edx
call frame_query
```

因此帧查询首参数资源号来自EDX，第二参数帧号来自ECX；两条16位MOV都不清高字：

```text
resource_id = (post_update_edx & 0xFFFF0000) | record.field_4a
frame_index = (post_update_ecx & 0xFFFF0000) | record.field_4c
```

现代函数显式接收两个更新后snapshot。定向测试锁定资源`0xBBBB0066`和帧号`0xAAAA0000`，不得把任一值擅自零扩展。

帧查询失败后，原函数会在`mov ecx,[eax]`首次解引用处进入故障域。typed provider失败只在该点停止，不发布source、不绘制。

## 4. source发布与软件绘制

帧record可用时，函数先读取`+0`并发布source到共享槽，然后以：

- X/Y直接取两个入口参数，不减动作偏移；
- 宽高取帧record u16；
- flags取动作记录`+0x18`完整32位；
- 第六物理tail固定0；

调用软件blitter一次。函数不发布帧record全局，也不读取帧record palette。

fixed tail由typed调用清空source palette和request auxiliary表达。indexed8源在完整首word和几何检查通过后，于首次palette读取点得到`palette_out_of_bounds`。

正常`completed`、`clipped_out`或`opacity_disabled`经过通用公共后缀，清目标高度、水平位移、纵向phase、opacity、RGB和跳行并保留放大位。其他typed-stop不清入口共享状态。

更新失败时旧EAX为0；成功路径最终EAX沿用通用blitter返回。现代以`LegacyBlitResult`中的执行状态与selection表达该typed返回，不增加包装结果门或陈旧latch。

## 5. 双向追溯

- `0x00450B60..0x00450B82`：动作号、base variant 0、更新器与零返回门；
- `0x00450B84..0x00450B94`：ECX帧号和EDX资源号双陈旧高字查询；
- `0x00450B99..0x00450BA7`：首次帧解引用、source发布与固定tail；
- `0x00450BA9..0x00450BC2`：动作flags、记录宽高、入口Y/X和软件绘制；
- `0x00450BC7..0x00450BCA`：栈回收与返回。

C++到LST反向追溯覆盖专用持久记录、两项入口写、更新失败前缀、两个寄存器snapshot、帧查询、source发布、六个物理绘制参数、公共后缀和返回；没有未解释基本块、callee、共享访问或出口。

## 6. 验证与动态差分

定向测试覆盖：

- 入口动作号与base variant 0覆盖，其他状态由真实更新器维护；
- 资源`0x0066`、帧0正常查询，入口X/Y原样绘制；
- ECX和EDX高16位分别保留到帧号和资源号；
- 更新失败保留入口写并阻断帧查询；
- 帧查询失败不发布source；
- 正常公共后缀清理并保留放大位；
- indexed固定空tail在palette读取点停止且不清共享状态。

battle聚合目标零warning构建及定向测试通过。

当前没有原版更新后ECX/EDX、独立持久动作记录、帧record、共享blitter状态和framebuffer联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整51行LST、唯一caller和三个关闭callee已完成固定状态闭环。
