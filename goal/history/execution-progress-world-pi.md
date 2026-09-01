# 执行历史：世界、B7与剧情前置

状态：冻结历史；不得作为当前执行状态或行为真值。

来源：重构前`execution-plan-pi.md` v853第631..1368行，世界/B7及剧情前置流水。

完整性与当前资料入口见[`../execution-history-index-pi.md`](../execution-history-index-pi.md)。当前状态见[`../execution-state-pi.md`](../execution-state-pi.md)。

---


    新游戏 `TALK100` 到第一句有声对白的有限剧情闭包现已接入普通世界帧：TALK1..4
    定位/窗口读取、持久剧情位、角色定位与隐藏、画面清除、等待、图片动作、颜色过渡、
    镜头平移、片头视频端口、标题及对话消息生产均复用现有 owner。解释器只承诺该路径
    实际可达的 opcode 集，未恢复分支不移动指令指针并明确返回延期状态。全函数第二次
    LST 核对纠正了 opcode 120 第三个动作参数必须零扩展、前两个参数必须符号扩展的
    边界；真实 `TALK100` 回归逐个越过片头、三段等待、两幅图片、镜头完成门、标题关闭
    门并抵达第一句对白。Linux/Windows LLVM 完整应用均为 180/180 CTest 通过，Windows
    EXE 成功链接；未启动任何原版或重写版 EXE。

    第一句对白后的下一有限剧情切片已接通 opcode `10/11/20/39/94/95`，
    并恢复 `sub_42DAF0/sub_42E280/sub_42D920` 三个剧情角色路径 owner。角色动作链、
    两阶段路径请求/完成查询、72 槽路径消费、空闲帧完成回调和场景标志门已进入
    同一生产路径。六个 opcode 和三个 owner 均在实现后再次逐出口核对 LST；
    复核纠正了离屏预推进每个方向字节应移动 16 像素，以及后继动作链必须比较
    未遮罩的 16 位 opcode 原值。真实 `TALK100` 回归现可连续通过十个对话、两段
    真实 A* 剧情路径、等待/动作/场景门并抵达下一句对白。Linux 与 Windows LLVM
    完整应用均为 180/180 CTest 通过，Windows EXE 成功链接；未启动任何原版或
    重写版 EXE。下一实现边界为继续真实剧情时首个实际命中的未恢复 opcode；
    不因数值连续性扩展为全量 opcode 研究。

    后续真实 `TALK100` 切片已恢复 opcode `76` 与其调用的
    `sub_42E5A0` 角色挂起/路径状态 owner：两角色按动作偏移计算世界中心，
    首角色面向次角色后清理动作差量与等待，再保存 72 槽路径光标并对齐格点。
    opcode 处理器与 owner 均在实现后再次从入口到全部出口核对 LST，未发现
    实现与汇编的逻辑差异。真实回归继续通过五个对话，在窗口指令指针
    `1369` 处精确停于下一个未恢复 opcode `71`，该指令保持未消费。
    Linux/Windows LLVM 完整应用均为 180/180 CTest 通过，Windows EXE
    成功链接；未启动任何原版或重写版 EXE。下一实现边界为 opcode `71`。

    下一段真实 `TALK100` 切片已恢复 opcode `71/72/77/78`：前两项按原地址
    `0x004B9F68 + slot * 0x98` 绑定或清除八槽 HeadSgn 动作记录，后两项按
    `FFF0` 当前来源替换规则设置或清除角色动作等待覆盖并立即刷新动作。SDL 角色绘制
    现通过原地址 token 解析实际 HeadSgn owner，不复制动作记录。四个处理器均在实现后
    再次从入口、查找失败分支到公共循环尾完整核对 LST，未发现逻辑差异；opcode 77/78
    查找失败时原版使用未初始化栈宽度，重写保持指令未消费并显式报告角色缺失，不伪造
    固定步长。真实回归继续通过 23 段对白、GUID 250/251 的 46 帧双角色路径、两次
    HeadSgn 生命周期及 GUID 248/249 重定位，在窗口指令指针 `2949` 处精确停于下一个
    未恢复 opcode `45`。Linux/Windows LLVM 完整应用均为 180/180 CTest 通过，
    Windows EXE 成功链接且未启动任何原版或重写版 EXE。下一实现边界为 opcode `45`。

    下一段真实 `TALK100` 切片已恢复 opcode `45` 及其调用的
    `sub_42E740/sub_40D460` 角色动作与 MAPS 源记录更新链：`FFF0` 只在处理器入口
    替换为当前来源，命中运行时角色后写入零扩展动作号并置位 `0x1000`，仅当紧随其后的
    原始 opcode `10/11/45` 解析为同一角色时跳过立即刷新；未命中时则按原哨兵字段规则
    回写 MAPS 角色源记录。处理器、两个 helper 和所有出口在实现后再次完整核对 LST，
    未发现逻辑差异。真实 MAPS 回归补齐角色源原始标志后，`TALK100` 已通过第四段
    40 帧路径，并继续越过六次等待、六段对白和 29 帧路径，在指令指针 `3585` 精确停于
    未恢复 opcode `107`。Linux `core` 为 175/175，Linux/Windows LLVM 完整应用均为
    180/180 CTest 通过；Windows EXE 成功链接且未启动任何原版或重写版 EXE。
    下一实现边界为 opcode `107`。

    opcode `107` 已按 `0x0042B50F..0x0042B5ED` 恢复角色 action 进度等待：
    `FFF0` 替换为当前来源；角色存在时先以 packed AP 状态低字节验证阈值上限，阈值
    合法时再等待高字节的一基当前下标达到阈值；非法阈值或角色不存在均诊断后消费，
    不进入等待。实现后再次逐分支核对 LST，未发现逻辑差异。定向测试覆盖等待、完成、
    非法阈值和角色缺失四条路径；真实 `TALK100` 回归已消费该指令，并在指令指针
    `3591` 精确停于未恢复 opcode `59`。Linux `core` 为 175/175，Linux/Windows LLVM
    完整应用均为 180/180 CTest 通过；未启动任何原版或重写版 EXE。下一实现边界为
    opcode `59`。

    opcode `59` 已按 `0x0042967B..0x0042968E` 及公共尾
    `0x0042C7E6..0x0042C7F6` 恢复音效播放：读取四字节指令中的 `u16` 音效号，使用
    当前音效档位调用样本播放链，指令指针前进四字节并立即让出本帧。SDL 端口已接入
    实际 `SND` 样本管理器；初始音效档位 `6` 由 `sub_425040` 的
    `0x00425048/0x0042505D` 恢复。处理器和调用参数在实现后再次完整核对 LST，未发现
    逻辑差异。定向测试验证音效号、步长和让出合同；真实 `TALK100` 回归请求音效
    `0x73`，继续执行后续 opcode `52`，并在指令指针 `3611` 精确停于未恢复 opcode
    `53`。Linux `core` 为 175/175，Linux/Windows LLVM 完整应用均为 180/180 CTest
    通过；Windows EXE 成功链接且未启动任何原版或重写版 EXE。下一实现边界为
    opcode `53`。

    opcode `53/74` 已按三分量插值协议恢复：`53` 对
    `0x0042949D..0x004294B6` 的有符号倒计时执行原地等待，只有 `<= 0` 才消费两字节
    并同帧继续；`74` 对 `0x00429D43..0x00429D6B` 只清零三个浮点增量和倒计时，保留
    当前值及目标值，再经 `0x00427E84` 消费并继续。两个处理器均在实现后再次逐分支
    核对 LST，未发现逻辑差异。定向测试覆盖正倒计时等待、零倒计时通过以及取消时的
    精确字段保留；真实 `TALK100` 回归通过 `sub_4146F0` 模拟实际跨帧更新后，继续越过
    opcode `74`、第七次等待、后续角色动作及 32 帧路径，在指令指针 `3641` 精确停于
    未恢复 opcode `18`。Linux `core` 为 175/175，Linux/Windows LLVM 完整应用均为
    180/180 CTest 通过；Windows EXE 成功链接且未启动任何原版或重写版 EXE。下一实现
    边界为 opcode `18`。

    opcode `18` 已按 `0x0042845A..0x004284BD` 恢复角色路径完成轮询：原始选择值
    不执行 `FFF0` 替换，直接沿既有角色查找规则解析（包含 `FFFE` 当前选中角色），
    并调用已恢复的 `sub_42D920` owner。helper 返回非零时消费四字节并继续；返回零时
    仍清除角色标志 bit 31 与动作等待字段，但不消费指令，因此在同一次解释器调用中
    重试。该旧实现 checkpoint 未构成 P2 closure；后续独立审计发现它漏发公共 join 的
    previous18，并过早要求完整 path runtime，本轮第十四组已按 LST 修正。
    旧定向测试覆盖匹配路径槽完成及零返回同调用重试；真实 `TALK100` 回归越过该指令和
    第八次等待，在指令指针 `3675` 精确停于未恢复 opcode `58`。Linux `core` 为
    175/175，Linux/Windows LLVM 完整应用均为 180/180 CTest 通过；Windows EXE 成功
    链接且未启动任何原版或重写版 EXE。下一实现边界为 opcode `58`。

    后续真实 `TALK100` 闭包已恢复 opcode `58/104/88`。opcode `58` 按
    `0x0042B1F1..0x0042B282` 分配并初始化主图片动作节点、前插主链并立即让出；
    opcode `104` 按 `0x0042B47E..0x0042B4B4` 清文字控制 bit 28、符号扩展两项布局值，
    并在下一次文字 action 创建时以该值替代第二组角色方向偏移，随后恢复文字全局默认
    状态；opcode `88` 按 `0x0042A727..0x0042A751` 只清 packed-row 与角色头像两条链，
    将符号扩展的战斗号与 `0x80000000` 合并后写入主帧 battle request 并让出。三个
    处理器及其调用状态在实现后均再次逐分支核对 LST，未发现逻辑差异。定向测试覆盖
    `0xA4` 节点初始化/前插、文字布局的负值与复位，以及两链清理、负战斗号符号扩展和
    请求时序。真实 `TALK100` 继续通过两段对白、第二个音效和 33 帧角色路径，最终提交
    battle id `98` 的 `0x80000062` 请求；测试在该帧停止剧情推进，等待现有主帧战斗
    消费链处理。Linux `core` 为 175/175，Linux/Windows LLVM 完整应用均为 180/180
    CTest 通过；Windows EXE 成功链接且未启动任何原版或重写版 EXE。下一边界由战斗
    请求消费链决定，不越过该帧臆测后续剧情。

    战斗请求消费链的首个资源切片已建立独立 `battle` 模块，并接入 SDL 战斗初始化
    入口。`sub_46E0B0` 的有效资源路径按 `0x200 + battle_id * 4` 读取 FIGTALK 相对
    偏移，再从 `0x200 + offset` 零填充并读取固定 `0x8000` 字节窗口；
    `sub_45F130/sub_45F1B0` 按原调用顺序两次打开并读取 battle.ffd 的 `0x2714`
    字节头，以文件偏移 `0x1F44` 的有符号计数表累计记录下标，再按
    `0x2714 + ordinal * 0x10C` 读取固定战斗记录。三个函数实现后均再次从入口到全部
    出口独立核对 LST，确认有效游戏数据路径与汇编一致；平台文件适配层只对缺失、短读
    和越界输入给出显式失败，不复制原 Win32 无效句柄误判和持久句柄存储缺陷。真实
    battle id `98` 回归解析出 FIGTALK 相对偏移 `0x1914`、记录下标 `97`、ordinal
    `32` 和敌方数量 `1`。Linux/Windows LLVM 完整应用均为 181/181 CTest 通过；
    Windows EXE 成功链接且未启动任何原版或重写版 EXE。下一有限边界为继续恢复
    `sub_451B10` 的状态重置、队伍、背景及敌我对象建立，再进入战斗脚本 VM 首个实际
    命中的未恢复 opcode。

    `sub_451B10` 的下一有限切片已恢复初始队伍选择、四套固定玩家阵型、派生锚点及
    敌方记录布局。内部位 `30..33` 只在查询结果恰好等于一时按原槽顺序紧凑化，四名
    角色资源号固定为 `1/2/8/17`；一至四人的固定屏幕坐标、围绕 640 的显示镜像和
    围绕 624 的锚点镜像均已进入独立 battle setup 状态。敌方资源号、状态位和 X/Y
    分别按 battle.ffd 记录 `+0x9C/+0xBC/+0xCC/+0xEC` 的原步长读取。实现后再次从
    `0x00451D06..0x00451D88` 与 `0x00451F49..0x00452277` 的切片入口核对至全部
    分支出口；第二次复核据 `cmp eax,1` 修正了“任意非零都入队”的过宽判断。真实
    battle id `98` 回归得到一名初始玩家及敌方 `resource=400,x=175,y=303`。
    Linux/Windows LLVM 完整应用均为 182/182 CTest 通过；Windows EXE 成功链接且
    未启动任何原版或重写版 EXE。下一有限边界为战斗背景资源及敌我对象构造，不把
    当前布局状态伪报为可显示战斗画面。

    新游戏进入剧情后一秒退出的运行时回归已修正：`sub_40C020` 在 GUID 相等后对
    角色标志 bit 28 执行 `test`，分支为零时才返回当前索引；此前实现误写成 bit 28
    非零才命中，导致 `TALK100` 的来源角色无法解析并以 `role_not_found` 终止世界
    会话。实现、所有依赖测试和证据说明已统一到“跳过 bit 28 非零记录，返回首个清零
    记录”；`sub_40C020/sub_40C100` 已再次逐出口核对 LST。Linux `core` 177/177、
    Windows LLVM 完整应用 182/182 CTest 通过；Windows EXE 成功链接且未启动。

    上述 GUID 修正后的实机日志证明剧情继续推进至 opcode `120`，随后因目标角色不在
    当前运行时角色表而退出。按 `0x0042BAB8..0x0042BC27` 重新收敛该处理器：目标角色
    存在时分别对 action/base 作 `i16` 符号扩展、对 variant 作 `u16` 零扩展，`FFFF`
    字段保持原值，再清动作等待、刷新 action 并置位 `0x1000`；目标角色不存在时不报错，
    而是以三个原始 `u16` 操作数及 `flags_or=0x1000` 调用 `sub_40D460` 对应的 MAPS
    source patch，并同样消费十字节继续执行。三类分支测试全部通过。世界剧情致命日志
    同时增加 TALK 窗口文件/偏移、指令 IP、原始指令字、首操作数、已执行指令数、来源
    GUID、受控角色和角色总数，后续若仍退出可直接定位下一条差异。Linux `core`
    177/177、Windows LLVM 完整应用 182/182 CTest 通过；Windows EXE 成功链接且未
    启动任何游戏 EXE。随后新增独立真实初始世界回归，不再使用人工补齐的剧情角色表：
    它确认首图运行时 33 个角色不含 GUID `123/240`，并精确验证 `TALK100` 在视频边界
    前生成 `123,561,8,0` 与 `240,561,0,1` 两次 MAPS source patch 后继续执行。该回归
    已独立注册为 CTest，避免同类缺失角色分支再次只在实机暴露。Linux `core` 178/178、
    Windows LLVM 完整应用 183/183 CTest 通过；等待实机复测新 EXE。

    B7 选择热点与 MAPS 四记录 helper 随后完成闭环：`sub_40DB40/sub_40DB60` 恢复
    有序热点计数、严格开区间首命中、命中索引及完整 miss 返回；`sub_40DBC0` 确认
    `0x004C8BE8..0x004C8BF8` 是五双字 sentinel 节点，现代 RAII owner 释放所有消息
    热点而不误清无关对话状态；`sub_40DD60` 按源/目标 `0x34/0x38` 字节布局逐字段
    恢复，并保留目标尾部两字节。当前 `MAPS.DAT +0x18 = 0x185A`，四条物化记录哈希
    已固定。四项均完成 LST→C++、C++→LST 双向追溯和汇编独立 UT；114 项当前关闭
    62 项，剩余 52 项。

    B7 三个相邻短 helper 随后完成闭环：`sub_40DC30` 恢复事件链完整 32 位 id 比较、
    头到尾首命中和空/miss 返回；`sub_40DD10` 恢复移动步长完整 dword 写回及原值返回，
    并从调用点补回 `sub_40E0B0` 首帧前初始化值 `0x10`；`sub_40DD40` 恢复
    135 个全一 dword 的 `0x21C` 对象槽重置及 `0xFFFFFFFF` 返回。三项均完成
    LST→C++、C++→LST、所有调用点反向追溯和独立 UT；对象槽的所有现代清空点已统一
    复用同一 helper。114 项当前关闭 65 项，剩余 49 项。

    B7 世界朝向链随后完成闭环：`sub_40E030` 恢复从当前受控角色的 `world` 坐标与
    内嵌 action `+0x2C/+0x30` 范围字段计算中心，再经 `sub_411E20` 的 32 位回绕距离、
    五度正弦查表和角度量化以及 `sub_411F00` 的 16 扇区表返回八方向。原版四个包装层
    调用点、两个额外直接算法调用点与现代碰撞/面向/鼠标/VM 路径已双向逐项对应；独立
    UT 固定了角色前部同名字段不得误用、坐标回绕和完整方向量化。114 项当前关闭
    68 项，剩余 46 项。

    B7 全局初始化 `sub_40E0B0` 随后完成有限闭环：逐段恢复剧情位、变量 0、延迟地图、
    stream 状态、移动步长与世界动作记录的初始化合同，并将 battle 的四条 `0x60`
    队员扩展状态、audio 后端状态及 item/persistence 链表明确转交其实际 owner。
    双向复核纠正了整体清空 VM 状态、stream 默认值、对话框八条动作被提前更新以及
    新游戏载图后才重建长期 owner 四项顺序/范围错误；重复初始化 UT 固定汇编未写字段
    必须保留。114 项当前关闭 69 项，剩余 45 项。

    B7 地图装载进度界面 `sub_40ED60` 随后完成闭环：`-1` 重置的 RNG 与选择性动作初始化、
    flag 70 首次装载抑制、32 位回绕的 `progress * 394 / 100`、包含终列的 30 像素渐变、
    背景与标记动作、两次音频维护及唯一画面提交均已完成 LST→C++ 与 C++→LST 反复逐块
    收敛。新游戏只在原初始化和完成时点接入 `-1/100`；`sub_425BE0/sub_426DF0` 持有的
    可见中间进度不在 SDL 层伪造。DirectDraw 裸表面改由受控 framebuffer 和统一
    presentation 端口承担，异常写入范围归入平台隔离。Linux `core` 182/182、Windows
    LLVM `app` 187/187 CTest 通过，未启动任何 EXE。114 项当前关闭 70 项，剩余 44 项。

    B7 地图名称查询 `sub_40EFD0` 随后完成闭环：MAPS payload `+0x50` 相对目录、
    16 位键对完整 32 位逻辑地图参数的比较、未对齐 `%Q` 标记逐字节扫描、空名称与
    NUL 结尾复制、首个 `0xFFFF` 终止检查均已完成 LST→C++、C++→LST 反复逐基本块
    收敛。两个直接调用点已反查；地图会话恢复当前地图名及原版 CP950“`不知道`”回退，
    特殊模式列表消费留给其实际 owner。当前 `MAPS.DAT` 名称目录含 346 个唯一键，键
    81 的十字节“`威尼斯酒屋`”已由真实数据测试固定；越界目录、缺失终止标记及目标
    容量不足只作为现代平台隔离。114 项当前关闭 71 项，剩余 43 项。

    B7 世界角色表生命周期重置 `sub_40F3B0` 随后完成闭环：任意负最高索引只跳过释放，
    非负索引按包含端检查角色 `+0x38` 并调用释放，随后无条件清零完整
    `256 * 0xD8` 物理表，再对全部 256 个 `+0x40` 动作子记录调用 `sub_40DC00`。
    双向收敛期间纠正了 `vector::clear()` 保留容量而不等价于原 `free` 的差异，现代
    owner 现真正释放非零标记对应的载荷；固定物理尾部、越界损坏状态和进程关闭手工释放
    分别由受检 span、显式失败及 RAII 隔离。两处原调用者均已反查，完整 256 项最终状态、
    负值分支、包含端和三种现代边界由独立 UT 固定。Linux `core` 183/183、Windows
    LLVM `app` 188/188 CTest 通过，Windows EXE 成功链接且未启动。114 项当前关闭
    72 项，剩余 42 项。

    B7 世界物品链关闭 `sub_40F410` 随后完成闭环：玩家普通库存首、四个必需队伍哨兵
    根和 64 个逐槽可空角色物品根按原三段次序处理；每条链均从头推进，每个普通节点和
    哨兵均先释放 `+0xAC` 说明再释放本体，根最终写空。现代实现保留四个 16 位字段和
    `0xA0` 字节定义快照，以 `std::list/vector/optional` 隔离裸指针；缺少四个必需根的
    原崩溃域在任何写入前事务式拒绝。总关闭的 `release_0040f410` 槽已接入真实 owner，
    Linux `core` 184/184、Linux `app` 189/189、Windows LLVM `app` 189/189 CTest
    通过，两端应用均成功链接且未启动。114 项当前关闭 73 项，剩余 41 项。

    B7 packed-row 效果链关闭 `sub_40F500` 随后完成闭环：`dword_4BAB9C` 空首直接
    返回；非空时每轮先推进全局首，再严格按 `+0x0C` 行偏移数组、`+0x10` 行长度数组、
    `0x18` 字节节点本体的顺序释放。现代既有 `LegacyPackedRowEffect` 六个 16 位字段、
    两个 vector 和 list 链已由显式 helper 接管，确保非零容量真实归还并保留逐头销毁
    次序；反向复核发现直接 `pop_front()` 会晚于数组释放推进链首，现以先 `splice`
    摘头再释放修正；调用点反查发现 `sub_40A570` 对应主过渡端口仍为空，也已转发到
    真实 owner。opcode 88、世界 owner 重建及 SDL `release_0040f500` 均接入同一合同。
    五个原调用点和无返回值消费已反查，定向 UT 覆盖空链、三节点、空数组及两种数组
    所有权组合。Linux `core` 184/184、Linux `app` 189/189、Windows LLVM `app`
    189/189 CTest 通过，两端应用均成功链接且未启动。114 项当前关闭 74 项，剩余
    40 项。

    B7 世界移动 action 链关闭 `sub_40F540` 随后完成闭环：空的 `dword_4AD3E8`
    直接返回；非空时每轮读取节点 `+0xB0`、先推进全局首、再释放整个 `0xB4` 节点。
    既有 `LegacyMovingActionNode` 已用静态断言固定完整 action、坐标、float 运动字段和
    旧 next 槽；显式 helper 以先 `splice` 摘头再 `pop_front()` 保留销毁顺序，不把
    容器整体 `clear()` 当作汇编证明。四个无参数调用点及无 EAX 消费均已反查，世界
    owner 重建、主过渡和 SDL `release_0040f540` 已接入实际跨帧链；定向 UT 覆盖
    三节点、最终空根和入口空链。Linux `core` 184/184、Linux `app` 189/189、Windows
    LLVM `app` 189/189 CTest 通过，两端应用均成功链接且未启动。114 项当前关闭
    75 项，剩余 39 项。

    B7 角色头顶 action 链关闭 `sub_40F570` 随后完成闭环：空的 `dword_4BA6E0`
    直接返回；非空时每轮读取节点 `+0xB0`、先推进全局首、再释放整个 `0xB4` 节点。
    既有 `LegacyRoleHeadActionNode` 已用静态断言固定完整 action、四个运动 word、保留段
    和旧 next 槽；显式 helper 以先 `splice` 摘头再 `pop_front()` 保留销毁顺序，不把
    容器整体 `clear()` 当作汇编证明。五个无参数调用点及无 EAX 消费均已反查，世界
    owner 重建、主过渡、opcode 88 和 SDL `release_0040f570` 已接入实际跨帧链；定向
    UT 覆盖三节点、最终空根和入口空链。Linux `core` 184/184、Linux `app` 189/189、
    Windows LLVM `app` 189/189 CTest 通过，两端应用均成功链接且未启动。114 项当前
    关闭 76 项，剩余 38 项。

    B7 对话消息链关闭 `sub_40F5A0` 随后完成闭环：`dword_4ACF48` 空首仍执行最终
    掩码；非空时每轮读取节点 `+0x48`、先推进全局首、再释放 `+0x38` 文本分配和
    整个 `0x4C` 节点。现代 helper 以先 `splice` 摘头、显式归还 text vector、再
    `pop_front()` 保留销毁顺序；caption 随节点由 RAII 归还是所有权适配，不误记为
    汇编显式释放。`dword_4A9920` 按完整 32 位值只保留 bit 15，其他对话字段保持不变。
    四个无参数调用点及无 EAX 消费均已反查，世界 owner 重建、主过渡和 SDL
    `release_0040f5a0` 已接入实际 owner；定向 UT 覆盖三节点、空链、掩码和无关字段
    保持。Linux `core` 184/184、Linux `app` 189/189、Windows LLVM `app` 189/189
    CTest 通过，两端应用均成功链接且未启动。114 项当前关闭 77 项，剩余 37 项。

    B7 picture-action 双链关闭 `sub_40F5E0` 随后完成闭环：先逐头处理
    `dword_4B7C70`，再无条件转入 `dword_4B8968`；两段均读取节点 `+0xA0`、先推进
    对应全局首、再释放完整 `0xA4` 节点。既有物理节点已固定 `+0x08` action 和
    `+0xA0` next，opcode 58/153 分别前插两个实际 owner；显式 helper 以先 `splice`
    摘头再 `pop_front()` 保留销毁顺序和双链先后，不再整体替换 owner。四个无参数
    调用点及无 EAX 消费均已反查，世界 owner 重建、主过渡和 SDL
    `release_0040f5e0` 已接入同一合同；定向 UT 覆盖 primary 两节点、secondary 一节点、
    两个最终空根和重复空链。Linux `core` 184/184、Linux `app` 189/189、Windows LLVM
    `app` 189/189 CTest 通过，两端应用均成功链接且未启动。114 项当前关闭 78 项，
    剩余 36 项。

    B7 角色粒子发射器四链关闭 `sub_40F630` 随后完成函数级闭环：严格按四个连续
    `0x10` 槽递增处理，每槽读取节点 `+0x00`、先推进该槽链首、再释放完整 `0x10`
    节点，最后以 16 个 dword 无条件清零完整 `0x40` emitter 状态。既有
    `LegacyAniRoleParticleEffect` 改为显式逐槽逐链 release；token 池在链处理后真正
    归还 backing storage，不再由 `vector::clear()` 保留容量。三个无参数调用点及无
    EAX 消费均已反查，SDL 总关闭和 world owner 重建已绑定实际 effect；
    `sub_40C130/sub_411D00` 的完整外围调用次序以及普通角色绘制端口尚未接入
    `sub_415EE0` 的缺口继续分别归入其外层 B7 行，不在本 helper 中伪造完成。定向 UT
    覆盖四槽 `2/1/0/1` 节点、完整字段清零和重复空 reset。114 项当前关闭 79 项，
    剩余 35 项。Linux `core` 184/184、Linux `app` 189/189、Windows LLVM `app`
    189/189 CTest 通过，两端应用均成功链接且未启动。

    B7 ANI 四槽漂移位置重置 `sub_40F670` 随后完成函数级闭环：严格按
    `0x004B86F8/0x004B8708/0x004B8718/0x004B8728` 的物理顺序，只把四个
    `0x10` 槽的首 dword 写为 `0x7FFFFFFF`，保留各槽 `y`、两个速度字段和四个动作
    记录。三个无参数调用点均不消费返回时的 EAX；既有
    `LegacyAniDriftEffect::reset_positions()` 经双向反查零差异，总关闭现已绑定实际
    drift owner，世界 owner 重建继续复用同一合同。`sub_40C130/sub_411D00` 的完整
    外围次序仍归入其各自 B7 行。定向 UT 污染并核对全部四槽字段；Linux `core`
    184/184、Linux `app` 189/189、Windows LLVM `app` 189/189 CTest 通过，两端应用
    均成功链接且未启动。114 项当前关闭 80 项，剩余 34 项。

    B7 framebuffer 变形节点链清空 `sub_40F690` 随后完成函数级闭环：确认
    `dword_4AC9B8` 是静态 `0x2C` 哨兵 `0x004AC990` 自身的 `+0x28` next，而非独立
    链头；非空时严格先把当前节点 next 写回哨兵，再依次释放节点 `+0x20/+0x24`
    缓冲区和节点本体，重读 head 后逐头继续，空链直接返回。既有
    `LegacyDeformationList::clear()` 的逐轮 `unique_ptr` owner 转移经双向反查零差异，
    保留哨兵及其自有缓冲区，世界 owner 重建已接入同一合同；唯一调用者
    `sub_40C130` 的完整外围次序仍归入其 B7 行。定向 UT 覆盖三节点、重复空清理和
    哨兵复用；Linux `core` 184/184、Linux `app` 189/189、Windows LLVM `app`
    189/189 CTest 通过，两端应用均成功链接且未启动。114 项当前关闭 81 项，剩余
    33 项。

    B7 世界角色过渡清理 `sub_40F6D0` 随后完成函数级闭环：唯一调用者
    `sub_40A570` 从角色 1 到 `count-1` 逐项调用，并在每次返回后清零角色 `+0x26`；
    入口 bit 31 早退、固定 72 个 `0x21C` 对象槽、16 位角色匹配、低四位对齐、受控角色
    跳过方向读取、两张八项步长表、`0xBBFFFFFF` 掩码、路径完成、bit 11 动作覆盖恢复、
    `sub_4321E0` 前后两次 one-shot 清理和最终 bit 31/wait 清零均已逐基本块双向收敛。
    SDL 主过渡不再使用空端口，已绑定实际角色、对象槽、表面、剧情路径和动作 owner；
    定向 UT 另以越界 cursor 证明受控角色分支完全不读方向，并覆盖 helper 失败仍执行全部
    最终写入。Linux `core` 184/184、Linux `app` 189/189、Windows LLVM `app`
    189/189 CTest 通过，两端应用均成功链接且未启动。114 项当前关闭 82 项，剩余
    32 项。

    B7 角色空间行链插入 `sub_411490` 随后完成函数级闭环：物理
    `role* + group` ABI、带符号 Y 除 16 向零截断、三个组的 20 行前缀、空链、单节点
    Y/GUID 双条件、多节点首插、三条件中插及尾插均已逐基本块双向收敛；遍历时故意不
    比较 next 节点 Y 的非常规行为保持不变。九个直接调用点均已反查，全部显式传入同一
    角色 `flags & 3`，且无一消费返回 EAX；C++ 不再把独立 group 参数隐含在 helper
    内部。裸指针链以一基索引和边界失败隔离，正常域排序和写入不变；定向 UT 另固定
    每条插入分支、负一行、显式组参数及现代越界边界。Linux `core` 184/184、Linux
    `app` 189/189、Windows LLVM `app` 189/189 CTest 通过，两端应用成功链接且未启动。
    114 项当前关闭 83 项，剩余 31 项。

    B7 角色空间行链解链与重插 `sub_411530` 随后完成函数级闭环：物理
    `guid_u32 + group_u32 + first_row_i32 + mode_u32` ABI、有符号地图高度、包含端逐行
    扫描、角色 16 位 GUID 对完整 32 位参数比较、多节点行首、单节点行首、第二节点和
    深层前驱四类解链均已逐基本块双向收敛。目标 next 始终清零；末参数等于一时保留
    解链状态并返回目标角色，其他值调用 `sub_411490(role, flags & 3)` 后返回零。十九个
    直接调用点已全部反查，收敛为十七次重插、两次只解链，其中仅 `0x004299F6` 消费
    返回并覆盖完整 `0xD8` 角色记录。现代结果把诊断状态与一基角色返回索引分开保存，
    组、行、索引和环路边界只隔离原损坏输入域；重插失败不回滚已完成解链。定向 UT
    固定全部解链形态、跨组跨行重插、32 位 GUID 高位不别名、扫描边界和损坏链隔离。
    Linux `core` 184/184、Linux `app` 189/189、Windows LLVM `app` 189/189 CTest
    通过，两端应用成功链接且未启动。114 项当前关闭 84 项，剩余 30 项。

    B7 角色空间行首工作区重建 `sub_411620` 随后完成函数级闭环：无参物理 ABI 先按
    `4A9A04/08/0C` 顺序释放三张旧表，再分别分配并清零 `height * 4 + 0xA0` 字节，
    即每组 `height + 40` 个四字节行首；零高度仍保留前后各 20 行的 `0xA0` 字节。
    唯一调用点 `0x00425F68` 已反查，调用者不消费最终零 EAX。既有地图业务 owner
    不再隐式 resize，而是显式复用独立重建合同；现代只对原 32 位分配字节数回绕和
    分配失败返回错误，正常域保持先释放、后三次顺序分配和全清零。定向 UT 固定三表
    污染后重建、同高度重复重建、零高度范围及回绕隔离。Linux `core` 184/184、Linux
    `app` 189/189、Windows LLVM `app` 189/189 CTest 通过，两端应用成功链接且未启动。
    114 项当前关闭 85 项，剩余 29 项。

    B7 世界瞬态状态重置 `sub_411D00` 随后完成函数级闭环：零参数入口严格按
    `40F500/40F540/40F570/40F5A0/40F5E0/40F630/40F670` 次序释放七类实际 owner，
    再交错清零 blue/green/red 三通道 step，并分别把 64 项选择表填为 `0xCFCF`、
    64 项逐行复制行数填 1、宽度填 4，最后清零帧计数器和 64 项 u32 像素偏移。
    反向复核确认颜色当前值、目标值和倒计时不在写集合中，不能随 step 一起清零；唯一
    调用点 `0x0040823A` 也不消费最终零 EAX。SDL 世界生命周期已绑定同一聚合合同，
    选择存储从单哨兵占位扩为完整 64 项；定向 UT 固定所有 owner、填充范围和必须保留
    的颜色及 drift 字段。Linux `core` 185/185、Linux `app` 190/190、Windows LLVM
    `app` 190/190 CTest 全部通过，两端应用成功链接且未启动。114 项当前关闭 86 项，
    剩余 28 项。

    B7 普通世界外层帧 `sub_4120B0` 随后完成独立闭环复核：重新锁定
    `0x004120B0..0x00412926` 和唯一调用点 `0x0040AA90`，先按地址正向核对 HeadSgn、
    玩家/相机移动、72 槽地图角色、1 基队伍角色、相机平移、选择滚动、两次音频、
    framebuffer 组合、倒计时、调试叠层、唯一呈现、玩家帧后账本、tile 动画和视口恢复，
    再从 `run_legacy_world_frame` 的所有修改与端口反向定位原槽。两轮均未发现有效运行域
    的顺序、范围、常量、门控或状态差异，现有 C++ 无需修改；有界 owner、失败诊断和
    presentation port 只隔离原越界、空指针、宿主资源及 DirectDraw 所有权边界。Linux
    `core` 185/185、Linux `app` 190/190、Windows LLVM `app` 190/190 CTest 全部通过，
    两端应用成功链接且未启动。114 项当前关闭 87 项，剩余 27 项。

    B7 世界画面组合入口 `sub_412930` 随后完成独立闭环复核：完整范围
    `0x00412930..0x00412BD3` 的 full/partial clip、activity/clear-only/normal 三主体、
    两次可能清屏、四底图选择、全部 service/control 短路、normal 十九个 stage 与公共
    尾部均完成正向逐块和反向逐调用收敛。四个直接调用点均不消费 EAX；普通世界已接线，
    持久化截图、ANI 递归重绘和特殊模式捕获分别转交 B11、既有 ANI port 与 B9 owner。
    新增 talk target 有效时 phase 7/8 边界 UT，确认 phase 8 在 service `0x51` 查询前
    跳过固定数字绘制；有界 framebuffer/raster/background/stage port 仅隔离原无效指针、
    资源和 DirectDraw 状态。Linux `core` 185/185、Linux `app` 190/190、Windows LLVM
    `app` 190/190 CTest 全部通过，两端应用成功链接且未启动。114 项当前关闭 88 项，
    剩余 26 项。

    B7 的 16 位对齐世界底图入口 `sub_412BE0` 随后完成独立闭环：物理范围
    `0x00412BE0..0x00412D29` 的 service `0x48/0x13` 短路、40×30 与局部 24×24
    tile 区域、首 cell、行跨度修正、动画层偏移以及 hidden/transparent/opaque 分派，
    均完成 LST→C++ 和 C++→LST 反复核对。反向核对发现局部区域 left 恰为零时，汇编
    会保留默认修正并按 `map_width - 16` 推进下一行；现代坐标循环此前无意正常化了该
    旧行为，现已原样恢复并加入回归 UT。统一 background renderer 继续承载最终像素
    语义，没有为旧裸指针重新引入不安全所有权；另一组 UT 固定动画层只偏移 tile 索引
    而不偏移 flags，并同时验证三种 cell 分派。Linux `core` 185/185、Linux `app`
    190/190、Windows LLVM `app` 190/190 CTest 全部通过，两端应用成功链接且未启动。
    114 项当前关闭 89 项，剩余 25 项。

    B7 的 16 位未对齐世界底图入口 `sub_412D30` 随后完成独立闭环：物理范围
    `0x00412D30..0x00413219` 的对齐回退、相机 cell/余量拆分、负 cell 调整、地图宽高
    截断、service `0x48/0x13` 短路、四边与内部 tile 分派均完成双向反复核对。反向
    核对发现 service-13 局部路径只画内部完整 tile，单轴已对齐时仍保留一格边界；继续
    追入 `sub_4170E0` 后确认普通路径也只有最外圈 tile 受当前 raster clip 约束。两处
    现代正常化均已恢复，并为单轴边界、负相机原点和“外圈裁剪、内部不裁剪”补齐回归
    UT。Linux `core` 185/185、Linux `app` 190/190、Windows LLVM `app` 190/190
    CTest 全部通过，两端应用成功链接且未启动。114 项当前关闭 90 项，剩余 24 项。

    B7 的 8 位索引对齐世界底图入口 `sub_413220` 随后完成独立闭环：物理范围
    `0x00413220..0x0041336C` 的 service 短路、对齐区域、首 cell、动画层、hidden、
    `0x200 + tile_index * 0x100` 源地址、palette 转色及 index-1 透明分派完成双向
    反复核对。反向核对确认 `sub_412BE0` 的 left-zero 行推进特例在本入口同样存在；现代
    实现此前把它误限于 direct-16，现已扩到 indexed 路径并补齐独立回归 UT。Linux
    `core` 185/185、Linux `app` 190/190、Windows LLVM `app` 190/190 CTest 全部
    通过，两端应用成功链接且未启动。114 项当前关闭 91 项，剩余 23 项。

    B7 的 8 位索引未对齐世界底图入口 `sub_413370` 随后完成独立闭环：物理范围
    `0x00413370..0x0041386E` 的对齐回退、相机余量与负 cell 修正、地图截断、service
    `0x48/0x13` 短路、普通四边和严格内部遍历均完成双向核对，并独立追入
    `sub_4170E0/sub_4175B0/sub_417650`。核对确认 service-13 跳过四边只画内部完整
    tile，普通路径只有外圈受 raster clip；两项行为与像素布局无关。现代实现此前误限于
    direct-16，本轮以最小改动解除限制并补齐 indexed 专用 UT：focus `(320,240)`、相机
    `(0,7)` 固定首写 `(144,57)`、506 个现代唯一 cell 与 129536 次写入，另覆盖负相机
    X=149、窄 edge clip、hidden/index-1 透明与不透明、非零动画层、右下截断及短 palette /
    source。物理 aligned fallback 由唯一调用者证明在当前调用域不可达；统一 renderer
    保持平台适配而不新增公共判别器。Linux `core` 185/185、Linux `app` 190/190、
    Windows LLVM `app` 190/190 CTest 全部通过，两端应用成功链接且未启动。114 项当前
    关闭 92 项，剩余 22 项。

    B7 的普通空间角色外层扫描 `sub_413870` 随后完成独立闭环：物理范围
    `0x00413870..0x0041390B`、`0x00412A8D` 唯一调用点以及两个直接 callee 的必要
    cdecl 边界均先于 C++ 独立复核。无参数/plain `retn`、正常 `EAX=3` 且调用者忽略、
    `EBX/EBP/ESI/EDI` 保存，group `2→0→1`、`trunc_toward_zero(cameraY/16)-20`、每组
    `0x46` 行、`u32(row)<u32(mapHeight+20)`、物理 `row+20`、null head 与
    draw→重读 `+0x2C` 低字→可选 audio→重读 `+0x00` next 的顺序均完成 LST→C++→LST
    双向收敛。实现删除了错误的空角色 span 整体早退；三个行头数组的 eager bounds 校验、
    一基链接与环检测保留为平台适配。独立 UT 新增 `-17/-15` 与极端 camera、H=0/H=3
    全 null 空 span、prefix/suffix poison、同行多节点与组序、高字 gate、callee callback
    mutation 及既有无效链/短数组向量；定向 synthetic/real 测试通过。Linux `core`
    185/185、Linux `app` 190/190、Windows LLVM `app` 190/190 CTest 全部通过，两端应用
    成功链接且未启动游戏 EXE。`sub_413910/sub_413CA0/sub_413EA0/sub_413F00` 继续保持
    `pending_audit`，原版 framebuffer/audio/jitter 动态 oracle 仍阻断。114 项当前关闭
    93 项，即 `43 assembly_exact + 50 platform_adapted + 21 pending_audit`。

    B7 的普通空间角色绘制 `sub_413910` 随后完成独立闭环：完整物理范围
    `0x00413910..0x00413C96`、`sub_413870:0x004138CF` 唯一调用点、一个入口参数的
    cdecl/plain `retn`、callee-saved 寄存器、drawable 失败返回零与其余出口返回一，以及
    12 个 direct/IAT 调用点均先于修改完成复核。实现纠正了两项旧时序：
    `0x00413934..0x00413957` 的 world X/Y 与 camera left/top 只冻结一次并贯穿残影、
    主图、加色、覆盖层与粒子；`0x00413A16` 的 mode flags 在主图前捕获，而加色坐标在
    主图后重读 `+0x28/+0x2A` 及 action draw offsets。标签按 LST 重读 live role X/Y 和
    draw Y、继续使用冻结 camera，并把 `u32(len*11)` 回绕结果按有符号值向零除二。
    post-main 与 load-frame callback mutation UT 固定全部顺序和坐标，既有真实 TSW
    framebuffer hash 不变。SDL production seam 把单次位置音效接到 sample manager 和
    调用时受控角色监听者，把角色粒子接到持久 `LegacyAniRoleParticleEffect`、共享 action/RNG
    及调用时 viewport/runtime ports，把标签接到
    内建 16 色转换和 12 点 text framebuffer；三条 adapter 定向测试分别验证真实 manager、
    与 direct seeded particle update 相同的结果/状态及 framebuffer 像素。受检
    frame/overlay/label/role lookup 保留为平台适配。Linux `core` 185/185、Linux `app`
    191/191、Windows LLVM `app` 191/191 CTest 全部通过，两端应用均成功链接且未启动游戏
    EXE。`sub_413CA0/sub_413EA0/sub_413F00` 未改且继续 `pending_audit/not_inherited`；原版
    framebuffer/audio/particle/text/jitter 动态 oracle 仍阻断。114 项当前关闭 94 项，
    即 `43 assembly_exact + 51 platform_adapted + 20 pending_audit`。

    B7 的普通角色距离音频 `sub_413CA0` 随后完成独立闭环：完整物理范围
    `0x00413CA0..0x00413E96`、`sub_413870:0x004138DF` 唯一调用点、一参数
    cdecl/plain `retn`、六个直接调用边界、入口 32 位回绕距离与 x87 转换、packed
    scheduler、距离/bit 门、GUID 查找、有限/无限 sample 启动、两个 i16 角色数组、
    volume/pan 和外部停止均完成双向逐基本块收敛。复核纠正 play 后 Y/listener/mix/
    sound-id 与 volume 后 X/listener/sound-id 的 reload 时点；低字零 scheduler、距离
    `512/513`、负平方和、参数回绕及受检失败副作用均由独立 UT 固定。SDL 四个距离音频
    空端口已接到实际 `LegacySampleManager`，每帧同步当前受控角色索引；受检角色/数组与
    sample owner 保持为平台适配。Linux `core` 185/185、Linux `app` 191/191、Windows
    LLVM `app` 191/191 CTest 全部通过，两端应用成功链接且未启动游戏 EXE。
    `sub_413EA0/sub_413F00` 继续 `pending_audit/not_inherited`；原版 audio/particle/text/
    framebuffer/jitter 动态 oracle 仍阻断。114 项当前关闭 95 项，即
    `43 assembly_exact + 52 platform_adapted + 19 pending_audit`。

    B7 的 group-0 bit-29 外层扫描 `sub_413EA0` 随后完成独立闭环：完整物理范围
    `0x00413EA0..0x00413EFE`、`sub_412930:0x00412A88` 唯一 service-11 条件调用点、
    无参/plain `retn`、唯一一参数 cdecl callee 边界、group-0 行首、向零 camera-top 商减五、
    `EBX=-10..29` 四十槽、有符号 map-height 退出、负行跳过、bit-29 门和 callee 后
    next reload 均完成 LST→C++→LST 双向逐基本块收敛。实现删除了全 null 行头时错误的
    `roles.empty()` 整体早退；`-17/-15`、H=3/H=0 空角色、极端相机、精确四十槽、组隔离、
    同行链序和回调改 next 均由独立 UT 固定。受检 row-head/link/frame owner 与失败状态
    保持为平台适配；`sub_413F00` 继续 `pending_audit/not_inherited`，不继承外层真实 TSW
    哈希。Linux `core` 185/185、Linux `app` 191/191、Windows LLVM `app` 191/191
    CTest 全部通过，两端应用成功链接且未启动游戏 EXE。原版 framebuffer/jitter 动态
    oracle 仍阻断。114 项当前关闭 96 项，即
    `43 assembly_exact + 53 platform_adapted + 18 pending_audit`。

    B7 的单个 bit-29 角色绘制 `sub_413F00` 随后完成独立闭环：完整物理范围
    `0x00413F00..0x00413FDD`、`sub_413EA0:0x00413EE2` 唯一调用点、一参数
    cdecl/plain `retn`、恒一返回、drawable mask、冻结 camera/world/mode/TSW key、严格
    `(-320,960)` 水平开区间、post-load `field_28/field_2a/draw_offset_x` 重读、两条回绕
    坐标公式与固定透明 flags 均完成 LST→C++→LST 双向逐基本块收敛。复核发现旧实现会在
    frame load 后错误重读 world X/Y 与 mode，现改为 load 前快照；mutation UT 同时证明
    三个 draw 字段继续读取 load 后 live 值，固定 resource/frame `7/8`、坐标 `(88,200)`
    与 flags `0x80000017`。继续追入 TSW lookup/load 边界后确认混合 frame dword 的高
    16 位在 cache key 和物理读取前均被屏蔽；现代缺帧隔离仍保留原 opacity step 四的先行
    写入。受检 frame owner 保持为平台适配。Linux `core` 185/185、Linux `app` 191/191、
    Windows LLVM `app` 191/191 CTest 全部通过，两端应用成功链接且未启动游戏 EXE。
    原版 framebuffer/jitter 动态 oracle 仍阻断。114 项当前关闭 97 项，即
    `43 assembly_exact + 54 platform_adapted + 17 pending_audit`；下一精确停点为
    `0x00413FE0 sub_413FE0`。

    B7 的开发调试叠层 `sub_413FE0` 随后完成独立闭环：完整物理范围
    `0x00413FE0..0x00414567`、`sub_4120B0:0x004126E8` 唯一调用点、两个实际 i32 参数、
    调用者多余常量 2、无统一 EAX 返回合同、入口文字样式、两个精确等一开关、五次
    38×28 collision-grid 扫描、七条固定文字、地图事件与附近角色循环均完成
    LST→C++→LST 双向逐基本块收敛。复核纠正三项旧时序：cell 低字节事件号改在任何文字
    callback 前冻结；DIV-zero 隔离改为保留 MAct/Mouse 两次先行调用；事件 flag 数字改在
    低位查询 callback 后第三次重读。找到的空事件名仍发出一次 NUL draw，角色第二行读取
    summary callback 后的 GUID/Talk/Path/flags。受检 framebuffer/cell/event/role owner、
    256 字节格式边界与 DIV trap 使分类保持 `platform_adapted`。Linux `core` 185/185、
    Linux `app` 191/191、Windows LLVM `app` 191/191 CTest 全部通过，两端应用成功链接且
    未启动游戏 EXE。原版 framebuffer/text 动态 oracle 仍阻断。114 项当前关闭 98 项，
    即 `43 assembly_exact + 55 platform_adapted + 16 pending_audit`；下一精确停点为
    `0x00414570 sub_414570`。

    B7 的脚本相机逐帧平移 `sub_414570` 随后完成独立闭环：完整物理范围
    `0x00414570..0x004145EF`、`sub_4120B0:0x0041268C` 唯一调用点、无参数 ABI、
    `ESI/EBX` 保存、无统一 EAX 返回合同、双 remaining 入口门、共享双轴更新体、四条
    viewport 回绕加法、两条 remaining 回绕减法及结果恰好为零时的 step 清理均完成
    LST→C++→LST 双向逐指令收敛。独立向量固定 dormant step、一轴非规范零 remaining/
    非零 step、`remaining = ±1, step = ±2` overshoot 和 `INT32_MIN - 1` 回绕；纯 32 位
    状态变换无 unsafe pointer、callback 或平台替代，分类为 `assembly_exact`。Linux
    `core` 185/185、Linux `app` 191/191、Windows LLVM `app` 191/191 CTest 全部通过，
    两端应用成功链接且未启动游戏 EXE。114 项当前关闭 99 项，即
    `44 assembly_exact + 55 platform_adapted + 15 pending_audit`；下一精确停点为
    `0x004145F0 sub_4145F0`。

    B7 的普通角色闪烁残影 `sub_4145F0` 随后完成独立闭环：完整物理范围
    `0x004145F0..0x004146E2`、`sub_413910:0x00413A0E` 唯一调用点、六参数 cdecl、
    `sub_4170E0` 唯一 direct callee、frame 高度二分/四分、counter parity/bit3/low3
    分支、坐标回绕、flags mask、零 auxiliary、共享 jitter 继承和 `0x004995D4` 全 16 项
    RGB 色表均完成 LST→C++→LST 双向逐指令收敛。独立向量固定 counter
    `0/1/2/8/9/10`、负奇数 action offset、奇数 frame height、height 3 的零 target/零
    displacement 特例及完整色表。受检 role/frame owner 与 typed blit request/port 使分类
    保持 `platform_adapted`。synthetic/real roles 两项定向 CTest 通过；Linux `core`
    185/185、Linux `app` 191/191、Windows LLVM `app` 191/191 CTest 全部通过，两端应用
    成功链接且未启动游戏 EXE。114 项当前关闭 100 项，即
    `44 assembly_exact + 56 platform_adapted + 14 pending_audit`；下一精确停点为
    `0x004146F0 sub_4146F0`。

    B7 的世界帧颜色过渡 `sub_4146F0` 随后完成独立闭环：完整物理范围
    `0x004146F0..0x004147DE`、`sub_412930:0x00412BCB` 唯一调用点、一参数 cdecl、三项
    x87 zero/unordered 门、可选 countdown 减一、current 累加/target→step bit 复制两分支、
    三次 `sub_489654` qword→low-dword 向零转换及 `sub_420490` 完整 `0x4B000` 像素 RGB
    偏移均完成 LST→C++→LST 双向逐指令收敛。独立向量固定 mixed NaN、参数零、入口负
    countdown、`4294967040.0F → -256`、NaN/无穷转换零及末像素 read guard。owned
    framebuffer、typed format 与只读 guard word 使分类保持 `platform_adapted`。
    frame-color 与 world-frame composition/runtime synthetic/real 五项定向 CTest 通过；
    Linux `core` 185/185、Linux `app` 191/191、Windows LLVM `app` 191/191 CTest 全部
    通过，两端应用成功链接且未启动游戏 EXE。114 项当前关闭 101 项，即
    `44 assembly_exact + 57 platform_adapted + 13 pending_audit`；下一精确停点为
    `0x004147E0 sub_4147E0`。

    B7 的图片动作链更新绘制 `sub_4147E0` 随后完成独立闭环：完整物理范围
    `0x004147E0..0x004148ED`、三个一参数 cdecl 调用点、动作更新、载帧、绘制、音效、
    sound word 清零、完成值精确等一摘链及 next reload 均完成 LST→C++→LST 双向逐指令
    收敛。复核发现旧现代实现把 frame miss 当作可继续诊断，错误执行其后的音效、摘链和
    后续节点；现于原 `0x00414843 [eax]` 首次帧解引用点返回 `frame_load_failed`，并让 frame
    runtime 在对应主/副 stage 停止。独立 callback 向量固定 update→frame→draw→audio
    之间的 action/坐标/sound/completion 重读。受检 `std::list`/frame owner 和 typed ports
    使分类保持 `platform_adapted`。picture-actions 与 world-frame composition/runtime
    synthetic/real 五项定向 CTest 通过；Linux `core` 185/185、Linux `app` 191/191、
    Windows LLVM `app` 191/191 CTest 全部通过，两端应用成功链接且未启动游戏 EXE。
    114 项当前关闭 102 项，即 `44 assembly_exact + 58 platform_adapted + 12
    pending_audit`；下一精确停点为 `0x004148F0 sub_4148F0`。

    B7 的选择序列临时相机滚动 `sub_4148F0` 随后完成独立闭环：完整物理范围
    `0x004148F0..0x004149A1`、`sub_4120B0:0x00412691` 唯一调用点、无参数 ABI、首项
    `0xCFCF` sentinel/map 22 两门、word 游标 sentinel 回零、signed i16 坐标、countdown
    回绕/重载、原 left/top 保存及四边临时平移均完成 LST→C++→LST 双向逐指令收敛。
    独立向量固定奇数游标不对齐、零 countdown 载入负 interval、`INT32_MIN` 递减回绕、
    无效 span 隔离和帧尾固定 640×480 恢复。受检 selection span 使分类保持
    `platform_adapted`。selection-scroll 与 frame-coordinator 两项定向 CTest 通过；Linux
    `core` 185/185、Linux `app` 191/191、Windows LLVM `app` 191/191 CTest 全部通过。
    该状态机已接入 coordinator，两端应用成功链接且未启动游戏 EXE。
    114 项当前关闭 103 项，即 `44 assembly_exact + 59 platform_adapted + 11
    pending_audit`；下一精确停点为 `0x004149B0 sub_4149B0`。

    B7 的世界软件鼠标与右边条 `sub_4149B0` 随后完成独立闭环：完整物理范围
    `0x004149B0..0x00414B58`、无参数 ABI、普通世界/特殊模式/商店三处调用、Delete 变体
    15、右边条 movement/idle/hot-corner/Talk 门、主鼠标 variant reload、非致命更新失败、
    frame load 与 blit 均完成 LST→C++→LST 双向逐指令收敛。复核发现旧现代实现把右边条
    helper 的 frame miss 当可忽略返回，错误继续主鼠标；原 helper 会在 `0x0040EC30 [eax]`
    首次 frame 解引用停止。现返回 `edge_frame_unavailable` 并让 world runtime 在 cursor
    stage 停止。独立 callback 向量固定 frame load 后 action offset/flags/opacity 与 mouse
    X/Y live reload。typed input/action/frame/blit ports 使分类保持 `platform_adapted`。
    cursor 与 world-frame-runtime synthetic/real 三项定向 CTest 通过；Linux `core`
    185/185、Linux `app` 191/191、Windows LLVM `app` 191/191 CTest 全部通过。普通世界
    已接入原槽，特殊模式/商店调用点留给各自模块且不复制 owner；两端应用成功链接且未
    启动游戏 EXE。114 项当前关闭 104 项，即 `44 assembly_exact + 60 platform_adapted + 10
    pending_audit`；下一精确停点为 `0x004151F0 sub_4151F0`。

    B7 的 LMF indexed object 绘制 `sub_4151F0` 随后完成独立闭环：完整物理范围
    `0x004151F0..0x004153CB`、无参数 ABI、`sub_412930:0x004129C8` 唯一调用、空链早退、
    `0..30` 三十一轮链头重载、每序号首个相交节点、视口相对 clip、两轴 parallax、blit
    和全屏 clip 恢复均完成 LST→C++→LST 双向逐指令收敛。独立向量固定 draw callback 后
    下一 ordinal 的 live 节点字段重读，以及大 factor 的 32 位 `IMUL` 回绕与 signed `/16`
    朝零截断。owned reverse span 与 typed clip/blit ports 使分类保持 `platform_adapted`。
    indexed-objects 与 world-frame-runtime synthetic/real 三项定向 CTest 通过；Linux
    `core` 185/185、Linux `app` 191/191、Windows LLVM `app` 191/191 CTest 全部通过。
    该 owner 已接入原槽，地图 72 真实 `1072x1024x16` stream 进入 runtime blitter；
    两端应用成功链接且未启动游戏 EXE。114 项当前关闭 105 项，即
    `44 assembly_exact + 61 platform_adapted + 9 pending_audit`；下一精确停点为
    `0x00425B50 sub_425B50`。

    B7 的世界地图运行状态清理 `sub_425B50` 随后完成独立闭环：完整物理范围
    `0x00425B50..0x00425BDA`、无参数 ABI、`sub_40F160/sub_424B90` 两处调用、两个地图
    分配释放、25 dword 状态清零、256 个 `0xD8` 角色逐项 `+0x38` payload 释放后立即
    清零、54 dword 全 `0xFF` 无角色 sentinel 及 72 个 `0x21C` 对象槽重置均完成
    LST→C++→LST 双向逐指令收敛。复核发现 SDL 新游戏旧世界释放误用了 `sub_40F3B0`
    reset，导致清零后重建 action；现改用 `clear_legacy_world_role_table`，固定本函数不做
    action 初始化的差异。RAII session、checked absent-role owner 与 split 64+8 slots 使
    分类保持 `platform_adapted`。role-lifecycle、role-transfer、new-game transition 与
    runtime-session synthetic/real 五项定向 CTest 通过；Linux `core` 185/185、Linux
    `app` 191/191、Windows LLVM `app` 191/191 CTest 全部通过，两端应用成功链接且未
    启动游戏 EXE。114 项当前关闭 106 项，即
    `44 assembly_exact + 62 platform_adapted + 8 pending_audit`；下一精确停点为
    `0x00425BE0 sub_425BE0`。

