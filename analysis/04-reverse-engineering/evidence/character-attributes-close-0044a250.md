# 关闭角色属性页面并返回上一层菜单 `0x0044A250`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x0044A250..0x0044A277`，26行、2个call，无FUNCTION CHUNK。直接caller为44A050，另由43B480安装为callback；callee为已关闭44A030与43B480。

函数先直接调用44A030，按first、second顺序释放两份owner且不清owner值；随后对interaction mode执行u16预减。结果精确为0时清active owner，否则保持。最后把新mode零扩展为u32传给43B480并透传其EAX。入口mode为0时按u16下溢为FFFF，不清active owner，并以65535分派。

44A050删除`commit_character_attributes` opaque端口，在原flags bit4位置直接调用本typed helper；caller继续只计一次44A250调用并截取最终返回低字节。回调前释放、阶段写回和active owner清理均已完成，不在回调失败时回滚。

UT覆盖释放顺序、mode 1→0、零时清active owner、callback参数0、返回透传与两次直接call；另覆盖mode 0→FFFF、active owner保持和callback参数65535。既有44A050测试改为真实typed提交，并隔离每个case的可变interaction snapshot。

workpack双生成稳定为`163/227`，SHA256均为`180a48130e57cca5100e7803dfd27b860a64ce2b6375308d57e39bb09820c2fe`；下一单元`0x0044A280`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
