# 剧情 VM 角色转向并挂起 `0x00429DA6`

## 结论

opcode76固定长6字节。它分阶段读取两个selector：第一selector在`+2`，仅它支持把`0xFFF0`替换为当前source GUID；第一lookup完成后才读取`+4`第二selector，第二参数保持raw。两个角色都存在时，以各自world坐标加动作范围`field_2c/field_30 * 8`组成中心点，计算距离和量化朝向，更新第一角色action并刷新，最后调用`sub_42E5A0`等价helper挂起第一角色。成功后推进6、发布normalized previous76并same-call继续。

现有C++的合法域中心点、朝向和helper复用正确，但存在三处差异：整条6字节预验掩盖第一lookup unsafe点；`story_paths` owner检查错误地早于action写入/刷新；成功漏发previous76。现已修正。

## 汇编顺序

1. 读取第一selector；仅`FFF0`替换source GUID；
2. `sub_40C0D0`第一lookup，失败诊断仍以index -1读角色字段；
3. 此后才读取第二selector并raw lookup；
4. 以wrapping u32中心坐标调用`sub_411E20`，再以`sub_411F00`映射朝向；
5. 第一角色`action.base_variant=0`、`variant_delta=0`；距离至少4才写量化朝向；
6. `wait_remaining=0`并调用`sub_4321E0`刷新；刷新失败只诊断，仍继续；
7. 调`sub_42E5A0`挂起第一角色；
8. 推进6、`ESI=1`、公共join发布previous76并same-call继续。

## 非法域与typed适配

两次lookup失败原版都继续以index -1读写角色数组前内存。现代分别在同一unsafe点返回`role_not_found`：第一失败发生在第二selector读取前；第二失败发生在任何action写入前。第二selector的literal`0xFFF0`不会替换。

固定全局路径owner映射为nullable`runtime.story_paths`。owner缺失在action三字段写入与刷新之后、挂起helper首次访问前返回`runtime_unavailable`，保留之前副作用。helper checked失败映射为`role_path_failed`并保留已发生的action/helper副作用。

## 边界

- `0x7FFC`只有opcode+第一selector：第一lookup missing先返回`role_not_found`；第一lookup命中才在第二读取点`operand_out_of_range`；
- 完整记录位于`0x7FFA`时，action更新、挂起、IP=`0x8000`和previous76先完成，下一fetch再返回`instruction_out_of_range`。

## 真实资产锁

- 449条物理记录、450个entry probes；
- TALK1/2/3/4分布`181/140/56/72`；
- 全部raw`0x004C`、长度6；
- 204种selector组合；第一selector有5条`0xFFF0`，第二selector无`0xFFF0`，两者均无`0xFFFE`；
- 原始offset、word、长度逐条核验零错误。

真实回放使用`TALK1.DAT@0x00004A68`：GUID191朝向GUID1，完成action刷新、bit31挂起、推进6、previous76和精确尾下一fetch失败。

## 测试覆盖

- ordinary same-call继续到opcode14；
- 四raw alias第一missing且第二不可读；
- 第一命中后第二截断；
- 第一`FFF0`替换、第二literal`FFF0` missing；
- owner缺失但action副作用先完成；
- `0xC04C`精确尾完整成功；
- TALK1真实记录与真实opcode21前缀链unsafe点；
- 剧情VM三项测试通过。

## 分类

分类：`platform_adapted`。合法域中心点、朝向、action刷新、挂起、IP、previous和same-call均与汇编一致；原版两处index -1越界和固定全局owner改为typed失败。
