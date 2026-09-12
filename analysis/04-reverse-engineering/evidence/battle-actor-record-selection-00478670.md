# 战斗角色资料记录选择 `0x00478670`

状态：`platform_adapted`、`unit_tested`、`caller_reclaimed:2/2 implemented startup sites`。完整四处物理xref均已登记；另外两处caller属于后续待审工作包，当前生产源码不存在对应opaque调用。

## 1. 完整LST边界与ABI

权威函数为`0x00478670..0x0047868A`，共27字节、12条实际指令、0个call、3个条件分支与2个`retn 4`，没有外部`FUNCTION CHUNK`或中段入口。

入口为thiscall：`ECX`是Group-A actor token，`[ESP]`是真实caller返回地址，`[ESP+4]`是一个32位选择参数。正常返回读取真实返回地址并执行`retn 4`，因此ESP增加8字节。ECX与EDX全程保持；EAX返回所选记录token或零。

## 2. 互斥字段读取与两个出口

函数严格按机器顺序执行：

1. `0x00478670`读取完整32位栈参数到EAX，`0x00478674`执行完整32位`TEST EAX,EAX`；
2. 参数为零时，`0x00478678`只读取`actor+0x04`的live来源记录token，再于`0x0047867B`执行第二次TEST；非零token从`0x0047867F`的首个`retn 4`返回；
3. 参数非零时，`0x00478682`只读取`actor+0x00`的角色基础记录token，再于`0x00478684`执行第二次TEST；非零token从`0x0047868A`的末尾`retn 4`返回；
4. 任一所选token为零时，`0x00478688`执行`xor eax,eax`，再从末尾RET返回。

两项actor字段严格互斥读取。即使两个字段token相等，也不能根据token内容反推或改选另一字段；typed结果分别记录实际选择了`+0x00`还是`+0x04`。

## 3. flags、栈与typed-stop

MOV、条件跳转和RET不修改flags。两次TEST均令CF/OF为零，ZF/SF/PF来自完整32位EAX，AF未定义；零token路径最后由XOR产生同类逻辑flags。参数读取前停止保留入口flags及入口EAX，字段读取前停止保留首次TEST结果，RET读取停止保留第二次TEST或XOR结果。

独立typed-stop覆盖：

- `0x00478670`参数栈读取；
- `0x00478678`仅零参数路径的`actor+0x04`读取；
- `0x00478682`仅非零参数路径的`actor+0x00`读取；
- `0x0047867F`来源token非零路径的返回地址读取；
- `0x0047868A`角色token非零或任一零token路径的返回地址读取。

每个停止点保留当时EAX/ECX/EDX、ESP、EIP、flags、已完成参数/字段/返回地址读取计数、分支选择和XOR执行状态。成功RET记录参数值与caller返回地址两个栈读取，并把ESP增加8；RET读取失败不清理参数。

## 4. canonical owner与typed接口

`LegacyBattleGroupAConfigurationState::actor_record_token`和`source_record_token`分别是`actor+0x00/+0x04`的唯一owner。`select_legacy_battle_actor_record()`直接读取这两个现有字段，不复制记录、不建立平行token槽、不增加null替代或提前双字段读取。

## 5. startup两处caller回收

`0x00451B10`中的两个已实现物理caller已直接组合typed接口：

- 随机补位分支`0x00452511`，真实返回地址`0x00452516`；
- 顺序补位分支`0x00452646`，真实返回地址`0x0045264B`。

两处caller都在调用前执行`mov edi,1`和`push edi`，因此固定选择参数为1，ECX固定指向首个Group-A actor，必须读取首个actor的`+0x00`基础记录token。返回后先压入该token，再压入当前新角色token并进入既有护援materialization；不得按token内容改选`+0x04`。selector typed-stop保留已完成的候选role、坐标、mirror与基础token发布，但阻断当前护援materialization、角色激活、剩余候选、角色进度和startup消息后缀。

旧`LegacyBattleStartupCall::supplemental_seed`只保留原ordinal并改名为`reserved_supplemental_seed`，生产调用数为零。

## 6. 后续caller合同

完整xref中的另外两处尚未进入modern生产路径：

- `0x00481109`压入参数0，必须选择同一actor的`+0x04` live来源记录，返回地址为`0x0048110E`；
- `0x00481ADD`压入参数1，必须选择同一actor的`+0x00`角色基础记录，返回地址为`0x00481AE2`。

两处所属caller工作包实现时必须直接组合本typed接口；本工作包不伪造尚不存在的生产caller。

## 7. 测试与动态差分

leaf测试覆盖参数零/非零、两项token零/非零、互斥字段读取、相同token别名、五类读取故障、两个RET、TEST/XOR flags、ESP及EAX/ECX/EDX。startup测试覆盖随机与顺序两个物理站点、固定参数1、首个Group-A actor owner、两处真实返回地址、reserved旧槽零调用，以及末尾RET typed-stop对当前护援和剩余startup后缀的抑制。

最终注入式战斗定向`1/1`、Linux core `199/199`、AddressSanitizer/UBSan `199/199`、Linux app `205/205`及连续十轮完整core `10/10`均通过；四份正式有效stderr和十轮core stderr全部为空。新文件全量clang-format及旧文件changed-range apply/`--dry-run --Werror`通过，源码warning、测试失败、sanitizer finding和runtime error扫描无命中。inventory由权威生成器连续双跑逐字节一致，SHA-256为`6e832b90dd4b17830f2b63f7c1ca2ce7b98b7e534ad12f1b92e895f761bc6386`；TMP审计为`confirmed_entries=0`、`errors=[]`。未启动原版或OpenSWD3游戏程序。

原版动态差分登记为`blocked_runtime_oracle`：缺少完整Group-A actor首双token、参数/字段/返回地址异常内存页、四处caller EAX/ECX/EDX/ESP/flags与SEH联合捕获后端。该缺口不影响完整LST、静态访问顺序、typed部分提交或固定状态测试结论。
