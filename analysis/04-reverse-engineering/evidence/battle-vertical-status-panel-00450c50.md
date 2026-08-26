# 战斗纵向状态面板绘制 `0x00450C50`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与调用图

权威LST完整范围为`0x00450C50..0x00450F8E`，从`proc`到`endp`共363行，没有外部`FUNCTION CHUNK`。

cdecl六参数依次为动作号、X、Y、中段数量、填充偏移和selector。三个caller均位于`0x00464270`，地址为`0x00464675`、`0x004646F8`和`0x00464799`；实际调用固定动作号`0x232A`、Y `0x9E`、中段数量4，X分别为`0x190`、`0x19E`和`0x192`，后两参数来自战斗共享状态。

直接callee为四次动作更新`0x004321E0`、四次帧查询`0x004315D0`、最多多次软件blitter`0x004170E0`、两次clip设置`0x00416FF0`和一次x87向零转换`0x00489654`。动作更新、帧查询和blitter由已关闭typed接口直连；clip与转换按完整callee指令语义内联为typed状态转换。

## 2. 动作号与四阶段记录生命周期

入口动作号先与`0xFFFF`，高16位永久丢弃。函数共准备四个动作阶段：

```text
顶部：selector == 1 ? base_variant 0x1E : 0x1A
中段：base_variant 0x18
填充：base_variant 0x19
底部：selector == 2 ? base_variant 0x1F : 0x1B
```

selector比较完整32位值。顶部阶段不清动作record，只覆盖动作号和base variant；旧wait、缓存键、mode及其他字段仍可影响首次更新。测试以旧`wait_remaining=1`和预装资源低字证明顶部没有被选择性或完整initializer重置。

顶部正常绘制完成后，函数才以`ECX=0x26; EAX=0; rep stosd`清零完整152字节record。中段、填充和底部各自在更新前再次完整清零，然后只写动作号与对应base variant。任何更新、查询或绘制typed-stop都阻断尚未到达的下一次清零。

## 3. 更新后寄存器高字

四次帧查询都只覆写更新后寄存器的低16位，不能把资源号或帧号擅自零扩展：

- 顶部、填充、底部：资源号低字取record `+0x4A`，高字保留更新后ECX；帧号低字取`+0x4C`，高字保留更新后EAX；
- 中段：资源号低字取`+0x4A`，高字保留更新后EAX；帧号低字取`+0x4C`，高字保留更新后EDX。

modern以四组可变register snapshot表达。固定状态测试分别使用不同高字，锁定四次provider参数为：

```text
B2B2:0060 / A1A1:0000
C3C3:0061 / D4D4:0001
F6F6:0062 / E5E5:0002
2828:0063 / 1717:0003
```

每次查询成功后发布共享source。查询失败只清对应typed frame可用性，保留此前阶段已发布的旧source，并在原首次frame解引用点停止。

## 4. 顶部与中段绘制

四阶段所有blitter调用均使用record mode flags、frame u16宽高、共享source和固定空tail。typed调用清空本次palette/auxiliary；indexed帧在首次palette读取点停止。

顶部帧直接绘制于入口`(X,Y)`。随后：

```text
panel_content_top = Y + top.height
accumulated_height = top.height
```

中段frame发布后才检查中段数量。数量signed非正时不绘制；正数时从0递增，逐片绘制于：

```text
X = entry_x + 2
Y = entry_y + top.height + middle.height * index
```

所有加法和乘法按x86低32位回绕。循环结束后，不论数量符号，都以`middle.height * entry_middle_count`计算比例所用总中段高度；底部位置累计值只增加实际绘制次数乘帧高。这一差异在负数量域必须保留。

每个正常blitter调用立即执行公共后缀，清目标高度、水平位移、纵向phase、opacity、RGB和跳行，并保留放大位；任何typed-stop保留该次调用前的共享状态并阻断后续阶段。

## 5. 精确比例算术

填充frame查询并发布source后，原函数执行：

```text
scaled = trunc_x87((7.0 / maximum_count) * repeated_middle_height)
divisor = maximum_count - 7
quotient = signed_div(repeated_middle_height - scaled, divisor)
displacement = current_count * quotient
```

`7.0`来自原单精度常量但值精确；`fild`、`fdivr`、`fimul`在x87扩展精度中执行，`0x00489654`临时设置向零转换并返回qword低32位。modern使用80位`long double`分步计算并向零转i64，再保留低32位；无穷、NaN或范围外转换映射为x87 indefinite qword低字0。

signed `idiv`只在原除法点检查。`maximum_count == 7`在完成填充frame查询/source发布和x87计算后进入`ratio_divide_by_zero`；`INT_MIN / -1`进入`ratio_divide_overflow`。不得前移门或伪造后续clip。

