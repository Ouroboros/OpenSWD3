# 战斗角色当前坐标查询 `0x00478600`

状态：`platform_adapted`、`unit_tested`、`caller_reclaimed:21/21`。Workpack 288的六个caller函数与二十一个物理callsite已全部回收；inventory由权威生成器关闭。

## 1. 完整LST边界与ABI

权威函数为`0x00478600..0x0047861E`，共31字节、7条实际指令、无callee、无分支或跳转、无外部FUNCTION CHUNK，末尾为`retn 8`。

指令顺序固定为：

```text
00478600  mov edx,[esp+4]
00478604  mov ax,[ecx+0D66h]
0047860B  mov [edx],ax
0047860E  mov ax,[ecx+0D68h]
00478615  mov ecx,[esp+8]
00478619  mov [ecx],ax
0047861C  retn 8
```

入口`ECX`是actor token，两个栈参数依次是X输出指针和Y输出指针。函数只读actor current X/Y word并只写两个输出word，不读取selector，也不访问alternate坐标；因此它与mode-gated `0x004783B0`是两个独立typed API。

## 2. 有序访问、alias与部分提交

`query_legacy_battle_actor_current_coordinates(...)`严格实现六个可故障访问点：第一输出指针读取、actor X读取、X写入、actor Y读取、第二输出指针读取、Y写入。每个typed-stop只保留已经到达的读写、寄存器和入口flags，不执行后续访问。

X成功写入后，Y源读取、第二指针读取或Y写入停止均保留X部分提交。实现不预先snapshot两个actor字段，也不合并两个输出：`out_x == out_y`时Y最终覆盖X；输出指针指向actor坐标字段时，后续Y读取必须观察此前X写入造成的真实alias副作用。输出dword仅替换低word，高word保持caller局部原值。

## 3. 寄存器与flags

第一条指令先把X输出token装入EDX。X源读取只替换EAX低word；Y源读取再次只替换EAX低word。第二输出指针读取把Y输出token装入ECX。正常返回固定为：

- `AX=current_y`，EAX高word保留入口值；
- `EDX=out_x token`；
- `ECX=out_y token`。

所有指令均为MOV或RET，不修改算术flags；完成和任一typed-stop都保留caller到达`call`前的flags。六类停止分别保留当时尚未被后续MOV覆盖的EAX/ECX/EDX，不以正常返回值回填。

## 4. canonical owner与reserved边界

Group-A caller优先解析`LegacyBattleStartupState::party[index]`；没有startup的独立turn helper才回退到`LegacyBattleActionDispatchState::group_a_action_execution[index]`。Group-B caller统一解析`LegacyBattleStartupState::group_b_lifecycle[index].action_execution`。坐标继续由既有startup/action/lifecycle owner唯一保存，没有新增平行actor数组。

旧端口枚举地址和ordinal只以reserved名称保留。脚本SDL adapter、turn gate和opponent action17 adapter均不再把reserved槽转发到`0x00478600`，生产路径对该raw地址零调用；caller直接组合本typed leaf。

## 5. 二十一个caller回收

六个caller函数中的二十一个物理站点全部回收：

- 效果角色位移`0x0045BD90`四处：`0x0045BE39`、`0x0045BE99`、`0x0045BF5A`、`0x0045BFBA`；
- 调试叠加`0x0045DEE0`一处：`0x0045E270`；
- 目标选择入口`0x004620D0`一处：`0x00462234`；
- 脚本分派`0x00469D20`十三处：`0x0046A694`、`0x0046A7C6`、`0x0046BA42`、`0x0046BAB5`、`0x0046BB44`、`0x0046BB9D`、`0x0046C610`、`0x0046C8AA`、`0x0046C929`、`0x0046C97D`、`0x0046CA77`、`0x0046CACB`、`0x0046CD72`；
- 回合角色推进门`0x00471540`一处：`0x004716FD`；
- Group-B行动十七`0x004763D0`一处：`0x00476544`。

每个caller保留自身输出局部位形、EAX高word、ECX/EDX残值、入口flags、成功后的调整/publication与typed-stop后的第一条未执行后缀。效果和脚本循环还保留动态count重读、此前完成actor与当前X部分提交；turn gate与action17停止则阻断调整、publication、frame source、blit、倒计时及父级收尾。

## 6. 测试与动态oracle缺口

leaf测试覆盖正常返回、六个访问停止、输出同址、输出覆盖actor字段、X部分提交、EAX高word、ECX/EDX和flags。caller测试覆盖二十一个地址的canonical owner、两套脚本scratch、循环回边与live count、turn gate无调整及正负十六、action17正负二十五、栈局部完整位形、六类停止矩阵、reserved ordinal/空adapter槽、生产零调用和所有后缀抑制。Group-A父级另以不同的startup party与action-execution坐标验证查询来源，并在startup Y读取停止时验证X局部前缀和父级后缀抑制。Group-B opponent父级从唯一action dispatch状态显式透传action17两个未初始化dword栈局部residue；成功用例验证两个非零高word参与完整32-bit `-0x19`及publication flags，Y读取停止用例验证X低word前缀、两个高word和全部父级后缀抑制。

最终门禁通过changed-range格式、战斗定向`1/1`、Linux core `199/199`、ASan/UBSan `199/199`、Linux app `205/205`、连续十轮core、inventory双生成、TMP分类与完整unstaged/staged发布审计，fresh reviewer最终返回PASS；四份正式stderr为空，日志无OpenSWD3源码warning、测试失败、sanitizer finding或runtime error。inventory为`288/422 = 278 platform_adapted + 10 assembly_exact + 134 pending_audit`，SHA-256为`23c9d965fd4b9c964bc2803605411e7d9a7b50dbfbc94dc2993d603d7bc65ed7`。

原版动态差分仍为`blocked_runtime_oracle`：缺少完整Group-A/Group-B actor、异常栈与输出内存页、寄存器/flags/SEH及二十一处caller联合捕获后端。该阻塞不削弱完整LST、typed访问顺序、caller回收或固定状态测试结论。
