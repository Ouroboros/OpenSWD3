# OpenSWD3 当前执行状态

状态版本：1

最后更新：2026-09-01

本文件是可覆盖更新的即时快照，不是历史日志。每次恢复GOAL先读本文件；工作包、门禁或阻塞变化时直接改写对应条目，不在末尾累加旧轮次。

## 1. 唯一当前任务

- 阶段：B · 按模块逆向、实现与验证。
- 模块：10 · 战斗状态机、AI与数值系统。
- 当前工作包：`audit_order=270`，`0x00477920`固定键曲线显式count/percent设置。
- 状态：实现、caller回收、UT、证据和inventory已进入中间提交；最终门禁与发布收尾尚未完成。
- 下一候选：`audit_order=271`，`0x004779F0`；在第270项最终收尾前不得开始。

## 2. 仓库快照

- 分支：`main`。
- 第270项中间提交：`99471b16`，标题`feat: 设置战斗固定键曲线并回收调用方`。
- 该提交尚未推送。
- 本次计划重构已验证，随当前未推送提交链等待统一发布。
- `goal/HANDOFF.md`必须保持不存在。

## 3. 第270项已完成并有现存证据的部分

- 完整LST主体：`0x00477920..0x004779EF`；97个物理行、67条指令、3个call、5个jump、5个局部标签、2个返回点，无外部chunk。
- 两个物理caller：`0x0040FC2B`、`0x0040FEA7`，均位于已关闭Dialog `0x0040F890`。
- typed实现、共享fixed-curve owner、allocator、原访问点typed-stop和两个caller回收已落地。
- Linux core：`194/194`通过，日志`../build/workpack270/core-final.log`。
- battle/special_modes聚合定向：`2/2`通过，日志`../build/workpack270/directed-final.log`。
- changed-range格式检查通过，日志`../build/workpack270/format-final.log`。
- inventory连续双生成逐字节一致：`270/422 = 260 platform_adapted + 10 assembly_exact + 152 pending_audit`。
- inventory SHA256：`88a400c9d95a1a8d9a92776068221bccb87eec36b8ace592b09950a3ce4b918f`。

技术细节只读以下当前资料：

- [`../analysis/04-reverse-engineering/evidence/battle-fixed-curve-set-00477920.md`](../analysis/04-reverse-engineering/evidence/battle-fixed-curve-set-00477920.md)
- [`../analysis/04-reverse-engineering/evidence/party-dialog-main-0040f890.md`](../analysis/04-reverse-engineering/evidence/party-dialog-main-0040f890.md)
- [`../analysis/04-reverse-engineering/inventory/battle-function-workpack.tsv`](../analysis/04-reverse-engineering/inventory/battle-function-workpack.tsv)

## 4. 第270项剩余动作

计划文档集合已完成重构与完整性验证；该文档任务没有改变第270项技术结论。后续按顺序完成：

1. 重新运行最终AddressSanitizer门禁；此前进程因用户要求暂停而停止，不能使用其部分输出作为通过证据。
2. battle与special_modes聚合定向连续10轮。
3. Linux app完整门禁。
4. 更新`analysis/04-reverse-engineering/modules/battle.md`和本状态快照中的最终验证结果。
5. 执行release审计：源码warning、测试失败、sanitizer/runtime error、格式、TMP、链接、游戏进程、`HANDOFF.md`和暂存范围。
6. 完整审阅本轮剩余差异，提交收尾文档，推送`main`。
7. 按严格五段模板发送TG；实际门禁计数从最终日志读取，不使用历史硬编码值。
8. 完成第270项任务状态后，才把当前步骤推进到`audit_order=271`。

## 5. 当前阻塞

- 原版固定曲线链、allocator堆、x87 control/status、Dialog记录/局部槽和两个callsite联合寄存器捕获后端缺失；动态差分为`blocked_runtime_oracle`。
- 模块10更早工作包仍有原版framebuffer、audio、particle、text、shared-jitter、Miles/Bink及部分handler动态差分阻塞。
- 这些阻塞不妨碍第270项完整LST静态闭合、原位置typed-stop与Linux门禁，但不得宣称`original_diff_verified`。

## 6. 执行限制提示

- 未经用户明确许可，不启动原版或OpenSWD3游戏EXE。
- 构建只使用`./build.sh core`、`./build.sh app`和`./build-asan.sh`。
- 长任务使用受管process；项目命令使用仓库内`TMPDIR/TMP/TEMP`。
- 提交、发布、TG、TMP和compile_commands规则以[`execution-rules-pi.md`](execution-rules-pi.md)为准。
