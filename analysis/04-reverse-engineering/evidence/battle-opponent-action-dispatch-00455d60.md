# 战斗对手动作分派 `0x00455D60`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整LST范围

权威主体为`0x00455D60..0x0045662F`，完整993行、51个静态call站点、33个`loc_`标签。函数另有外部FUNCTION CHUNK：

```text
0x00498370..0x00498384
size = 0x15 bytes
18 LST lines
1 call
loc_498370 + SEH_455D60
```

完整函数包含52个call站点。chunk在deformation构造抛异常时释放刚分配的owner，并跳MSVC异常展开。modern先取得legacy owner token，再用`std::make_unique`构造typed对象；catch释放该token并继续抛出。

唯一caller为尚未关闭的`0x004576A0`，当前不提前计数。

## 2. 入口、动作域与访问时机

第一参数按低32位建立角色组B token：

```text
0x00525508 + group_b_index * 0x2B28
```

首次对象访问即查询动作号，结果只取AX。与`0x004539B0`不同，本函数没有terminal查询，也没有主0后的fallback动作号。

动作域：

- 100：直接返回1；
- 200：对第二参数指定的组A对象调用单对象callee，返回0；
- 300：对组A对象执行结束callee，完整EAX等于1时current actor写`0xFFFF`并返回1；
- 其他大于100值：返回0；
- 1–17：进入固定稀疏jump table。

组B入口索引只在动作号查询点检查。第二参数不做入口预验；仅case 1、6、7、200和300在首次对应组A/组B对象callee处检查。动作100、未识别大动作、case 10及switch default不访问第二参数。

## 3. 17项稀疏jump table

有效case固定为9项：

```text
1, 2, 6, 7, 10, 11, 12, 15, 17
```

case 3–5、8、9、13、14、16以及动作0均走default，返回0且只执行入口动作查询。

## 4. case 1：按side mode攻击

### side mode为0

第二参数解释为组A目标。准备成功后：

- action pending写1；
- action pending auxiliary清零；
- packed action低word写目标索引；
- 调用组B source与组A target的pair commit。

blocking effect为0且视觉commit完整EAX等于1时，发布selected target、frame refresh并把framebuffer写`0xFF`。公共尾清accumulator、selection high word、selection word，current actor写`0xFFFF`，对source设置延迟300并返回0。

### side mode非0

第二参数解释为组B目标。准备后同样写pending与packed低word，但明确不调用pair commit。视觉commit参数只保留accumulator，另两项固定0。成功时先写映射表目标槽与selected target，再清framebuffer。

公共尾只清accumulator和current actor，故selection两个word保持陈旧值；不设置延迟，返回1。

## 5. case 2：选择与deformation生命周期

首次进入由action runtime低wordbit15控制：

- 置bit15；
- 保存选择snapshot；
- 18项target identity填全1；
- input mode低word写1；
- 18项selection workspace清零；
- 特殊查询完整EAX等于1时置deformation active并申请44字节owner；
- owner非零才构造`640×480, origin 0,0, field 200×200`typed deformation。

runtime低wordbit0未置位时返回0。置位后清accumulator与两个selection word；deformation active为1且对象非空时按析构→owner释放顺序清理，随后active清零、fade active写1并返回1。

原函数没有角色组计数夹值，也没有上一分派case 2的battle flag早退。

## 6. case 6与7

### case 6

phase低word为0时：

- 以组B source、目标索引和组A target启动目标phase；
- 对组A target依序发布mode 1和clear mode 1；
- 发布stage `target+8`；
- 三个颜色factor写-12；
- primary suppression写1；
- effect stage以参数0启动；
- phase低word与input mode低word写1。

随后每帧查询目标phase完成。完成时input mode保持1，phase与suppression清零，current actor写`0xFFFF`，fade active写1并返回1；未完成返回0。

### case 7

组A target完成后，先对target设置延迟0，再发布stage `target+4`。目标索引位于0..3时只递增packed actor最低byte，保留高24位并允许`0xFF→0x00`回绕。两个共享active target恰等于该stage时分别清0或写全1并清辅助门。返回1。

## 7. case 10–12

