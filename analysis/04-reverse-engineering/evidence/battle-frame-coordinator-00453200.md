# 战斗逐帧画面协调器 `0x00453200`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整LST范围

权威LST函数为`0x00453200..0x00453570`，完整412行、44个静态call站点、18个标签，无外部FUNCTION CHUNK。函数无参数；导航调用图中同一上层战斗状态机存在33个直接call站点。

## 2. 入口、音乐与首个早退

入口固定把活动dword写1。随后调用music gate；只有完整EAX等于1且抑制byte为0时，才以共享路径和mode 0启动音乐，再把共享runtime handle传给commit。start EAX被commit覆盖，但后续阶段不使用该值。

接着严格执行六个已关闭typed阶段：第一项组合帧鼠标输入与目标解析，第二项组合逐帧输入与指令分派，第三项组合角色预处理，第四和第五项是metric与角色顺序重建，第六项组合战斗调试快捷键总处理。第六项的E键路径返回0时立即返回EAX 0：

- 活动dword保持1；
- 不锁目标surface；
- 不执行任何绘制、输入、截图或尾阶段；
- 音乐与前五阶段副作用保留。

帧鼠标输入直接复用live鼠标、热点vector、启动party映射、角色顺序、两组数量、最终角色、输入选择和已关闭TSW像素查询；完整普通ECX/EDX直接进入输入分派，启动表、映射、角色顺序、完成槽、marker或命令流typed-stop保留音乐与帧输入前缀并阻断全部后续阶段。输入分派继续复用20条输入记录、typed DIK快照、动作工作区、prompt、对话和相同热点owner；普通返回不形成成功门，完整ECX/EDX传给角色预处理，其typed-stop保留第一阶段及自身前缀。角色预处理typed-stop保留前两阶段与自身前缀并阻断metric以后全部流程。调试快捷键复用相同DIK与共享状态；其typed-stop保留metric和顺序重建副作用并阻断surface lock。六项前置阶段已无opaque call。

## 3. 目标surface与渲染门

第六阶段非零后：

1. 以固定target surface token调用lock；
2. 把返回token发布到共享当前目标指针槽；
3. 立即以surface token与lock返回token调用unlock。

随后渲染中止dword等于1时直接返回活动dword 1。lock/unlock副作用已完成，固定帧和后续阶段均不执行。

## 4. 选择延迟与交互发布

入口读取选择mode和选择值。仅当`mode==1 && value==0xFFFFFFFF`时把mode写0。

刷新分支要求同时满足：

```text
input_source != 0xFFFFFFFF
selection_active == 0
selection_enable == 1
selection_mode == 0
```

- 16位延迟小于`0x10`：只执行word递增，保留u16回绕；
- 延迟不小于`0x10`：直接组合已关闭攻击顺序出队；它按无界28字节步长跳过角色查询精确返回1的组A记录，把首个可用或空记录完整七dword复制到共享输出，随后按原规则左移并重置尾部；
- 新值不是全1：延迟word清零、active写1、auxiliary保存新值；
- 新值仍为全1：不清延迟、不置active、不改auxiliary。

旧选择刷新callee枚举只保留reserved数值且不再调用。出队内部唯一尚未关闭角色查询转接为窄端口；子typed-stop保留lock/unlock和出队输出/记录前缀，阻断选择帧和后续全部帧。

选择值与角色metric优先索引是同一物理七dword输出记录的首dword，后六dword也收敛到同一metric owner；全局重置按原物理写集合清完整七dword。输入源复用启动状态第一条`0x1C`记录的`+0x00`，选择active与mode分别复用最终角色selection gate与frame gate。调试快捷键或后续角色阶段的同址写入会真实影响本帧后续判断，不保留旧独立副本。

交互可用dword最终严格等于：

```text
selection_value == 0xFFFFFFFF && selection_source == 0
```

## 5. 主绘制阶段与固定帧

随后固定执行：

1. 直接组合已关闭选择帧：处理完成角色替换、十类message绘制、目标轮转与角色标记；旧frame-stage槽只保留reserved数值且零调用；
2. 直接调用已关闭`0x00453580`画面效果，以其共享pending rotation作入口参数；
3. 当`conditional_mode != 1 || conditional_submode == 1`时执行条件stage；
4. 直接组合已关闭双方完成数协调：扫描组A对象双门和组B mask链，满足阈值后发布message及组门；
5. 直接组合已关闭待执行动作提交：按入口总数遍历live角色顺序，处理ready标记、角色发布和记录移除；
6. 直接调用已关闭`0x0045C010`效果总协调步进。

