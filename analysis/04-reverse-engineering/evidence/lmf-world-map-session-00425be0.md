# LMF 世界地图会话组合：`0x00425BE0..0x004267D3`

状态：`assembly_exact`、`asset_verified`、`platform_adapted`；原程序动态差分待统一 oracle

唯一行为来源：`swd3.exe.lst`。IDA 原型和旧证据只用于导航，闭环以完整函数体与唯一调用点为准。

## 1. ABI、调用点与返回

`sub_425BE0` 是两参数 cdecl：

```text
arg_0 +0x04 = map_id
arg_4 +0x08 = runtime map-state pointer
```

唯一调用点 `sub_40C130:0x0040C6AB..0x0040C6C7` 先压入状态地址 `dword_4B7930`，再把归档地图号按 `0xFFFF` 截断后压入；调用后由调用者清理 8 字节实参，并立即测试 EAX。成功尾部 `0x004267C7` 明确返回 1；公共失败尾部 `0x00425DE8` 明确返回 0。

现代入口以完整 `u32` 接收归档地图号；当前调用链的 descriptor 值保持原调用者的 16 位有效域。原裸状态指针由 `LegacyWorldMapSession` 拥有型结果替代，成功/失败由明确状态枚举表示。

## 2. 完整装载顺序

独立 LST→C++→LST REVIEW 收敛后的合法域顺序为：

```text
LMF 路径与尾索引查找
→ 固定 0x2000 头读取并确认 MSFp/MSF2
→ progress 15
→ header +0x04 seek、高 word 门、五字段与名称
→ sub_411620 三组空间工作区重建
→ sub_426840 CM 获取/生成
→ progress 60
→ 地表原样表读取与原地 word 压缩
→ 地表压缩块读取、解压与临时输入释放
→ 地表后变长记录及事件头插转换
→ progress 65
→ 相对引用目录
→ progress 70
→ header +0x14 目录及第一类角色/空间链
→ progress 75
→ header +0x18 索引对象目录
→ 每个对象：seek、头读取、payload seek、payload 读取、解压、sub_401B70 后处理、输入释放
→ progress 80
→ header +0x1C 目录及第二类角色/空间链
→ progress 85
→ 临时 owner 释放、文件关闭、返回 1
```

原函数完成后，调用者才附加 MAPS 角色并运行 `sub_40F280` 格绑定。现代 `pre_role_binding_stage` 和受检格绑定保留在更外层组合入口，不提前进入上述 `sub_425BE0` 内部阶段。

物理子格式继续分别由以下证据约束：

- [`lmf-tail-index-00425cfa.md`](lmf-tail-index-00425cfa.md)
- [`lmf-map-header-00425dfd.md`](lmf-map-header-00425dfd.md)
- [`lmf-surface-grid-0042605b.md`](lmf-surface-grid-0042605b.md)
- [`lmf-post-surface-records-00426195.md`](lmf-post-surface-records-00426195.md)
- [`lmf-referenced-record-directory-00426256.md`](lmf-referenced-record-directory-00426256.md)
- [`lmf-offset14-directory-004262cc.md`](lmf-offset14-directory-004262cc.md)
- [`lmf-indexed-object-directory-0042642d.md`](lmf-indexed-object-directory-0042642d.md)
- [`lmf-offset1c-directory-00426660.md`](lmf-offset1c-directory-00426660.md)
- [`lmf-map-business-004261ce-00426798.md`](lmf-map-business-004261ce-00426798.md)

## 3. 七个进度点

首轮旧实现只在 SDL 外层调用 `-1/100`，并把事件、两类角色和索引对象后处理集中到完整物理读取之后。反向 REVIEW 共纠正三类真实差异：

1. 内部进度实际是 `15/60/65/70/75/80/85`，旧摘要漏掉 `0x00426049` 的 60；
2. `progress 15` 位于头签名确认后、`header +0x04` seek 和名称复制前，不能放在完整 header reader 返回后；
3. 事件、`+0x14` 角色、索引对象后处理和 `+0x1C` 角色必须分别在 65/75/80/85 前完成。

`LegacyLmfReadObserver::map_header_signature_ready` 把 15 保留在真实物理读取器中间；世界会话阶段化构建业务状态；索引对象 consumer 改为逐物理对象调用，保证多对象时不会把全部 `sub_401B70` 后处理推迟到目录读完。

