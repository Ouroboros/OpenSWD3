# 标准模式mode strip `0x0043D470`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与边界

唯一行为真值为`swd3.exe.lst`。函数物理范围为`0x0043D470..0x0043D52F`，94行，唯一caller是已关闭`0x0043C820`的`0x0043C8DA`。直接callee为两次viewport owner `0x00416FF0`、最多多次资源record owner `0x004315D0`和资源绘制owner `0x004170E0`。

C820原`prepare_frame`高层占位实际吞掉了整个D470，不对应独立frame准备callee。现删除该占位，port只保留精确的viewport、resource load和resource draw边界；循环、资源ID/variant、共享handle owner、坐标和EAX均在typed helper内。

## 2. viewport与候选窗口

入口先调用viewport owner：`x=10,y=1,width=206,height=478`，忽略其EAX。随后读取current mode，按u32回绕计算candidate=`mode-2`和signed upper=`mode+2`。X从6开始，每次迭代无论candidate是否绘制都加40。

candidate只有同时满足以下条件才绘制neighbor：

- signed candidate≥0。
- signed candidate≤11。
- candidate不等于当前mode。

有效neighbor以`resource_id=0x2439`、`variant=candidate`加载record。helper把record dword0发布为typed `active_render_resource_handle`，读取u16 `+0x0C/+0x0E`为width/height，并以`x=current_x,y=61,width,height,0,0`调用draw owner。

资源加载失败在原record解引用点typed-stop；不会执行该draw、后续candidate、center或viewport恢复。

## 3. mode重读BUG兼容

每次neighbor draw完成后，原程序重新读取共享current mode，再用新值计算`mode+2`循环上界；跳过candidate时不重读。typed helper保持这一顺序。

UT从mode5开始，在第一次draw后把mode改为6。实际neighbor variants变为`3,4,5,7,8`，X为`6,46,86,166,206`，并最终以variant6绘制center。这证明实现不是预先固化五项范围。

## 4. center与返回

neighbor循环结束后，以最后一次已读取current mode加载`resource_id=0x243A`。发布handle后以固定`x=86,y=58,width,height,0,0`绘制center。

最后调用viewport恢复：`x=0,y=1,width=640,height=478`，原样返回其EAX。稳定mode5时neighbor variants为`3,4,6,7`，X为`6,46,126,166`；center variant5。mode0时跳过`-2,-1,0`，只绘variants1/2于X126/166，再绘center variant0。

## 5. C820回接与验证

C820在split bar之后、alias首读之前直接调用本helper：

- 完成时把viewport恢复EAX作为空列表路径返回值。
- typed-stop时传播`mode_strip_stopped`，保留入口viewport和此前draw，不读alias。

定向UT覆盖mode0、mode5、draw后mode改为6、两个viewport精确参数、0x2439/0x243A资源顺序、X/Y、width/height/handle透传、共享handle最终值、restore EAX `-77`及首个resource失败不恢复viewport。

`special_modes.legacy_initial_menu`定向测试通过。workpack连续两轮稳定为`43/227`，SHA256均为`33cc4028a439a928bb19dd5af0b63664af1eda7993a6dbac96dc03dbb49de35d`；下一独立单元为`0x0043D530`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
