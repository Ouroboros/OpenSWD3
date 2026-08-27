# 战斗攻击顺序记录登记 `0x0045EDF0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`callers_reclaimed`。

## 1. 范围与ABI

权威LST完整主体为`0x0045EDF0..0x0045EE69`，从`proc`到`endp`共71行、37条实际指令、0个call、6个跳转、5个局部标签，没有外部`FUNCTION CHUNK`。

函数为两个参数cdecl叶函数：第一参数是完整u32类型，第二参数是完整u32登记值。记录物理区间固定为`0x00524788..0x00524980`，共18条、每条步长`0x1C`。typed实现直接复用战斗启动与全局重置已经持有的唯一18条`LegacyBattleStartupResetRecord`，不建立平行攻击顺序数组。

## 2. 类型分派

入口EAX先从第一参数重载，再连续执行两次低32位递减：

- 第一次递减后为0，即入口类型1，进入类型1扫描；
- 第二次递减后为0，即入口类型2，进入类型2扫描；
- 其他位形立即返回，EAX为`type-2`低32位结果，ECX/EDX保持入口值，记录区完全不读。

类型判断不按低word或非零归一；只有完整值1和2有效。

## 3. 固定18槽扫描

两条有效类型路径都先把ECX清零并把EAX写为物理首地址。每轮先读取当前记录`+0x00`完整dword；只有精确`0xFFFFFFFF`才为空槽。非空时严格执行：

1. EAX增加`0x1C`；
2. ECX增加1；
3. 以EAX和固定尾地址作signed `<`比较。

因此恰好扫描18条，不读取第19条，也不因typed容器长度增加而扩大范围。18条全满时直接返回EAX=`0x00524980`、ECX=18、EDX保持入口值，不写记录，也不返回显式成功码。

owner短于实际扫描需求时，只在下一条`+0x00`真实读取处typed-stop；已扫描记录和EAX/ECX前缀保持。正常集成始终绑定完整18条共享记录。

## 4. 空槽写入与返回寄存器

空槽索引为`index`时，两条路径都按`index*0x1C`的低32位offset返回EAX，并只改当前记录两项：先写`+0x00`完整值，再写`+0x08`类型word。其余`0x1C`布局字节全部保留。

类型2在shift形成最终offset前把第二参数读入ECX，因此正常尾为：

```text
EAX = index * 0x1C
ECX = value
EDX = entry EDX
record.value_00 = value
record.value_08 = 2
```

类型1先把第二参数读入EDX，再计算offset，因此正常尾为：

```text
EAX = index * 0x1C
ECX = index
EDX = value
record.value_00 = value
record.value_08 = 1
```

不清记录其他字段，不把物理地址转换为宿主指针，也不把满表静默返回改成错误。

## 5. caller回收与验证

三个静态caller均已关闭并直接组合typed登记：

- 画面转场低概率敌方事件在角色mode不等于1时以类型2登记敌方索引，旧事件opaque槽保留reserved枚举值且不调用；本函数EAX offset继续成为caller live EAX；
- 角色动作主分派case25在目标status发布后以类型2登记已选组B索引；子typed-stop保留status写并阻断current actor清理与成功尾；
- 组B逐帧更新在update gate精确1且message不等于103时以类型2登记当前索引；子typed-stop保留更新前缀并阻断余下角色帧。

动作分派与组B帧通过同一action context绑定战斗启动记录；画面转场直接使用同一个startup reset owner。后续已关闭待执行动作提交继续经待关闭记录移除callee消费该物理记录区。

定向测试覆盖无效类型0/3的双递减EAX、类型1/2寄存器差异、步长与首空扫描、只写值和类型word、其他字段保留、18槽满表、短owner真实读取停点，以及三个caller直连、旧opaque调用清零和两类caller typed-stop前缀。

当前缺少原版18条攻击顺序记录的动态轨迹、三个caller输入寄存器、后续记录消费链及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
