# 剧情 VM 镜头顶部等待 `0x0042D170`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042D170..0x0042D1A9`，共同join `0x0042B0AE..0x0042B0C8`，audio return `0x0042D4D7..0x0042D4F3`

opcode：191

## 1. record与控制流

物理长度固定4：

```text
+2  i16 expected viewport top
```

handler按以下顺序执行：

1. 读取camera pan剩余X dword；非零直接进入active路；
2. 否则读取剩余Y dword；
3. 两项都为0时不读`+2`，不访问viewport owner，固定`IP += 4`、发布previous191并same-call，无audio；
4. active路才读取`+2 i16`并sign-extend；
5. 读取完整viewport top dword并与sign-extended operand比较；
6. 相等时固定`IP += 4`、发布previous191并same-call，无audio；
7. 不等时不推进，发布previous191，经common join service audio一次并yield。

camera step X/Y不参与门条件。handler不修改pan或viewport状态。

## 2. common join与尾边界

函数入口`0x00427955 xor esi,esi`，首轮fetch在`0x00427B32`保存carry0。mismatch不设置ESI，故`var_28 | ESI == 0`，从`0x0042B0C2`进入`0x0042D4D7 _AIL_serve`一次后返回。相等或无移动路径通过`0x0042D18E mov esi,1`进入same-call fetch。

原版无移动分支完全不读名义operand，但仍推进4。因此只剩opcode两字节的`0x7FFE`无移动记录会先把IP推进到`0x8002`、发布previous，再由下一fetch返回越界。active路在同位置必须读取operand并失败。完整四字节精确尾的active equality先提交IP `0x8000`与previous，再由下一fetch失败；mismatch则留在原IP并audio-yield。

现代直接复用actual `LegacyWorldCameraPanState`和`LegacyWorldCameraRect` owner。缺pan owner在首个remaining X读取点typed-stop；active且operand可读后，缺viewport owner在原top读取点typed-stop。无移动不要求viewport owner。

## 3. 真实资产

`story-vm-talk-linear-records.tsv`锁定13条物理记录/13 probes，全部位于`TALK4.DAT`、raw `0x00BF`、长度4：

```text
0x0002FC5F..0x0002FD07，步长0x0E
operand = 800, 1120, 1440, ..., 4640
```

13条全部执行物理字节检查、active equality same-call与active mismatch audio-yield双向回放。完整TALK文件字样候选为`00BF=111`，`40BF/80BF/C0BF=0`；其余98个base字样不是线性入口，未冒充opcode记录。

## 4. 测试与收敛

synthetic覆盖四raw alias、remaining X/Y分别active、signed i16极值、full-dword viewport top比较、equality/mismatch、step-only忽略、无移动未读operand与未访问viewport、pan/viewport owner访问顺序、active operand截断、inactive两字节尾、active四字节equality/mismatch精确尾、previous、same-call与audio-yield。

LST→C++：两个remaining短路、条件operand、完整top比较、三路IP/previous/continuation与common audio均逐块映射。

C++→LST：没有检查step、无移动时读取operand、提前要求viewport、修改camera状态、equality audio或mismatch推进。typed owner检查只隔离原版裸全局失效域。

Story VM synthetic/real/initial-session 3/3、SDL app编译、Linux core 186/186与app 192/192完整门全部通过。workpack双生成稳定hash为`c2b315e9c30150665c280eaea16653a05853358ef93f0b573326640cde6ba914`。未启动原版或OpenSWD3游戏EXE。
