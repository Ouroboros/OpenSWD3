# 剧情 VM 主帧执行门清零 `0x0042D041`

状态：`platform_adapted`、`unit_tested`、`asset_absence_verified`、`external_dependency_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042D041..0x0042D057`，common join `0x0042B0AE..0x0042B0C8`

opcode：180 / `OP_180_CLEAR_FRAME_EXECUTION_GATE`

## 1. 完整u32 owner

handler无operand，物理长度固定2。机器先把物理script pointer加2，再把完整`dword_4CC2AC`覆盖为0，随后把u16逻辑IP加2。

`dword_4CC2AC`是进程期主帧执行门，不是VM私有bool：

- WinMain初始化和debug modal关闭路径写1；
- F8在video inactive时把任何非零值变成0、把0变成1，video active时强制写1；
- 主消息泵和战后世界路径只按完整dword的零/非零门控帧执行；
- opcode180无条件覆盖为精确0。

现代唯一owner是`app::WindowEventState::frame_execution_gate`完整u32。SDL剧情runtime直接借用该字段；不建立VM镜像，不把写入降为bool。任意高位非零旧值都被完整清零，旧值为0时重复执行幂等。

## 2. 控制流与顺序

handler直接跳到common join，没有设置`ESI=1`。因此全局写完成后依次：

```text
logical IP += 2
previous = 180
service audio once
return yielded
```

该路径不读取后继，也不在同一VM调用继续。完整记录位于`IP=0x7FFE`时，gate、IP=`0x8000`、previous180和audio全部完成后直接yield，不触发下一fetch。

modern缺actual owner binding时在原`dword_4CC2AC`写点返回`runtime_unavailable`：保留IP、previous和gate，不service audio。该nullable binding只隔离现代接线失败，正常SDL路径始终绑定actual process owner，因此分类为`platform_adapted`。

## 3. 资产锁与验证

完整线性TALK目录没有opcode180记录，使用`asset_absence_verified`。全文件双字节候选为：

```text
00B4 = 41
40B4 = 0
80B4 = 0
C0B4 = 30
```

这些候选均位于operand、文本或其他非线性入口字节中，不能冒充真实记录。

synthetic覆盖四raw alias、完整u32高位清零、零值幂等、actual owner、gate/IP/previous/audio顺序、精确窗口尾不fetch及缺owner typed-stop。F8 window-event、frame-dispatch、runtime integration消费者依赖与Story VM synthetic/real/initial-session共6/6通过，SDL app编译通过。Linux完整门core 186/186、app 192/192通过。未启动原版或OpenSWD3游戏EXE。
