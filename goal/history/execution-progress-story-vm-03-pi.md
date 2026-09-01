# 执行历史：剧情VM第三段

状态：冻结历史；不得作为当前执行状态或行为真值。

来源：重构前`execution-plan-pi.md` v853第2451..2985行，剧情VM P2后段与P3验收。

完整性与当前资料入口见[`../execution-history-index-pi.md`](../execution-history-index-pi.md)。当前状态见[`../execution-state-pi.md`](../execution-state-pi.md)。

---


- 剧情VM P2第九十六组`0x0042BAB8` / opcode120完成独立闭环。机器在lookup命中后按
    `+4/+6/+8`分阶段读取并提交action/base/variant，前两项符号扩展、第三项零扩展；随后
    清wait、refresh并置role bit12。lookup失败按`+8/+6/+4`读取raw word后提交MAPS patch。
    修复旧case整条预验、lookup前读取全部operand及遗漏previous120三项差异。800条真实记录/
    808 probes、live/missing双回放、四alias、特殊selector、部分提交及两类精确尾通过。
    Story VM 3/3、Linux core 186/186及app 192/192均以exit 0通过；workpack双生成稳定hash为
    `00e548e81c9b13245adc58ee4ded3fae5cb35c16f1ae9d3fbe2ec757b51ed58d`。
    未启动游戏EXE。现代显式opcode保持135；对外进度为已实现135/198、已验收131/198；
    内部workpack96/146，即`13 assembly_exact + 83 platform_adapted + 50 pending_audit`。

- 剧情VM P2第九十七组`0x0042BC2C` / opcode121完成独立闭环。机器无operand，
    对完整text-control u32执行`AND FBFFFFFF`只清bit26，再经共享尾+2、发布previous121并
    same-call继续；无helper、audio或平台适配。815条真实记录/815 probes、四库精确尾、
    四raw alias、幂等清位及其他位保持通过。Story VM 3/3、Linux core 186/186与app 192/192通过；
    workpack双生成稳定hash为
    `9b71ae7e09dc8b017dac006363746171bb0a72909c1d1ca3e465b1edf562c437`。
    未启动游戏EXE。现代显式opcode增至136；对外进度为已实现136/198、已验收132/198；
    内部workpack97/146，即`14 assembly_exact + 83 platform_adapted + 49 pending_audit`。

- 剧情VM P2第九十八组`0x0042BC3D` / opcode122完成独立闭环。机器无operand，
    把world/player/dialog共享的进程级速度模式完整dword清零，再经共享尾+2、发布previous122
    并same-call继续；无helper或audio。现代VM借用SDL玩家控制实际owner，固定裸全局缺失只在
    原写点typed-stop。7条真实记录/7 probes、四库精确尾、四raw alias、任意非零值、幂等清零
    及共享owner通过。Story VM 3/3、Linux core 186/186与app 192/192通过；workpack双生成稳定hash为
    `5912c0880303fc39275195afa59a5dd4065c9b7114cf6d9b00845605784b4fff`。
    未启动游戏EXE。现代显式opcode增至137；对外进度为已实现137/198、已验收133/198；
    内部workpack98/146，即`14 assembly_exact + 84 platform_adapted + 48 pending_audit`。

- 剧情VM P2第九十九组`0x0042BC4C` / opcode123完成独立闭环。机器按三层u32相对链
    定位8字节Scene_Music表并首匹配key；成功先把raw `+2/+4` dword写入表，再读取/写入
    `+6`，保留尾word且完全不读`+8`。miss才按`+8/+6/+4/+2`读取诊断值。现代直接借用可写
    MAPS payload，在原裸读写点增加typed bounds stop。当前MAPS表314项；71条真实记录/71 probes
    全部命中，四库代表记录均从窗口`0x7FF8`只提供前8字节完成回放。四raw alias、FFF0仅匹配替换
    但字面回写、重复key首匹配、分阶段operand/target部分提交及全部MAPS链边界通过。Story VM 3/3、
    Linux core 186/186与app 192/192通过；workpack双生成稳定hash为
    `e33856eabc8f6e41e6599e8316d3feb0fdc64e20965df715110115e25d3ee692`。
    未启动游戏EXE。现代显式opcode增至138；对外进度为已实现138/198、已验收134/198；
    内部workpack99/146，即`14 assembly_exact + 85 platform_adapted + 47 pending_audit`。

- 剧情VM P2第一百组`0x0042BCF5` / opcode124完成独立闭环。机器无operand，对完整
    text-control u32执行`AND FDFFFFFF`只清bit25，再经共享尾+2、发布previous124并
    same-call继续；无helper、audio或平台适配。完整线性TALK目录为0条记录/0 probes，
    以asset absence锁定；四raw alias、幂等清位、其他位保持及精确尾通过。Story VM 3/3、
    Linux core 186/186与app 192/192通过；workpack双生成稳定hash为
    `0656af8b134e8b2e85ca179bf9b1be6a292bcecf833364fb504af6ac322f3077`。
    未启动游戏EXE。现代显式opcode增至139；对外进度为已实现139/198、已验收135/198；
    内部workpack100/146，即`15 assembly_exact + 85 platform_adapted + 46 pending_audit`。

- 剧情VM P2第一百零一组`0x0042BD06` / opcode125完成独立闭环。机器在读取脚本前尾插
    8字节节点、分配并清零256字节buffer；随后逐字节复制到首个`%Q`，先提交terminator后IP，
    再分阶段写回`%Q\\0`，发布previous125、service audio并yield。现代以进程期VM state的
    RAII list承接只追加且无消费者的原全局链，以typed-stop隔离两次unchecked malloc、跨窗口读
    和无界buffer写；合法域顺序不变。完整线性TALK目录为0条记录/0 probes，以asset absence锁定；
    四raw alias、空文本、多节点持久链、缺terminator、copy/suffix部分提交及精确尾通过。
    Story VM 3/3、Linux core 186/186与app 192/192完整门通过；workpack双生成稳定hash为
    `65bf7585ece3c3a682438f6b22f679be7a901574a865de57baf357a9ae41bc79`。
    未启动游戏EXE。人工语义增至137行，现代显式opcode增至140；对外进度为已实现140/198、
    已验收136/198；内部workpack101/146，即`15 assembly_exact + 86 platform_adapted +
    45 pending_audit`。

