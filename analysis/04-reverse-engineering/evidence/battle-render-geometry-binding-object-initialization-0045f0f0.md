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

原版唯一caller为`004518F0`，由静态表中的`004518E0`尾跳进入。
现代调用方直接传入实际共享绑定对象和需写入内存镜像的32位几何地址，
不传递绑定对象地址作为调用协议。初始化返回void，删除寄存器结果、
写入计数以及两层纯转发包装。

SDL在几何静态初始化之后直接初始化`battle_runtime_.render_binding_object`；
入战资源读取继续借用这一对象。初始化只写几何地址和30条记录。
`legacy_battle_definition_archive.cpp`仍按旧偏移读取对象字节，因此保留
精确内存布局和32位地址数据，不将它替换成宽度不同的宿主指针。
DAT读取本身的寄存器协议属于独立未完成范围。

## 5. 验证与动态差分

定向测试检查实际对象的`0x31F4`尺寸、30条ordinal、全部五步四分桶、
头部及保留区逐字节保持和完整32位几何地址写入；三个关键offset继续
由编译期断言约束。删除寄存器回显、计数与转发包装断言。
setup入口同时执行DAT头部、记录读取和入战调用方测试。

core日志为`build/tmp/runtime/render-binding-direct-core-resumed.log`；
ASan及SDL日志为`build/tmp/runtime/render-binding-direct-{asan,sdl}.log`。
core和ASan的setup定向测试各1/1通过，SDL编译链接通过，未启动游戏。
ASan保留既有outcome-resolution测试第133行整数窄化警告。
源码双向复核确认实际写入、未写区域、固定循环边界及SDL调用顺序；
删除的返回字段没有业务消费者，`git diff --check`通过。

当前缺少原版完整绑定对象内存、后续资源文件读入、几何owner共享状态和寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
