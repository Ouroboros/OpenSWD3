# 标准模式数据库forward释放 `0x0043F080`

状态：`platform_adapted`、`unit_tested`

## LST与实现

唯一行为真值为`swd3.exe.lst`。范围`0x0043F080..0x0043F0CA`，49行；caller为D880与F000。

函数逐节点先把`forward_head`写为next：

- `text_index != 0xFFDC`：把节点next写为当前adjustment head，再把adjustment head写为该节点；不调用free。
- `text_index == 0xFFDC`：严格先释放节点`+0xAC` token，再释放节点本体。
- 循环直至forward head为null。

新增typed release helper及recycled/value/node计数。4885A0保持最小release port，未提前关闭。

## caller回接

F000现直接调用F080后再执行F0D0。D880也直接调用F080；原`release_external_forward_list`整块port删除。D880后续再次读取forward head的循环按原LST保留，但F080正常返回时head必为null，因此是原始死路径；recycled adjustment池不被D880擅自释放。

UT覆盖非FFDC节点回收到既有adjustment head、FFDC token/node双释放顺序、F000旧链先清理，以及D880后续optional/runtime/storage顺序。

## 验证

定向测试通过。workpack双生成稳定为`59/227`，SHA256均为`102301e70a3dbe32d400c0dac85fd0360cf8e0d323732186a16a726b1fe00875`；下一单元`0x0043F0D0`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
