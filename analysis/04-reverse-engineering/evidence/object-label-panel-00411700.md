# 可选文本输入对象标签面板 `0x00411700`

状态：`platform_adapted`

## 1. LST锁与caller回收

权威范围为`swd3.exe.lst`的`0x00411700..0x004117E5`，共123行，无外部`FUNCTION CHUNK`。唯一caller是标题/新游戏过渡整帧`0x004490C0`的`0x004497CE`。

`0x004CACD8`已由全局owner审计确认为菜单拥有的可选文本输入对象指针。callee关闭后，现代`render_legacy_title_menu_frame`在原`LABEL_78`位置、overlay update/poll之前直接调用typed标签面板；不再留opaque命令。callee typed-stop时caller保持此前对象状态查询并停止后续overlay副作用。

## 2. 固定顺序

1. 以对象ID调用准备入口`0x00480AD0`。
2. 依次读取对象x、y。
3. 查询资源`0x2449` frame0；发布资源首dword为当前source owner。
4. 在`(x-108,y-46)`以资源u16宽高、flags0、auxiliary0 blit。
5. 向64字节全局缓冲读取对象文字。
6. 用`CharNextA`逐字符前进；步长恰为1且当前byte按i8为负时替换为`@`。
7. 直接用已关闭RGB owner计算`(21,15,8)`的双lane颜色；在原`destination owner,(x,y),style4`绘制原始字节文字。
8. 读取对象文字metric，并用32位回绕计算`x + 11*metric`。
9. 调用已关闭矩形效果边界：`(x+11*metric,y,11,22,20,13,0,5)`；其返回值原样作为函数返回。

## 3. 字节合同

原目标使用CP950/Big5 DBCS：`0x81..0xFE`且后续仍有非NUL字节时按2字节前进，第二字节即使是`0x40('@')`也不得单独改写。ASCII保持；`0x80`等非lead孤立高位byte及末尾无trail的lead按1字节前进并替换为`0x40`。空串仍执行颜色、文字绘制、metric和矩形效果。

文字长度必须在64字节缓冲内留出NUL。无终止符只在原后续字节读取点typed-stop，保留已经完成的背景owner发布与blit。

## 4. 算术与平台边界

背景坐标减法及`x+11*metric`均为x86 32位回绕，不使用64位夹值。颜色直接复用`rendering::legacy_pack_color_pair`。资源解析、对象坐标/文字/metric、背景blit、原始字节文字和矩形framebuffer作用由窄端口承接；SDL smoke提供无资源依赖的最小适配，不启动游戏EXE。

## 5. 验证

`special_modes.legacy_initial_menu`覆盖：正常对象调用顺序；资源2449和固定偏移；Big5 pair与两个孤立高位byte；RGB packed值；destination owner、style4与矩形参数；资源缺失前缀；64字节无终止符前缀；空串仍绘制；极值坐标和metric回绕；以及`0x004490C0`在overlay update/poll前直接调用和callee停止传播。
