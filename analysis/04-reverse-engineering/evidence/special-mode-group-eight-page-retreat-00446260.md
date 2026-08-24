# G08整页后退 `0x00446260`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00446260..0x004463EB`，194行，无FUNCTION CHUNK。3B480保存callback地址；455E0在主、次第一动态Y域直接调用。

模式snapshot大于等于500时直接复用已关闭C670运行时整页后退。模式2在selection31时只发布selection；其余路径直接复用BC60，以window/local/step13三个实际读取参数后退，再由B9A0发布可见链、BC90按13重计，低字节OR 3，发布local、播放46，并由B9C0/B9E0更新文本。

BC60原ABI虽压入五参，但权威LST只读取window offset、local cursor和step；typed helper有意收窄到这三项，不伪造total/visible副作用。链越界停止规则与其他三方向控制一致。

模式3从槽0向后寻找首个非FFFF marker，完整四槽均为FFFF时typed-stop。模式5/10/11分别写动作0、外层行0、列0。模式15以步长8复用BC60并OR 0x0300。

455E0两处第一动态callee已回收为本typed helper；至此四个方向控制均无通用opaque dispatch端口。UT覆盖模式2十三步回页、可见链/文本、首个party槽、固定首值、模式15八步回页及大于等于500委派；caller回归与独立ASan通过。

workpack双生成稳定为`129/227`，SHA256均为`7d6d77c60d3e180023b100df41f9c0c920267c95c22cfd639bea8fdcff3762fc`；下一单元`0x00446420`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
