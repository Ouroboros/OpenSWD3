# 剧情 VM P1 完整 handler 工作包

状态：P1 scope lock 完成；146 个 handler 全部为 `pending_audit`，不继承旧语义、C++ case、资产观察或 CFG 的完成状态。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

锁定函数：`0x00427920..0x0042D4F3 sub_427920`

LST SHA-256：`701732b5481ba34876b62ca97535c9463f65ec3feb2ed745c03772dd4bc3ad8b`

## 1. 可重复生成边界

`analysis/tools/build_story_vm_dispatch_inventory.py` 已从旧 ASM/PE 双源改为只读取完整 LST：

- 指令 guard 从 LST 地址行读取；
- 两张一级跳转表通过 LST `dd offset` 标签解析；
- 每个可见表行的首个 dword 同时与 LST little-endian 机器字节核对；
- 两张内部 selector 跳表直接从 LST byte column 重建；
- 157/73 字节 selector 表逐地址从 LST byte column 重建；
- 输入 hash、98/94 表项、11 条分派范围、2 张内部表、198 个显式值、146 个唯一入口和完整 14 位域均有硬断言。

重跑后：

- `story-vm-opcode-dispatch.tsv` 与旧基线逐字节相同；
- `story-vm-entry-target-groups.tsv` 与旧基线逐字节相同；
- `story-vm-internal-opcode-switches.tsv` 与旧基线逐字节相同；
- `story-vm-dispatch-ranges.tsv` 只把两处证据描述从 `PE dwords` 改为 `LST dwords`。

这证明旧 198/146/25 数字没有因切换真值来源而漂移，同时移除了已失效的 `swd3.exe.asm` hash 和 PE 读取依赖。

## 2. 146 个 handler 工作行

`analysis/tools/build_story_vm_handler_workpack.py` 每次先调用上述 LST 生成器，再写：

- `inventory/story-vm-handler-workpack.tsv`：146 个唯一一级入口；
- `inventory/story-vm-runtime-paths.tsv`：17 条非 handler 顶层路径。

handler 表按最小 opcode 排定 `audit_order`，每行包含：

- 入口地址、完整 opcode 组、是否共享、一级 dispatch 来源；
- 两个内部 refinement 的归属；
- 已有 length/control 规则，仅作导航；
- `0..124` 人工语义行存在性，仅作导航；
- 当前 C++ case 存在性，仅作导航；
- 当前 TALK 资产观察，仅作导航；
- static triage direct calls 映射出的候选端口模块，仅作导航；
- unresolved edge 导航；
- 固定 `closure_status = pending_audit` 和独立 LST 重审规则。

生成器硬断言：

```text
198 explicit opcodes
146 unique handler entries
25 shared entries
50 modern C++ case labels
146 pending_audit
0 closed handlers
```

25 个共享入口由实际 dispatch rows 重新分组，不从文档抄录。最大组仍是 `0x00427B8F` 的 `1-6,89-90` 八个变体；两个内部 refinement 分别挂在 `0x0042B070/0x0042B074` 和 `0x0042C567`。

## 3. 显式 opcode 域

工作包锁定的 198 个显式值是：

```text
0..193
1024
1025
1026
16383
```

它们对应 146 个入口。其余 14 位域通过 runtime path 表压缩为默认非法范围：

```text
194..1023
1027..16382
```

默认入口与 opcode 0 共用 `0x0042D230`，但 handler 工作表只列显式 opcode 0；两个巨大默认范围保留在 runtime path 表，避免把“146 个显式 handler 组”与“完整 14 位默认覆盖”混为一谈。

## 4. 17 条非 handler 路径

`story-vm-runtime-paths.tsv` 独立锁定：

1. 入口/未激活 gate；
2. 初始 Talk 文件和 0x8000-byte window 装载；
3. u16 fetch 与 `& 0x3FFF` decode；
4. 98 项主表；
5. 94 项次表；
6. 默认非法入口；
7. 6 dword + 157 byte numeric refinement；
8. 9 dword + 73 byte flag refinement；
9. handler-specific window transfer 导航集；
10. 公共 continue/yield join；
11. 特殊值 1024；
12. 特殊值 1025；
13. 特殊值 1026；
14. TalkEnd 16383；
15. handler fatal return-zero；
16. ordinary/inactive return-one；
17. initial-load-failure return-zero。

现有 31 行 control-transfer 目录标出窗口/控制转移 opcode `15-17,21-24,32-33,35-36,41,87,110-111,126-127,129-130,138,161,163-168,184-187`。这只是定位集合；P2 必须逐所属 handler 重新证明目标读取、同文件/跨故事装载、IP 清零、同帧继续与失败顺序。

## 5. 导航覆盖不等于关闭

当前导航统计：

- 人工语义：125 opcode，落在 100 个 handler；46 个 handler 尚无人工语义行；
- 现代 C++：50 个 case label；40 个 handler 的显式 opcode 全有 case、4 个仅部分有 case、102 个无 case；
- 当前 TALK 资产：143 个 opcode 有观察记录、55 个未观察；对应 109 个 handler 有任一观察、37 个完全未观察；
- static triage：146 个 handler 当前没有 unresolved edge，但该 CFG 明确是过近似导航，不证明分支可行性或业务语义。

因此 P1 没有把任何一行标为已实现。`all` C++ case presence 也不能关闭 handler：共享入口仍可能有不同 operand、修饰位、等待、异常或窗口路径，旧实现同样必须按所属组重审。

## 6. 候选端口依赖

每个 handler 至少归入 `story_scene`，并按 static triage direct call 结合 `module-function-ownership.tsv` 生成候选依赖。146 行覆盖：

```text
story_scene     146
runtime_platform 48
world_map        44
special_modes    21
asset_runtime    16
resource_io      12
audio_video      11
rendering         3
input_time_rng    3
battle            2
```

这些列只决定 P2 需要准备哪些窄端口和相邻合同，不证明 callee side effect。每组关闭时必须重新从该 handler 的完整 LST 控制流核对真实调用可达性、顺序和 reload 点。

## 7. P2 起点

P2 严格按 `audit_order` 逐组执行。第一个停止点是默认非法入口：

```text
entry = 0x0042D230
explicit opcode = 0
full default ranges = 194..1023, 1027..16382
```

该组必须同时覆盖显式 opcode 0 与完整默认范围的同入口合同；只有完成 LST→测试→实现→C++→LST 循环后，才可把工作表第一行从 `pending_audit` 改为关闭状态。下一组才是共享入口 `0x00427B8F` 的 `1-6,89-90`，不得按当前剧情命中顺序跳组。