- 剧情VM P2第一百零二组共享`0x0042BDBC` / opcodes126、127完成独立闭环。两路先解析
    FFF0/FFFE及角色；live miss时按机器先分配0xD8临时角色并从mutable MAPS source物化，再保存
    完整u32 base variant后读取u16 expected。126在相等时重载，127在不等时重载；taken才读取
    u32 target、service audio并同文件offset0重载，not-taken完全不读target且固定+10；两路释放
    临时owner后发布previous并same-call。现代只在unchecked malloc、缺MAPS source陈旧heap读和
    I/O失败点typed-stop。opcode126锁定236条真实记录/236 probes，代表记录重载后同调用执行
    `1026→FFFF`；opcode127零资产，以asset absence与共享variant synthetic锁定。Story VM 3/3、
    Linux core 186/186和app 192/192通过；workpack双生成稳定hash为
    `b86d511d69dcb4fcc103b8bf9c5f36e84c992f60167a67deee021c0137e7d3d9`。
    未启动游戏EXE。人工语义增至139行，现代显式opcode增至142；对外进度为已实现142/198、
    已验收138/198；内部workpack102/146，即`15 assembly_exact + 87 platform_adapted +
    44 pending_audit`。

- 剧情VM P2第一百零三组`0x0042BE8A` / opcode128完成独立闭环。机器先读signed i16
    delta再读u16 item id，调用玩家库存共享helper的mode0；恢复完整u16回绕、signed判断、
    quantity B上限99、B向A搬运、A耗尽删除、FFDC规范化、MON定义节点前插及helper失败仍
    固定+6/previous/audio/yield。现代直接借用实际player_inventory；裸链/unchecked malloc
    由RAII与typed-stop隔离。新节点MON定义使用窄端口；SDL当前明确走原loader-failure而不
    伪造空定义，真实loader留给B9。397条真实记录/399 probes及item971 +1代表记录回放通过。
    Story VM 3/3、Linux core 186/186和app 192/192均以exit 0通过；workpack双生成稳定hash为
    `85e129df6e2dcaa6fe6c2ee941c97ab02cc84827fb6dd9222dd6f30854c9fe82`。
    未启动游戏EXE。人工语义增至140行，现代显式opcode增至143；对外进度为已实现143/198、
    已验收139/198；内部workpack103/146，即`15 assembly_exact + 88 platform_adapted +
    43 pending_audit`。

- 剧情VM P2第一百零四组共享`0x0042BEFE` / opcodes129、130、167、168完成独立闭环。
    四路都先查玩家普通库存，再查64槽角色物品哨兵root；129/130判断两类任一存在/全部不存在，
    167/168只消费角色root谓词但仍保留前置玩家查询。玩家item id屏蔽节点高两位，角色root使用
    完整u16且不遍历child链；taken-only target重载与not-taken +8 same-call均已锁定。现代借用
    既有item owner，以布尔存在性和空root typed-stop隔离裸指针求和/解引用。48条真实记录及
    四variant代表回放通过。Story VM 3/3、Linux core 186/186和app 192/192均以exit 0通过；workpack双生成稳定hash为
    `c643aa41a11fb76122f70411ad5b83e286ddfa41c07caa4a9938f4aedfb33dfe`。
    未启动游戏EXE。人工语义增至144行，现代显式opcode增至147；对外进度为已实现147/198、
    已验收143/198；内部workpack104/146，即`15 assembly_exact + 89 platform_adapted +
    42 pending_audit`。

- 剧情VM P2第一百零五组`0x0042BF72` / opcode131完成独立闭环。机器先读队伍链index与
    item id；valid链先执行masked-id短路，miss才按完整id执行mode1数量upsert，再清definition
    bit15并按`0x8000>>index`资格位决定保留或执行flagged-first mode0减一。恢复u16/i16回绕、
    signed 99夹值、flagged负结果删除后继续unflagged扫描、FFDC与分阶段unsafe点。现代借用
    四条实际队伍哨兵链与既有MON窄端口；SDL loader延期时在原空返回解引用点typed-stop，不
    伪造节点。线性资产零记录，以asset absence和synthetic锁定。Story VM 3/3、Linux core
    186/186及app 192/192通过；workpack双生成稳定hash为
    `859d5b1f12fa5b3690852fc79270745ee227e64c5ca9034a82dd66e05e67a43d`。
    未启动游戏EXE。人工语义增至144行，现代显式opcode增至148；对外进度为已实现148/198、
    已验收144/198；内部workpack105/146，即`15 assembly_exact + 90 platform_adapted +
    41 pending_audit`。

- 剧情VM P2第一百零六组`0x0042C033` / opcode132完成独立闭环。机器分阶段读取role
    group、slot和item id；两项index有效时masked查玩家库存，命中后深拷贝旧角色root，以
    玩家item覆盖root并规范数量，再按mode0先把旧item加回库存、后扣除source item。恢复
    两次说明深拷贝、完整定义快照、loader miss继续及首个更新删除source后的UAF危险点。
    现代借用实际64槽角色item owner；raw next覆盖造成的旧child泄漏和玩家tail别名不制造
    交叉list owner，改为清理独立child并登记平台差异；原双说明泄漏由RAII隔离。线性资产
    零记录，以asset absence和synthetic锁定。Story VM 3/3、Linux core 186/186与app
    192/192完整门均以exit 0通过；workpack双生成稳定hash为
    `1fe28bc8bc3d6ef6bf4b9a43616a7a3a44286c9e61ebcd4452ce1635d9e7a9f1`。
    未启动游戏EXE。人工语义增至145行，现代显式opcode增至149；对外进度为已实现149/198、
    已验收145/198；内部workpack106/146，即`15 assembly_exact + 91 platform_adapted +
    40 pending_audit`。

- 剧情VM P2第一百零七组`0x0042C234` / opcode133完成独立闭环。机器先释放旧256字节
    商品ID owner并分配替换owner，再从`+2`逐u16扫描到零、复制非零ID并补终止word；成功后
    写special mode请求`0x80000002`，按`4+2*N`推进、发布previous133并yield。现代以进程期
    vector承接owner，保持替换先于scan；127项为最大合法容量，缺terminator、128项越界和
    fixed global写分别在原危险点typed-stop。SDL已接实际frame coordinator的special-mode owner；
    B9后续消费ID列表并物化商品链，不在VM伪造。线性资产锁定22条记录/22 probes，TALK1/TALK3
    分布15/7，真实六项代表回放通过。Story VM 3/3、Linux core 186/186与app 192/192完整门
    均以exit 0通过；workpack双生成稳定hash为`41681a72d859e8b5fa10484f7f4569abcca5485a678c841502d53b6f45b88653`。
    未启动游戏EXE。人工语义增至146行，现代显式opcode增至150；对外进度为已实现150/198、
    已验收146/198；内部workpack107/146，即`15 assembly_exact + 92 platform_adapted +
    39 pending_audit`。