- case 10：对组B source调用固定参数0，返回0；
- case 11：查询成功后依序push `0x1000`、clear mode 0、finalize mode 8，overlay gate写1，返回1；
- case 12：查询成功后依序clear mode 1、pop `0x1000`、finalize mode 8，overlay gate写1。processed低byte按signed比较不小于group-B count时，scene gate清零、message state写99。返回1。

case 11的查询站点同时保留ECX source与栈上的source token；typed端口显式记录两份。

## 8. case 15：对手wave记录生成

phase低wordbit15未置位时：

1. phase低word写`0x8019`，保留高word；
2. 调用初始化callee，读取两个独立u16输出：special action与spawn count；
3. 对每个spawn：
   - 以当前group-B count选择新组B对象；
   - reset该对象；
   - 清零164字节scratch；
   - 写32字节记录中的action、x=240、y与runtime；
   - 第2项y=350，其余y=220；
   - mirror门等于1时调用mirror并以u16写`640-x`；
   - scratch准备参数低word覆盖为special action，高word保留`32*index`陈旧位；
   - scratch更新返回后只覆盖EAX低word为special action，保留callee EAX高word；
   - commit到固定记录token；
   - 对新对象pop `0x400`；
   - group-B count低32位加1；
   - 固定调用三个battle stage。

循环没有现代上限。第9项只在首次新组B对象访问处typed-stop，前8条记录、计数及全部callee副作用保留。

初始化后及后续调用均先对phase低word减1，再测试低word低15位。非零返回0；归零时current actor写`0xFFFF`、phase低word与spawn count清零并返回1。若已初始化位形错误地传入低word0，原行为是回绕到`0xFFFF`，不夹到完成。

## 9. case 17：对手阶段完成

查询成功后固定执行clear mode 1、finalize mode 8、target mode 1并写overlay gate。processed counter只递增最低byte，保留高24位。

processed低byte按signed比较不小于group-B count时：

- 504字节workspace全部清零；
- action pending auxiliary写1；
- scene gate清零；
- message state写99；
- 从workspace首地址开始每28字节写一个全1dword，共18项。

special action非零时继续查询固定组B base。返回0且`group-B count - processed == 1`时：

- group-B count写1；
- processed最低byte写callee AL，即0；
- special action清零；
- post battle counter清零；
- 再调用三个battle stage。

返回1。

## 10. closed callee与窄端口

直接复用已关闭：

- deformation typed构造与析构；
- framebuffer前缀清屏。

其余35个唯一角色、AI、记录、模式与stage callee保持单一typed token端口。端口中的物理地址、scratch token与记录token均为`compat::u32`，不转主机指针。端口reply只在callee实际可写的共享槽发布accumulator、selection word、special action与spawn count。

## 11. typed故障点

- 入口组B：首次动作号查询；
- 按case解释的组A/组B target：首次目标对象callee；
- case 1映射：视觉commit成功后的首次映射元素写；
- case 15新对手：每次wave的首次组B对象reset；
- framebuffer：先发布refresh并写满owned前缀，再在首个越界像素停止。

不对动作号、spawn count、phase、processed byte、group-B count或callee返回增加现代合理化。

## 12. 验证与动态差分

定向测试覆盖：

- 入口组B越界；
- 动作100与未识别大动作的target延迟访问；
- 动作200/300及200的首次组A target停点；
- case 1双side的不同pair、返回值、selection清理和延迟；
- case 2 deformation分配、构造、析构与owner释放；
- case 6完整初始化/完成；
- case 7低byte回绕与两个active target清理；
- case 15双wave镜像、220/350坐标、callee EAX高word、三个stage、完成位形；
- case 15第9项在8条完整副作用后typed-stop；
- case 17 workspace零后18个间隔全1头与special collapse；
- 9个有效case逐项smoke；
- 动作0与8个稀疏槽只执行入口callee；
- framebuffer越界前refresh与完整owned前缀；
- 37个唯一原callee中35个端口边界全部存在，另2个deformation callee直接复用。

当前缺少原版组A/B对象、37类callee共享副作用、wave scratch与记录、AI表、DirectDraw framebuffer、allocator和SEH联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
