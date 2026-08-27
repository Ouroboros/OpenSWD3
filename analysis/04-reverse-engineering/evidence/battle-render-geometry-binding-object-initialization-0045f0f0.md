# 战斗渲染绑定对象初始化 `0x0045F0F0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整范围与ABI

权威LST完整主体为`0x0045F0F0..0x0045F128`，从proc到endp共34行、27条实际指令、0个call、1个跳转、1个局部标签，没有外部`FUNCTION CHUNK`。

函数是单参数thiscall：ECX为绑定对象token，唯一栈参数是渲染几何owner token，并以`retn 4`回收参数。EBX、ESI、EDI按callee-saved规则恢复；正常返回EAX和ECX均为入口绑定对象token，EDX为最后一轮`cdq`后与3的结果0。

## 2. 精确物理对象布局

已锁定绑定对象token为`0x004FF5B8`，几何owner token为`0x0053B0B8`。typed对象保持精确`0x31F4`字节布局：

- `+0x0000`：渲染几何owner token；
- `+0x0004..+0x2717`：后续战斗资源读取使用的`0x2714`字节头部；
- `+0x2718..+0x3103`：本函数不触碰的保留区；
- `+0x3104..+0x31F3`：30条、每条8字节的索引记录。

本函数只写`+0x0000`和30条索引记录。头部与保留区保持入口字节，不因现代值初始化被错误清零。物理地址始终只作`compat::u32` token，不转换为主机指针。

## 3. 30条固定记录

EBX从0、EDI从0开始，ESI指向`this+0x3108`。每轮严格执行：

1. 把当前EBX写到`ESI-4`，即记录首dword ordinal；
2. EAX取当前EDI，`cdq`后EDX与3；
3. EDI加5；
4. EAX加EDX并算术右移2位；
5. ESI加8，再把EAX写到新ESI的`-8`，即同一记录第二dword；
6. EBX加1，EDI按i32 signed小于150时继续。

正常入口轨迹共30轮，ordinal为`0..29`，第二dword为`floor(index*5/4)`，即从0递增到36。实现不建立运行时可变上限，不改写对象其他字节。最后一轮进入时EDI为145，`cdq/and`形成EDX 0；加5后signed比较恰好退出。

## 4. caller回收

唯一caller是已关闭`0x004518F0`固定参数包装器。它先压入几何owner token，再把绑定对象token写入ECX，调用本函数并直接返回。旧`LegacyBattleRenderGeometryBindingObjectInitializationPort`已删除；包装器和相邻静态thunk现在接收唯一typed绑定对象owner并直接组合本初始化，不再允许端口伪造返回值。

包装器返回值因此固定为绑定对象token，且内部初始化次数仍为1。相邻静态thunk只转发同一typed owner，不产生第二份对象状态。

## 5. 验证与动态差分

定向测试覆盖精确`0x31F4`尺寸、三个关键offset、30条ordinal、全部五步四分桶、头部与保留区逐字节保持、任意测试token写入、EAX/ECX/EDX返回、固定token包装器和静态thunk直连。

当前缺少原版完整绑定对象内存、后续资源文件读入、几何owner共享状态和寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