- 剧情VM P2第一百零八组`0x0042C2C6` / opcode134完成独立闭环。机器仅接受selector1..4，
    按四项`0x38`进程记录对三项current执行分阶段u16回绕加法，再按i16上限与下限夹值；first
    非正时先清零并把下一word自修改为基础opcode144，随后清后二项负值与transient，固定+10、
    发布previous134并same-call。现代以VM进程状态承接当前所需中性字段，剩余B10/B11记录加载
    不伪造；下一word越过`0x8000`只在原unsafe write点typed-stop。opcode144保持独立pending，
    本组只验证same-call抵达它，不提前实现或计数。线性资产锁定47条记录/47 probes；真实四项
    恢复链与first减100自修改链回放通过。Story VM 3/3、Linux core 186/186与app 192/192完整门
    均以exit 0通过；workpack双生成稳定hash为`37f485fae159c515dc79998397ffdbf19d8ed9187f7d845287ce5ce32b67317b`。
    未启动游戏EXE。人工语义增至147行，现代显式opcode增至151；对外进度为已实现151/198、
    已验收147/198；内部workpack108/146，即`15 assembly_exact + 93 platform_adapted +
    38 pending_audit`。

- 剧情VM P2第一百零九组`0x0042C3B0` / opcode135完成独立闭环。机器无operand，按顺序写
    special-input mode=4、high-priority submode=1、auxiliary=0、state=3，再调用`sub_406D30`
    重置输入/菜单工作区及三条存档预览；成功后+2、service audio、发布previous135并yield。
    现代四项状态均接实际frame/SDL owner，四个binding按原写点分阶段typed-stop。跨B9/B11的
    reset helper以可失败窄port转交；SDL当前明确失败于四写之后，不伪造IP/audio/previous成功。
    线性TALK零记录，以asset absence、四alias、精确尾、成功替身顺序和五级失败synthetic锁定。
    Story VM 3/3、Linux core186/186及app192/192完整门均以exit0通过；workpack双生成稳定hash为
    `7d726b762aefa5837d46e4223809fc5449f9a28c3cce50efe754864020d366fc`。未启动游戏EXE。
    人工语义增至148行，现代显式opcode增至152；对外进度为已实现152/198、已验收148/198；
    内部workpack109/146，即`15 assembly_exact + 94 platform_adapted + 37 pending_audit`。

- 剧情VM P2第一百一十组`0x0042C3F7` / opcode137完成独立闭环。机器无operand，先按music
    control bit23调用既有stream transition或直接清mode/pending，随后发布world音乐request、清scene
    三槽、按`0xFF5CFF00`清flags、+2、发布previous137并same-call继续；无audio。VM state补齐world三槽，
    与既有scene三槽共同承接原连续六dword owner；SDL transition复用实际stream manager。Win32
    `wsprintfA→nullsub_1`只覆盖无消费者诊断scratch，以平台适配省略。60条真实记录/60 probes及四库
    代表回放、四alias、bit23双路、helper三mode、六槽初始化和精确尾通过。Story VM 3/3通过，
    Linux core186/186及app192/192完整门均以exit0通过；workpack双生成稳定hash为
    `652b3142613a6445b0b835379e316c2d51ad63ebe36149489293ee2eab05acb3`。未启动游戏EXE。
    人工语义增至149行，现代显式opcode增至153；对外进度为已实现153/198、已验收149/198；
    内部workpack110/146，即`15 assembly_exact + 95 platform_adapted + 36 pending_audit`。

- 剧情VM P2第一百一十一组`0x0042C49E` / opcode138完成独立闭环。机器先读`+6` selector，
    live路径再按`+2/+4/+8`计算角色到不对称tile中心的i32回绕平方距离，并以x87向零结果严格
    比较`radius<<4`；taken-only读取`+10` target并复用同文件重载，missing/not-taken固定+14。
    FFF0保持“受控index低word作GUID”原语义，FFFE直接受控index；越界只在首次角色数组访问
    typed-stop。负平方和保持x87 integer-indefinite低dword零。117条真实记录/117 probes、四库
    代表重载、四alias、strict边界、分阶段短尾、load失败及精确尾通过。Story VM 3/3通过，
    Linux core186/186及app192/192完整门均以exit0通过；workpack双生成稳定hash为
    `200a3b772592664da87f2854faca73bf0b13c4dded193c45e6385b42f8c014ac`。未启动游戏EXE。
    人工语义增至150行，现代显式opcode增至154；对外进度为已实现154/198、已验收150/198；
    内部workpack111/146，即`15 assembly_exact + 96 platform_adapted + 35 pending_audit`。

- 剧情VM P2第一百一十二组`0x0042C6DD` / opcode141完成独立闭环。机器分阶段读取并零扩展
    `+2 mode`与`+4 pending fade divisor`，只配置transition state，不改current fade、不调用backend、
    无audio；成功+6、发布previous141并same-call。修复旧裸case整段预检和漏发previous：pending截断
    保留已写mode。124条真实记录/124 probes、四库代表回放、四alias、两级截断和精确尾通过。
    Story VM 3/3、Linux core186/186与app192/192完整门均通过；workpack双生成稳定hash为
    `d9a83235eeea155bb798d19169ac1626f0e7c430b25f4849b5c71893ec4031e6`。未启动游戏EXE。
    人工语义增至151行，现代显式opcode保持154；对外进度为已实现154/198、已验收151/198；
    内部workpack112/146，即`15 assembly_exact + 97 platform_adapted + 34 pending_audit`。

- 剧情VM P2第一百一十三组`0x0042C732` / opcode142完成独立闭环。机器逆序读取
    `+6 transition/+4 seconds/+2 minutes`并调用既有倒计时初始化器mode0；现代直连普通世界实际
    countdown owner和共享flag bitset，保留u32回绕、双aux清零、双flag顺序。common join normal carry=0，
    +8、previous142后audio一次并yield，不same-call。2条真实记录/2 probes、四alias、缺尾、typed owner、
    精确尾通过。Story VM 3/3、Linux core186/186与app192/192完整门均通过；workpack双生成稳定hash为
    `4466302631798123286e02cc3ceb611712fc7766af08761f3813a960a8c7287f`。未启动游戏EXE。
    人工语义增至152行，现代显式opcode增至155；对外进度为已实现155/198、已验收152/198；
    内部workpack113/146，即`15 assembly_exact + 98 platform_adapted + 33 pending_audit`。