选择帧typed-stop保留交互可用发布及此前全部帧副作用，并阻断画面效果；画面效果typed-stop保留已完成选择帧及效果内部真实前缀，并阻断全部后继stage。双方完成数协调复用当前角色、双方数量、最终角色计数、动作phase/packed计数、结果暗化门、启动组门与message唯一owner；前一角色帧的post-call ECX/EDX由显式snapshot进入，正常尾EDX成为待执行动作零角色早退快照。其子typed-stop阻断待执行动作和后续帧，旧第一后继stage只保留reserved枚举值且不再调用。待执行动作提交复用唯一metric顺序/数量、启动ready槽、actor publication和activation latch，并直接组合攻击顺序移除；移除左移尾源与效果总协调器`intensity_records[0]`共用同一物理owner，旧pending移除端口槽只保留reserved数值且不再调用。其typed-stop保留角色帧、完成数协调及publication前缀并阻断效果协调和固定帧。旧第二后继stage只保留reserved枚举值且不再调用。效果总协调器复用主帧端口内唯一角色metric、效果步进和18槽记录状态；子typed-stop阻断固定帧，普通返回值不等于1时只对共享UI dword的低word OR 1，高16位原样保留。旧opaque完成门枚举和测试桩已删除。

然后直接调用已关闭`0x00450270`，固定资源`0x234D`、帧0、坐标`(0,384)`。frame unavailable或blitter typed-stop在原首次访问/绘制点阻断后续流程；不伪造选中角色、跨模块队列、输入或截图尾。

## 6. 选中角色面板

选择来源为0时跳过整个面板，后续低word调用使用固定帧callee后的显式陈旧ECX snapshot。

选择来源非零时，原顺序为：

1. 清零固定`0x26`个dword动作记录；
2. 写动作号`0x233B`与base variant 0；
3. 直接调用已关闭动作更新器；返回失败也继续；
4. 第一次读取角色映射表；
5. 用第一次映射完整u32作为第二次映射索引；
6. 第二次映射值作为面板left；
7. 第一次映射值只覆盖DX为动作记录`field_4a`，保留其高16位，形成九宫格资源号。

映射缺失typed-stop发生在动作记录清零、字段写和动作更新之后，保留这些前缀副作用。该面板动作记录同时是已关闭列表框所访问的同一物理owner；两个caller各按权威时序清零和更新，不建立第二份记录。

面板直接调用已关闭九宫格helper：

```text
resource = (first_mapping & 0xffff0000) | action.field_4a
left = second_mapping
top = 397
right = wrapping_i32(left + 0x74)
bottom = 467
opacity = 0
flags = 0x80000008
```

九宫格失败在typed状态阻断。

特殊面板抑制dword非零时跳过角色对象与独立动作帧，后续ECX高字取九宫格callee后snapshot。

## 7. 角色对象与独立动作帧

未抑制时，选择来源必须在`8..17`，并按已锁定角色组A基址和步长得到物理token。角色callee返回非零时跳过独立帧，后续ECX沿用该callee完整snapshot。

角色callee返回0时，首次读取选择来源对应的X/Y坐标，再直接调用已关闭`0x00450B60`：

```text
action = 0x2391
x = selected position.x
y = selected position.y
```

独立动作helper内部仍需要其动作更新callee后的ECX/EDX陈旧高字，聚合请求显式提供这两个snapshot；helper完成后，后续低word调用使用独立帧callee后的另一个ECX snapshot。三个snapshot不互相替代。

## 8. 陈旧ECX低word与跨模块直连

面板路径汇合后，LST只执行`mov cx, gameplay_word`。modern保留此前选定snapshot高16位并替换低16位，再把完整u32传给下一战斗stage。

随后四个后置战斗stage仍为后续工作包typed端口。之后立即直连已关闭跨模块helper，顺序不可交换：

1. packed-row效果链更新/绘制；
2. 角色头顶动作链更新/绘制；
3. 对话链更新/绘制；
4. 一个后续战斗stage；
5. 倒计时`(400,8,0)`；
6. 倒计时`(10,8,1)`。

packed-row、头像链和对话使用各自真实typed owner，不在battle复制平行列表。对话只有`idle/completed`继续；surface、文本或控制typed失败阻断。倒计时只接受completed、inactive和suppressed三种正常状态。

## 9. 内部bit 17与返回3

两个倒计时后调用`0x0040DC50(0x11)`。该callee不是键盘DIK查询；它读取内部bit表：

```text
byte_index = 0x11 >> 3
mask = 1 << (0x11 & 7)
return (flags[byte_index] & mask) != 0
```

modern直接对typed bit-span执行相同访问。span缺失只在byte 2真实访问点typed-stop，保留此前全部绘制和倒计时副作用。bit置位时立即返回完整EAX 3，不执行后续stage、surface尾或截图。

## 10. 后置stage与surface尾

bit未置位后：

- 独立调试叠加门dword精确等于1时直接组合已关闭战斗调试叠加层；旧opaque模型的条件方向相反，现已按LST纠正；
- 叠加层正常返回后直接组合已关闭结果判定前置流程；其双侧计数门可调用全帧暗化、暂停音频与尚未关闭的结果整理；
- 结果判定正常返回后直接组合已关闭上下文提示绘制；叠加层、结果判定或提示typed-stop保留各自前缀并阻断后续颜色与surface阶段；
- 共享颜色计数小于等于0且共享初始化门不等于1时，直接调用已关闭颜色初始化器并传入`24,24,24,0,0,0,8`，再把共享门写0；
- 固定以参数1调用finalize callee。

