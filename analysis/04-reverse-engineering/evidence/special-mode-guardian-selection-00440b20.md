# 护驾槽与列表推进 `0x00440B20`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。完整范围`0x00440B20..0x00440C13`，101行；caller为407F0三处和40FB0一处。callee为B9A0、B9C0、B9E0、BB80、BC90、442050、4420F0、4429B0及485610。

函数按interaction mode精确分三路：

- mode0先调用4420F0，把guardian slot按32位环绕加一；signed结果大于等于11时归零。随后调用442050，按`party_selector.low16 * 16 + slot`读取独立dword文本索引表，直接复用B9E0发布文本，再调用4429B0和sample命令46。最终EAX为sample命令返回值；本路不改mode flags。
- mode1直接复用BB80推进`total/list_offset/local/visible=10`窗口；按新offset直接复用B9A0形成FCD64可见链头，BC90重算最多10项的visible count，再按`offset+local`直接复用B9C0/B9E0发布选择文本。随后调用4429B0、sample命令46，并仅将mode flags低字节OR 0x30；最终EAX为写回后的完整mode flags，而不是sample返回值。
- 其他mode不调用helper，按原`mode-1`返回。

FCD64已从无语义dword修正为`visible_record_head`指针owner。4420F0/442050/4429B0与485610仍由窄typed port隔离；已关闭的BB80/B9A0/BC90/B9C0/B9E0全部直接复用。链推进只在原裸`next`读取点typed-stop；允许null可见链经过BC90计数为零，再在原B9E0节点解引用点停止。dword文本表越界与B9E0失败同样保留此前副作用。

407F0中的三处440B20调用已删除opaque invoke边界并直接调用本helper；caller通过继承selection port提供尚未闭环的平台边界，且传播typed-stop。

UT覆盖mode0槽10到0回绕、party high16索引、文本、sample与flags不变；mode1窗口跨页、可见链、选择文本、sample、最终flags EAX；短链、null最终节点、文本表越界，以及其他mode的`mode-1`返回。

定向测试通过。workpack双生成稳定为`73/227`，SHA256均为`a9e1c7fd99b9cb6532ec4a5b9d8567901790281a4fc089bc938a5bfc814a9d4d`；下一单元`0x00440C20`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
