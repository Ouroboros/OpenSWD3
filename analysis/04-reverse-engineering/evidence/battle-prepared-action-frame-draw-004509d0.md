# 战斗持久动作槽准备后帧绘制 `0x004509D0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与调用图

权威LST完整范围为`0x004509D0..0x00450A48`，从`proc`到`endp`共59行，没有外部`FUNCTION CHUNK`。

cdecl四参数依次为动作号、动作槽索引、目标X、目标Y。三个caller位于`0x004648AB`、`0x00464932`和`0x00464BC1`。

callee依次为已关闭动作更新器`0x004321E0`、帧查询`0x004315D0`和软件blitter`0x004170E0`各一次。

## 2. 0x98槽地址与写入顺序

LST用LEA/SHL链计算：

```text
offset = low32(index * 0x98)
record = 0x004FD798 + offset
```

函数不做signed索引检查。现代实现先按u32保留乘法回绕；只有结果仍对齐完整typed `LegacyActionRecord`且落入owner span时才解引用，否则在原首个`[record+0]`写点typed-stop。

该规则保留真实回绕别名：索引`0x20000000`乘`0x98`低32位为0，映射槽0。无法表示为完整typed记录的中间地址只隔离unsafe域，不提前改变有效槽行为。

有效槽严格依次写：

1. `record+0x00 = arg_0`动作号；
2. `record+0x08 = 0`，即base variant；
3. 以同一持久记录调用动作更新器。

更新器返回0时立即结束；两项写和更新器已产生的失败前缀不回滚，不查询帧、不发布source。

## 3. 更新后ECX陈旧高字

成功更新器固定返回EAX=1，因此随后`mov ax,[record+0x4C]`得到零扩展帧号。

资源号路径只执行`mov cx,[record+0x4A]`，ECX高16位保留`0x004321E0`成功出口的命令路径值。typed更新器不模拟物理ECX，包装显式接收`action_update_ecx_snapshot`：

```text
resource_id = (ecx_snapshot & 0xFFFF0000) | record.field_4a
frame_index = record.field_4c
```

不得擅自把资源号零扩展。定向测试以snapshot `0xBEEF1234`和字段`0x0066`锁定查询资源`0xBEEF0066`。

## 4. 帧发布与绘制ABI

帧查询成功后，函数只把`frame+0`的source发布到`0x004CD730`；它没有写`0x004FD78C`帧record全局。typed state因此记录current frame/source，但不宣称旧帧record发布。

软件绘制参数：

- X/Y直接取入口参数，不减动作记录偏移；
- 宽高取帧record u16；
- flags取动作记录`+0x18`完整32位；
- 第六物理tail固定0。

固定空tail意味着typed调用清空palette和auxiliary；indexed8帧在完整源长度通过后于首次palette读取点得到`palette_out_of_bounds`。正常公共后缀清单次请求、RGB和跳行状态并保留放大位；typed-stop不清。

## 5. 双向追溯

- `0x004509D0..0x004509E9`：0x98低32位槽偏移、动作号首写；
- `0x004509EB..0x004509FF`：base variant清零、动作更新与零返回门；
- `0x00450A01..0x00450A11`：帧号AX零高字、资源号CX陈旧高字及帧查询；
- `0x00450A16..0x00450A24`：source发布与固定tail0；
- `0x00450A26..0x00450A3F`：动作flags、记录宽高、入口Y/X和软件绘制；
- `0x00450A44..0x00450A48`：栈与ESI恢复。

C++ typed实现直接复用已关闭动作更新器和blitter；没有复制动作状态机、修正坐标、发布不存在的帧record或偷用帧palette。

完整正向和反向追溯没有未解释基本块、参数、共享写、callee或出口。

## 6. 验证与动态差分

定向测试覆盖：

- 槽1只写入口动作号与base variant 0，邻槽不变；
- 真实动作命令流产出资源、帧号和flags；
- ECX snapshot高16位保留到资源号；
- 入口X/Y原样绘制及正常公共后缀；
- 索引`0x20000000`回绕别名槽0；
- 非owner回绕地址在首写点停止；
- 更新失败保留两项有序写；
- indexed帧固定空tail触发palette typed-stop。

battle聚合目标零warning构建及定向测试通过。

当前没有原版动作更新后ECX、持久动作槽、帧record、共享blitter状态和framebuffer联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整59行LST、三个关闭callee直连和固定状态验证已经闭环。
