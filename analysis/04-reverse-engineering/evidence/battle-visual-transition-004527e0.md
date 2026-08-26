# 战斗画面转场与后置事件 `0x004527E0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整LST范围

权威LST函数为`0x004527E0..0x00453093`，共1002行、74个静态call站点、37个标签，无外部FUNCTION CHUNK。单参数只取低16位作为转场mode。

## 2. 入口、surface与raw缓冲顺序

入口严格执行：

1. 以固定参数`0xC0`和capture source调用准备callee；
2. active latch写1；
3. 两个raw全局token和两个本地转换token清零；
4. 创建固定格式`0x2711`临时surface，并由第二display surface执行一次虚调用；
5. 才依次申请两个`0x96000`字节raw缓冲。

modern保留“临时surface先于分配”的顺序；raw缓冲用typed token与`307200`个u16建模。

## 3. 第一快照与像素复制

首快照固定取第二display surface。原循环共480行：

- `stride_words = arithmetic_sar(pitch_bytes,1)`；
- 每行源偏移以32位signed `imul`回绕；
- 每行复制`0x140`个dword，即640个u16；
- 目标每行增加`0x500`字节；
- 目标末偏移达到`0x96000`时结束。

modern逐行复制并保留低32位乘法。raw缓冲或surface越界在首个真实行访问typed-stop；已经复制的行和已完成的两次分配保留，不伪造unlock或cleanup。测试以十行短缓冲证明第十一行停止。

快照成功后调用图像转换，固定几何`640×480`、bpp 16，返回token作为第一转换图像。

## 4. 首次场景绘制与可选第二快照

随后依次执行场景准备、两个全局阶段、已关闭`0x00450270`固定资源0帧绘制、状态word发布，再由第一display surface呈现目标surface。

`0x00450270`已直接调用typed helper，不保留opaque边界；固定资源`0x234D`、X=0、Y=384、帧索引0。frame unavailable或blitter typed-stop阻断后续转场。

mode低word不小于1时，固定从第一display surface复制第二raw快照并转换；mode 0仍分配第二raw缓冲，但不访问、不转换。

接着锁定目标surface、立即unlock但保留原指针语义，发布第一转换图像为当前source，并以参数`0,0,640,480,0,0`绘制一次全屏图。第五参数为0；只有后述mode 0滑动帧使用`0x20`。

## 5. 34帧进入循环

局部初值：

```text
transition_x = 1
transition_y = -50
counter = 9
negative_frame = 0
scale_step = 0
vertical_scale = 1024
```

每帧公共尾固定：`transition_x += 2`、`transition_y -= 2`、`counter += 18`、`negative_frame--`，创建临时surface并对目标surface执行虚调用。循环条件产生恰好34帧。

### mode 0

每帧写三个偏移latch为`-12,-12,transition_x+640`，再以：

```text
x = negative_frame
y = transition_y
width = 640
height = 480
effect = 0x20
last = 0
```

执行全图绘制。不清目标，不修改scale_step。

### mode 1

每帧：

```text
sine = trunc_x87(sin(i * dbl_499D08 * dbl_499D00) * 256.0)
scale = (64 - trunc(counter / 10)) << 4
```

清零`0x25800`个dword目标像素，再以`+sine`和`-sine`各转换一次；scale X/Y相同。

LST常量bit pattern为：

- `dbl_499D08 = 0x40091EB860000000`；
- `dbl_499D00 = 0x3F9F07C1F07C1F08`；
- `256.0 = 0x4070000000000000`。

以原x87 `fild/fmul/fsin/fistp qword`和截断控制字独立生成并冻结34项：

```text
0,24,48,72,95,117,138,158,176,193,208,221,232,241,248,253,255,
255,253,248,242,232,221,208,193,176,158,138,117,95,72,48,24,0
```

不依赖宿主libm。

### mode 2

每帧发布：

```text
scale_x = (256 - scale_step) << 2
scale_y = vertical_scale
y_offset = wrapping_i32((-scale_step) << 5)
```

清目标后执行一次转换。负偏移以u32左移再bit-cast，避免C++负数左移未定义行为。

其他mode不执行mode专属绘制，但仍完成34次公共临时surface与虚调用。

## 6. 第二阶段

进入循环后先恢复`0,0,640,480`clip，再次锁定目标surface。

