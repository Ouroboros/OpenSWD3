# 剧情 VM 下一对话 flag bit18 抑制：0x0042CBB0

状态：`assembly_exact`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原程序动态差分仍为`blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042CBB0..0x0042CBCB`

opcode：`160`，四个raw alias为`00A0/40A0/80A0/C0A0`。

## 1. 入口与物理记录

handler只有六条指令：

```text
0x0042CBB0  mov esi,1
0x0042CBB5  add ebx,2
0x0042CBB8  mov dword_4CF73C,esi
0x0042CBBE  add word ptr [ebp+0],2
0x0042CBC3  mov saved_script_pointer,ebx
0x0042CBC7  jmp loc_42B0AE
```

因此记录固定为2字节，只包含opcode。handler不读取operand，不调用helper，不访问平台owner，也没有条件分支或失败出口。

## 2. 状态owner与完整生命周期

完整LST对`dword_4CF73C`只有三处代码xref：

- opcode160在`0x0042CBB8`把完整dword写为1；
- 共享对话caller在`0x00427D50`与常数1比较；值不等于1时才在`0x00427D58`置record flag `0x00040000`；
- 对话成功排队后在`0x00427E67`把完整dword清零。

因此opcode160建立一次性“下一条成功对话不置flag bit18”状态。若后继对话在排队前失败，共享对话handler不会执行清零，值1必须保留给后续重试或下一条成功对话。

`sub_40E0B0`没有该全局的xref，重复世界初始化不拥有或重置它。现代状态使用`LegacyWorldStoryVmState::next_dialog_flag18_suppression`完整`u32`字段；不能缩成bool，也不能参与对话坐标计算。

## 3. IP、previous与同次调用

入口把物理脚本指针`EBX`和context内u16 IP都增加2，并保存新物理指针。`ESI=1`进入`loc_42B0AE`后：

- 当前normalized opcode发布到previous owner；
- `ESI`令common join继续条件非零；
- 解释器直接回到fetch循环，在同次调用读取后继指令；
- 不进入`_AIL_serve`尾部，因此无audio service、无yield。

当opcode位于`IP=0x7FFE`时，写1、推进到`0x8000`和发布previous160均先完成；随后同次调用的下一fetch才返回越界。

## 4. 资产事实

线性TALK目录锁定558条物理记录和558个entry probe：

```text
TALK1.DAT  244
TALK2.DAT  117
TALK3.DAT  113
TALK4.DAT   84
```

558/558均为raw `0x00A0`、长度2。按物理后继追踪，全部记录都会在最多两条中间指令后进入共享对话handler：

```text
160 -> 105 -> 6   427
160 -> 6          130
160 -> 91 -> 89     1
```

真实回放选择`TALK1.DAT@0x00005975`和`TALK1.DAT@0x0000762E`，分别覆盖前两种链；第三种唯一链由线性目录固定，并由opcode91与89各自既有真实回放交叉验证。

## 5. 测试边界

synthetic固定：

- 四种raw alias都覆盖任意旧dword并写成1；
- 固定推进2、发布previous160、零audio并同次调用后继；
- `IP=0x7FFE`先完成全部副作用，再由下一fetch失败；
- `160 -> dialog2`同次调用时，dialog record不置bit18，成功排队后状态清零；
- 后继dialog缺`%Q`时，保留值1、IP2和previous160。

真实TALK回放固定直接对话链和先清text bit27再对话链，均由opcode160抑制bit18一次并在成功排队后清零。

## 6. 分类与验证

handler只写VM已有完整`u32`owner并执行解释器内固定控制流，没有平台替代或不安全内存边界，分类为`assembly_exact`。

Story VM synthetic、real、initial-session-real三项定向CTest 3/3通过。Linux core 186/186与app 192/192完整门均以exit 0通过。workpack双生成稳定为`126/146 = 20 assembly_exact + 106 platform_adapted + 20 pending_audit`，hash为`cf60317b840a18c4c554d27f63587c42102d3cab33149f0f3989834ad7d2e007`。未经许可未启动原版或OpenSWD3游戏EXE。
