# 炼妖祭坛波面动画 `0x004400A0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。完整函数范围`0x004400A0..0x004404C1`，453行；唯一caller为E800 phase3。callee只有439070有界RNG和485610 sample。E800虽然压入`countdown,0x78,0x18`三项，函数体不读取栈参数，而读取FCAB4及显示全局；typed端保留该事实，直接使用state countdown与framebuffer边界。

## 状态owner

- FCD08/FCB9C：两块120项signed横向位移buffer，对应state small buffer 2/3。
- FCAA0/FCD0C：两块220项signed纵向位移buffer，对应state large buffer 2/3。
- FCB90/FCA9C/FCA98：FDE0第二至第四槽的120×220 source/left/right surface；FDE0平台边界现直接填充四份state-owned像素数组，同时保留32位legacy token。
- FCA94+0x200：D530建立的256项位移镜像表，即`mirrored_values[0x80+signed displacement]`。
- FCAC8：0..119 ring offset。原函数在内层递增局部副本但从不用于地址计算；typed端不制造伪效果，只保留函数尾全局递增。
- CC2FC/CC5C8/CD76C：framebuffer pitch bytes、高度和像素base，由最小RenderPorts提供。

## 倒计时分支

1. countdown在50..109时执行`8*countdown-400`次稀疏dword复制；每次以`random(0x3354)`选择source surface中的两个相邻u16，并同时写left/right surface。
2. countdown小于80且低三位不等于7时sample `0xB7`。
3. countdown等于110时sample `0x208`，随后把52800字节source完整复制到left/right两份surface。
4. countdown 0..119按`countdown/40`选择强度3/5/7；对120项横向和220项纵向buffer分别生成`random(strength)-strength/2`，buffer2加、buffer3减，均保留low16环绕。
5. countdown 120..140不再调用RNG，对四块signed buffer逐项执行`9*value/10`向零截断衰减。
6. 当signed `180-countdown >= 40`时执行投影。countdown钳制0..120；第一基址为`0x8C+height/2+clamp`，第二为`0x17C+height/2-clamp`，行步进为pitch/2像素。两个投影均使用small buffer2、large buffer2和D530镜像表；第二投影不使用buffer3是原始可观察BUG。
7. 每个投影严格遍历220×120；负目标地址跳过，其他目标写对应surface像素。投影后`FCAC8++`，达到120时全局写0，但EAX保留120。countdown 141及以后跳过投影并返回pitch/2。

镜像索引、稀疏source索引和framebuffer上界分别在原裸读取/写入点typed-stop；此前sample、RNG、buffer及surface副作用不回滚。E800删除opaque `draw_countdown`操作，直接聚合helper；失败发生在countdown自增前。

UT覆盖强度3扰动、B7 sample、双52800像素投影、51帧稀疏复制、110帧208 sample与双全拷贝、120帧signed衰减、141帧跳过、ring 119→0但EAX120、三类typed-stop及E800传播。独立ASan执行通过。

定向测试通过。workpack双生成稳定为`68/227`，SHA256均为`4513c6aaa7e58e15263a25b63bf082803abe11c5183f58929362251fc3baf986`；下一单元`0x004404D0`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
