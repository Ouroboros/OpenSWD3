# 剧情 VM 画面颜色过渡取消 `0x00429D43`

## 结论

opcode74固定长2字节、无操作数。它按顺序把颜色过渡的red/green/blue step和signed countdown四个dword清零，保留三个current与三个target。随后推进2、置`ESI=1`，经公共join发布normalized previous74，并在同一次VM调用继续取下一指令。

现有C++的四字段与顺序正确，但漏发previous74；现已补齐。

## 字段映射

- `dword_4A9A00`：red step；
- `dword_4A94B8`：green step；
- `dword_4C97F0`：blue step；
- `dword_4A9934`：signed countdown。

映射同时由`sub_4146F0`颜色过渡证据、opcode52写入和opcode53等待读取交叉确认。opcode74不写current/target，不调用渲染或刷新函数。

## 流控与边界

`00429D43..00429D61`依次完成四次写零，`00429D6B`跳`00427E84`共享`+2/ESI=1`尾，再到`0042B0AE`发布previous并same-call继续。

完整记录位于`0x7FFE`时，四次清零、IP=`0x8000`和previous74先完成；下一fetch返回`instruction_out_of_range`。因此不能把精确尾误建模为yield。

现代`frame_color == nullptr`在第一次状态访问前返回`runtime_unavailable`，不推进IP、不发布previous；这是原版固定全局的最小typed owner适配。

## 真实资产锁

- 13条物理记录、13个entry probes；
- TALK1/2/3/4分布`1/2/1/9`；
- 全部raw`0x004A`、长度2；
- 原始offset、word、长度逐条核验零错误。

真实回放使用`TALK1.DAT@0x00005461`，放在`0x7FFE`精确尾，验证四字段清零、current/target保留、previous74及下一fetch失败。

## 测试覆盖

- ordinary same-call继续到opcode14，最终previous由14覆盖；
- 四raw alias精确尾，均先完成清零与previous74；
- 空owner首次访问前停止；
- TALK1真实记录；
- 剧情VM三项测试通过。

## 分类

分类：`platform_adapted`。有效owner域内四写顺序、保留字段、IP、previous与same-call行为与汇编一致；仅固定全局改为nullable typed owner。
