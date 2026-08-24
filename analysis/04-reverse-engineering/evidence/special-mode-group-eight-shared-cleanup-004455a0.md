# G08共享清理 `0x004455A0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x004455A0..0x004455D1`，30行，无FUNCTION CHUNK。code callers为尚未关闭的446700与446FE0多处；FC0另把它安装到二级清理表索引11。

函数先调用尚未关闭4482E0执行共享选择记录清理。成功返回后，按LST精确清零D094、D160、D150、D08C以及列表offset/local六个owner；不清FC648、D0A0、可用计数、布局owner或workspace token。最后读取workspace token交给4885A0等价typed释放端口，并原样返回释放结果，不把token擅自清0。

typed state已纠正45430/455A0的清零集合：五项前置owner单独保存，FC648所在索引2在455A0保留；offset/local独立；D0A0为record zero，发生在45430记录初始化之后；两个尾部owner继续独立。

4482E0保留最窄typed端口。记录清理停止时保留callee已提交状态，不执行六项清零或workspace释放。UT覆盖精确清零/保留集合、token重读与不清零、释放返回值，以及记录清理停止前缀。

workpack双生成稳定为`124/227`，SHA256均为`5919a127123d8d781f676b1ac687330f5af7710014a65e4d4a17a5b283a489e4`；下一单元`0x004455E0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
