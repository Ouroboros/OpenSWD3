# 模式1按左向操作减少当前值 `0x0044E260`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0044E260..0x0044E315`，105行，无外部FUNCTION CHUNK。caller为`0x0044DBC0`和`0x0044F920`。DBC0在左侧点击区域调用本函数，E330为右侧对称入口。

函数按level 1..4分派；其他level无副作用并保留`level-1`返回域。

## 2. 四层行为

- level1：打包模式低两位减1并在0饱和，只清写低两位，保留其他30位。
- level2：按`window+cursor`的32位结果执行B9A0 signed推进，选中记录原布局`+6`映射typed `combined_value`。数量先做u16减1，再按i16比较：结果小于等于0时强制写0，不播放声音；结果大于0时播放样本`0x00B9`。两条路径最终都把左向动作状态写2。
- level3：只清打包低字节bit2。
- level4：只清打包低字节bit3。

数量夹零路径原EAX为记录指针；modern结果通过typed `selected_record`和`returns_selected_record`表达，不伪造宿主地址的32位截断值。

## 3. typed-stop与验证

B9A0选择越过null时只在原`[eax+6]`读取前停止，不写数量、不播放声音，也不发布动作状态。signed夹零保留`0x8001 -> 0x8000 -> 0`等原域行为。

UT覆盖level1高位保留和低两位减值；level2从2减到1时播放B9并写状态2；从`0x8001`减到signed非正值时夹0、返回typed记录且无声音；level3/4分别清bit2/3；短链在数量读取前停止。

workpack双生成稳定为`209/227`，SHA256为`cf2d2a345f231ca302e2964d7e4f5f62fe125ff72489b9307517f812357380d0`。`0x0044DBC0`继续等待callee闭环；下一单元为`0x0044E330`。
