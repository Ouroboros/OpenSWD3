# 剧情 VM 帧时钟等待 `0x00429B62`

## 结论

`sub_427920` 的 opcode67 是一条固定4字节、自修改的两阶段等待指令。`+2`低15位是duration，bit15是已初始化阶段标记：

- bit15清：保存duration与当前accepted-frame clock，在脚本word中置bit15，原地yield；
- bit15置：用`u32(current-start)`与保存的duration比较；`elapsed <= duration`原地yield，只有`elapsed > duration`才清bit15、推进4并同调用继续。

初始化、等待和完成三路都经过共同出口并发布normalized previous67。现有C++的自修改、u32回绕与严格`>`原本正确，但三路都漏发previous；完成路same-call后若下一handler再发布，则最终previous由下一handler覆盖。

唯一行为依据是`swd3.exe.lst`机器指令。本handler不依赖相邻时钟opcode34..37的完成状态。

## 指令布局

```text
+0  u16 opcode67
+2  u16 duration_phase
       bits0..14 duration
       bit15     phase marker
```

handler先零扩展读取完整word，再`test AH,0x80`。现代要求完整4字节可读；截断在任何状态、自修改与previous前typed-stop。

## 阶段一：初始化并原地yield

bit15清时顺序为：

1. `dword_4CF6B0 = zero_extend(+2)`；
2. `or byte ptr [instruction+3], 0x80`，只设置脚本operand bit15；
3. `dword_4CF6B4 = dword_4AAECC`；
4. 不推进IP，`ESI`保持0；
5. 共同出口发布previous67并yield。

现代字段：

- `LegacyWorldStoryVmState::wait_duration`对应`dword_4CF6B0`；
- `wait_started_at`对应`dword_4CF6B4`；
- 可写`state.window`承载脚本自修改。

## 阶段二：无符号等待与完成

bit15置时，原版读取当前clock、start和duration，执行32位：

```text
elapsed = current - start
if elapsed <= duration: wait
else: complete
```

`sub`与`jbe`共同证明减法回绕和比较都为unsigned。完成条件严格为`>`，不是`>=`。

等待路径：

- operand、duration、start均不改；
- IP不推进，`ESI=0`；
- 发布previous67并yield。

完成路径：

1. `operand &= 0x7FFF`，恢复原duration word；
2. 共享尾推进IP 4；
3. `ESI=1`；
4. 发布previous67；
5. 同调用继续取下一指令。

`wait_duration`与`wait_started_at`在完成后不清，保持最后一次初始化值。

## 时钟owner

`dword_4AAECC`是accepted-frame采样时钟。SDL Story VM runtime使用`frame_preparation_state_.frame_clock.sampled_milliseconds`；初始化与后续等待都在同一接受帧时钟域读取，未另建计时源。

现代`u32`减法天然保留从`0xFFFFFFFF`到0的回绕。

## previous与组合影响

三条共同出口都写previous67：

- phase初始化yield；
- phase等待yield；
- phase完成并same-call continue。

因此此前把67作为“不会覆盖previous”的测试哨兵是不正确的。修正后：

- opcode62/63/64同调用进入等待67时，最终previous为67；
- opcode16同调用跳转到真实67记录时，最终previous为67；
- 67完成后同调用进入opcode59时，最终previous再被59覆盖。

## 边界

- `0x7FFE`只容纳opcode word：operand截断，无副作用；
- phase初始化位于`0x7FFC`：可自修改并发布previous，IP保持`0x7FFC`后yield；
- phase等待位于`0x7FFC`：原地发布previous并yield；
- phase完成位于`0x7FFC`：清bit、IP推进到`0x8000`、发布previous，然后下一fetch返回`instruction_out_of_range`。

## 真实资产锁

对`story-vm-talk-linear-records.tsv`全部opcode67 entry逐条回读TALK文件：

- 1118条物理记录、1130个entry probes；
- TALK1/2/3/4分布`460/232/273/153`；
- 全部raw`0x0043`、长度4；
- 40种duration，源记录bit15全部为0；
- 最常见duration为300/200/600/400/500，分别209/119/98/96/93条；
- 原始offset、word与长度逐条核验零错误。

真实回放使用`TALK1.DAT@0x000044F7`，duration 2000。tick12345初始化；tick14345因elapsed==duration继续等待；tick14346完成并同调用进入后续opcode59。

## 测试覆盖

- 四种raw alias阶段初始化、duration/start、自修改与previous；
- elapsed==duration等待、duration+1完成；
- 完成后清bit、推进4并同调用执行下一音效；
- start`0xFFFFFFF0`跨u32回绕，在elapsed`0x20`等待、`0x21`完成；
- operand截断；
- phase初始化与phase完成的`0x7FFC`精确尾；
- TALK1 duration2000完整三阶段真实回放；
- opcode16→67、opcode62/63/64→67组合的最终previous更新；
- 剧情VM三项测试通过。

## 双向收敛与分类

实现逐项对应`test AH,80h`、两项全局写、自修改OR/AND、`sub`+`jbe`、共享+4尾和共同previous出口。C++→LST REVIEW未发现剩余差异。

分类：`assembly_exact`。运行时使用已确立的accepted-frame clock owner；无平台近似或业务降级。原程序动态差分仍为`blocked_runtime_oracle`，但静态机器语义、真实资产与现代运行时组合已闭环。
