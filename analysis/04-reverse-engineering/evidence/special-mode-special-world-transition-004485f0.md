# 特殊模式世界转场准备 `0x004485F0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x004485F0..0x0044863E`，35行、3个call，无FUNCTION CHUNK；末尾以jmp尾调用406D30。唯一code caller为`0x00446700`。

函数先用尚未关闭的44D520分别深克隆inventory根和当前选择记录链，发布两个clone token；随后直接调用已关闭482E0清理原选择链。清理typed-stop发生时两份clone已发布，原head当前节点已弹出，且不发布转场状态、不调用共享调度。

清理完成后依次发布mode=5、enabled=1、zero=0、layout=3，并把共享调度返回值原样返回。platform adaptation把两个全局clone根归入`LegacyStandardModeSpecialWorldTransitionRuntime`，44D520仍保持最窄clone port，待其owner独立关闭后回收。

446700的2D9分支移除了错误的“删除2D9动作”抽象，直接调用本helper。原caller在返回后还会再次写同四个全局并调用406D30，因此typed实现明确保留两次相同发布和两次调度；第二次返回值为最终EAX。

UT覆盖clone顺序和token、原选择链清空、四owner值、两次完全相同发布、两次调度、最终返回值，以及记录释放停止时clone保留、head已弹出且零发布零调度。独立ASan通过。

workpack双生成稳定为`141/227`，SHA256均为`14f2d879cd729cf1d68f0781f0d57a7206fd19a6bff784dc471043d8897317db`；下一单元`0x00448650`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
