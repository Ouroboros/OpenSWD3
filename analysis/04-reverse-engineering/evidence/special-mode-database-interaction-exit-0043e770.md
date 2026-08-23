# 标准模式数据库交互退出 `0x0043E770`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与直连

唯一行为真值为`swd3.exe.lst`。函数范围`0x0043E770..0x0043E7DF`，72行；DA30有两个direct call点，E3D0有一个direct call点，B480另以callback地址绑定。直接callee B480、D880和E3D0现均复用已关闭typed实现。

新增typed owner：4FB8A8 callback primary word、B480 callback state，以及生命周期减为0时清除的4B8740 owner。cleanup通过ExitPorts暴露D880 cleanup ports适配器，不复制D880释放逻辑。

## 2. phase 1

1. lifecycle phase按u16减1，保留回绕。
2. 结果为0时清lifecycle-zero owner。
3. 以减后的lifecycle为secondary、4FB8A8为primary直接调用B480。
4. 尾调D880，D880最终storage release EAX即E770返回EAX。

UT覆盖lifecycle2→1触发B480 G08 secondary dispatch、15类storage cleanup及最终EAX；另覆盖lifecycle1→0先清zero owner，再由D880把lifecycle写回1。

E3D0 phase1检测物品`0x1BB0`存在时现直接调用E770，不再经过generic target port。

## 3. 其他phase

- phase2：写interaction phase1及primary action `0x232A/0x3B`，保留switch index EAX1。
- phase3/4：尾调已关闭E3D0；E3D0 typed-stop映射为E770 `commit_stopped`，DA30再传播为`database_exit_stopped`。
- phase5：写interaction phase1并保留EAX4。
- 其他phase：不写owner，返回`phase-1`。

UT覆盖phase2 action、phase3委派/sample2E、phase5和default EAX；DA30所有E770路径均直接执行，generic target事件为空。

## 4. 验证

定向测试通过。workpack双生成稳定为`56/227`，SHA256均为`223801a61e31fb0f348ba9ad946424aa7ac1490b0d6ddcc7ac5fcdab209aa243`；下一单元为`0x0043E800`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