- 剧情VM P2第一百一十四组`0x0042C766` / opcode143完成独立闭环。无operand；机器先把实际
    primary countdown ticks写为FFFFFFFF，再按16→76→18清共享flag，+2、previous143、audio一次并yield。
    其余primary/secondary字段不改，缺countdown binding在第一写点typed-stop。4条真实记录/4 probes、
    四alias、audio时序和精确尾通过。Story VM 3/3、Linux core186/186与app192/192完整门均通过；workpack双生成稳定hash为
    `4bbe3497e9cd49136ab03b71aee2df395d779ade0f2ef1b920c1f80d959a2353`。未启动游戏EXE。
    人工语义增至153行，现代显式opcode增至156；对外进度为已实现156/198、已验收153/198；
    内部workpack114/146，即`15 assembly_exact + 99 platform_adapted + 32 pending_audit`。

- 剧情VM P2第一百一十五组`0x0042C79D` / opcode144完成独立闭环。机器先提交mode4，再只读
    `+2 u8`：0重复mode4、1改mode5、其他保持4，`+3`未读；随后high state清零→reset helper→
    submode/aux/special-input依次清零，+4、previous144、audio一次并yield。五级binding、helper失败和
    selector截断均保留已提交写；1条真实记录/1 probe、四alias、精确尾/缺padding尾通过。opcode134
    synthetic与真实损伤自修改链已更新为same-call执行本handler，但不重复计算134。Story VM 3/3、
    Linux core186/186与app192/192通过；workpack双生成稳定hash为
    `35b0a6eb0b03b5d578ea75e15f09cc825c09a2e43063868440d714a80f4a4fb6`。未启动游戏EXE。
    人工语义增至154行，现代显式opcode增至157；对外进度为已实现157/198、已验收154/198；
    内部workpack115/146，即`15 assembly_exact + 100 platform_adapted + 31 pending_audit`。

- 剧情VM P2第一百一十六组`0x0042C7FB` / opcode147完成独立闭环。机器固定调用bit-set helper
    置共享剧情flag70，保留其余0x400-byte位集；+2、previous147、audio一次并yield。32条真实记录/
    32 probes、四库代表回放、四alias、幂等置位、完整位隔离与精确尾通过。Story VM 3/3、
    Linux core186/186与app192/192通过；workpack双生成稳定hash为
    `f53620af0616d36e2ea4ae683a61929096eccfde12125f5648708ee9b0427414`。未启动游戏EXE。
    人工语义增至155行，现代显式opcode增至158；对外进度为已实现158/198、已验收155/198；
    内部workpack116/146，即`16 assembly_exact + 100 platform_adapted + 30 pending_audit`。

- 剧情VM P2第一百一十七组`0x0042C81A` / opcode148完成独立闭环。机器固定调用bit-set helper
    置共享剧情flag19，保留其余0x400-byte位集；+2、previous148、audio一次并yield。线性资产0记录/
    0 probes，以asset absence、四alias、幂等置位、完整位隔离与精确尾锁定。Story VM 3/3、
    Linux core186/186与app192/192通过；workpack双生成稳定hash为
    `a4e188779e2fe3bd9123e596ab0e6b8ab8166c8489d94a632aba31e95a0c3d2e`。未启动游戏EXE。
    人工语义增至156行，现代显式opcode增至159；对外进度为已实现159/198、已验收156/198；
    内部workpack117/146，即`17 assembly_exact + 100 platform_adapted + 29 pending_audit`。

- 剧情VM P2第一百一十八组`0x0042C839` / opcode149完成独立闭环。机器固定调用bit-clear helper
    清共享剧情flag19，保留其余0x400-byte位集；+2、previous149、audio一次并yield。线性资产0记录/
    0 probes，以asset absence、四alias、幂等清位、完整位隔离与精确尾锁定。Story VM 3/3、
    Linux core186/186与app192/192通过；workpack双生成稳定hash为
    `e9c5bf9ff9793e59b21ce4e4e1b1e09eb4e2ebb6e5cdf16dffcfa53bdf7a4d29`。未启动游戏EXE。
    人工语义增至157行，现代显式opcode增至160；对外进度为已实现160/198、已验收157/198；
    内部workpack118/146，即`18 assembly_exact + 100 platform_adapted + 28 pending_audit`。

- 剧情VM P2第一百一十九组`0x0042C858` / opcode150完成独立闭环。机器分阶段读取signed i16
    X/Y并左移4，恢复两个不可达sentinel原始bug、X/Y signed夹值、targetX发布、targetY保持及双速度
    清零；复用普通世界ANI follower实际owner，缺binding在第一写点typed-stop。线性资产0记录/0 probes，
    以asset absence、四alias、分阶段截断、原始bug和精确尾锁定。Story VM 3/3、Linux core186/186
    与app192/192通过；workpack双生成稳定hash为
    `6091d22d00690aea93bf1422d19e27aa2db51f97a63494c5d0860f69e4407231`。未启动游戏EXE。
    人工语义增至158行，现代显式opcode增至161；对外进度为已实现161/198、已验收158/198；
    内部workpack119/146，即`18 assembly_exact + 101 platform_adapted + 27 pending_audit`。

- 剧情VM P2第一百二十组`0x0042C8F8` / opcode151完成独立闭环。机器依次读取四个signed i16，
    两个目标坐标左移4后提交，两个速度仅符号扩展后提交；current坐标保持。复用普通世界ANI follower
    实际owner，缺binding在第一写点typed-stop。线性资产0记录/0 probes，以asset absence、四alias、四阶段
    截断、signed极值和精确尾锁定。Story VM 3/3、Linux core186/186与app192/192通过；workpack双生成稳定hash为
    `6d5b83573e5bd837bb0f60cab36b976995b70e98a8d8eac894749613a21d745c`。未启动游戏EXE。
    人工语义增至159行，现代显式opcode增至162；对外进度为已实现162/198、已验收159/198；
    内部workpack120/146，即`18 assembly_exact + 102 platform_adapted + 26 pending_audit`。

