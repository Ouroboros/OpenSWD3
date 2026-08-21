# 剧情 VM 战斗请求 `0x0042A727`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；战斗模块消费端仍属后续模块范围。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042A727..0x0042A755`；释放helper `0x0040F500..0x0040F539`、`0x0040F570..0x0040F597`

opcode：`88`

## 1. 固定副作用顺序

handler不是先读取4-byte记录，而是严格：

1. 调`sub_40F500`释放`dword_4BAB9C` packed-row效果链；
2. 调`sub_40F570`释放`dword_4BA6E0`头像动作链；
3. 从`ip+2`读取signed i16 battle ID；
4. `request = sign_extend_i16(id) | 0x80000000`；
5. script pointer加4；
6. 写`dword_4A94AC = request`；
7. 16位IP加4，common join发布normalized previous88；
8. `ESI=0`，请求提交后yield，不执行下一条剧情指令。

modern此前在任何释放前whole-record检查，并把三个typed owner合并预检；同时漏发previous88。本轮按原访问点拆开并修正。

## 2. 两个释放helper

`sub_40F500`逐节点执行：先把全局head写为node `+0x14` next，再依次释放node `+0x0C`行offset、`+0x10`行length和node本体，然后重读全局head。

`sub_40F570`逐0xB4头像节点执行：先把全局head写为node `+0xB0` next，再释放当前node并重读head。

modern分别复用`release_legacy_packed_row_effects`和`release_legacy_role_head_actions`。typed owner缺失按原访问阶段停止：

- packed-row owner缺失时，头像链和operand均不访问；
- 头像owner缺失时，packed-row链已经清空；
- operand截断时，两条链都已经清空；
- battle request owner缺失时，两条链已清且operand已经读取，但IP/previous仍未发布。

`sub_40F540`移动动作链不在handler调用序列内；modern `LegacyMovingActionList`必须保持不变。

## 3. 位宽与请求值

battle ID不是zero-extended：`MOVSX word`先扩展为i32，再OR高位。代表值：

```text
0000 -> 80000000
7FFF -> 80007FFF
8000 -> FFFF8000
FFFF -> FFFFFFFF
```

原版不验证ID范围；0仍提交仅含高请求位的值。现代保持u32位型，后续战斗状态机按原主帧合同消费。

## 4. 资产锁与测试

线性TALK目录含52条物理记录/52 probes：

```text
TALK1.DAT 20
TALK2.DAT 7
TALK3.DAT 8
TALK4.DAT 17
```

全部raw `0x0058`、长度4。当前资产battle ID均为正值，范围16..290；这不缩小handler的完整signed i16语义。

real CTest独立回放：

- `TALK1.DAT@0x00005547`：battle ID98，request `0x80000062`；
- `TALK4.DAT@0x0001F694`：battle ID290，request `0x80000122`。

两条均置于精确窗口尾，验证packed-row/头像链清空、移动链保留、request、IP=`0x8000`、previous88与yield。

synthetic覆盖四raw alias、四个signed边界、两个释放owner的分阶段失败、释放后operand截断、battle owner缺失和移动链保留。剧情VM三项为3/3。

分类：`platform_adapted`。合法owner域的释放顺序、两条且仅两条链、signed request、IP、previous和yield保持；固定裸全局/手工释放及越界operand由typed list owner和确定性失败替代。战斗消费端未在VM层伪造成功。
