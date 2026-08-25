# 后退特殊模式模式1的打包子模式或记录选择 `0x0044DFF0`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0044DFF0..0x0044E0B9`，97行，无外部FUNCTION CHUNK。caller为`0x0044DBC0`和`0x0044F920`。函数与DF00共享状态及typed端口，但按原指令保留方向和帧标志差异。

函数先按`level`分支：

- level1：把打包模式低两位减1并在0饱和；只清写低两位，完整保留其他30位，返回新打包值。
- level2：执行记录选择后退。
- 其他level：无副作用，返回`level-2`的32位结果。

## 2. level2顺序

1. 本地cursor按32位回绕减1并立即发布。
2. 仅当结果符号位为1时，把cursor置0。若window按i32大于0，再把window减1，并从完整工作区head按signed新window推进可见head。
3. 从当前可见head调用F7D0，最多重计13条并发布新可见数。
4. 以`window+cursor`的32位结果调用B9C0语义索引完整工作区，解析该记录文本到128字节共享buffer。
5. 播放样本`0x00BF`，保留完整sample owner。
6. 对帧标志低字节OR `0x03`。这与DF00的`0x30`不同，不能共用方向无关常量。
7. 对相同索引调用D6B0：目标只取低16，运行计数保持32位。由其返回记录调用FAF0并发布四名角色差值。

B9C0与D6B0索引域不同，不能合并为一次查找。

## 3. typed-stop

可见head正向推进越过null、B9C0目标缺失、共享文本失败、D6B0循环/缺失及FAF0停止均只在原读取/调用点停止。窗口减值和已推进到null的可见head必须保留。D6B0停止仍发生在文字解析、声音和帧标志之后。

## 4. 验证

UT覆盖level1高30位保留、低两位减1和零饱和；level2 cursor下溢后window回退、可见head与可见数重建、FFDC文本“無”、样本BF、帧标志bit0/1和四次角色查询；window为0时只钳cursor；短链在已发布新window和null可见head后停止。

workpack双生成稳定为`206/227`，SHA256为`cb684d216267cc7870783222bbed787335416c0b12ec54fd80b786c68ad2d313`。`0x0044DBC0`继续等待callee闭环；下一单元为`0x0044E0C0`。
