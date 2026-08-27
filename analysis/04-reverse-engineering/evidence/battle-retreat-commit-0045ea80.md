# 战斗撤退提交 `0x0045EA80`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 范围、ABI与调用图

权威LST完整主体为`0x0045EA80..0x0045EB3C`，从`proc`到`endp`共82行、50条实际指令、4个call、3个跳转、1个局部标签，没有外部`FUNCTION CHUNK`。

ABI为cdecl，唯一参数是组A索引。唯一caller是已关闭玩家动作分派`0x004539B0`的动作3完成路径。四个callee为选中组A对象就绪查询、首个组A对象状态查询、固定文字显示和样本播放；前三项仍属后续工作包，保留语义窄端口。样本请求固定编号`0x008C`并读取live混音等级，继续沿现有sample command边界提交。

## 2. 无边界对象定位与精确1门

入口按原移位/减法/LEA序列计算：

```text
object = 0x005029D0 + index * 0x2F34  (low 32 bits)
```

不检查索引是否小于10，不把旧地址转换为宿主指针。第一callee以该token作为this；返回EAX不精确等于1时立即返回完整EAX/ECX/EDX，不查询首对象、不显示提示也不写共享状态。返回2等非零值不能视为成功。

## 3. 首对象查询与“無法撤退!!”分支

第一门成功后，无条件以固定组A首对象token调用第二callee。callee后才读取live battle mode；测试`AH & 2`等价于完整dword bit`0x200`。

以下任一条件进入同一失败分支：

- 首对象查询EAX为0；
- battle mode bit`0x200`置位。

失败分支严格先显示固定CP950字节串`B5 4C AA 6B BA 4D B0 68 21 21`，即“無法撤退!!”，参数为`(280,10,50,text,0x40000002)`；随后读取live样本混音等级，以编号`0x008C`播放。函数完整尾返回来自样本callee，文字返回不参与控制流。失败分支不修改任何成功状态。

## 4. 成功写序与宽度

首对象查询非零且mode bit清零时，先把live组B数量低byte读入CL，再把EAX清0。随后严格按原指令顺序：

1. 两个撤退完成门分别写1；
2. 共享结果latch写0；
3. 撤退辅助latch写0；
4. 调试重置门写0；
5. 调试叠加显示门写0；
6. 选择对象token写`0xFFFFFFFF`；
7. 共享message写0；
8. 只把动作packed counter低byte替换为组B数量低byte，保留高24位；
9. 暗化门写1。

函数返回EAX 0。ECX保留首对象查询返回的高24位并只替换CL为组B数量低byte；EDX保留该查询完整返回。组B数量只在成功路径、第二callee和mode判断之后读取。

## 5. 单一typed owner与全局重置

battle mode与调试重置门继续由唯一调试快捷键state port持有。调试叠加显示门从逐帧state副本移到独立虚共享gate port，动作分派、逐帧绘制和全局重置使用同一存储。

两完成门、辅助latch和选择token由唯一撤退提交state port持有；结果latch/暗化门、message、组B数量和packed counter分别复用既有结果、共享phase、actor metric和动作状态。全局重置清两完成门与选择token，保持未在写集合中的辅助latch；调试叠加门按原write program清零。

## 6. caller回收

玩家动作分派动作2/3公共路径在动作3收束时，仍先清选择状态、执行选择计算、发布frame fade并调用固定300帧延迟callee；随后直接组合typed撤退提交。旧`0x0045EA80`地址调用从源码清零。

原caller忽略撤退提交返回并立即返回自身结果；typed caller同样只累计内部端口调用，不把撤退失败改成动作分派typed-stop，也不补写成功状态。

## 7. 验证与动态差分

定向测试覆盖u32索引回绕、首callee返回0/1/2、首对象查询零、mode bit阻断、固定CP950文字与五参数、live signed混音等级、样本尾寄存器、成功九项写序/宽度、CL陈旧高位、callee后mode动态重读、全局重置别名，以及动作3 caller直连和旧地址调用清零。

当前缺少原版两类角色对象查询、文字渲染、样本管理器、九项共享全局轨迹及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
