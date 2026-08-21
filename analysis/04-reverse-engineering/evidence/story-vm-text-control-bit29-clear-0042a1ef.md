# 剧情 VM 文本控制 bit29 清除 handler：0x0042A1EF

状态：`assembly_exact`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原程序动态差分仍为`blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042A1EF..0x00427E95`

opcode：`80`

## 1. 完整指令合同

handler本体与共享尾为：

```text
0042A1EF  mov edx, dword_4A1360
0042A1F5  and edx, 0DFFFFFFFh
0042A1FB  jmp loc_427E7E
00427E7E  mov dword_4A1360, edx
00427E84  add word ptr [ebp+0], 2
00427E89  add ebx, 2
00427E8C  mov [esp+var_50], ebx
00427E90  mov esi, 1
00427E95  jmp loc_42B0AE
```

因此有效行为只有：

1. 读取32位`dword_4A1360`；
2. `&= 0xDFFFFFFF`，只清bit29；
3. 在共享写回点更新同一全局；
4. IP精确推进2；
5. common join发布normalized previous80并same-call重新fetch。

无operand、helper、分配、callback或独立yield。现代owner是`LegacyWorldStoryVmState::text_control_flags`，直接u32按位写与汇编等价。

## 2. 位宽与顺序

`mov edx`、`and edx`、`mov dword`固定整个32位owner；不能仅改某个byte或把其余位重置。测试以`0xA0000000`证明bit31保留而bit29清除为`0x80000000`。

写回发生在IP推进前；随后`ESI=1`进入`0x0042B0AE`。现代case按相同顺序先清位，再+2，再发布previous80，最后`continue`；不能把它改成yield，也不能遗漏previous publication。

## 3. raw alias、窗口和真实资产

一级fetch先执行`raw_word & 0x3FFF`，因此`0050/4050/8050/C050`都归一为opcode80。四alias均在`0x7FFE`精确尾执行：清位、IP=`0x8000`、previous80完成后，下一fetch才返回`instruction_out_of_range`。ordinary测试随后同调用执行opcode14，证明same-call continuation。

线性TALK目录含opcode80物理记录2256条/2256 entry probes：

```text
TALK1.DAT 609
TALK2.DAT 453
TALK3.DAT 507
TALK4.DAT 687
```

全部raw为`0x0050`且`decoded_length=2`，无异常长度或高位alias。real CTest从`TALK1.DAT@0x00004520`读取原始两字节记录，置于`0x7FFE`回放，验证真实记录的clear/advance/previous/exact-tail合同。

## 4. 验证与分类

synthetic、real、initial-session-real三项定向CTest为3/3。Linux core 186/186与Linux app 192/192在提交前执行；未启动原版或OpenSWD3游戏EXE。

分类：`assembly_exact`。合法域内32位owner、掩码、写回、IP、common join publication与same-call均直接对应原指令，不含平台适配。