finalize之后直接调用已关闭三通道颜色累加：固定递减请求，按共享九float与计数执行step零门、`current += step`或`step = target`、x87向零转换，并只调整framebuffer前`0x3C000`像素。overlay门与颜色累加读取同一typed计数；颜色framebuffer失败阻断surface与截图尾。

随后：

- special surface gate任意非零且mode flags的bit`0x100`未置位：以固定selector`0x2711`解析primary surface并从target surface执行整surface虚操作；零token只在立即vtable访问点typed-stop；
- gate为0或mode bit已置位：直接组合已关闭纵向位移，以16项signed表构造两组矩形Blt，中间按固定1280字节行宽清framebuffer暴露带，并按live节拍推进phase。

## 11. 截图尾与最终返回

截图请求与调试P键复用唯一typed状态。截图请求dword等于1时：

1. 16位计数器递增并回绕；
2. 零扩展新word后加1000；
3. 格式化`c:\\snap\\%d.bmp`；
4. 直接调用已关闭16位framebuffer BMP writer，固定`640×480`；
5. 无论writer结果如何，截图请求清零。

截图编号不会携带陈旧EDX高字。测试锁定`0xFFFF -> 0 -> 1000`和精确路径。

普通尾返回活动dword1。函数只有三类汇编正常返回：首阶段门返回0、内部bit返回3、其余路径返回活动dword1。

## 12. 双向追溯

- `0x00453200..0x0045325D`：活动、音乐、双opaque stage、角色预处理、metric、顺序、完成门与零早退；
- `0x0045325E..0x00453286`：target lock/unlock、渲染门与返回1；
- `0x0045328C..0x0045331C`：选择mode、延迟刷新和交互可用发布；
- `0x0045331C..0x00453379`：选择帧直连、画面效果直连、条件阶段、角色帧顺序、双方完成数与待执行动作提交直连、效果协调、UI低word和固定帧；
- `0x0045337F..0x00453431`：选中动作记录、双映射、九宫格、角色对象和独立帧；
- `0x00453434..0x00453482`：ECX低word、四stage、三类跨模块队列、两倒计时；
- `0x00453491..0x004534A3`：调试叠加精确门、结果判定前置流程及上下文提示typed直连；
- `0x00453485..0x00453490`：内部bit与返回3；
- `0x00453491..0x00453514`：可选/固定stage、overlay、三通道颜色累加直连、整surface提交与纵向位移分支；
- `0x00453514..0x00453570`：截图word、路径、BMP写入、请求清零和活动返回。

C++到LST反向追溯覆盖完整412行、44个静态call站点和18个标签。

## 13. 验证与动态差分

定向测试覆盖：

- 音乐启动/commit、双opaque、typed角色预处理与调试快捷键顺序，以及E键第六阶段零早退；
- target lock/unlock后渲染门返回活动1；
- 选择延迟、攻击顺序出队直连、七dword共享输出、角色查询转接、旧opaque槽清零、active/auxiliary发布与交互门，以及选择帧typed-stop传播与旧frame-stage槽零调用；
- UI dword只改低word且保留高16位；
- 主frame stage后画面效果直连及其typed-stop前缀；
- 角色帧后双方完成数协调直连、post-call寄存器snapshot、组A/组B消息发布与旧第一opaque槽清零；
- 完成数协调后live数量/顺序改写、待执行动作提交直连、旧第二opaque槽清零及子typed-stop阻断效果与绘制；
- 固定帧直连、ECX高字/低word组合；
- packed-row、头像、空对话与双倒计时直连；
- 内部bit 17返回3及缺失bit表真实访问typed-stop；
- 调试叠加精确等于1门、正常组合和子typed-stop后续阻断；
- 结果判定双侧算术、全帧暗化直连、窄结果端口和子typed-stop后续阻断；
- 上下文提示300帧门、30项switch、鼠标/角色四路提示、偏移动作帧直连及子typed-stop后续阻断；
- 角色预处理工作区typed-stop阻断metric与后续帧；
- 三通道颜色初始化、共享门与尾寄存器，以及同帧颜色累加、计数递减与`0x3C000`前缀；
- 任意非零surface门、整surface零token typed-stop、纵向位移双矩形提交及子typed-stop截图阻断；
- 截图计数word回绕、路径、writer调用与请求清零；
- 面板动作更新、双映射、九宫格、角色组A token、独立动作帧和第三类ECX snapshot；
- 映射缺失发生在面板动作更新副作用之后；
- battle聚合目标零warning，普通定向通过。

当前没有原版剩余战斗callee、攻击顺序出队角色查询与无界相邻内存轨迹、共享选择/队列/对话/倒计时状态、调试叠加字体/文字/角色查询状态、结果判定计数/音频/整理状态、上下文提示计数/鼠标/动作帧状态、九float与计数、DirectDraw target surface、内部bit表、寄存器snapshot与BMP文件联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