- B7 继续按函数级停止线完成 `0x00425BE0 sub_425BE0` 的独立闭环。唯一调用点确认
    为两参数 cdecl，EAX 返回 0/1；六轮 LST→C++→LST REVIEW 纠正并固定头签名确认中间的
    progress 15、后续 `60/65/70/75/80/85`、事件/两类角色的分阶段构建、CM 原槽及逐
    索引对象 consumer。28 个显式 `_AIL_serve` 静态点全部恢复，动态次数为
    `22 + referenced_record_count + 5 * indexed_object_count`；进度函数内部维护不与其
    合并。双对象首 consumer 失败向量固定第一个对象第五次维护后立即短路，observer 在
    同步装载返回前卸载。真实 `huge.lmf` 地图 22/24/500 的七进度、维护公式、业务状态
    和既有 framebuffer 哈希继续通过。完整门禁为 Linux `core` 185/185、Linux `app`
    191/191、Windows LLVM `app` 191/191 CTest；两端应用成功链接，未启动原版或
    OpenSWD3 游戏 EXE。114 项当前关闭 107 项，即
    `44 assembly_exact + 63 platform_adapted + 7 pending_audit`；下一精确停点为
    `0x00426840 sub_426840`。

- B7 继续完成 `0x00426840 sub_426840` 独立闭环。唯一调用点确认四参数 cdecl：地图号、
    LMF 地图偏移、受原 word 门控的 CM 相对偏移与 `0x200` 路径 scratch；EAX 映射基址由
    `sub_425BE0` 写入地图状态 `+0x20`。多轮 LST→C++→LST REVIEW 恢复 14 个直接
    `_AIL_serve` 静态点，动态次数固定为打开失败 1、hit 5、空目录 5、普通 miss 8、每次
    淘汰额外 1；hit 计数清零严格位于槽读取后的维护点之后。空目录保持先生成后提交，一般
    miss 保持先提交后生成，hit 保留目录物理尾部而 miss 截断尾部；目录写回后显式关闭。
    runtime → render → file CM source 已把 SDL audio owner 传到原 CM 槽，真实地图 24 的
    首次生成与第二次 hit 各固定 5 次直接维护。Windows 首轮门禁暴露测试在原版独占打开的
    `mcache.dat` 上并发读文件；改用第 4 次维护抛测试哨兵并在 RAII 关闭后验证未写回，未改
    生产语义。非零短目录和无淘汰候选在原越界点前受检停止，因此分类为
    `platform_adapted`；`sub_426DF0/sub_4270F0/sub_427140` 仍须独立审计。最终完整门禁为
    Linux `core` 185/185、Linux `app` 191/191、Windows LLVM `app` 191/191 CTest，
    两端应用成功链接且未启动游戏 EXE。114 项当前关闭 108 项，即
    `44 assembly_exact + 64 platform_adapted + 6 pending_audit`；下一精确停点为
    `0x00426DF0 sub_426DF0`。

