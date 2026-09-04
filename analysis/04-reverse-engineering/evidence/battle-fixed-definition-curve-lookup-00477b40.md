# 战斗定义上限与固定数量查询 `0x00477B40`

状态：`platform_adapted`、`unit_tested`、`callers_reclaimed`。

## 1. 完整LST范围

唯一行为真值为`swd3.exe.lst`。完整主体为`0x00477B40..0x00477BC5`，从`proc`到`endp`共67个物理行、39条实际指令、4个call、3个跳转、3个局部标签和2个返回点，没有外部`FUNCTION CHUNK`。四个call是命中与缺键两条分支各一次`0x00476DB0` MON定义读取和`0x00478220`定义说明清理。

三个物理caller为：

- `0x0041080F`，位于已关闭的队伍对话整页填充`0x00410730`；
- `0x00442D0A`，位于已关闭的护驾属性摘要`0x00442CA0`；
- `0x0044ADC1`，位于已关闭的角色属性重算`0x0044AB00`。

## 2. 固定链搜索与MON顺序

入口先把完整第二参数装入EAX，仅以AX和记录`+0x04`的word比较。固定根`0x004B8A00`无条件参与第一次比较；不读取根count，也不采用`0x00477A20`的“非零count才从首节点开始”规则。未命中时逐节点读取`+0x00` link，零link进入缺键分支；非零link的下一次真实访问是后继`+0x04`键读取。循环不增加现代上限或环检测。

链搜索完成后才调用`0x00476DB0`。传给definition loader的是完整EAX参数，loader内部仍按其既有合同以low16选择MON目录。读取完成后无条件调用`0x00478220`：scratch `+0xA0`说明token非零时释放并清零token，零时EAX变零。MON正常open失败不是typed-stop；scratch保持清零，后续仍发布零上限。definition loader在输出、临时流或说明分配的原访问点typed-stop时，不执行清理和任何输出写入。

## 3. 命中路径

清理后严格按以下顺序执行：

1. EAX装入第一个输出token；只把ECX低word替换为scratch `+0x44`的maximum；
2. 把maximum写入第一个输出；
3. EAX装入第二个输出token；
4. 从命中记录`+0x06`读取count，只把EDX低word替换为count；
5. 把count写入第二个输出；
6. EAX固定返回1。

因此第一个输出故障时尚未装入第二个输出token；记录`+0x06`故障时maximum已经写出，但EDX仍保留清理后的完整值；第二个输出故障时maximum和count读取均已完成。成功返回时ECX高word来自说明清理、低word为maximum；EDX高word来自说明清理、低word为count。

## 4. 缺键路径

零link后仍先读取并清理MON定义。随后ECX装入第一个输出token、EDX低word替换为maximum、EAX提前装入第二个输出token，再依次写maximum和零count，最后`xor eax,eax`返回0。

这条分支不读取任何记录`+0x06`。首个输出故障时EAX已经是第二个输出token，与命中分支不同；第二个输出故障保留已完成的maximum写入。成功返回时ECX保持第一个输出token，EDX低word为maximum。

## 5. 三个caller回收

`0x00410730`在第二分类mask命中时直接调用typed helper：count作为附加值，maximum作为分母；第一分类固定曲线已完成的副作用保留，随后第三分类和低ID固定数量覆盖顺序不变。查询typed-stop不绘制当前行。

`0x00442CA0`在slot 7/8且seed不是`0xFFDC`时直接调用typed helper，并写`count | maximum << 16`到cache `+0x48`。`+0x44`和`+0x48`两个sentinel先完成；typed-stop阻断`+0x4C` sentinel及后续结果。

`0x0044AB00`只在贡献7和8的kind严格为`0x33`时查询，以count作value、maximum作divisor，继续执行原两级整数除法`(-1000 * count) / maximum / 100`。maximum为零仍在原`idiv`位置停止；helper typed-stop保留16次属性应用、16个word回加及此前modifier副作用。

旧`query_pair_added_value`、`query_guardian_slot_pair_attributes`和`query_character_attributes_scale`三个opaque端口及对应fixture数据全部删除。三个caller共同复用唯一`LegacyBattleFixedObjectStatePort`和`LegacyBattleMonDatabasePort`。

## 6. 验证与阻塞

独立leaf测试覆盖根命中、缺键、MON说明释放、正常open失败、definition typed-stop、根/后继键访问、命中记录`+0x06`访问、两个输出写入顺序及两条分支不同的寄存器残值。caller回归覆盖队伍对话第二分类及刷新、护驾slot 7 packed结果与sentinel前缀、角色属性type-33缩放和三处typed-stop传播。

发布验证为定向测试`2/2`、Linux core`194/194`、AddressSanitizer`194/194`、Linux app`200/200`、连续10轮完整core、changed-range clang-format、库存连续双生成和release审计；最终日志零OpenSWD3源码warning、测试失败、sanitizer finding或runtime error。工作包为`273/422 = 263 platform_adapted + 10 assembly_exact + 149 pending_audit`，库存SHA256为`0dc0635f89b2702353f3c66632074833f77e09cbb1e8e217d5015eb2e8492521`。未启动原版或OpenSWD3游戏程序。

原版固定链、MON文件/说明堆、两个stack输出槽及三个caller联合寄存器捕获后端缺失，`original_diff_verified`登记为`blocked_runtime_oracle`。该阻塞不影响完整LST静态闭环、typed故障隔离和Linux验证。
