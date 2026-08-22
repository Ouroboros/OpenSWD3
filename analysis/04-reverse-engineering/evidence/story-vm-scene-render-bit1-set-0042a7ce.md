# 剧情 VM 场景渲染 bit1 置位 `0x0042A7CE`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042A7CE..0x0042A7E9`；公共退出 `0x0042B0AE..0x0042B0C8`

opcode：`94`

## 1. Handler语义与顺序

机器块无operand/helper：

```text
EDX = dword_4C9A18
current_record += 2
EDX |= 2
store current_record
write dword_4C9A18 = EDX
context.IP += 2
jump common join with ESI still 0
```

common join发布normalized previous94；因handler未把ESI置1，调用`_AIL_serve`恰好一次后跨帧返回。它不fetch同调用下一条，也没有其他callback。

旧modern把94/95合并为一个numeric case，bit写和yield存在，但漏发previous。现已只拆分并修正opcode94；opcode95保持独立pending，不能继承本结论。

## 2. Owner适配与边界

原版以dword读写`dword_4C9A18`，本handler只OR低字节bit1，完整高24位原样保留；其消费者也以低字节bit0/bit1控制world composition与路径recenter。modern实际owner是`LegacyWorldFrameState::runtime_flags`的u8低位运行标志，并由`LegacyWorldStoryVmRuntime::scene_render_flags`显式接入；对bit1操作与原版可观察低位完全一致，其他六个低位保持。

原版固定全局不为空。modern typed owner为空时在原始dword读取点前返回`runtime_unavailable`，不写flag、不推进IP、不发布previous。

两字节记录从`0x7FFE`开始时，OR2、IP=`0x8000`、previous94和yield均完成；不会错误地因下一fetch失败覆盖结果。

## 3. 资产锁与测试

线性TALK目录锁定39条物理记录/39 probes，全部raw `0x005E`、长度2：

```text
TALK1/2/3/4 = 14/7/3/15
```

real CTest预读`TALK1.DAT@0x000049F4`并放到精确窗口尾；scene flags `A5→A7`、IP=`8000`、previous94和yield通过。

synthetic覆盖四raw alias、精确尾、bit已置时`FF→FF`、其他低位保持、typed owner失败。Linux Story VM三项为3/3。

分类：`platform_adapted`。OR2、两字节长度、IP、previous与yield保持；原dword全局映射到已集成的u8低位owner，owner缺失在原读取点typed-stop。