- 剧情VM P2第一百二十一组`0x0042C936` / opcode152完成独立闭环。机器按currentX/targetX
    优先于currentY/targetY短路比较；任一轴未到达时原地等待，两轴到达才+2，但三路均发布previous152、
    audio一次并yield，完成路不same-call。复用普通世界ANI follower实际owner，缺binding在首读typed-stop。
    线性资产0记录/0 probes，以asset absence、四alias、两级短路、等待/完成及双精确尾锁定。Story VM 3/3、
    Linux core186/186与app192/192通过；workpack双生成稳定hash为
    `076621f2f562364e9303d8c90993ef87a7bbb2686a585ad1ae05d017cfd76adb`。未启动游戏EXE。
    人工语义增至160行，现代显式opcode增至163；对外进度为已实现163/198、已验收160/198；
    内部workpack121/146，即`18 assembly_exact + 103 platform_adapted + 25 pending_audit`。

- 剧情VM P2第一百二十二组`0x0042C95B` / opcode155完成独立闭环。map22保持debug-only
    no-op；其他地图依次提交受控角色完整dword tileX/tileY及mapID，再以固定参数同步reload，失败保留三写。
    共享load request扩为u32 tile，MAPS/preload显式截低16，物化后完整坐标覆盖且不重建空间绑定，保留原始
    陈旧绑定。线性资产0记录/0 probes，以asset absence、四alias、map22、失败、共享loader宽度和精确尾
    锁定。Story VM 3/3、runtime-session 2/2、Linux core186/186和app192/192通过；workpack双生成稳定hash为
    `b7916c286045b72244dffc5970768c3577125037b45174c398a14597eac07248`。未启动游戏EXE。
    人工语义增至161行，现代显式opcode增至164；对外进度为已实现164/198、已验收161/198；
    内部workpack122/146，即`18 assembly_exact + 104 platform_adapted + 24 pending_audit`。

- 剧情VM P2第一百二十三组`0x0042C9CE` / opcode156完成独立闭环。deferred map signed非正值
    保持debug-only no-op；正值以完整dword tile和受控角色action同步reload，helper成功返回后才清理
    deferred三项，失败完整保留。唯一线性TALK记录/1 probe、四alias、signed门、reload时序、失败和
    精确尾通过。Story VM 3/3、Linux core186/186和app192/192通过；workpack双生成稳定hash为
    `3d0d0a3dd14d56fb5da99ffac94d7dd8effa5ad8cfd2af1569516096f4c9cf10`。未启动游戏EXE。
    人工语义增至162行，现代显式opcode增至165；对外进度为已实现165/198、已验收162/198；
    内部workpack123/146，即`18 assembly_exact + 105 platform_adapted + 23 pending_audit`。

- 剧情VM P2第一百二十四组`0x0042CA3A` / opcode157完成独立闭环。map22只读取map word，
    保持deferred状态并跳过X/Y；其他map按map→X→Y逐项i16符号扩展立即提交，截断保留前序写。
    两路固定物理+8、previous157并same-call，无audio。线性资产0记录/0 probes，以asset absence、
    四alias、符号边界、未读尾、三级截断和双精确尾锁定。Story VM 3/3、Linux core186/186、
    app192/192通过；
    workpack双生成稳定hash为`f0fc54a38745763a3b8972d652c4c55416714e75c91ab467e4fd93914c173850`。
    未启动游戏EXE。人工语义增至163行，现代显式opcode增至166；对外进度为已实现166/198、
    已验收163/198；内部workpack124/146，即`19 assembly_exact + 105 platform_adapted +
    22 pending_audit`。

- 剧情VM P2第一百二十五组共享`0x0042CA7C` / opcodes158–159完成独立闭环。两路忽略
    自身`+2` word，从`+4`扫描`%Q`文件名，并读取下一指令`+2`作为Video/Music/root类别；
    158覆盖复制data→launch，159只删除source，文件API失败均忽略。marker后先触发host-frame门、
    更新IP，再lookahead和文件操作，最后previous并same-call，无audio。SDL以配置data root、
    launch directory和显式焦点后台运行例外承接CD/Win32路径、固定buffer与激活门。两opcode均
    线性资产0记录/0 probes，以asset absence、八alias、内嵌NUL、lookahead截断、API失败和精确尾
    锁定。Story VM 3/3、Linux core186/186及app192/192通过；workpack双生成稳定hash为
    `877ad3234198000501c55b2a47de08c99da86766ab01b8539a3a6ee899e8b110`。未启动游戏EXE。
    人工语义增至165行，现代显式opcode增至168；对外进度为已实现168/198、已验收165/198；
    内部workpack125/146，即`19 assembly_exact + 106 platform_adapted + 21 pending_audit`。

- opcode160前置审计发现已提交共享对话handler把`dword_4CF73C`误映射为水平居中bool，
    实际LST只在对话record flags处与1精确比较：非1置bit18，1抑制，成功排队后清零。
    已独立删除无依据的半宽扣减，恢复完整dword one-shot、失败保留及默认bit18；Story VM 3/3、
    Linux core186/186及app192/192通过，完整门lifecycle exit 0。此fix不实现或验收opcode160，
    公开进度与workpack保持
    已实现168/198、已验收165/198及125/146。

- 剧情VM P2第一百二十六组`0x0042CBB0` / opcode160完成独立闭环。机器无operand，
    把完整下一对话控制dword写1，推进2、发布previous160并same-call；无audio或yield。
    下一次成功对话抑制record bit18并清零该值，对话排队前失败则保留。558条真实记录/
    558 probes全部基础raw、长度2；三类后继链均在最多两条中间指令后进入对话，两条代表
    链真实回放通过。Story VM 3/3、Linux core186/186及app192/192完整门均以exit0通过；
    workpack双生成稳定hash为`cf60317b840a18c4c554d27f63587c42102d3cab33149f0f3989834ad7d2e007`。
    未启动游戏EXE。
    人工语义增至166行，现代显式opcode增至169；对外进度为已实现169/198、已验收166/198；
    内部workpack126/146，即`20 assembly_exact + 106 platform_adapted + 20 pending_audit`。

- 剧情VM P2第一百二十七组`0x0042CBCC` / opcode161完成独立闭环。机器在读取signed
    story ID前service audio，再由唯一helper切换TALK文件、目录项和窗口；成功共四次audio，
    新IP归零、previous161并same-call，源IP不加4。现代RAII资源port保持窗口未覆盖尾；Win32
    打开失败的同步退出和陈旧物理指针域以保留前两次audio的typed load failure承接。90条真实
    记录/90 probes全部基础raw、长度4；89个目标ID映射TALK2/3/4为87/2/1，全部目标首opcode
    为1026。真实story2037回放同调用进入TALK2并提交sound193。Story VM 3/3、Linux core
    186/186和app 192/192通过。workpack双生成稳定hash为
    `963960243d29c9674623319eff54bd274344218463faf2cf17c0323bdc8c2052`。未启动游戏EXE。
    人工语义增至167行，现代显式opcode保持169；对外进度为已实现169/198、已验收167/198；
    内部workpack127/146，即`20 assembly_exact + 107 platform_adapted + 19 pending_audit`。

