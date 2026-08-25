# 记载页翻页时按signed起点定位当前窗口指针 `0x0044D050`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0044D050..0x0044D06C`，24行，无callee、无外部FUNCTION CHUNK。直接caller为`0x0044B560`和`0x0044B6E0`。

函数接收signed数量、32位目录基址和输出字段地址：

1. EAX读取并保留输出字段地址，作为最终返回值。
2. 无条件把目录基址完整写入输出字段。
3. 数量signed小于等于0时直接返回，不前移。
4. 数量signed大于0时至少执行一次循环；每次把输出字段按32位回绕加2、数量减一，直到数量为0。

因此最终窗口指针为`base + 2 * count`，但只在count为正时应用；count为负不能按unsigned产生大循环。typed结果保留循环次数，并把原EAX表达为输出字段地址token`0x004FD2FC`，不暴露宿主引用地址。

## 2. caller回收

`0x0044B560`和`0x0044B6E0`删除`rebuild_page`命令，直接把当前page start按位解释为signed count，以记载目录owner为基址，写入`system_menu_window_context`。随后直接调用`0x0044D010`统计最多5项。

前一页越过0时仍先把scroll和page start清0，因此D050只写目录owner；后一页使用新起点，正常情况下前移10字节。D050无回调，两个caller不再增加helper count。

## 3. 验证

UT覆盖count=-1、0和3：非正数量只写基址，正3次从`0xFFFFFFFC`依次回绕到2，返回值始终为`0x004FD2FC`。caller用例覆盖起点0和5时窗口指针分别为`owner`及`owner+10`，并验证旧`rebuild_page`命令和helper计数均已删除。

workpack双生成稳定为`180/227`，SHA256为`92dbe19817bc174522aee518bc4eab5faa96ba74a964a18f7663b7b827bf9142`；下一项为`0x0044D070`。