## 4. `_AIL_serve` 的 28 个静态点

完整函数体有 28 个 `_AIL_serve@0` 静态调用点。循环执行后的动态次数为：

```text
22 + referenced_record_count + 5 * indexed_object_count
```

其中七个 `sub_40ED60` 进度调用内部自己的两次音频维护不计入这 28 个显式点。首次装载被 flag 70 抑制时，进度函数提前返回，但 `sub_425BE0` 的显式维护仍必须执行。

现代观察器按原操作位置恢复：

- 尾索引：路径准备 2 次、打开后、尾部读取后、成功释放尾缓冲后；
- 头：`header +0x04` seek 成功后；
- CM 前和 progress 60 后；
- 地表：原表读取后、原表转换后、压缩块读取后、解压并释放临时输入后；
- 地表后窗口读取后、progress 65 后；
- 每条相对引用读取后、progress 70 后；
- `+0x14` 窗口读取后、progress 75 后；
- 索引目录 seek 后和读取后；每个对象在对象 seek 后、头读取后、payload seek 后、payload 读取后、后处理并释放输入后各一次；
- `+0x1C` seek 后和读取后；progress 85 后一次。

合成测试以一条引用和一个索引对象固定完整 28 次维护与七个进度的交错；双对象首 consumer 失败向量确认第一个对象仍执行第 5 次维护，随后立即停止，不进入第二对象、progress 80 或 `+0x1C`。

## 5. CM、工作区与 8 位 palette

`0x00425F68 sub_411620` 只重建三块 `(height * 4 + 0xA0)` 空间工作区，不是 CM loader。真正的 CM 调用是 `0x00426044 sub_426840`；其结果写入状态 `+0x20` 后才调用 progress 60 并继续地表读取。CM 子链由 [`cm-cache-runtime-00426840-004272b8.md`](cm-cache-runtime-00426840-004272b8.md) 约束。

8 位 palette 属于调用者 `sub_40C130` 的成功后阶段：从 CM 首 `0x200` 字节取得 256 个小端 RGB555 项并执行 forward 转换；tile 数据从 `0x200 + tile_index * 0x100` 开始。现代 `LegacyWorldRenderSession` 继续拥有 CM、palette 和地图会话，未把 palette 伪造成外部文件。

## 6. 平台适配边界

本函数归类 `platform_adapted`：

- 单一原文件句柄和裸暂存缓冲改为分阶段 `LegacyFile`/vector/RAII owner；
- 裸全局状态、链表和空间表改为 typed session、角色 vector 与受检索引；
- 损坏 seek/read/count/乘法、分配和解压失败在对应原始危险点停止；
- CM 缓存写入 OpenSWD3 自有缓存目录，不改写原游戏 `Data`；
- 资源观察器只恢复合法域进度、音频维护和逐对象 consumer 顺序，不把渲染 owner 倒置回资源层。

这些适配不改变当前资产上的字段宽度、32 位回绕位置、目录顺序、角色筛选、逐对象后处理、进度或音频维护顺序。原程序损坏资源上的越界、残留缓冲读取和未受检指针不复现。

## 7. 验证

- 合成地图会话固定完整物理顺序、统一 map offset、七进度里程碑、28 次维护交错、observer 返回前卸载、逐对象 consumer 失败短路及所有首失败边界；
- 地图业务 UT 固定事件反向链、两类角色、三组空间链和格绑定；
- 索引对象 UT 固定逐对象 command-stream 转换、字段缩放与失败索引；
- 真实 `huge.lmf` 地图 22/24/500 固定偏移、尺寸、地表输出、事件数、角色数 49/29/1、七进度和 `22 + refs + 5 * objects` 维护总数；`progress 15` 时完整 header 尚未提交；
- 渲染会话继续固定地图 24 RGB565 整帧哈希 `0x947C15A53487BF9A` 和 8 位地图 4 哈希 `0xF00691829E9FE2D5`；
- 本轮定向 CTest：indexed objects、map business、map/render/runtime session synthetic/real 共 8/8 通过；增量构建 0 warning、0 error。

完整门禁最终通过：Linux `core` 185/185、Linux `app` 191/191、Windows LLVM
`app` 191/191 CTest；两端应用均成功链接。未启动原版或 OpenSWD3 游戏 EXE。
