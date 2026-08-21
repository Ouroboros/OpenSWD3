# 剧情 VM 选择滚动表清除 `0x00429AD2`

## 结论

`sub_427920` 的 opcode64 只把 `word_4ACE70` 起 64 个选择滚动 word 全部写成 `0xCFCF`。它不修改 opcode63 建立的滚动 interval、remaining、cursor 或视口 left/top 快照。清表后走共享 `+2` 尾，令 `ESI=1`，经共同出口发布 normalized previous64 并在同一次解释器调用继续取指。

唯一行为依据是 `swd3.exe.lst` 的机器码和指令。opcode63证据用于确认共享 owner，但不向本 handler 继承完成状态。

## 汇编顺序

`0x00429AD2..0x00429AE3`：

```text
ECX = 0x20
EAX = 0xCFCFCFCF
EDI = &word_4ACE70
rep stosd
jump 0x0042D1EA
```

`rep stosd` 固定执行 32 次，即写 128 字节/64 个 word。共享尾 `0x0042D1EA..0x0042D1FB` 随后：

1. 当前指令指针与 Talk context IP 各加 2；
2. `ESI=1`；
3. 跳到共同出口，发布 normalized previous；
4. 同调用继续取下一指令。

handler 没有 operand，也不读取 camera 或 `LegacyWorldSelectionScrollState`。

## 状态保留

opcode64只清 `world_selection_words_`。以下opcode63/选择滚动状态全部保持：

- `cursor_word_index`；
- `frames_remaining`；
- `frame_interval`；
- `saved_left`；
- `saved_top`；
- 当前 camera left/top/right/bottom。

后续帧看到首项 `0xCFCF` 时，选择滚动 consumer 直接判定 inactive；frame tail也因此不恢复旧视口，但保存字段本身仍保留原值。

## 平台 owner 与失败顺序

现代复用opcode63已接入的 `world_selection_words_` typed owner。owner缺失时在第一项写入前返回 `runtime_unavailable`，选择表、IP、previous与所有滚动状态均保持。有效owner路径使用固定64项`std::array`，与原版全局128字节块等长。

## IP、previous 与精确尾

opcode64成功后推进2并同调用继续。指令位于`0x7FFE`时，64项清表、IP=`0x8000`与previous64先完成，下一次取指才返回`instruction_out_of_range`。

owner缺失则不推进、不发布previous。

## 真实资产锁

对`story-vm-talk-linear-records.tsv`的全部opcode64 entry逐条回读原始TALK文件：

- 共8条，TALK1/2/3分布`3/1/4`，TALK4为0；
- 8/8全部raw`0x0040`、长度2、单entry probe；
- 原始偏移、word、长度与probe核验零错误。

真实回放使用`TALK1.DAT@0x00025E07`。执行后64项全为`0xCFCF`，滚动cursor、interval、remaining与视口快照保持，previous64发布，并同调用进入下一条等待。

## 测试覆盖

- 四种raw alias；
- 64个不同初值全部清为`0xCFCF`；
- cursor、interval、remaining、saved left/top全部保持；
- 选择表owner缺失在所有副作用前typed-stop；
- `0x7FFE`精确尾先完成清表与publication，再由下一fetch失败；
- TALK1真实记录回放；
- 剧情VM三项定向测试全部通过。

## 双向收敛与分类

实现逐指令对应`mov ECX`、`mov EAX`、固定64-word fill与共享`+2`尾；C++→LST REVIEW未发现额外状态写入、错误yield或边界差异。

分类：`platform_adapted`。适配仅为原版进程全局的typed owner与缺失owner失败隔离；有效域行为为固定128字节清表、状态保留和same-call continuation。
