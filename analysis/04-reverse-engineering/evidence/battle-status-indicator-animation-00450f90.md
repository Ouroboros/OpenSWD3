# 战斗状态指示器动画 `0x00450F90`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、调用图与共享状态

权威LST完整范围为`0x00450F90..0x004510FF`，从`proc`到`endp`共170行，没有外部`FUNCTION CHUNK`。函数无参数，唯一caller为`0x004539B0`内的`0x00454A14`。

直接callee为有界随机`0x00439070`、动作更新`0x004321E0`、帧查询`0x004315D0`、软件blitter`0x004170E0`和提示音包装`0x00485670`。动作更新、帧查询和blitter直连已关闭typed接口；有界随机与音频所有权分别以端口注入，调用边界和参数保持可观测。

四个共享word/dword显式映射为：

- `0x004FDF70`：32位tick计数；
- `0x004FDF74`低word：左右状态；高word：完成后hold计数；
- `0x004A74C8`低word：亮度；高word：亮度衰减计数；
- `0x004FD638`：持久0x98字节动作record。

## 2. tick零初始化门

入口只在tick完整32位值为0时初始化状态。

若完成hold高word按unsigned比较大于等于1，函数把左右状态低word和hold高word同时清零，不消费随机。否则调用完整有界随机helper `random_bounded(2)`，只把返回AX写入左右状态低word，高word保持原值。

`0x00439070`本身以`0xFFFF / bound`建立拒绝阈值并循环推进旧随机表，直到值低于阈值后取unsigned余数；modern在本函数边界只调用一次等价有界随机端口，不用CRT `rand()`替代，也不额外消费随机。

测试锁定随机0选择左侧、随机1选择右侧，以及hold非零时清两个状态word且随机调用数为0。

## 3. 动作record、更新与帧查询

函数不在入口清动作record。它只覆盖：

```text
action_id = 0x2329
base_variant = side_state == 1 ? 3 : 2
```

比较是低word完整值精确等于1；其他非1值选择变体2。旧wait、缓存键和其他字段在更新前保留。普通返回0路径也不清record。

动作更新失败时，动作号和变体写已经发生，但帧查询、绘制、tick和亮度均不前进。

更新成功后只执行：

```text
resource = (post_update_eax & 0xFFFF0000) | record.field_4a
frame = 0
```

资源低word覆写更新后EAX的AX，高16位保持陈旧snapshot；帧号固定完整0。查询成功后发布source，不发布全局frame record。查询失败在原首次frame解引用点typed-stop，保留此前共享状态。

测试以旧wait和预装资源低字证明record没有入口清零，并锁定资源高字snapshot。

## 4. 指示器帧绘制

绘制坐标按低32位LEA链计算：

```text
X = 0x104 + side_state * 50
Y = 200
```

左右状态0/1对应X=260/310。帧u16宽高、record mode flags和共享source传给blitter，第六物理tail固定0；typed调用清空实际palette/auxiliary。indexed帧在首次palette读取点停止。

正常`completed`、`clipped_out`或`opacity_disabled`立即执行公共后缀，清目标高度、水平位移、纵向phase、opacity、RGB与跳行并保留放大位。其他typed-stop不执行tick、亮度、翻转或提示音，也不清共享状态。

## 5. tick与25帧packed-word亮度推进

绘制正常后tick按u32加1回绕，再把结果解释为signed EAX，以`cdq; idiv 25`判断signed余数是否为0。

若不是25的倍数，AX直接取亮度低word。

若是25的倍数，原指令先读取完整packed亮度dword，执行低32位加倍，再把结果AX同时写入低亮度word和高衰减word：

```text
new = u16(packed_intensity * 2)
intensity = new
intensity_countdown = new
```

高word旧值不会影响乘积低16位，但两个目标word都取同一个AX；不得把两个word分别加倍。测试以旧高word`0x9999`和低word31锁定第25 tick得到低62、高先62后衰减为61。

## 6. 亮度阈值完成出口

AX按unsigned与`0x40`比较。亮度达到或超过64时：

1. 两个亮度word均写1；
2. tick清零；
3. 若左右状态低word精确等于1，hold高word按u16加1；否则不改；
4. 完整152字节动作record以`rep stosd`语义清零；
5. 返回1。

左右状态低word本身不在该出口清零。下一次tick零调用若hold非零，才走第2节的双word状态清零门。

这是唯一返回1出口。测试锁定tick 24、亮度32在绘制后变为64，随后亮度复位、tick清零、右侧hold加1、record逐字节全零且不播放声音。

## 7. 衰减、状态翻转与提示音

亮度低于64时，函数先按u16递减高衰减word。若结果非零，直接返回0。

若高word减到0：

```text
intensity_countdown = AX
side_state = side_state == 0 ? 1 : 0
play_indicator_sound(sound_id=0x2E, level=1)
return 0
```

非25倍tick的AX是亮度低word；25倍tick的AX是刚加倍值。状态翻转只区分完整低word是否为0：0变1，任何非零值变0。

`0x00485670`将两个入口都按u16解释，再把level换算为底层混音参数；本caller固定传`0x2E,1`，其返回1被丢弃。modern音频端口记录相同两个u16参数，本函数随后固定返回0。

若高衰减word原为0，u16递减回绕为`0xFFFF`，不会翻转或播放声音。

## 8. 双向追溯

- `0x00450F90..0x00450FCC`：tick零门、hold双word清零或`random(2)`；
- `0x00450FCC..0x00451011`：变体2/3、持久record更新、资源陈旧EAX高字与固定帧0查询；
- `0x00451016..0x00451052`：source发布、X=260+50×状态、Y=200、记录尺寸/flags和固定空tail绘制；
- `0x00451057..0x0045108E`：tick u32递增、signed除25和packed亮度低字加倍；
- `0x0045108E..0x004510CF`：unsigned 64阈值、亮度/tick复位、右侧hold增加、record清零和返回1；
- `0x004510D0..0x004510FF`：高word衰减、归零重载、状态翻转、固定提示音和返回0。

C++到LST反向追溯覆盖所有共享读写、五个callee、两个返回值、record清零及失败前缀；没有未解释基本块、外部chunk或出口。

## 9. 验证与动态差分

定向测试覆盖：

- tick零的随机0/1两侧选择；
- hold非零时双word清零且不消费随机；
- 入口旧wait/resource/canary保留与更新后EAX资源高字；
- 左右X=260/310、固定Y=200与实际像素；
- 高衰减word归零后的重载、状态翻转和`0x2E,1`提示音；
- 第25 tick packed低字加倍、低于阈值后高word再减1；
- 达64唯一返回1出口、右侧hold增加及record逐字节全零；
- 更新失败、帧失败的tick/亮度停止前缀；
- indexed固定空tail在palette读取点停止，阻断公共后缀和所有计数副作用。

battle聚合目标零warning构建及定向测试通过。

当前没有原版有界随机表、动作更新后EAX、四个共享计数word、frame record、共享blitter状态、提示音后端和framebuffer联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整170行LST、唯一caller和五个callee已完成固定状态闭环。
