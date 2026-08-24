# 炼妖祭坛原图surface释放 `0x004405C0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。完整函数范围`0x004405C0..0x00440623`，48行；caller为E3D0 phase3一次和E800 phase3两处。唯一callee为4885A0释放器，共四次。

严格顺序：

1. 写FC320/fourth reset为0。
2. 写FCD20/interaction phase为4。
3. 无条件按FCB90、FCD1C、FCA9C、FCA98顺序调用4885A0；对应FDE0 surface槽`1,0,2,3`，即使token为0也不跳过。
4. 四次释放完成后才把四token全部清0。
5. FCAC8/animation ring offset清0。
6. EAX保留第四次释放返回值。

新增共享`LegacyStandardModeAltarSurfaceReleasePorts`和`release_legacy_standard_mode_altar_surfaces`。E3D0删除`update_database_phase_3`整块边界，phase3直接执行后再检查原countdown阈值并sample；E800两个完成路径删除opaque `complete_phase`事件，直接执行并返回第四释放EAX。四份state-owned像素数组代表已释放storage的悬空内容，严格不清零。

UT覆盖非零token的`1,0,2,3`释放顺序、phase/fourth-reset写入、四token与ring清零、像素残留、E3D0阈值后续副作用及E800完成帧直接集成。

定向测试通过。workpack双生成稳定为`70/227`，SHA256均为`7f31cf74c9bdf76ee10c791d34d4097a845162e41fb58664da7921228e74f575`；下一单元`0x00440630`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
