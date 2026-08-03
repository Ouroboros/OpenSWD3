# 辅助文本对话框汇编证据：0x00425470

状态：平台无关字节合同已实现；SDL3 文本界面待实现

来源：`swd3.exe` 完整汇编

函数范围：`0x00425470` 至 `0x0042556C`

## 1. 消息和返回

- `WM_INITDIALOG (0x110)`：初始化编辑框，返回一；
- `WM_COMMAND (0x111)`：处理提交或取消，返回零；
- `WM_CLOSE (0x10)`：以数值零结束对话框，返回一；
- 其他消息：返回零。

`WM_COMMAND` 只使用 `wParam` 低 16 位：

- ID `1`：提交；
- ID `0x40C`：以结果零结束；
- 其他 ID：无动作。

## 2. 初始化

编辑控件 ID 为 `0x40A`。初始化顺序为：

1. 发送 `EM_SETLIMITTEXT (0xC5)`，`wParam = 8`；
2. 发送数值消息 `0x46A`，`wParam = 1`，`lParam = 0x42`；
3. 把全局初始字节串 `sz` 写入编辑控件。

`0x46A` 的具体平台含义不影响当前公共合同，先保留原数值，不按名称猜测。

## 3. 提交的七字节行为

提交调用：

```text
GetDlgItemTextA(hDlg, 0x40A, FileName, 8)
```

`cchMax = 8` 包含结尾 NUL，因此即使编辑框允许输入八字节，实际提交最多只
复制七字节。第八字节被静默丢弃。

- 复制后长度为零：显示原文本/标题的普通消息框，不结束对话框，也不改写
  已提交全局字节串；
- 非空：把最多七字节复制到全局 `String`，以结果一结束对话框。

这是 ANSI 字节合同，不是 Unicode 字符合同。截断可以发生在现代 UTF-8
多字节序列中间；初步 1:1 阶段必须保留，不能改成八个码点或自动修复无效
UTF-8。

## 4. 当前实现

实现与 UT 位于：

- `include/openswd3/app/auxiliary_text_dialog.hpp`；
- `src/app/auxiliary_text_dialog.cpp`；
- `tests/unit/app/auxiliary_text_dialog_test.cpp`。

UT 覆盖低字命令、空输入、旧值保持、七字节截断、嵌入 NUL、多字节中间
截断以及结果 `0/1`。SDL3 编辑框、文本输入/IME、警告界面和真实工具链
验收尚未完成，因此 `0x00425470` 当前为 `partial`。
