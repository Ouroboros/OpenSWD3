# 比较四名角色当前装备与候选替换后的属性差值 `0x0044FAF0`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0044FAF0..0x0044FD26`，278行，无外部FUNCTION CHUNK。六个caller为模式1至6的处理函数；callee为角色存在查询`0x0040DC50`和已关闭的护驾/装备属性应用`0x0044D6E0`。

函数依次处理角色ID `0x1E..0x21`。不存在角色不写输出。存在角色先发布候选raw `+0x46`类别位是否命中角色固定掩码`0x8000/0x4000/0x2000/0x1000`，即使命中失败仍继续计算差值。

每名存在角色从同一56字节基础属性复制两份：

- “当前”副本严格重读并应用该角色16个固定槽记录，每次把`record+0x0C`属性payload交给D6E0。
- “候选”副本只处理前11槽。逐槽读取掩码：先算`(candidate_flags & mask)`，再清结果bit15，只有结果仍精确等于mask时应用候选；否则重读并应用该固定槽。后5槽不应用。

现代`record_bytes`保存完整176字节raw记录；record版D6E0解码入口现明确从`+0x0C`建立payload span，避免把raw头部误作属性源。

## 2. 发布结果

原函数虽在栈上计算多项u16差，最终只发布三项：

1. `i16(candidate.word19-current.word19) + i16(candidate.word8-current.word8)`。
2. `i16(candidate.word20-current.word20) + i16(candidate.word9-current.word9)`。
3. `i16(candidate.word11-current.word11)`。

每项减法先按u16回绕，再按i16符号扩展；前两项随后用32位加法。

## 3. typed-stop

替换掩码短表、固定槽短表、null固定槽及D6E0临时属性不可用均只在原读取/应用点停止。此前角色存在查询、类别匹配发布、槽重读和属性应用前缀保留；当前角色三项差值仅在全部27次应用完成后发布。

## 4. 验证

UT使用完整raw偏移构造来源，覆盖单个存在角色16次当前应用与11次候选/固定应用，得到`{10,10,5}`；验证四次角色查询、26次固定槽读取和27次D6E0调用。另覆盖含bit15替换掩码不能命中、短掩码表、短固定槽表及首次属性应用停止。

workpack双生成稳定为`202/227`，SHA256为`5265a1be06ff9df6f26597fb0cfc4c064984dcc1f1ddce698a92149ea6d73bdf`。`0x0044DBC0`继续等待callee闭环；下一单元为`0x0044E9D0`。
