# 战斗运行时固定对象销毁 `0x0045EA30`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 范围与调用图

权威LST完整主体为`0x0045EA30..0x0045EA70`，从`proc`到`endp`共33行、21条实际指令、3个call、2个跳转、2个局部标签，没有外部`FUNCTION CHUNK`。

唯一caller为已关闭总销毁`0x004251B0`。三个call分别是：

1. 已关闭战斗渲染资源清理`0x00433D70`；
2. 已关闭group-A双资源清理`0x00475180`；
3. 已关闭组B资源释放`0x00476A60`。

渲染资源清理、group-A双资源清理和group-B资源释放现均已直接组合typed实现。组B只在深层CRT释放callee保留窄端口；`0x00476A60`整函数opaque边界已回收。

## 2. 固定group-A十对象

渲染清理返回后，ESI固定从`0x005029D0`开始。每轮以当前ESI作为this调用已关闭双资源清理，再执行u32加`0x2F34`，与固定尾地址`0x005201D8`按signed `<`比较。因此循环精确执行十次，对象token依次为：

```text
0x005029D0 + index * 0x2F34, index = 0..9
```

每槽typed owner保存行动者`+0` primary token和`+0x2BC4` secondary token。cleanup固定先处理secondary、再处理primary；每个非零token通过待审`0x004885A0`窄释放端口，callee成功后才清字段。token为零不触发释放，但十次cleanup调用不会因动态角色数、空token或callee返回值省略。

每槽EAX/EDX从前一槽线程传递，ECX在调用前由当前对象token覆盖；最后一个group-A cleanup结果继续作为首个group-B析构的入口寄存器。旧整函数opaque槽已删除。

## 3. 固定组B八对象与尾返回

组A结束后ESI无条件改为`0x00525508`。每轮以当前ESI作为this调用组B析构，再执行u32加`0x2B28`，与固定尾地址`0x0053AE48`按signed `<`比较。

循环精确执行八次，对象token依次为：

```text
0x00525508 + index * 0x2B28, index = 0..7
```

每槽typed直连组B资源释放：先从对象`+0x0C`读取token到EAX；零token跳过CRT端口并留下EAX零，非零token只在端口正常返回后清token及modern内联资源。EDX从前一槽线程传递，ECX在每次调用前由当前对象token覆盖。现代组Bowner缺失只在首个真实字段访问处typed-stop，不伪造后续七槽完成。

第八次组B释放返回后，函数只恢复ESI并`retn`；pop不改EAX/ECX/EDX，因此完整尾返回来自最后一个组B释放。typed结果显式保留三项寄存器，不把EAX规范化为bool或调用计数。

## 4. 渲染资源唯一owner

首call直接复用`LegacyBattleStartupState::render_geometry`唯一存储，严格执行：

1. 非零辅助buffer token调用释放端口后清零；
2. 非空surface行偏移释放；
3. 非空primary行偏移释放。

即使三项渲染资源都为空，固定10次组A和8次组B析构仍全部执行。对象地址继续使用`compat::u32` token，不转换为宿主指针。

## 5. 总销毁caller回收

已关闭总销毁原本把本函数保留为通用地址枚举操作。该枚举值现只保留reserved数值槽；总销毁在字体与前一资源释放之后直接调用typed战斗运行时销毁，再继续下一固定资源阶段。

SDL关闭端持有唯一战斗启动/渲染状态并调用同一typed入口。group-A与group-B均只在`0x004885A0`allocator释放保留窄平台端口；当前宿主空后端返回零寄存器，但固定十次group-A cleanup、八次group-B字段读取、零token跳过、非零token成功后清owner、渲染清理和总销毁顺序不被省略。

## 6. 验证与动态差分

定向测试覆盖非空/空渲染资源、辅助token释放、三项资源清零、十槽group-A双token顺序与清零、完整10/8对象token序列、八槽group-B非零释放、八槽全零跳过、owner缺失首槽停止、固定步长/尾地址、第八个group-B完整寄存器返回，以及总销毁中typed调用的精确位置。应用生命周期、窗口销毁和平台集成测试共同锁定caller顺序。

当前缺少原版10个group-A对象、8个group-B对象、动态164-byte资源、`0x004885A0`allocator释放和完整寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