- B7 继续完成 `0x00426DF0 sub_426DF0` 独立闭环。两处调用确认五参数 cdecl；IDA 漏计的
    `+0x10` scratch 实参完全未读，`+0x14` 在头读取成功后接收声明输出大小，EAX 所有出口
    恒为 0 且调用者不消费。多轮 LST→C++→LST REVIEW 恢复循环外 4 个、每块 3 个直接
    `_AIL_serve`，成功动态次数为 `4 + 3 * chunk_count`；每块写后 progress 保留
    `15 + floor(45 * index / chunk_count)`。块数继续使用原版
    `(chunk_size + declared_size) / chunk_size`，精确整除时多读/解压一块、写零字节的 BUG
    已由双块合成向量固定为 10 次维护与 `15/37`。真实地图 24 四块 generator 为 16 次维护、
    `15/26/37/48`；首次完整 loader 为 21 次，第二次 hit 为 5 次，render 总公式更新为
    `43 + refs + 5 * indexed_objects`，完整进度为
    `15/15/26/37/48/60/65/70/75/80/85`。同一 progress/audio owner 已经 loader → render →
    SDL 纵向传入。固定 malloc、全局 LMF 文件、错误路径泄漏和未检查解压/写入以
    RAII/checked owner 替代，失败停在对应危险点，因此分类为 `platform_adapted`；
    `sub_4270F0/sub_427140` 仍须独立审计。最终完整门禁为 Linux `core` 185/185、Linux
    `app` 191/191、Windows LLVM `app` 191/191 CTest，两端应用成功链接且未启动游戏 EXE。
    114 项当前关闭 109 项，即
    `44 assembly_exact + 65 platform_adapted + 5 pending_audit`；下一精确停点为
    `0x004270F0 sub_4270F0`。

