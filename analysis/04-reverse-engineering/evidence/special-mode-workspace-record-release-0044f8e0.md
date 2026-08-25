# 释放特殊模式工作区记录链 `0x0044F8E0`

状态：`assembly_exact`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0044F8E0..0x0044F912`，29行，无外部FUNCTION CHUNK。唯一caller为`0x0044E9D0`模式2退出路径；唯一callee为释放器`0x004885A0`。

循环顺序为：

1. 读取当前head；null直接返回。
2. 读取当前记录next并立即发布为新head。
3. 释放当前记录`+0xAC`文本owner。
4. 释放当前176字节记录。
5. 从已发布head继续。

该指令序列与已关闭的`0x0044D5A0`完全同形，仅head来源不同：D5A0接收head地址参数，F8E0固定使用特殊模式工作区head。因此typed实现直接复用`release_legacy_player_item_chain`，不保留第二份逻辑或opaque端口。

## 2. typed-stop

原始自环在释放首节点后会再次读取已释放节点。适配只在该重复读取点停止；此前head发布、文本owner释放和记录释放均保留，head仍为原自环地址。

## 3. 验证

UT独立覆盖空链、两节点链的owner/记录释放顺序和调用数，以及自环首次完整释放后的typed-stop前缀。

workpack双生成稳定为`199/227`，SHA256为`2edf4dbc77046c302e446c5bb577071702b4005562114e68dffc865dbafc9390`。`0x0044DBC0`仍等待callee闭环；下一依赖叶子为`0x0044FA40`。
