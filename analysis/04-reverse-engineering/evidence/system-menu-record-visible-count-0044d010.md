# 在记载页翻页后从当前起点统计最多5项可见记录 `0x0044D010`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0044D010..0x0044D040`，27行，无callee、无外部FUNCTION CHUNK。直接caller为`0x0044B560`、`0x0044B6E0`和`0x0044BDA0`。

函数先按32位回绕计算`list_owner + 2 * page_start`并把可见数量清0，然后读取当前u16记录：

- 首项为0时立即返回当前记录地址和数量0。
- 首项非零时，先检查当前数量是否已达到5；未达到则数量加一、地址前进2字节、发布新数量，再读取下一u16。
- 数量达到5时停止循环。因此5项可见记录后仍会额外读取第6个词；第6个词无论0或非零都不增加数量。

typed目录固定为128个u16。null owner和索引越界只在原始u16读取点停止；可见数量清0、此前计数、地址推进和返回残值均保留。尾部从索引123开始连续5项时，会先发布`visible_count=5`和索引128的残值，再在额外第6个词读取点停止。

`legacy_return_value`按原32位地址算术表达为`list_owner + 2 * next_record_index`，不使用宿主64位容器地址。

## 2. caller回收

- `0x0044BDA0`打开“记载”页时直接清光标标志、可见数量和起点，再调用typed计数；删除`prepare_record_page`命令。
- `0x0044B560`和`0x0044B6E0`先直接调用已关闭的`0x0044D050`，按signed起点计算当前窗口指针，再调用typed计数；`rebuild_page`和`count_visible`命令均已删除。
- 计数typed-stop时两个翻页caller立即返回，不继续夹scroll或OR光标低字节，保留原始停止前缀。

## 3. 验证

UT覆盖null owner、初始索引越界、首项0、连续3项、连续6个非零词仍只发布5项、索引123尾部额外读取停止；另覆盖三个caller直连、窗口指针定位后目录计数，以及typed-stop后不执行scroll夹取和光标OR。

workpack双生成稳定为`179/227`，SHA256为`5fb8078badf7c567f65cad818b97e2e34331b3d29ba71d67a6468745a4079d4a`；下一项为`0x0044D050`。