- opcodes163–164前置审计发现共享current logical map owner被错误收窄为u16，且已提交
    opcode155把机器固定的map22/tile59重载误译为current map/deferred tile。已独立把runtime与
    session load request恢复为完整u32 map，SDL取消截断；opcode155继续完整保存deferred返回信息，
    但按LST固定重载22/59/59；opcode156完整转发正deferred map dword；opcode62在MAPS物理边界
    显式保持current map低16继承与比较。Story VM 3/3、MAPS/preload/post-materialization/
    runtime-session依赖8/8、Linux core186/186及app192/192完整门均通过。该fix不实现或验收
    opcodes163–164，
    公开进度与workpack保持已实现169/198、已验收167/198及127/146。

- 剧情VM P2第一百二十八组`0x0042CBFF` / opcodes163–164完成独立闭环。两个opcode先把
    map operand按i16符号扩展并与完整current logical map dword比较；163在不等时taken，164
    在相等时taken。taken-only读取u32 target，audio一次后同文件重载IP0、发布previous并
    same-call；not-taken不读target、不audio，固定推进8并same-call。27条真实记录/27 probes
    全部基础raw、长度8，163/164分布3/24，四库分布7/1/14/5，所有target首opcode均为1026；
    两种谓词代表回放通过。Story VM 3/3、Linux core186/186及app192/192完整门均通过。
    workpack双生成稳定hash为
    `02263faf78cebbbe41242f9da0cc75b876ed70a6c826121fad675aaa615363a2`。
    人工语义增至169行，现代显式opcode增至171；对外进度为已实现171/198、已验收169/198；
    内部workpack128/146，即`20 assembly_exact + 108 platform_adapted + 18 pending_audit`。

- 剧情VM P2第一百二十九组`0x0042CC35` / opcodes165–166完成独立闭环。handler按
    玩家masked链、64个角色完整ID root的顺序查询item，并把两类命中节点的两个signed数量
    相加；非零总和分别以`>=`/`<=`阈值决定重载，零总和保留165固定sequential、166固定
    reload的原始特例。taken-only读取u32 target并audio、同文件IP0重载、previous/same-call；
    not-taken固定+10 same-call。真实资产锁定TALK4三条opcode165、零条opcode166，三target
    均有效，首opcode为165/1026/1026，代表等号taken回放通过。Story VM 3/3、Linux core186/186
    及app192/192完整门均通过。workpack双生成稳定hash为
    `d87edbf824b295082f08eed8ad70f544c911b065733fd5c742ea2dadfa73bbdf`。
    人工语义增至171行，现代显式opcode增至173；对外进度为已实现173/198、已验收171/198；
    内部workpack129/146，即`20 assembly_exact + 109 platform_adapted + 17 pending_audit`。

- 剧情VM P2第一百三十组`0x0042CCF7` / opcodes170–173完成独立闭环。170/172分别
    清除mode17/18的nullable 52字节文本owner及剧情位，171/173扫描`%Q`变长文本，提交IP后
    释放旧owner、分配并清零新slot，以`lstrcpyA`首NUL语义复制并置对应剧情位。四路均发布
    previous、audio一次并yield；缺terminator和第52字节复制危险点按原副作用顺序typed-stop。
    真实资产21条/21 probes，170/171/172/173分布9/3/5/4，四库分布8/4/5/4，四种变体代表
    回放通过。Story VM 3/3、Linux core186/186及app192/192完整门均通过。workpack双生成稳定hash为
    `7dca629343e9a62f954f5e9927ea8c20860df2ae6e9352f8335627e7aaecd258`。
    人工语义增至175行，现代显式opcode增至177；对外进度为已实现177/198、已验收175/198；
    内部workpack130/146，即`20 assembly_exact + 110 platform_adapted + 16 pending_audit`。

- 剧情VM P2第一百三十一组`0x0042CDED` / opcode175完成独立闭环。handler对完整ANI
    control dword只OR bit4，写actual activity owner后固定+2、发布previous175并same-call；
    不audio、不yield，其他31位和已置位幂等语义保持。现代ANI flags恢复u32，并通过窄port
    连接SDL实际owner。完整线性资产为0条，以absence锁定；四raw alias、高位保持、写序和
    精确尾通过。ANI owner依赖与Story VM共4/4、Linux core186/186及app192/192通过。
    workpack双生成稳定hash为`e8bcf2e610f7288501dd87532ea6bf7be0b0fe324c398694cffa032efbf75436`。
    人工语义增至176行，现代显式opcode增至178；本地进度为已实现178/198、已验收176/198；
    内部workpack131/146，即`21 assembly_exact + 110 platform_adapted + 15 pending_audit`。

- 剧情VM P2第一百三十二组`0x0042CE12` / opcode176完成独立闭环。handler对完整ANI
    control dword只清bit4，写actual activity owner后固定+2；因未置same-call carry，common join
    发布previous176、audio一次并yield，不读取后继。其他31位和已清幂等语义保持。完整线性
    资产为0条，以absence锁定；四raw alias、高位保持、写序、audio序和精确尾不fetch通过。
    ANI owner依赖与Story VM共4/4、SDL app编译、Linux core186/186及app192/192通过。
    workpack双生成稳定hash为`5410e17b1737a80b1281dc5e87a5dd66063125fe5429651c2ebbc16787732dd5`。
    人工语义增至177行，现代显式opcode增至179；本地进度为已实现179/198、已验收177/198；
    内部workpack132/146，即`22 assembly_exact + 110 platform_adapted + 14 pending_audit`。

- 剧情VM P2第一百三十三组`0x0042CE32` / opcode177完成独立闭环。handler恢复raw bit15
    两相队伍集合协议：setup在原IP提交脚本/dialog/player/history/party角色状态后audio-yield；
    poll逐项清已到达成员状态，只有`matched+1`精确等于live party count才清marker并推进2，
    等待和完成都previous、audio一次并yield。现代复用actual角色、dialog、party count和玩家
    历史owner；缺history/count在原访问点保留前序副作用后typed-stop。29条线性记录/29 probes
    全部raw base，TALK3/4分布1/28；两条代表setup→complete回放及四alias synthetic通过。
    player-history、party transfer与Story VM共5/5、SDL app编译、Linux core186/186及
    app192/192通过。workpack双生成稳定hash为`a97360fa0c39ea5f3bb6ee2d998e6051997e7abcc92cfcaf8a73f93b5f0bd1f1`。
    人工语义增至178行，现代显式opcode增至180；本地进度为已实现180/198、已验收178/198；
    内部workpack133/146，即`22 assembly_exact + 111 platform_adapted + 13 pending_audit`。