商正常后依次发布：

```text
fill_start = displacement + panel_content_top + fill_offset
fill_clip_bottom = displacement + scaled + panel_content_top + fill_offset
if (fill_clip_bottom - fill_start < 5): fill_clip_bottom = fill_start + 5
if (current_count + 7 >= maximum_count):
    fill_clip_bottom = panel_content_top + repeated_middle_height
panel_content_bottom = panel_content_top + repeated_middle_height
```

全部整数算术低32位回绕，比较为signed。测试锁定`maximum=10,current=2,total=12`得到`scaled=8, quotient=1, start=25, bottom=33`；也锁定`current+7 >= maximum`在五像素下限之后覆盖bottom。

## 6. 局部clip与填充循环

比例发布后调用`0x00416FF0(X,Y,X+32,fill_clip_bottom)`。该helper只把负left/top夹到0，只把right/bottom上限夹到screen宽高，再以signed低32位减法发布宽高；不对负right/bottom做下限夹取。

`scaled <= 0`时不绘制填充片。正值时从进度0开始，以填充帧高推进，条件在每次绘制后检查：

```text
do:
    draw(x + 5, fill_start + progress)
    progress += fill.height
while progress < scaled
```

因此scaled不是帧高整数倍时最后一片可以越过目标进度，依赖局部clip截断。填充帧高为0时不推进；偶数帧高在极端正scaled下也可能因signed 32位回绕永远无法命中可达正阈值。typed实现先按步长与`2^32`的最大公因数完成全域可达性检查，在非终止域仍保留首个draw及其公共后缀后报告`fill_loop_nonterminating`，局部clip保持不恢复。

所有填充调用正常结束后，无条件调用`0x00416FF0(0,0,640,480)`恢复逻辑全屏clip；helper仍按实际screen上限夹值。填充draw typed-stop、除法故障或零帧高无限域均不会提前恢复。

## 7. 底部与返回值

恢复全屏clip后，底部record再次完整清零并按selector选择变体。查询成功后绘制于：

```text
X = entry_x
Y = entry_y + top.height + middle.height * actual_middle_draw_count
```

底部正常出口没有再改EAX；`add esp`不影响寄存器，因此完整返回值就是末次blitter EAX，而不是帧宽、动作更新值或typed状态。modern显式接收`final_blit_eax_snapshot`，只在底部blitter正常公共后缀后发布。

三个caller不消费该返回值，但callee合同仍保留。更新失败的原EAX为0；其他原访问故障域由typed status停止，不伪造正常返回。

## 8. 双向追溯

- `0x00450C50..0x00450C86`：动作号低字、顶部变体选择及首record非清零入口；
- `0x00450C8A..0x00450CDD`：顶部更新、陈旧EAX/ECX高字、查询/source发布与绘制；
- `0x00450CE2..0x00450D89`：第一次完整清零、中段变体、更新、陈旧EAX/EDX高字及signed循环；
- `0x00450D89..0x00450DE5`：真实/入口中段高度分离、第二次清零与填充查询；
- `0x00450DEA..0x00450E81`：x87比例、向零转换、signed idiv、回绕坐标与五像素/末段门；
- `0x00450E86..0x00450EE6`：局部clip、填充do-while及实际帧高推进；
- `0x00450EEA..0x00450F35`：全屏clip恢复、第三次清零和底部变体；
- `0x00450F35..0x00450F8E`：底部更新、陈旧EAX/ECX高字、查询/source发布、绘制及完整EAX返回。

C++到LST反向追溯覆盖全部四个动作阶段、四组寄存器高字、三次inline `rep stosd`、两个signed循环、x87和整数比例、两次clip、固定tail、共享后缀与所有出口；没有未解释基本块、callee、共享访问或返回。

## 9. 验证与动态差分

定向测试覆盖：

- 首阶段保留旧wait/resource，后三阶段逐次全清；
- selector 1选择顶部1E/底部1B，selector 2选择顶部1A/底部1F；
- 四阶段资源和帧号的不同陈旧寄存器高字；
- 顶部、中段、填充、底部的坐标、次数、颜色覆盖和末次EAX返回；
- x87比例、signed商、五像素门之后的末段覆盖；
- 局部clip发布及640×480恢复；
- 分母零在填充source发布后停止；
- 第二阶段更新失败、第三阶段帧失败的真实record/source前缀；
- indexed顶部固定空tail在palette读取点停止，且不执行首次record清零和公共后缀。

battle聚合目标零warning构建、定向测试和独立ASan通过。

当前没有原版四次动作更新后寄存器、共享计数、x87结果、clip状态、四组frame record、共享blitter状态和framebuffer联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整363行LST、三个caller和五类callee已完成固定状态闭环。
