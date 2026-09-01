# 战斗 LEVEL.DAT 等级需求读取 `0x00477290`

状态：`platform_adapted`、`unit_tested`、`caller_reclaimed`。

## 1. 完整权威范围

唯一行为真值为 `swd3.exe.lst`。函数主体为 `0x00477290..0x004773F7`，从 `proc` 到 `endp` 共 186 个物理行、123 条带机器码和真实助记符的实际指令、10 个静态 call、9 个跳转、7 个局部标签和 3 个返回点，没有外部 `FUNCTION CHUNK`。

四个直接 caller 及调用点为：

- `0x00411030` 于 `0x00411244`：队伍成员 selector 16 写入后的 field14 更新；
- `0x0043AAA0` 于 `0x0043AC9A`：四入口菜单动画的等级差值文字；
- `0x0044A280` 于 `0x0044A533`：角色属性页面的等级需求值；
- `0x00467C50` 于 `0x00467CD2`：战斗升级资格的 signed 经验门。

此前计划把本函数导航命名为“战斗定义文字释放”，与权威 LST 不符。实际职责是共享、惰性打开 `level.dat`，按 group/level 读取一条固定窗口记录并发布其中最后一个 tag 0 值。

## 2. 共享文件会话与目录寻址

函数先检查共享 open 标志。未打开时在固定路径缓冲区形成 `level.dat` 路径并按原参数打开；失败返回 0，不分配流，且 open 标志保持未置位，下一次调用继续重试。成功后发布共享 handle 和 open 标志；后续所有 caller 复用同一会话。

目录槽计算严格保持 32 位回绕：

```text
index = group * 100 + level
slot  = 0x70 + 4 * index
```

第一次 seek 到目录槽，读取 4-byte little-endian 相对偏移。读取结果和 bytes-read 不构成现代成功门：短读留下的陈旧 dword 仍参与后续计算。记录绝对偏移为 `relative + 0x200`，同样按 32 位回绕。第二次 seek 到记录后固定读取 `0x400` 字节。

真实 `/mnt/e/Game/swd3/LEVEL.DAT` 为 8206 字节。只读回归固定了两个样本：

- group 1、level 2：目录相对偏移 1612、文件偏移 2124、输出 20；
- group 2、level 50：目录相对偏移 4950、文件偏移 5462、输出 63950。

两次连续查询验证只打开一次文件，但每次仍执行两次 seek、两次 read、一次分配和一次释放。

## 3. 固定流与 tag 解释

目录读取后调用 `0x00487C10` 分配固定 `0x400` 字节流，并立即按原 `rep stosd` 语义全清零。分配结果为零时，原程序会在第一次清零 store 处故障；typed 实现返回 `stream_zero_typed_stop`，不伪造正常失败。

记录流先读取首 word：

- 非零：立即调用 `0x004885A0` 释放流并返回 0；
- 零：进入 tag 循环。

循环保持原低 word 解释和推进顺序：

- tag 0：先把 stream cursor 推进 26 字节，再按 `rep movsd` 的 6 个 dword和末尾 1 个 word逐访问复制；把复制块 `+0x16` 的 dword 写到 caller 输出。多个 tag 0 依次覆盖，最后一个值获胜；
- tag 1：只跳过后续一个 word；
- tag 5：释放流并返回 1；
- 其他非零 tag：不跳 payload，继续从下一 word 取 tag。

不添加现代边界、计数或未知 tag 跳过规则。已知 `0x400` 窗口耗尽后，只在下一次真实 word/dword 访问处停止。

## 4. 故障前缀与寄存器

`LegacyBattleLevelRequirementLoadResult`保存目录槽、相对/绝对偏移、流 token/cursor、停止偏移、open/seek/read/allocation/release 次数、部分复制字节数、输出写次数和值，以及 live EAX/ECX/EDX。

- `stream_access_typed_stop`覆盖目录槽越界、tag word越界及 tag 0 的逐 dword/word复制越界；tag 0 已先推进的 cursor、已完成复制字节和剩余 ECX 均保留；
- `output_access_typed_stop`只发生在原 tag 0 的 caller 输出 store，保留此前完整26字节复制；
- 正常 tag 5 返回 EAX 1，首 word 非零或 open 失败返回 EAX 0；
- 正常返回路径都严格在原位置释放临时流，typed-stop 不伪造释放。

## 5. owner 与 caller 回收

`LegacyBattleLevelDatabasePort`及其 `LegacyBattleLevelDatabaseState`是唯一 typed 文件 owner。SDL 层持有唯一 `LEVEL.DAT` 文件、handle/stream token和共享 state；剧情 VM、菜单转场、角色属性渲染及战斗升级端口都转发到该 owner，不各自打开文件。

四个已关闭 caller 均删除旧 opaque LEVEL 查询：

- selector 16 先提交低 byte，再直连 loader；文件打开失败或首word非零的正常0返回仍把初始输出1写入field14；typed-stop保留低 byte且阻断field14、instruction offset推进和对话刷新等后续副作用；
- 四入口动画先完成 label 绘制，再直连 loader；typed-stop阻断等级文字及该项目后续副作用；
- 角色属性页在前22条命令后由函数体直接调用 loader；旧 `calculate_value` 枚举只保留 reserved alias且生产零调用；typed-stop不把一次未返回的调用计为完成 helper；
- 战斗升级在原经验门前直连 loader；frame coordinator 的旧查询槽保留 reserved 数值但生产零调用。

## 6. 验证范围

独立 loader 测试覆盖 synthetic tag 0/1/2/5、多个 tag 0 最后值覆盖、非零首 word、打开失败、零分配、输出不可访问、目录/流越界、32 位目录回绕、tag 0 部分复制及真实 LEVEL.DAT 双查询。四个 caller 均有分配清零故障回归，固定故障前已提交字段、绘制、命令计数、live 寄存器以及无伪释放。

最终验证为完整Linux core `190/190`、AddressSanitizer `190/190`和Linux app `196/196`全部通过；最终日志没有OpenSWD3源码warning、测试失败、sanitizer finding或runtime error。新增及触碰C++文件通过clang-format `--dry-run --Werror`，`git diff --check`通过。inventory双跑稳定为`262/422 = 253 platform_adapted + 9 assembly_exact + 160 pending_audit`，SHA256为`8c4f0d4aa39562f76f313ca9023206cb8bce026910f1afc8c16f29e25608ba50`。

当前缺少原版共享文件对象、短读后的真实陈旧缓冲、堆 token、四个 caller 输入和 EAX/ECX/EDX 联合捕获后端；`original_diff_verified` 登记为 `blocked_runtime_oracle`。
