# 护驾类别图标 `0x00442960`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00442960..0x004429A0`，41行，无FUNCTION CHUNK；唯一caller为425C0，共八处调用，现已全部回收为直接typed helper。

## 行为

四参数依次为action frame word、category、X、Y。

1. 以`(u16 action_frame_word, i32 category)`调用资源解析边界，对应4315D0。
2. 原程序把返回pointer写入`4FCD60`，但全LST只有该写、无任何reader；现代端不保留无消费者裸pointer。
3. 在原`mov edx,[eax]`解引用点检查资源是否存在；缺失时typed-stop，资源解析调用已发生，但不发布draw。
4. 存在时按原顺序读取source dword、height u16 at `+0x0E`、width u16 at `+0x0C`，发布`source,x,y,width,height,0,0`。
5. 返回draw的EAX。

资源结构与42130差值icon共用`LegacyStandardModeGuardianIconResource`，避免把平台资源pointer暴露给业务层。

425C0八项循环已不再发布`frame/category/x/y` opaque操作；每项直接调用42960，聚合operation及EAX，并在首个资源typed-stop时保留十一行、选框、mode0动作及category prepare副作用，且不执行末尾`id=0,variant=0x44`清理，符合原崩溃点前顺序。

UT覆盖frame/category解析键、source/X/Y/width/height/零参数、draw返回、资源缺失不draw、以及425C0首项失败的状态传播和action残值。定向测试通过。

workpack双生成稳定为`88/227`，SHA256均为`eb82ef1ba6acb130424d45a5c12c35f3486ebd9be8e72bad8f8c4b05ba4f3a57`；下一单元`0x004429B0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
