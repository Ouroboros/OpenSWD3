# 战斗十进制单位置位 `0x00450900`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与caller

权威LST完整范围为`0x00450900..0x004509C4`，从`proc`到`endp`共99行，没有外部`FUNCTION CHUNK`。

cdecl唯一参数为32位除数。唯一caller `0x004507A0`固定调用十次，除数从十亿递减到一；callee关闭后caller中的`LegacyBattleDecimalPlacePort`已删除并改为直接typed调用。

## 2. unsigned商与低16位显示门

函数读取共享剩余值`0x004FD788`为EAX，清EDX后执行unsigned `div divisor`。因此剩余值即使typed视图为负，其bit pattern仍按u32参与除法。

除数0在原`div`点触发处理器故障；现代实现只在该点typed-stop，入口共享状态不变。

完整32位商保存到ESI，但显示门只执行`test si,si`：

- 商低16位非零：进入绘制；
- 商低16位为0且leading为0：不查询帧，返回0，共享余数/leading不变；
- 商低16位为0且leading非零：仍进入绘制。

因此商`0x00010000`虽完整值非零，leading为0时仍属于跳过路径。不得把门提升为`quotient != 0`。

## 3. 资源号与帧号的非对称

帧索引始终是完整32位商ESI。

资源号来自`mov ax,color_word`的陈旧高字：

- 商低16位非零时，EAX仍是完整商，资源号高16位继承商高16位；
- 商低16位为0且leading非零时，EAX已被重载为完整leading值，资源号高16位继承leading高16位；
- 资源号低16位均取packed颜色槽高word `0x004FDD08+2`。

例如商`0x00010002`、颜色`0xABCD`产生资源`0x0001ABCD`、帧索引`0x00010002`；leading `0x12340001`、零商产生资源`0x1234ABCD`、帧0。typed实现不清理这些陈旧高字。

## 4. 固定空tail的两种绘制模式

查询成功后发布帧record与source。两条分支均：

- 目标X为共享X减16，使用32位回绕；
- 目标Y为共享Y；
- 宽高取帧record u16；
- 第六物理尾参数固定0，因此typed source palette和auxiliary均清空。

`word_4FD784 == 0x8000`时flags为`0x20`；其他值flags为0。

当前已关闭blitter对测试direct16源的flags `0x20`选择`unassigned_routine`并在该真实callee点typed-stop；余数与leading后缀不得提前执行。普通flags0路径正常完成。indexed8源即使帧record带palette，也因固定空tail在首次palette读取点得到`palette_out_of_bounds`，保留record/source发布而不更新余数。

## 5. 正常后缀与完整EAX

blitter正常公共后缀返回后：

1. `ESI &= 0xFFFF`，只保留商低16位；
2. 低32位计算`product = quotient_low16 * divisor`；
3. 以32位回绕执行`remaining -= product`并发布；
4. leading无条件置1；
5. 从当前帧record读取u16宽到AX。

第5步只覆盖EAX低16位；EAX高16位保留新余数高16位。完整返回值为：

```text
(new_remaining & 0xFFFF0000) | frame_width
```

caller `0x004507A0`只使用低16位推进X，但typed单位置位结果保留完整EAX。测试值`12,345,678`的千万位完成后余数`2,345,678`，完整返回锁定为`0x00230004`。

## 6. 双向追溯

- `0x00450900..0x0045090F`：共享值unsigned除法与完整商保存；
- `0x00450911..0x00450924`：商低16位、leading门与零返回；
- `0x00450925..0x00450945`：陈旧高字资源号、完整商帧号及record/source发布；
- `0x0045094B..0x00450990`：固定空tail、模式flags、X减16、共享Y与记录宽高；
- `0x00450991..0x004509AA`：软件blitter及商低16位乘除数；
- `0x004509AD..0x004509C4`：余数、leading发布与帧宽低word返回。

C++ typed实现逐项保留上述bit pattern、共享顺序和失败前缀；十位协调器直接复用该函数，没有剩余端口或重复单位置位逻辑。

完整正向和反向追溯没有未解释基本块、参数、共享写、callee或出口。

## 7. 验证与动态差分

定向测试覆盖：

- 除数0在帧查询前typed-stop；
- 商0与商`0x00010000`均按低16位门跳过；
- 商高字和leading高字分别继承到资源号；
- 完整商作为帧索引；
- 模式`0x8000`选择flags `0x20`并保留blitter typed-stop前缀；
- 普通零商强制绘制帧0并完成余数/leading/帧宽返回；
- indexed帧固定空tail触发palette typed-stop；
- 十位协调器直连后绘制帧1..8，锁定完整EAX高字、逐帧宽推进与帧查询失败阻断。

battle聚合目标零warning构建及定向测试通过。

当前没有原版单位置位共享状态、帧record和framebuffer联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整99行LST、关闭callee直连和固定状态验证已经闭环。
