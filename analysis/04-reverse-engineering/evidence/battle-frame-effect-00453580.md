# 战斗当前画面复合效果 `0x00453580`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整LST范围

权威LST函数为`0x00453580..0x004539A9`，完整508行、21个静态call站点、22个标签，无外部FUNCTION CHUNK。ABI为一参数cdecl/plain `retn`，caller清理参数。三个直接caller站点为：

- `0x00452904`与`0x00452D75`，同属已关闭战斗画面转场；
- `0x00453322`，属于已关闭战斗逐帧协调器。

三个caller均忽略返回EAX。本函数不同出口可能保留surface虚调用、软件blitter或普通状态运算残值，不把它伪造成稳定业务返回。

## 2. 入口source发布与全屏clip

入口先读取当前source token并发布到共享blitter source槽，然后固定调用clip owner：

```text
left = 0
top = 0
right = 640
bottom = 480
```

modern以typed source保存token、可写命令流、布局和u16宽高；token发布严格早于clip。所有通用blit固定使用入口source宽高、目标`(0,0)`和空palette/辅助尾，直接复用已关闭软件blitter及其正常公共后缀。indexed布局仍因固定空palette在原palette首次读取点停止，不擅自借用外部palette。

## 3. 双抑制门与零旋转路径

两个抑制dword任一非零时，完整跳过入口source绘制、旋转缓存、split带、pending rotation清零和颜色循环，直接进入公共全屏clip及阶段状态机。

两者都为零且入口rotation amount为0时：

1. 全屏绘制当前source，flags 0；
2. 直接调用已关闭`0x00451540`旋转缓存单帧绘制；
3. split抑制dword不等于1时更新u16 extent并绘制上下两条镜像带。

extent更新严格为：

```text
extent >= 192: 不变
20 <= extent < 192: extent = u16(extent + 22)
extent < 20: extent = u16(extent << 1)
```

两条clip与绘制顺序为：

```text
clip(0, wrapping_i32(192-extent), 640, 192)
blit(flags=0x28)
clip(0, 192, 640, wrapping_i32(192+extent))
blit(flags=0x28)
```

extent为0时仍执行两次零高clip和两次clipped-out调用；不现代化短路。

## 4. 非零旋转路径

入口rotation amount非零时：

- signed正值：直接调用已关闭literal图像模式3，shift为原值；
- signed负值：以低32位二补数取负，调用模式2；
- `INT_MIN`取负后bit pattern不变，closed rotation按非正shift普通返回，caller仍继续。

之后全屏flags 0绘制当前source，再直接调用已关闭`0x004515E0`旋转缓存动作播放，传入原signed rotation amount。动作初始/后续更新返回0属于原callee普通返回，外层继续；非法缓存索引、空owner、rotation/blit故障和非终止域才typed-stop。

零或非零路径完成后都把共享pending rotation dword清零。若此前source blit、source rotation或缓存callee在真实访问点typed-stop，不执行该清零。

## 5. 三通道颜色循环

只在双抑制门均为零且前述绘制完成后检查颜色循环dword。完整值等于1时：

1. 对颜色delta byte做signed扩展；
2. 把同一i32依次发布到红、绿、蓝三个共享槽；
3. 对固定target framebuffer的`0x3C000`像素依次直连红单通道、绿双lane、蓝双lane调整；
4. 三次全部正常返回后才执行delta byte加`0xFC`的u8回绕；
5. 新byte为0时写回`0x10`并清颜色循环dword，否则保持循环有效。

`0xFC`按-4传入颜色函数；不按252处理。颜色helper保留末像素dword look-ahead与绿/蓝双lane合同；modern使用framebuffer只读guard，不扩大逻辑像素范围。

## 6. 公共全屏clip与遭遇ID门

入口效果结束或被双抑制门跳过后，再次固定恢复`0,0,640,480`clip。

当前遭遇ID按i16符号扩展后，与完整expected i32比较。不同则跳过surface阶段和cadence，直接携带当前stage word进入尾部fade状态机。

ID相等后，只有`primary_suppression==1 || secondary_suppression==1`才执行surface阶段；其他非零值虽然跳过入口绘制，却不会进入此阶段。

## 7. 标准surface阶段

alternate surface mode为0时读取signed stage word：

- stage小于1：不读取surface表；
- stage不小于1：以完整signed stage作索引读取surface token，再由固定surface对象执行虚操作；effect flags固定`0x01000000`。

surface表typed-stop只发生在首次实际索引读取；此前source发布、两次clip与全部入口门副作用保留。DirectDraw对象和虚调用由窄平台端口表达。

## 8. alternate framebuffer阶段

alternate surface mode非零时：