- B7 继续完成 `0x004270F0 sub_4270F0` 独立闭环。唯一调用点实际压入五个 cdecl 参数：
    首个 map-id 与第四个路径 scratch 未读，第二/第三参数形成 CM 头地址，第五参数接收声明
    输出大小。零 CM offset 只经过 nullsub，不写输出也不维护；非零路径在 seek 前严格执行
    一个 `_AIL_serve`，以 u32 计算 `map_offset + cm_relative_offset + 0x10` 后读取一个 dword。
    调用者清理 `0x14` 字节并忽略 EAX。现代 size probe 复用同一 audio owner，以独立 archive
    reader 和显式状态隔离 archive-open、负 seek 与短读，所有合法域顺序保持不变，分类为
    `platform_adapted`。合成成功/archive-open/seek/read 失败各固定 1 次直接维护，零 offset
    固定 0 次；一块普通 miss 跨 helper 合计 16 次、单淘汰为 17 次。真实地图 24 probe
    输出 `3,706,880` 且维护 1 次；generator/loader/render synthetic/real 6/6 定向回归通过。
    最终完整门禁为 Linux `core` 185/185、Linux `app` 191/191、Windows LLVM `app`
    191/191 CTest，两端应用成功链接且未启动游戏 EXE。114 项当前关闭 110 项，即
    `44 assembly_exact + 66 platform_adapted + 4 pending_audit`；下一精确停点为
    `0x00427140 sub_427140`。

