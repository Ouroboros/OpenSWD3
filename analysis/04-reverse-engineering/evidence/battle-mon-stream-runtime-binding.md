# MON解析流的实际分配与释放

状态：具名接口迁移的定向测试、ASan及SDL链接验证通过。
当前批次的验证范围见[MON接口迁移验证](battle-mon-file-runtime-binding.md#具名接口迁移验证)。

## 当前接口

profile与definition加载器直接使用分配端口返回的1024字节存储。
`allocate_mon_stream(size)`返回分配标识和可写span，
`release_mon_stream(block_token)`释放对应分配。
SDL直接连接`LegacyBattleMonStreamRuntime::allocate`和`release`，
不再经过操作枚举或寄存器请求／回复包。

## LST对应与实际调用

`00476B73`和`00476EE7`申请400h字节，随后分别在`00476B87`和
`00476EFB`执行100h次STOSD。ReadFile使用同一分配地址，不另建一份
局部数组。实现按DWORD顺序清零；范围不足时在首次不可写DWORD之前
停止，保留此前写入并记录首次不可访问偏移，不继续读取或提前释放。
剩余ECX不作为业务接口的输出；可写范围及清零前缀仍须保持。

正常解析及正常的首tag失败路径仍在原位置释放。解析停止时，端口保留
分配；之后再次加载不得覆盖或释放前一次尚存的流。重复释放明确失败。

`LegacyBattleMonStreamRuntime`持有unique数组，并通过既有guest地址
预留器登记实际分配。分配失败返回零，交给加载器在原首个访问处停止。
SDL的具名分配／释放方法使用该持有者，不使用固定流地址或无操作释放。
不同加载器继续借用同一个MON文件会话。

这是堆与访问边界的平台适配，不声称复现原CRT的宿主地址或易失寄存器。

## 已发布解析存储行为的历史验证

仅暂存本批源码和测试，更新既有隔离快照进行验证；仍使用发布前的vector
说明接口，排除未提交的说明存储迁移及角色重置。

发布时的测试覆盖如下；历史通过结果不替代当前接口迁移的验证：

- 两种加载器的0、2、6、1023字节可写范围；
- 精确DWORD清零前缀、剩余ECX及读取/释放后缀抑制；
- 实际文件读取直接写入分配存储；
- 解析停止后存储保留、再次分配身份与数据独立；
- 正常失败返回释放当前分配、重复释放拒绝。

`proc_2d0a`隔离验证通过：setup与definition的core各1/1，ASan各1/1，
SDL构建通过。日志为`build/tmp/runtime/mon-stream-isolated-{core,asan,sdl}.log`
及`build/tmp/runtime/mon-stream-definition-isolated-{core,asan}.log`。
初次暂存准备因脚本匹配不唯一而停止，修正匹配后重跑；未改业务实现。
最终暂存测试已排除未使用的说明fixture并整理一处换行，setup的core/ASan
增量复验`proc_7646`的core/ASan各1/1通过，日志为
`build/tmp/runtime/mon-stream-final-{core,asan}.log`。
SDL与definition源码未再改变。全部11个暂存源码/测试文件与验证快照
逐字节一致，差异检查通过；隔离构建含既有警告，不宣称零warning。
完整敌方初始化及实际续玩仍未验收。