1. rotation amount非零则再次执行source literal旋转和旋转缓存播放，再清pending rotation；
2. 全屏flags 0绘制source；
3. 读取red/green/blue三个i16 factor与signed stage；
4. 每个factor先算术右移一位，再按低32位乘stage；
5. 以红、绿、蓝顺序一次调用已关闭三通道颜色调整，固定`0x3C000`像素。

负奇数factor的右移保持x86算术右移，不使用C++向零除法。乘法保留低32位回绕。

## 9. cadence与stage上限

标准或alternate阶段完成后读取signed cadence dword：

- cadence小于等于1：只执行低32位加1；
- cadence大于1：先置0，stage word执行u16加1，再按signed i16比较；大于2才夹为2，最后cadence加1成为1。

因此stage为`0x7FFF`时加一得到`0x8000`，signed小于等于2，不会被夹为2。modern使用显式bit-pattern回绕。

## 10. fade尾状态机

只有`fade_active==1 && fade_block==0`继续；否则直接返回。

### 10.1 stage小于1

严格清零：

- red、green、blue三个factor word；
- stage word；
- 当前遭遇ID写`0xFFFF`；
- primary suppression；
- secondary suppression；
- alternate surface mode；
- fade active。

不清pending rotation、split状态、颜色循环、cadence、fade block、selected surface或surface表。

### 10.2 stage不小于1

先执行stage word减一并写回。

若selected surface完整dword不等于全1且alternate mode为0，则以减一后的stage读取surface表，effect flags固定0，执行一次虚操作后立即返回；不执行fallback source blit。

否则执行一次全屏flags 0 source blit后返回。surface表越界发生在stage已减一之后；fallback blit typed-stop也保留stage写回。

## 11. closed callee回收

21个call站点分类为：

- clip owner 4次：直接typed状态转换；
- 通用软件blitter 6次：直接typed绘制；
- 旋转缓存单帧1次、播放2次：直接复用已关闭共享cache；
- literal source循环平移2次：直接复用已关闭可写命令流旋转；
- 红/绿/蓝单通道3次、RGB三通道1次：直接typed颜色函数；
- DirectDraw虚操作2次：保留窄平台端口。

已关闭callee没有复制平行实现。函数自身不创建source、frame cache或surface owner。

## 12. caller回收

`0x00453200`在主frame stage之后、条件stage之前直接调用本函数。typed-stop保留主frame stage并阻断全部后继stage、固定帧、跨模块队列、输入与截图。

`0x004527E0`在首张640×480 raw快照转换后、任何场景准备前第一次调用；mode 0第二阶段在首个临时surface操作后、重复场景准备前第二次调用。caller从同一raw快照直接复用已关闭`0x004014F0`编码结果建立可写命令流，不用token冒充主机指针。首次调用typed-stop保留双raw分配、480行复制与转换，不执行合成cleanup。

## 13. 双向追溯

- `0x00453580..0x004535BF`：source发布、全屏clip、双抑制门；
- `0x004535C5..0x00453711`：零/正/负rotation、全图、split带、旋转缓存；
- `0x00453716..0x00453793`：pending清零、颜色循环与delta byte回绕；
- `0x00453799..0x004537D5`：公共全屏clip、遭遇ID与双stage门；
- `0x004537D5..0x0045380B`：标准staged surface；
- `0x00453810..0x004538AF`：alternate旋转、全图与RGB factor；
- `0x004538B2..0x004538E3`：cadence与signed stage clamp；
- `0x004538E5..0x00453941`：fade门、stage减一、selected surface早退；
- `0x00453942..0x00453968`：fallback全图返回；
- `0x00453969..0x004539A9`：stage小于1的精确终态清零。

C++到LST反向追溯覆盖完整508行、21个静态call站点和22个标签。

## 14. 验证与动态差分

定向测试覆盖：

- source token入口发布、全屏绘制、两条split clip与最终clip恢复；
- extent小于20双倍、20起加22及192冻结三类分支；
- color byte正值归零重载16、`0xFC`负值符号扩展与非零回绕；
- 标准surface stage、effect flags、cadence和stage上限；
- alternate RGB三个half-factor乘积；
- fade stage减一、减一后surface索引、虚调用早退和stage零精确清理；
- surface表首次读取typed-stop前缀；
- 正rotation逐行字面像素结果、空缓存播放与`INT_MIN`非正shift；
- 空source首次全图blit typed-stop；
- 逐帧caller effect时点与阻断后继stage；
- 转场caller一次/两次调用计数及首次effect缓存故障的分配、复制、转换前缀；
- battle聚合目标零warning，普通定向通过。

当前没有原版DirectDraw surface对象、三项surface表、共享source/clip/blitter状态、旋转缓存、颜色格式、遭遇ID、全部阶段word与target framebuffer联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
