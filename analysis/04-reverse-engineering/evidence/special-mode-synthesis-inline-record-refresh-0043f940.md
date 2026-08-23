# 炼妖合成inline record刷新 `0x0043F940`

状态：`platform_adapted`、`unit_tested`

此处“数据库”实际是炼妖合成候选记录系统，不是通用数据库。唯一行为真值为`swd3.exe.lst`。范围`0x0043F940..0x0043FA68`，143行；唯一caller为F880。callee为已关闭B9A0/F7C0，以及未独立关闭的44D680、487C10、4885A0。

## 行为

1. 以F880传入的32位环绕`window_offset+list_selection`从active forward链定位节点；负索引保持head。越过null时在原B9A0 `node->next`读取点typed-stop。
2. 在覆盖前以F7C0判断旧inline record是否属于当前page，并保存旧`+4` ID。
3. selected节点`+6`引用数非零时，无条件释放旧inline `+AC` token，复制0xB0 record；复制后把inline token先清0，有效ID才克隆selected token，再固定`+8/+A=0`、`+6=1`，最后把selected引用数减1。
4. selected引用数为0时仍无条件释放旧token，但只重置token、ID=`FFDC`、`+8/+A=0`和`+6=1`，不清其余字节。
5. 旧ID非`FFDC`时调用44D680边界：旧record属于当前page则回active链，否则回inactive池；返回节点引用数按u16加1。null返回在原`[eax+6]`处typed-stop。
6. caller只忽略返回指针；typed结果仍保留selected/recycled节点返回、helper数和两类typed-stop。

Forward node补充原0xB0 payload、enabled、value及type owner；inline `+4`现在直接读取真实bytes，两份inline record默认同时把bytes与typed alias初始化为`FFDC`。487C10/lstrcpy和4885A0通过token clone/release最小边界表达；44D680保持回池边界，留待其owner工作包。

F880现把绝对索引直接传入本helper，已删除临时`record_source_combined_index` state及collapsed `refresh_database_records`接口。UT覆盖完整复制、token克隆、current/inactive池选择、两侧引用计数、零引用保留字节、返回指针及null-link typed-stop；独立ASan执行通过。

定向测试通过。workpack双生成稳定为`65/227`，SHA256均为`8226e719fca0962efc2b5131d6bcb7975a2767e74832a80aa00d752fb7c05cc4`；下一单元`0x0043FA70`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
