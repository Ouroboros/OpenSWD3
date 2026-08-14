# 角色粒子发射器四链关闭生命周期闭环

状态：`platform_adapted`、`assembly_exact`（原版有效链域）、`unit_tested`；原程序动态
差分仍为 `blocked_runtime_oracle`。

唯一行为依据是 `swd3.exe.lst` 的 `0x0040F630..0x0040F66B`。该函数按物理槽顺序逐头
销毁四条角色粒子链，随后把完整四槽状态写零。

## 1. ABI、调用点与物理布局

物理 ABI 无参数。三个直接调用点为 `sub_40C130:0x0040C5B7`、
`sub_411D00:0x00411D1A` 和 `sub_4251B0:0x0042521F`。三处调用后均继续执行相邻关闭
逻辑，没有读取 EAX；末尾 `xor eax,eax` 和 `rep stosd` 留下的零不是调用者消费合同。

外循环从 `0x004CACE0` 开始，每轮把槽地址增加 `0x10`，在到达
`0x004CAD20` 前继续，因此物理状态恰好是四个连续的 `0x10` 字节发射器槽。每槽
`+0x00` 是链首；动态节点同样是 `0x10` 字节，`+0x00` 是 next。既有
`LegacyAniRoleParticleEmitter`、`LegacyAniRoleParticleNode` 及静态断言固定了这两个
物理布局。

## 2. 精确控制流

对槽 0、1、2、3 依次执行：

1. 读取当前槽 `+0x00` 链首；空首直接进入下一槽；
2. 读取当前节点 `+0x00` 的 next；
3. 先把 next 写回当前槽链首；
4. 调用 `sub_4885A0(current)` 释放整个 `0x10` 节点；
5. 重读当前槽链首，非空则继续。

四槽处理结束后，函数以 `EDI = 0x004CACE0`、`ECX = 0x10`、`EAX = 0` 执行
`rep stosd`，无条件清零完整 `0x40` 字节，而不只是四个链首。

## 3. 所有权适配与调用接线

- `LegacyAniRoleParticleEffect` 是四槽和节点池的实际现代 owner。`release()` 按槽号递增，
  每轮先写回 `head_token`，再释放对应节点；全部链处理后才清零四个 emitter。
- 原版每个节点是独立裸分配。现代 one-based token 池避免在 64 位主机把指针截断到
  32 位；链处理完成后用空 vector 交换节点池，确保真正归还池存储，不再由旧
  `vector::clear()` 保留容量。
- SDL owner 重建和总关闭的 `release_0040f630` 已绑定同一实际 effect。
  `sub_40C130/sub_411D00` 的完整外围调用次序仍由它们各自尚未关闭的 B7 行复核，不在
  本 helper 内伪造。
- 无效 token、循环和共享节点在原版属于崩溃/破坏域。现代实现计入
  `corrupt_link_count/orphaned_node_count` 后清空专用 owner，作为明确的平台内存隔离；
  有效链域的循环、顺序和字段写入不因此改变。
- 普通角色绘制端口当前尚未接入 `sub_415EE0` effect update；该外层集成缺口归属
  `sub_413910` 的独立 B7 审计，不以本关闭函数的完成状态掩盖。

## 4. 双向收敛与测试

- LST→C++ 已覆盖四槽严格顺序、每槽空/非空、节点 `+0x00` next、先推进槽首、后释放
  节点、逐头循环、槽步长 `0x10`、末地址和最终 16 dword 清零；
- C++→LST 已反查 effect owner、token pool、world owner 重建和总关闭接线，没有清理
  其他 ANI effect 或把池容量保留误当作节点释放；
- 独立 UT 覆盖四槽释放计数 `2/1/0/1`、四节点最终归还、完整 emitter 字段归零和重复
  空 reset；
- 三个直接调用点均已反查，没有参数、EAX 消费、条件方向、字段宽度、槽数、循环边界
  或清零范围差异。
- Linux `core` 184/184、Linux `app` 189/189、Windows LLVM `app` 189/189 CTest
  全部通过；两端应用均成功链接，未启动任何 EXE。

核心实现为 `legacy_ani_role_particle_effect.cpp::LegacyAniRoleParticleEffect::release`；
SDL 接线位于 `main.cpp::SmokeShutdownPorts` 和 `SdlSmokeIdlePorts`。
