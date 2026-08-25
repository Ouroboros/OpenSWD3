# 角色/道具对话列表列头设置 `0x004103C0`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x004103C0..0x00410487`，共101行，无外部`FUNCTION CHUNK`。caller为共享Windows对话过程两条初始化/页面切换路径。

函数接收列表控件句柄，读取当前dialog page，构造一份原布局列描述并按索引发送`0x101B`插列消息。现代把窗口消息隔离为窄列插入端口，不在兼容核心持有HWND。

## 2. 列描述

每列固定：

- mask `0x0F`。
- format 0。
- text capacity 4。
- subitem等于当前列索引。

四列按原Big5字节与宽度为：

```text
0  物品    120
1  數量     64
2  號碼     64
3  附加值   96
```

附加值虽然是三个Big5字符共6字节，原`cchTextMax`仍固定4，现代请求不能按字符串长度“修正”。

## 3. 页码门与返回

current page以signed i32比较：小于5插四列，否则只插前三列。因此negative page也插四列。循环完成后固定返回1；零次分支在当前常量域不可达，但现代结果仍保持legacy return 1。

窗口端口不可用时只在当前插列消息处typed-stop，保留此前已经插入的列，legacy return槽仍为1。

## 4. 验证

`special_modes.legacy_initial_menu`覆盖page 4四列、page 5三列、negative page四列、全部Big5字节、宽度/mask/format/text capacity，以及第三列插入不可用时保留前两列。