- B7 继续完成 `0x00427140 sub_427140` 独立闭环。三个调用点都压入两个 cdecl 参数：
    第一槽是 stored slot，第二槽是 callee 未读的路径 scratch；caller 清理 8 字节并消费
    EAX 映射基址。函数第 1 个 `_AIL_serve` 位于旧 view/mapping/file 关闭前，第 2 个位于
    新槽只读 `OPEN_ALWAYS` 前；open 失败固定到此返回。open 成功后第 3 个点位于
    create-mapping 后，第 4 个位于 map-view 前；mapping/view 失败只记录诊断，仍复制当前
    路径并执行第 5 个点。现代 `read_legacy_cm_cache_unit` 在第 1/2 个回调间清空旧借用，
    在第 4/5 个回调间发布完整物理 bytes；RAII file 与 owned bytes 替代进程期裸映射，
    合法域顺序不变，分类为 `platform_adapted`。直接 UT 固定 open 失败 2 次、ready/empty
    5 次及旧借用失效点；一块普通 miss 共 21 次、单淘汰 22 次。真实地图 24 首次 loader
    为 26、hit 为 10，render 总公式为 `48 + refs + 5 * indexed_objects`；loader/render
    synthetic/real 4/4 定向回归通过。最终完整门禁为 Linux `core` 185/185、Linux `app`
    191/191、Windows LLVM `app` 191/191 CTest，两端应用成功链接且未启动游戏 EXE。
    114 项当前关闭 111 项，即 `44 assembly_exact + 67 platform_adapted + 3 pending_audit`；
    下一精确停点为 `0x004272C0 sub_4272C0`。

