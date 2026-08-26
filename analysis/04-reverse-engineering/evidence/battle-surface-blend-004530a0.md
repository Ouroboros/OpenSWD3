# 战斗双surface逐行混合 `0x004530A0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整LST范围

权威LST函数为`0x004530A0..0x004531F8`，完整171行、12个静态call站点、6个标签，无外部FUNCTION CHUNK。

调用点位于`0x00452DFE`。caller向callee实际传六个dword：两个display surface token、三个零和一次`random(3)`结果；callee只读取`arg_0`与`arg_4`，后四个参数全部未读。因此随机结果虽按原时点生成并传入，却不影响混合行为，也不形成越界域。

## 2. 屏幕surface建立

入口栈上保存两个480项数组。原顺序为：

1. `GetSystemMetrics(1)`读取屏幕高度；
2. `GetSystemMetrics(0)`读取屏幕宽度；
3. 以固定owner和宽高建立screen surface，返回token保存；
4. 把第一数组的480个dword清零；
5. 对第二数组执行480次`random(20)+15`写入。

第二数组后续从未读取。这480次随机调用和写入是原始可观察副作用，不能删除或用常量替代。

screen surface返回零时，原程序仍完成数组清零和480次随机写入，直到首次读取其vtable才故障。modern在该真实访问点typed-stop，不执行虚调用、逐行循环、临时surface或释放。

## 3. 外层循环与未使用随机表

每轮开始都重新从栈槽载入screen surface token，再调用其操作入口：

```text
object = screen surface
rectangle A = null
source = arg_4 / secondary display surface
rectangle B = null
trailing = 0, 0
```

返回值不消费。

`var_F24`完成计数在入口置零，但每次行偏移小于等于零都递增，不检查该行此前是否已经完成。

## 4. 480行逆序循环

行号固定从479递减到0，对应第一数组从高地址向低地址访问。每行执行：

```text
random = random(20)
delta = wrapping_i32(-30 - (random << 1))
offset[row] = wrapping_i32(offset[row] + delta)
if signed(offset[row]) <= 0:
    offset[row] = 0
    completed++
rectangle = {offset[row], row, 640, row + 1}
```

原程序对两个独立`tagRECT`各调用一次`SetRect`，值完全相同。modern直接建立两个值相同的typed rectangle；Win32 API返回值原本被忽略，不引入平台副作用。

随后调用screen surface操作入口：

```text
object = screen surface
destination rectangle = typed rectangle
source = arg_0 / primary display surface
source rectangle = typed rectangle
trailing = 0, 0
```

逐行surface操作返回值同样不消费。

随机callee的正常合同为`0..19`，因此零初值第一轮每行立即被夹为零，完成计数达到480。原实现仍继续一轮，因为外层条件是signed `completed <= 480`。第二轮同样逐行递增，计数达到960后退出。因此正常固定行为为：

- 未使用表随机：480次；
- 逐行随机：960次；
- 外层轮次：2；
- secondary整面捕获：2次；
- primary逐行操作：960次；
- rectangle pair：960组。

不增加现代循环上限；callee违反随机合同时仍按32位回绕和signed比较继续。

## 5. 每轮临时surface

每轮480行结束后，以固定owner和格式`0x2711`创建临时surface，再调用：

```text
object = temporary surface
rectangle A = null
source = screen surface
rectangle B = null
trailing = 0, 0
```

临时surface不由本函数显式释放。正常路径共创建并调用两次。

临时surface返回零时，原程序在立即读取vtable处故障。modern在该真实访问点typed-stop；保留已完成的整轮480行、480完成计数、此前960次总随机调用和481次surface操作，不伪造临时copy、第二轮或screen释放。

## 6. 退出与EAX

第二轮结束时完成计数960，signed `960 <= 480`为假。函数取回screen surface token：

- 非零：调用vtable `+8`释放，并原样返回release完整EAX；
- 零：原路径返回零，但正常控制流在首次vtable访问前已不可到达此尾部。

screen token不提前清零，两个临时surface不显式释放，六个入口参数中的后四项保持未读。

## 7. caller边界回收

`0x004527E0`的mode 0路径已删除opaque blend port并直接调用本typed helper。caller仍在调用前执行`random(3)`，并把结果放在第四个未读尾参数中；任意u32结果都不会触发typed-stop。

同次回收还移除了caller后置事件对`random(2)`和`random(100)`的人造范围typed-stop。权威LST只对前者执行零/非零判定、对后者执行inclusive区间比较；非约定返回值现在继续产生原普通分支/返回值。

## 8. 双向追溯

- `0x004530A0..0x004530C7`：栈、系统宽高和screen surface建立；
- `0x004530CE..0x00453101`：480项offset清零与未使用随机表写入；
- `0x00453105..0x0045311C`：外层reload与secondary捕获；
- `0x0045311F..0x004531AA`：479→0逐行随机、夹零、双SetRect和primary行操作；
- `0x004531B0..0x004531DA`：临时surface、screen copy和signed外层条件；
- `0x004531E0..0x004531F8`：screen token判空、vtable释放与EAX返回。

C++到LST反向追溯覆盖完整171行、12个静态call站点和6个标签。

## 9. 验证与动态差分

定向测试覆盖：

- 系统指标严格按1后0查询，原宽高传入screen surface创建；
- 480项未使用随机表全部写成`random+15`；
- 正常两轮共1440次随机、960次逆序行操作、960组同值rectangle、两次临时copy；
- 首行为Y=479、尾行为Y=0，right/bottom固定640与`row+1`；
- 六参数后四项改变不影响输出；
- 最终只释放screen surface并返回release完整EAX；
- screen surface零在480次随机后typed-stop；
- 首个临时surface零在第一轮完整副作用后typed-stop且不合成释放；
- caller以超出约定的`random(3)`结果仍完成blend，后置随机超出约定时按原比较域返回；
- battle聚合目标零warning，普通定向通过。

当前没有原版GetSystemMetrics、DirectDraw screen/temporary surface、虚操作、CRT随机状态与释放EAX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