- 剧情VM P2第一百三十四组`0x0042CF7C` / opcode178完成独立闭环。handler读取u16角色
    selector；FFF0替换为controlled index低word后仍执行ordinary GUID lookup，FFFE保留helper
    原生controlled选择。命中只置实际role flags的路径碰撞绕过位，miss静默；固定+4、previous178
    并same-call，无audio。6条线性记录/6 probes全部base raw，TALK1/3/4分布1/3/2，四种
    selector全量精确尾回放通过；四raw alias、幂等、FFF0/FFFE、bit28过滤和截断synthetic通过。
    role lookup与Story VM共4/4、SDL app编译、Linux core完整门186/186与app完整门192/192通过。workpack双生成
    稳定hash为`4f81a8737a17d648f1a87d8051cfdb84102ab84ece8e9a698cc9fedf890487a4`。
    人工语义增至179行，现代显式opcode增至181；本地进度为已实现181/198、已验收179/198；
    内部workpack134/146，即`23 assembly_exact + 111 platform_adapted + 12 pending_audit`。

- 剧情VM P2第一百三十五组`0x0042CFBD` / opcode179完成独立闭环。handler按机器顺序
    读取四项signed operand，复用actual framebuffer deformation list与进程CRT RNG，构造固定
    640×480 surface、脚本半径双倍field和相对origin，以固定半径24注入后才头插发布；固定+10、
    previous179并same-call，无audio。线性TALK为0条，以asset absence锁定；四raw alias、signed
    边界、固定注入半径、严格边缘、分阶段截断、actual owner、非法geometry和精确尾synthetic
    通过。frame-deformation、world-frame consumer与Story VM共5/5、SDL app编译、Linux
    core完整门186/186与app完整门192/192通过。workpack双生成稳定hash为
    `9540ef1aca0c6f0d8fb01aa40d8b41f01cd97209e5a04e8ecc96f04cf67c62a7`。
    人工语义增至180行，现代显式opcode增至182；本地进度为已实现182/198、已验收180/198；
    内部workpack135/146，即`23 assembly_exact + 112 platform_adapted + 11 pending_audit`。

- 剧情VM P2第一百三十六组`0x0042D041` / opcode180完成独立闭环。handler直接把
    actual process-level完整u32主帧执行门覆盖为0，再固定+2、发布previous180、audio一次并
    yield，不读取后继。现代复用F8、frame dispatch与runtime integration的同一owner；缺binding
    在原写点typed-stop。线性TALK为0条，以asset absence锁定；四raw alias、高位清零、幂等、
    写序、精确尾和owner边界synthetic通过。三个主帧门依赖与Story VM共6/6、SDL app编译通过，
    Linux完整门core 186/186、app 192/192通过。workpack双生成稳定hash为
    `5d4d702871e4796f6107f08952e4a63bff89bcd1dd0989ae7fb486b2c886d47e`。
    人工语义增至181行，现代显式opcode增至183；本地进度为已实现183/198、已验收181/198；
    内部workpack136/146，即`23 assembly_exact + 113 platform_adapted + 10 pending_audit`。

- 剧情VM P2第一百三十七组`0x0042B070` / opcodes181–185完成独立闭环。五路复用
    actual 64项完整u32脚本变量owner，恢复宽值set/add/sub-sign-clamp和unsigned GE/LE同文件
    条件重载；保留value/target读取顺序、variable0共享clamp、高index原地audio-yield重试、
    negative index首个unsafe点typed-stop，以及8/12字节same-call尾。唯一真实opcode184记录的
    30,000,000阈值not-taken/equality-taken双向回放通过；其余四opcode以asset absence与全alias
    synthetic锁定。Story VM 3/3与SDL app编译通过，Linux完整门core 186/186、app 192/192通过。
    workpack双生成
    稳定hash为`467534bf117feb1d21779a4bc148244f50d71959c21c01e3579af54cff8ba895`。
    人工语义增至186行，现代显式opcode增至188；本地进度为已实现188/198、已验收186/198；
    内部workpack137/146，即`23 assembly_exact + 114 platform_adapted + 9 pending_audit`。

- 剧情VM P2第一百三十八组`0x0042D05C` / opcodes186–187完成独立闭环。两路固定读取
    第二项0x38 party-member记录的17字段getter，按signed GE/LE与u16 threshold决定同文件重载；
    恢复0–5 i16、6–13 u16、14–15 i32及16 u8宽度，negative selector default0、高selector
    原地audio-yield重试、not-taken未读target的+10 same-call，以及taken loader audio+common audio
    后yield。现代把既有资源子集owner补全为中性0x38字段owner，B10/B11真实加载仍不伪造；
    两opcode线性资产均为0，以asset absence与全alias synthetic锁定。Story VM 3/3与SDL app编译
    通过，Linux完整门core 186/186、app 192/192通过。workpack双生成稳定hash为
    `e4b44778de22f0027ae4bd0197c1a9d904ccec4d86c5daf85c35b2fefc119ad2`。
    人工语义增至188行，现代显式opcode增至190；本地进度为已实现190/198、已验收188/198；
    内部workpack138/146，即`23 assembly_exact + 115 platform_adapted + 8 pending_audit`。

- 剧情VM P2第一百三十九组`0x0042D0D8` / opcodes188–190完成独立闭环。三路固定
    操作第二项0x38 party-member记录的17字段setter，恢复direct set、getter+i32 wrapping add/sub、
    0–13低u16、14–15完整u32及16低u8目标宽度；保留negative selector getter/setter default、
    high selector已读value后的原地audio-yield重试，以及+6、previous、same-call无audio。
    field16先写低byte，再按group2/result+1访问LEVEL.DAT，成功才覆盖field14；现代以B10可失败
    窄port承接，SDL明确返回非致命失败，不伪造LEVEL值。三opcode线性资产均为0，以absence与
    全alias synthetic锁定。Story VM 3/3、SDL app编译、Linux core 186/186与app 192/192完整门
    全部通过。workpack
    双生成稳定hash为`0850aaac5b64af4710ff9331cf6740834f210190dc911bee18c35699bfc7ae55`。
    人工语义增至191行，现代显式opcode增至193；本地进度为已实现193/198、已验收191/198；
    内部workpack139/146，即`23 assembly_exact + 116 platform_adapted + 7 pending_audit`。