- B7 继续完成 `0x004272C0 sub_4272C0` 独立闭环。无参数主块调用 constructor thunk 后
    跳入 function chunk；chunk 把 destructor thunk 注册到 `_atexit` 并返回无人消费的
    EAX。ctor 对 `0x004CF6E0..0x004CF72F` 写入 `0x50` 字节 unopened 文件 owner：首 dword
    为 `0xFFFFFFFF`，其余 19 个 dword 为零。全 LST xref 只有 ctor/dtor 两处，没有第三个
    业务消费者；因此析构中的 file handle 始终为零，不进入 view/mapping/file 的任何 OS
    cleanup 分支。现代 `LegacyFile` constructor/destructor 与 C++ RAII 覆盖所有有消费者的
    文件生命周期，这个 dead global 以证明性消除替代，分类为 `platform_adapted`；不伪造
    unit/asset 证据。最终完整门禁为 Linux `core` 185/185、Linux `app` 191/191、Windows
    LLVM `app` 191/191 CTest，两端应用成功链接且未启动游戏 EXE。114 项当前关闭 112 项，即
    `44 assembly_exact + 68 platform_adapted + 2 pending_audit`；下一精确停点为
    `0x00427300 sub_427300`。

- B7 继续完成 `0x00427300 sub_427300` 独立闭环。`sub_40A570:0x0040A74E` 唯一调用点
    确认无参数、caller 不清栈且忽略 EAX；四个 plain return 对应选择消费、NPC Talk、地图
    Talk 与普通尾部，没有统一返回值契约。完整基本块先后恢复 cache-only NPC hover、位九
    门控的选择链、NPC/地图 Talk 优先、角色相向、地图格 flag 与 script 掩码、右键八方向
    合成，以及活动对话中的四 dword 延迟输入复制。反复 C++→LST REVIEW 纠正两个差异：
    hover 只用入口 camera 快照，而 `0x00427682` 起必须重借 live camera 供地图格与方向；
    input/player span 曾在入口提前失败，现已下沉到 choice/role/map/direction/delayed/player
    的原首次解引用点，保留此前 flag query、cursor、choice index、event code 与 facing
    副作用。`sub_40C020(guid)` 结果只送入空诊断函数，现代不伪造业务 callback。typed role/
    event/input spans、camera borrow 与 owned Talk state 使分类保持 `platform_adapted`；SDL
    runtime 把 live camera 接回原帧槽。独立 UT 固定 hover 0x20/0x40 优先、cache miss、
    choice 非首采样、非致命 action 失败、Talk 两路径、camera reload、方向/延迟复制与全部
    unsafe-point 停止顺序；interaction 定向 1/1 通过。最终完整门禁为 Linux `core`
    185/185、Linux `app` 191/191、Windows LLVM `app` 191/191 CTest，两端应用成功链接且
    未启动任何原版或 OpenSWD3 游戏 EXE。114 项当前关闭 113 项，即
    `44 assembly_exact + 69 platform_adapted + 1 pending_audit`；B7 唯一剩余精确停点为
    `0x00402F80 sub_402F80`。

