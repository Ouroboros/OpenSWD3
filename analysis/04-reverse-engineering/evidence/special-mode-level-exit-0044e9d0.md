# 按四级层级退出特殊模式 `0x0044E9D0`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0044E9D0..0x0044EA47`，69行，无外部FUNCTION CHUNK。caller为`0x0044DBC0`、`0x0044E4A0`和`0x0044F920`；callee DA40、F8E0与D5A0均已关闭并由typed helper直接复用。

入口先形成`level-1`的32位返回值。分支严格为：

- level1：先减到0，调用DA40完整清运行环境；成功后才把特殊模式enabled清0。
- level2：先减到1；调用F8E0清工作区链；把第二份工作帧的`0x4B000`个u16复制回第一份暗帧；清transition flag bit1；最后按原指令继续调用D5A0清同一head。正常域第二次为空链，仍保留该冗余调用。
- level3：减到2，无其他副作用，返回2。
- level4：直接写2，不执行清理，保留入口形成的返回3。
- 其他level：不修改，返回原level减1的32位结果。

## 2. typed-stop

- level1若DA40在工作区循环链重复读取点停止，不清enabled。
- level2若F8E0停止，不复制帧、不清bit1、不执行第二次D5A0。
- 工作帧或暗帧短于`0x4B000`时，分别在原qmemcpy源/目标访问点停止；此前层级减一与首轮工作区清理保留。

## 3. 验证

UT覆盖level1清owner、两帧、工作区并禁用；level2释放记录、精确复制`0x4B000`像素且保留尾哨兵、清bit1和冗余空链清理；level3、4及其他值的返回与层级；level2自环停止；level2短工作帧停止。

workpack双生成稳定为`203/227`，SHA256为`80946fdbeaf33d878bed25a2aa980cc27788d2d69667ada335cd441201cc1716`。`0x0044DBC0`继续等待callee闭环；下一单元为`0x0044FD30`。
