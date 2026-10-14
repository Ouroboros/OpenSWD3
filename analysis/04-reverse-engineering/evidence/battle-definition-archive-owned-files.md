# 入战时读取FFD的实际文件所有权

## 本批边界

可观察结果：从实际battle.ffd读取绑定对象头部及战斗记录，短读保留
共享数据后缀，普通失败继续使用已有记录，真实访问故障保持资源现场。
调用链覆盖SDL入口、核心startup、两个加载函数及全部直接测试。
停止条件是协议删除、全调用方编译、行为测试与必要构建通过后发布。

删除通用文件FilePort及派生Runtime，打开/读取/寻址/关闭请求回复、
编号句柄、文件名/目标/局部栈地址、寄存器快照/回显、调用计数及
重复发布结果。函数使用真实路径、战斗ID、variant、共享对象和记录。
结果只报告业务状态、已读字节及记录选择数据。

## 行为依据与双向复核

[头部LST](battle-definition-archive-header-load-0045f130.md)覆盖
`0045F130..0045F1A2`；[记录LST](battle-definition-archive-record-load-0045f1b0.md)
覆盖`0045F1B0..0045F29D`。启动顺序见
[生产绑定](battle-definition-runtime-binding.md)。

头部按原只读、独占、OPEN_EXISTING参数打开，固定读取0x2714字节，
不根据读取结果新增停止分支；先发布同一对象的头部索引，再关闭。
原公布地址改为相对对象的0x1F48字节偏移，失败不改变原值。

记录重读同一头部，再按ID低WORD、signed BYTE count和variant、
signed前缀累计及DWORD回绕选择偏移；忽略寻址和读取失败，固定读取
0x10C字节，关闭后报告逻辑完成。拒绝路径关闭并保留记录；真实
对象访问故障仍停止后缀、不关闭、不写记录。无新增停点或夹值。

反向逐项复核文件参数、对象写入范围、发布时机、符号门、累计、
偏移计算、寻址失败后的读取、关闭与启动解码。每项业务行为均对应
上述LST；新集合只改变资源持有方式及消除编号，不改变顺序。

## 所有权与寿命

`LegacyBattleDefinitionArchiveFiles`持有实际LegacyFile唯一指针。
成功open后先登记对象，再借用指针；read和seek直接使用同一文件。
close真实成功后才删除所有权，失败保持登记。原有文件对象、集合
分配异常继续传播，不转换成伪成功或新typed-stop。

调用中发生对象访问故障时文件留在集合，集合销毁再执行既有析构。
SDL集合仍位于原加载作用域，核心startup仍借用调用方集合。没有
把所有路径改成局部RAII立即关闭，也没有扩大资源存活时间。
独立静态文件单例接线仍是另一未验收接口族。

## 测试证据

真实临时文件覆盖完整/空/短头部，0/1/3/0x10C记录读取、0xF2记录
解码、正负count、相等/越界/负variant、负前缀、ID截断、偏移回绕、
负寻址失败与超EOF。Linux目录触发实际读失败；实际描述符检查
访问故障后的保留及集合销毁关闭；主动使测试描述符失效，检查
系统close失败仍保留所有权。双文件检查独立游标及相互关闭隔离。

启动测试使用实际混合大小写文件和记录，检查再次进入时保留共享
数据、普通失败解码陈旧记录、短记录建场及故障阻断后缀。原资产
battle.ffd头部及首记录逐字节比较，battle98建场继续检查敌方400、
坐标175/303。仅用于协议回显的模拟夹具与断言已删除。

首轮编译发现误改的独立背景路径断言，恢复后编译通过。随后定位到
挂载文件系统大小写不敏感导致的路径字符串断言，改为检查实际
文件身份，不改变生产文件查找逻辑。失败日志保留，不计为通过。

最终受管进程`definition-owned-final-validation`退出0。core setup
1/1通过（9.93秒），ASan同目标1/1通过（17.33秒），SDL应用链接
通过。core和SDL无warning/error；ASan仅既有outcome-resolution
测试133行的WORD→BYTE窄化警告。旧失败运行不计为通过。

实际执行命令：

```text
./build.sh core --build-target openswd3_battle_legacy_battle_setup_tests --test --test-regex '^battle\.legacy_battle_setup$'
./build-asan.sh --build-target openswd3_battle_legacy_battle_setup_tests --test --test-regex '^battle\.legacy_battle_setup$'
./build.sh app --build-target openswd3
```

日志为：

- `build/tmp/runtime/definition-owned-core-final.log`
- `build/tmp/runtime/definition-owned-asan.log`
- `build/tmp/runtime/definition-owned-sdl.log`

全调用方扫描未发现已删除的archive请求/结果、文件接口、句柄字段、
返回寄存器及局部地址参数。完整源码、测试和证据差异已审查，
`git diff --check`通过。startup其他通用协议与调用计数仍待迁移。

Windows专属只读属性分支未在Linux运行。既有Unicode文件后端、
Linux共享模式差异及API失败读量规则保持。原版动态差分仍为
`blocked_runtime_oracle`，未启动游戏，不升级B11、WP316或整体验收。
