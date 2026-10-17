# 调试日志文字修正（第 1 遍复查）

exe 中“設定錯誤 … act[%d] qq[%d] dir[%d]”一类格式串共四条（tmp 扫描 swd32.exe）：

| 地址 | 原文 |
|---|---|
| 0x0049F66C | 設定錯誤 GUID[%d]\nact[%d] qq[%d] dir[%d] |
| 0x0049F6B0 | 設定錯誤\nGUID%d ,act[%d] qq[%d] dir[%d] |
| 0x0049FD0C | 設定錯誤 \nact[%d] qq[%d] dir[%d] |
| 0x004A03F0 | 頭定符號設定錯誤 \nact[%d] qq[%d] dir[%d] |

重建有 16 处把 0x0049F66C 的换行写成空格（「設定錯誤 GUID[%d] act[%d]…」，含 talk_event.cpp 的 LogActError），4 处把 0x0049FD0C／0x004A03F0 的换行写成空格（「設定錯誤  act[%d]…」）。exe 中没有空格版本。已全部改回 \n。

受影响的已验证函数（各自重新记 diff 与 exact）：0x00405430、0x00405500、0x00408E70、0x0040E0B0、0x0040F280、0x0040F6D0、0x004120B0、0x004147E0、0x004149B0、0x00414B60、0x00414CE0。
- 差异（diff 轮）：上述调试日志文字与 exe 不符。已改正。复核一致（exact 轮）。
