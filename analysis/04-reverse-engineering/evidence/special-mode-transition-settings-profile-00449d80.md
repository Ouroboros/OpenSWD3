# 特殊模式转场设置角色条件 `0x00449D80`

状态：`assembly_exact`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00449D80..0x00449FE6`，268行、14次`lstrcmpA`调用，无FUNCTION CHUNK。唯一直接caller为490C0。

所有匹配均以LST数据段原Big5字节为真值，主字符串与副字符串条件独立、按原顺序全部执行，不改写为互斥switch：

- 主字符串何然：primary words 6..9写25。
- 副字符串江如紅：secondary word 8写100，1与4写300。
- 主字符串楊坤碩：primary word 6写50，0与3写200。
- 主字符串古月聖：primary 9字节fill写FB。
- 主字符串輔子徹：primary words 6..9写30。
- 副字符串紋錦：secondary 9字节fill写F9。
- 主字符串鑄石子：primary word 7写100，2与5写300。
- 主字符串樂樂：primary word 8写70，1与4写250。
- 主字符串大米：primary word 8写100，primary 9字节fill写02。
- 副字符串真夢：secondary word 7写100，2与5写300。
- 副字符串紅珊瑚：secondary word 6写50，0与3写200。
- 主字符串寧采臣：primary words 7..9各按u16减5、word6按u16减10，并写refresh delay 500。
- 主字符串燕赤霞：primary words 7、2、5写0。
- 最后比较副字符串小倩；命中时secondary words 8、1、4写5，并保留原EAX=5返回，而不是比较结果0。

字符串比较保留NUL终止和三态返回；字段按原未对齐写域收束为10个primary u16、9个primary fill字节、9个secondary u16、9个secondary fill字节及refresh delay。490C0删除`prepare_transition_settings_runtime` opaque端口，在进入progress5的同帧直接调用typed条件表，再继续present与设置绘制。

UT覆盖九个主字符串和五个副字符串全部条件；验证双路条件可同次命中、FB/F9/02整段填充、u16减法、明确清零、最终小倩返回5，并验证490C0设置选择caller直接更新两组profile字段。

workpack双生成稳定为`156/227`，SHA256均为`5b5c54fe0fef3475178862b440d70077b3dd9b13e93af5a7b938c3220bf0bf4a`；下一单元`0x00449FF0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