- B7 最后完成 `0x00402F80 sub_402F80` 独立闭环。完整物理范围
    `0x00402F80..0x004040A4` 与唯一调用点从零重建：无参数、13 个 plain `retn`、caller
    忽略 EAX；隐藏开发热键的四键解锁、raw scan 顺序、9 个静态 Sleep 点、模式/显示/
    collision/间隔/资源/金钱/物品分支均恢复。normal 路径补齐 dialog message 与 choice
    sentinel `0x1000` 仲裁、热点数量抢占、列表方向、global lock、story flag `0x14`
    仅绕过阻挡调整、facing/menu/collision/motion 以及遇敌公共尾。反复 REVIEW 纠正三项
    差异：mouse/direction/dialog 统一使用 `[0x004CAE7C]` 单一 selection owner；idle phase
    衰减移动到所有 normal-control 早退之前；speed 200 ms delay 保留在 missing-input 原
    危险点之前。SDL 以真实 MAPS 阈值/区域、二次 RNG、story flags、音频停止、battle
    初始化、地图 view 与角色状态 owner 接入遇敌；Win32-only debug dialog/item 链通过窄
    port 明确平台适配，不污染默认关闭的正式路径。定向 CTest 6/6；Linux `core` 186/186、
    Linux `app` 192/192、Windows LLVM `app` 192/192 全部通过，三进程生命周期成功且未
    启动游戏 EXE。B7 全集最终为 `44 assembly_exact + 70 platform_adapted + 0
    pending_audit`，114/114 达到模块移交条件。

B7 P0 有限收口完成。
