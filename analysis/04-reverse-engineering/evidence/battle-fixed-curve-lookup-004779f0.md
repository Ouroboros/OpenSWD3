# 战斗固定键曲线值查询 `0x004779F0`

状态：`platform_adapted`、`unit_tested`、`closed_callers_reclaimed`。

## 1. 完整权威范围与调用图

唯一行为真值为`swd3.exe.lst`。完整主体为`0x004779F0..0x00477A13`，从proc到endp共28个物理行、13条实际指令、0个call、3个跳转、3个局部/返回标签和2个返回点，没有外部`FUNCTION CHUNK`。

五个物理callsite为`0x004107DA`、`0x00442CCA`、`0x0046F0D3`、`0x0046F148`和`0x00482F53`。前四个分别位于已关闭的队伍对话页`0x00410730`、护驾属性摘要`0x00442CA0`和组A内嵌资料应用`0x0046F030`，均已在原位置回收为typed直连。最后一个位于`audit_order=403`的待审共享服务`0x00482F10`，本轮保持原边界，不提前修改。

函数采用两个栈参数：固定根token和查询键word。入口完整加载根token到EAX，只以`mov cx`替换ECX低word为查询键并保留入口高word；EDX在函数全部路径均不修改。

## 2. 根参与扫描与返回合同

固定根不是纯哨兵。函数先比较`word [root+4]`；不匹配时严格循环：

1. 从当前记录`+0x00`完整读取next到EAX。
2. EAX为零时只对AX执行异或清零并返回。
3. next非零时比较`word [EAX+4]`，不匹配则继续读取该记录next。

根或动态节点命中后只读取`word [EAX+8]`到AX。成功EAX高word来自命中记录token，低word为值；ECX高word保留caller入口残值，低word为查询键；EDX保持入口。缺失路径的终端next读取已把完整EAX置零，因此`xor ax,ax`后返回EAX零，ECX和EDX保持上述合同。

函数不分配、不写入、不排序，也不增加现代链长限制、环检测或替代缺失值。

## 3. 唯一owner与typed-stop

固定根`0x004B9F00`、`0x004ACBA8`、`0x004B8A00`及动态20字节记录继续由唯一`LegacyBattleFixedObjectStatePort`持有，与相邻固定数量和曲线函数共享同一物理状态。实现不复制链，也不建立battle到special_modes的反向依赖。

原访问点typed-stop为：

- 根或动态记录`+0x04`键读取不可访问：EAX已发布当前token，ECX低word已替换为查询键，EDX未修改。
- 当前记录`+0x00`next读取不可访问：保留到达该记录时的EAX/ECX/EDX。
- 命中记录`+0x08`值读取不可访问：键比较已经完成，EAX仍为命中token。
- next非零但token未映射：停止在目标记录`+0x04`原读取位置。

停止点不伪造正常零结果，也不读取未到达的后缀。

## 4. 已关闭caller回收

- 队伍对话页page 0第一分类以item id查询根`0x004B9F00`，使用返回AX作为附加值并把分母设为0；typed-stop阻断当前行替换，保留清表及此前行前缀。第二分类pair查询和第三分类固定数量查询保持各自后续边界。
- 护驾属性摘要slot 0在seed不是`0xFFDC`时查询根`0x004ACBA8`，把返回AX零扩展写cache `+0x44`；429B0 caller在原调用前把ECX发布为`attribute_cache_token+0x140`，442B10/442BC0留下EDX scratch token `0x004FCD4C`，两者均传入typed查询。typed-stop保留首个`0xFFFFFFFF` sentinel并阻断`+0x48/+0x4C`后缀。
- 组A内嵌资料类型52与51均查询根`0x004B8A00`。类型52从返回AX取无符号右移一位的比例；类型51使用返回AX低word参与原两阶段截断算术。typed-stop保留资料读取前缀并阻断角色记录访问和写入。

三个已关闭caller的旧opaque查询声明、override和生产调用均已删除；为保持已有枚举数值稳定，仅保留`reserved_*`槽。`0x00482F10`仍为待审caller，不在本轮回收。

## 5. 验证与限制

叶函数UT覆盖根命中、动态节点命中、缺失、查询键高位截断、命中token高word保留、ECX低word替换、EDX不变，以及根键、命中值和未映射next的typed-stop前缀。

caller回归使用真实固定根/动态链状态，覆盖队伍对话第一分类、护驾slot 0、组A类型51/52、共享owner和caller级typed-stop；旧opaque名称全仓生产扫描仅剩reserved枚举槽。最终定向测试`3/3`、Linux core`194/194`、AddressSanitizer`194/194`、Linux app`200/200`、连续10轮完整core、changed-range clang-format、inventory稳定性和release审计全部通过，最终日志无OpenSWD3源码warning、测试失败、sanitizer finding或runtime error。

当前缺少原版固定曲线链、三个已关闭caller状态及五个callsite EAX/ECX/EDX联合捕获后端，动态差分登记为`blocked_runtime_oracle`；这不阻止完整LST静态闭合、原访问点typed-stop和Linux门禁。
