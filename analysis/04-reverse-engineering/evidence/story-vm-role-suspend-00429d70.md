# 剧情 VM 剧情角色挂起 `0x00429D70`

## 结论

opcode75固定长4字节。它原样读取`+2` selector，不替换`0xFFF0`，调用`sub_40C0D0`取得角色索引，再无条件把输出索引交给`sub_42E5A0`。合法角色完成挂起后推进4、置`ESI=1`，经公共join发布normalized previous75并same-call继续。

现代实现复用已存在的精确等价API`suspend_legacy_world_story_role`；opcode75此前没有C++ case。

## `sub_42E5A0`合法域

该helper已由`legacy_world_story_paths`模块独立实现与测试，主要保持：

- 若角色是受控角色，按camera/player transition把sub-cell坐标退回16像素边界，清四个transition并重定位；
- 扫描固定72个活动对象槽；type1槽保存cursor、role index与目的坐标供恢复；
- 非受控角色若仍处于sub-cell，先清surface占用，按槽内当前方向补齐坐标，再重建surface/空间链；
- 必要时清理动作/移动余数并调用原`sub_411530`等价路径；
- 最终对`role.flags`置bit31路径所有权标记；原helper返回1。

VM只负责selector解析、调用、IP/previous与same-call协议，不复制helper内部逻辑。

## 缺失selector与typed适配

`sub_40C0D0`先把输出写0；普通GUID未找到时`sub_40C100`再把输出写`0xFFFFFFFF`并返回0。opcode75忽略返回值，原版会以index -1计算`role_base-0xD8`并让`sub_42E5A0`继续越界读写。

现代不能安全复制该非法内存破坏，因此在同一lookup点返回`role_not_found`，不推进IP、不发布previous。`0xFFF0`仍是literal selector；`0xFFFE`保留共享受控角色解析。

固定全局路径owner在现代由nullable`runtime.story_paths`表示；缺失时在lookup成功后、helper首次状态访问前返回`runtime_unavailable`。helper中的surface、空间链或方向检查失败映射为`role_path_failed`，保留失败前已发生的副作用。

## 流控与边界

成功路径在`00429D8D..00429DA1`推进4、`ESI=1`并进入公共join。完整记录位于`0x7FFC`时，挂起副作用、IP=`0x8000`与previous75先完成，下一fetch再返回`instruction_out_of_range`。

## 真实资产锁

- 82条物理记录、82个entry probes；
- TALK1/2/3/4分布`19/43/18/2`；
- 全部raw`0x004B`、长度4；
- 65种selector，无`0xFFF0/0xFFFE`；
- 原始offset、word、长度逐条核验零错误。

真实回放使用`TALK1.DAT@0x00007A38`：selector GUID181；在完整`StoryPathHarness`下取得bit31所有权、推进4、发布previous75，再由精确尾下一fetch失败。

## 测试覆盖

- `0xC04B` alias与`0xFFFE`受控角色精确尾；
- 四raw alias的literal`0xFFF0` missing typed-stop；
- owner缺失、selector截断；
- TALK1真实记录；
- helper原有完整路径/槽/空间/失败测试；
- 剧情VM三项测试通过。

## 分类

分类：`platform_adapted`。合法角色域复用`sub_42E5A0`精确等价实现；原版index -1越界和固定全局owner改为typed失败，其余selector、调用顺序、IP、previous与same-call保持汇编语义。