- opcode191前置复核发现已提交opcode51等待分支漏掉common-join audio。LST从函数入口
    `ESI=0`、首轮fetch保存`var_28=0`，四个camera pan字段任一非零时直接到共同join；
    `var_28 | ESI`为零，必经`0x0042D4D7 _AIL_serve`一次后yield。现已补齐等待路
    previous→audio→yield，完成路仍保持+2、previous、same-call无audio；synthetic四字段、
    四alias、精确尾和真实TALK1等待→完成链均已更新，Story VM 3/3通过。公开实现/验收和
    workpack计数不变；双生成hash更新为
    `7e09e969ec860b7e6c16d2e78df427e755c2c504041df61019eec727a8e65155`。Story VM 3/3、
    Linux core 186/186与app 192/192完整门全部通过。

- 剧情VM P2第一百四十组`0x0042D170` / opcode191完成独立闭环。handler先按
    remaining X→Y短路判断camera pan活动，仅active时读取i16 expected top并与完整viewport top
    dword比较；无移动或相等固定+4、previous、same-call无audio，不等则原地previous、audio一次
    并yield。camera step不参与；无移动不读operand也不访问viewport，active按pan owner→operand→
    viewport owner顺序。13条真实记录/13 probes全部位于TALK4、base raw、operand800..4640，
    equality/mismatch全量回放通过；四alias、signed边界、owner顺序、未读operand和精确尾synthetic
    通过。Story VM 3/3、SDL app编译、Linux core 186/186与app 192/192完整门全部通过；
    workpack双生成稳定hash为
    `c2b315e9c30150665c280eaea16653a05853358ef93f0b573326640cde6ba914`。人工语义增至192行，
    现代显式opcode增至194；本地进度为已实现194/198、已验收192/198；内部workpack140/146，即
    `23 assembly_exact + 117 platform_adapted + 6 pending_audit`。

- 剧情VM P2第一百四十一组`0x0042D1AA` / opcode192完成独立闭环。handler复用
    actual完整u32 transition mode与current fade divisor，先短路mode恰好等于2，再检查current
    是否非零；任一等待条件成立则不推进，否则+2。三路均发布previous192、audio一次并yield，
    完成路不fetch后继；pending divisor和stream backend均不访问，所有音乐状态保持。线性资产
    为0，以absence与四alias、完整u32比较、短路、状态保持及精确尾synthetic锁定，归
    `assembly_exact`。Story VM 3/3、SDL app编译、Linux core 186/186与app 192/192完整门
    全部通过；接入192后把opcode134
    synthetic的固定unsupported后继改为`OP_1025`，134行为断言不变。workpack双生成稳定hash为
    `af1adc822dd7e246382ef33b72d0017ffbb5e501758d4bd4d8653d576dc8fe89`。人工语义增至193行，
    现代显式opcode增至195；本地进度为已实现195/198、已验收193/198；内部workpack141/146，
    即`24 assembly_exact + 117 platform_adapted + 5 pending_audit`。

- 剧情VM P2第一百四十二组`0x0042D1D5` / opcode193完成独立闭环。handler调用
    已接actual `LegacyVideoPlayer::legacy_progress()`的窄port恰好一次；progress非负原地发布
    previous193、audio一次并yield，负值则+2、previous、same-call无audio。恢复旧裸case两路
    漏发previous及active路漏audio；query时序、0/正值、-1/INT_MIN和两类精确尾synthetic通过。
    唯一真实记录`TALK1.DAT@0x0000450E`完成active/inactive双向回放，Story100长链继续查询一次。
    Bink wrapper由typed backend承接，归`platform_adapted`。Story VM与audio-video依赖4/4、SDL
    app编译、Linux core186/186与app192/192完整门通过。workpack双生成hash为
    `df4cc9cff50b34a98ebaef859f54b241ddb27a58ad43cf7f42df13e1e7af7ccd`。人工语义增至194行，
    现代显式opcode仍为195；本地进度为已实现195/198、已验收194/198；内部workpack142/146，
    即`24 assembly_exact + 118 platform_adapted + 4 pending_audit`。

- special opcode1024预审发现并独立校正已提交handler的common join audio。机器
    `0x0042B0AE`在`var_28|ESI==0`时固定发布previous、调用`_AIL_serve`一次并返回；旧C++
    最终确认26个handler组/31个opcode存在漏最终audio。新增统一
    `yield_from_common_join`窄helper，只接确实到达共同出口的路径；opcode85恢复显式+common
    两次audio，opcode96成功恢复两次内部+common共三次，CD preflight仍按`0x0042D4B6`
    一次audio且不发previous；186/187已正确两次，未误加第三次。后续全入边枚举又确认opcode135
    在`0x0042C3ED`内部audio后仍经common join，现恢复previous后第二次audio并锁定两次callback
    顺序。synthetic逐handler、真实资产和15/16/17/23/111/161长链累计计数已校正，Story VM 3/3、
    SDL app编译、Linux core186/186及app192/192通过。workpack计数不变，26行补
    `audio_service_tested`；双生成hash为
    `4b0b06c1df4bb912ea01bebf99be0799917c6c306ef14f6af09dcc5c35be935e`。公开进度保持
    已实现195/198、已验收194/198；内部workpack142/146。

- 剧情VM P2第一百四十三组`0x0042D200` / special opcode1024完成独立闭环。handler固定
    双指针+2并把调用期`var_28`置1；该latch不同于每fetch清零的ESI，会使本次调用后续全部
    common join持续发布previous、跳过common audio并same-call，直到1025清除或调用结束。
    现代用step栈内bool承接，并把全部64个共同出口统一接入窄helper；入口对齐门与opcode96 CD
    preflight仍绕过common。四alias、多个连续common join、下一step不泄漏、opcode135内部audio
    保留、非推进等待的原无限域和精确尾通过；4096 dispatch guard作为既有typed-stop隔离无限域，
    因而归`platform_adapted`。线性资产0条/0 probes。Story VM 3/3、SDL app编译、Linux core
    186/186与app 192/192通过。workpack/runtime-path双生成hash分别为
    `856190c62941e0c0af81d89381357dda47133ef4d131abb8f42aa1ad7d9d7f98`、
    `244f0d55c0ccd038e9391aba2d394161673d27b002864e4a08edf858f34daf3f`。本地进度为
    已实现196/198、已验收195/198；内部workpack143/146，即`24 assembly_exact +
    119 platform_adapted + 3 pending_audit`。