- mode 2：只再清目标一次；合计35次清零、34次转换。
- mode 1：当前source切换为第二转换图像；执行33帧。scale counter从`69*9=621`开始，每帧减18；清目标后按`-sine`、`+sine`顺序转换。进入+退出合计67次清零、134次转换。
- mode 0：创建/调用一个临时surface，重复场景准备、两个阶段、固定资源0帧绘制、状态word与display呈现；再创建/调用一个临时surface，调用`random(3)`并以两个display surface和随机值执行blend。

已关闭帧绘制更新与转换图像共享的旧source全局。modern显式记录source当前来自frame还是转换token：首次frame后立即被第一转换图覆盖；mode 0第二次frame后保留frame来源。

## 7. cleanup

正常视觉阶段完成后，按固定顺序非零释放：

1. 第一raw token；
2. 第二raw token；
3. 第一转换token；
4. 第二转换token。

raw全局token不写零，成为已释放陈旧token；modern保留token并标记released。随后active写0，再次恢复完整clip。任何此前typed-stop不执行合成cleanup。

## 8. 音乐路径

music gate严格等于1才处理。路径先复制data root，再按battle ID低word执行三个互不重叠的独立inclusive区间：

- `1..0x70`：`music/Battle_Europa01.mp3`；
- `0x72..0xB9`：`music/Battle_Arab01.mp3`；
- `0xC6..0x10E`：`music/Battle_China01.mp3`。

ID 0、`0x71`、`0xBA..0xC5`和大于`0x10E`不追加文件名，仍把data root传给music start。start固定mode零；随后music commit收到共享runtime handle。正常后续EAX从commit继续。

## 9. 两条低概率事件

若共享flags低字节bit`0x40`已置位，跳过全部随机事件并保留music gate/commit EAX。

否则先`random(2)`选分支，再共用一次`random(100)`：

### 分支0

只有随机值`55..60`进入。逐敌人查询mode；返回不等于1时调用事件callee，参数为固定2和敌人索引。随后逐队员依次执行消息准备与刷新。最后发固定文本token、flag`0x40000002`与坐标`(280,10,60)`。

### 分支1

只有随机值`27..32`进入。逐队员：mode返回1或actor特殊字段等于1则跳过；否则执行准备与reset，并扫描十槽数组，把第一个零槽写为`party_index+8`。数组已满则不写。随后逐敌人执行消息准备与刷新，发另一固定文本token和相同参数。

消息成功路径只对共享flags低字节OR `0x80`，并以更新后的完整dword作为EAX。随机范围未命中时返回`random(100)`完整EAX。

敌人第九槽或队员第十一槽在首次对象访问typed-stop，保留视觉、cleanup、音乐和此前事件副作用。

## 10. 双向追溯

- `0x004527E0..0x0045283D`：准备、active、token清零、临时surface；
- `0x00452840..0x004528DC`：双raw分配、第二surface首快照与转换；
- `0x004528DC..0x00452A1B`：首场景、直接frame helper、可选第一surface次快照、第一图发布；
- `0x00452A1B..0x00452C23`：mode 0/1/2的34帧进入循环与clip恢复；
- `0x00452C23..0x00452E06`：mode专属第二阶段；
- `0x00452E06..0x00452E6C`：四token条件释放、active清零和clip恢复；
- `0x00452E6C..0x00452F08`：music gate、三ID区间、start与commit；
- `0x00452F08..0x0045308C`：bit门、两条随机事件和公共消息；
- `0x0045308C..0x00453093`：保留路径EAX返回。

C++到LST反向追溯覆盖1002行、74个静态call站点、37个标签、四类正常尾值及全部循环。

## 11. 验证与动态差分

定向测试覆盖：

- 第二surface→第一raw、第一surface→第二raw的480×640实际像素复制；
- 两次`0x96000`分配、两次转换及四token释放顺序；
- mode 0/1/2的帧数、清零数、全图/转换数、临时surface与虚调用数；
- 初始全图effect零与mode 0滑动effect `0x20`；
- 冻结x87正弦与mode 1最终scale；
- 两次固定资源0帧typed直连及source ownership；
- Europe、Arab、China与gap ID音乐路径；
- 两条低概率事件、mode门、十槽首空写、消息flag与最终EAX；
- 十行短raw缓冲在第十一行停止，不unlock、不cleanup、active保持1；
- battle聚合目标零warning，普通定向与独立ASan定向均`1/1`通过。

当前没有原版DirectDraw surface、allocator、图像转换、音乐后端、74个callee联合寄存器与角色状态捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。角色、场景、blend和消息callee均按各自后续工作包保留typed端口。
