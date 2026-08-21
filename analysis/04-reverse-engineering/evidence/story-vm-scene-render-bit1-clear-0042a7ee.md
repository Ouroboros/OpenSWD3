# 剧情 VM 场景渲染 bit1 清除 `0x0042A7EE`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042A7EE..0x0042A809`；公共退出 `0x0042B0AE..0x0042B0C8`

opcode：`95`

## 1. Handler语义与顺序

机器块独立执行且无operand/helper：

```text
EDX = dword_4C9A18
current_record += 2
EDX &= FFFFFFFD
store current_record
write dword_4C9A18 = EDX
context.IP += 2
jump common join with ESI still 0
```

common join发布normalized previous95；ESI保持0，故立即跨帧返回，不fetch同调用下一条，无audio/callback。

旧modern numeric case已有低位clear和yield，但漏发previous95。本次独立补齐，不从opcode94继承验收。

## 2. Owner适配与边界

原版dword AND仅清低字节bit1，并保留完整高24位及其他低位。modern实际owner是已集成world-frame `runtime_flags`的u8低位标志，由`LegacyWorldStoryVmRuntime::scene_render_flags`接入；`& ~2`与原版可观察低位一致。

modern typed owner为空时在原始dword读取点前返回`runtime_unavailable`，不改flag、不推进IP、不发布previous。

从`0x7FFE`开始的完整记录先完成clear、IP=`0x8000`、previous95和yield，不进行下一fetch。

## 3. 资产锁与测试

线性TALK目录锁定25条物理记录/25 probes，全部raw `0x005F`、长度2：

```text
TALK1/2/3/4 = 9/5/2/9
```

real CTest预读`TALK1.DAT@0x00004A22`并放到精确窗口尾；scene flags `A7→A5`、IP=`8000`、previous95与yield通过。

synthetic独立覆盖四raw alias、精确尾、bit已清时`A5→A5`、其他低位保持、typed owner失败。Linux Story VM三项为3/3。

分类：`platform_adapted`。AND `FFFFFFFD`、两字节长度、IP、previous与yield保持；原dword全局映射到已集成u8低位owner，owner缺失在原读取点typed-stop。
