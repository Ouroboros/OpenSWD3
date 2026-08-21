# 剧情 VM 六字节音效请求 `0x0042B723`

状态：`platform_adapted`、有效运行域`assembly_exact`、`unit_tested`、`asset_absence_verified`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042B723..0x0042B738`

opcode：`113` / `OP_113_PLAY_SOUND_EFFECT_WITH_UNREAD_PADDING`

## 1. 记录与读取顺序

物理记录固定为6字节：

```text
+0  u16 opcode
+2  u16 sound id
+4  u16 未读padding
```

handler先读取当前全局sample mix level，再只读取`+2`的u16 sound id。`+4`从未解引用、比较或传给helper；即使窗口只有opcode与sound id四字节，机器仍提交音效并把物理指针与u16 IP各推进6字节。

sound id通过`mov ax`取得低16位；`sub_485610`又显式执行`AND 0xFFFF`，因此编号严格零扩展。0、`0xFFFF`及所有中间值均提交，handler不验证资源范围。

## 2. sample wrapper与返回忽略

113与已闭环opcode59复用`sub_485610`。wrapper把level先按i32执行wrapping `<<7`，再作signed向零`/11`，并以以下固定请求调用sample manager：

```text
sound id       u16
volume         wrapping(level << 7) / 11
pan            0
loop count     1
existing data  null
named aux      0
```

SDL `StoryVmPorts`复用`audio_video::play_legacy_sample`与实际`LegacySampleManager`。0编号、越界资源、无空闲sample或后端失败均不能控制VM：原调用者不测试`sub_485610`返回值，现代port同样不把返回值暴露给handler。

固定全局mix level由生产port按当前世界帧借用；sample manager的资源边界和SDL后端替代原Miles/裸目录，构成平台适配。

## 3. 推进、previous与让出

wrapper返回后，handler清理两个参数，进入共享六字节尾：

1. 物理指针与u16 IP各加6；
2. common join发布normalized previous113；
3. 正常入口zero continuation carry下执行一次`_AIL_serve`；
4. 返回1并yield，不在同调用读取后继。

本handler不写continuation carry，handler-specific `ESI`保持0。特殊指令可能继承的全局common-join carry仍属于独立`common_join` runtime path和P3组合验收，不从本handler闭环继承结论。

现代顺序为play request→IP+6→previous113→audio maintenance→yield。operand缺失在play之前停止；未读padding缺失不会停止。

## 4. 资产锁

线性TALK目录中opcode113为0条物理记录/0 probes，因此使用`asset_absence_verified`，不伪造real replay。

四文件原始双字节字样共有：

```text
0x0071 45
0x4071  0
0x8071  0
0xC071 10
```

这55处均不是已证明的线性指令入口，不能代替资产记录。四raw alias由synthetic覆盖。

## 5. 验证

synthetic覆盖四raw alias、sound id `0/1/0x1234/0xFFFF`、播放请求先于IP/previous/audio、后端结果不参与流控、sound operand截断、仅四字节可用时未读padding且IP到`0x8002`、完整六字节精确尾到`0x8000`，以及完成后不取后继。

Story VM synthetic、real及initial-session三项通过；Linux core 186/186、app 192/192通过。未启动原版或OpenSWD3游戏EXE。
