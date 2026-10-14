# 入战队员物品与姓名引用接线

范围：B11读档后入战的逐角色引用发布。B10保持315/422，
Workpack316和417均保持pending_audit；不关闭新的函数。

## 原版顺序

权威LST：451B10中的452287..45230C；46E870..46E880、
46E850..46E860、483FE0..483FEA。独立分析先记录于
`build/tmp/runtime/battle-startup-party-references-audit.md`。

- 452287先读人数；零人数不进入循环。角色地址为5029D0加索引乘2F34。
- 452296读4A75C8映射，4522BD调用属性聚合。
- 4522C2重读映射，4522CB读4A9490对应队伍根值。
- 46E870依次写角色2EC0、2EC4。零根或未知根值也照写，不解引用。
- 46E850依次写角色2EC8、2ECC，参数是固定玩家根地址4A9940，
  不是该地址中当前保存的根值。
- 4522E4再读映射，以49E148加来源乘16计算姓名地址。
  483FE0只写角色2560，不复制、读取或解码姓名内容。
- 4522FC重读人数，再继续下一角色。不得先聚合所有角色再批量绑定引用。

三叶子的参数都是完整DWORD，返回EAX为输入、ECX保持角色地址，
EDX和flags保持，callee弹出4字节。两次根写之间没有外部调用。

## 实现与存储

`bind_legacy_battle_startup_party_references`由核心startup和SDL调用，
均放在每名角色属性成功之后。删除请求中的party_values副本和姓名opaque调用。
来源超出四项物品根表时在读表位置停止，保留属性前缀；不进入引用发布和下一角色。
姓名映射在两个不改变映射的叶子后重读；姓名源仅借用span。

双写器增加引用view，原State接口委托同一实现。SDL把value view绑定到
既有动作current_list_index/next_list_index，把resource view绑定到
既有actor_list的resource_head_token/next_resource_head_token。
不向核心独立测试用pair holder复制第二份值。
核心standalone路径继续使用自身已有pair holder。

姓名token存入角色记录。SDL前两项字节借用initial_menu_state的两个姓名数组，
与剧情替换同源；后两项借用saved_role_names_的20/30偏移，各16字节。
读档在原对象上恢复这些字节，span不拥有存储，不复制姓名。

## 双向复核与验证

从循环入口逐项核对人数门、属性失败、映射重读、根值读取、两组双写、
姓名地址发布和下一角色。反向检查新增view只借用字段；范围检查对应
不可映射访问的typed-stop；span是宿主字节借用，不增加原版字符串读取。
零角色token的既有叶子停止合同和返回寄存器未改变。

固定状态测试覆盖非连续映射、诊断后切换映射、前一角色引用先于下一诊断、
诊断后无效来源保留41个主资料DWORD且阻断后续、完整未知根值、零根、
玩家根地址和值区别、姓名字节后续修改可见及再次绑定更换来源。
原有startup和叶子测试继续执行。

最终进程proc_0387退出0：core和ASan定向battle.legacy_battle_setup各1/1通过；
Linux SDL应用链接通过。日志：

- `build/tmp/runtime/party-reference-final-core.log`
- `build/tmp/runtime/party-reference-final-asan.log`
- `build/tmp/runtime/party-reference-final-sdl.log`

未启动游戏；没有新增原版动态差分或实机验证。

## 尚未完成

原始静态姓名49E148..49E187共64字节，后两项默认字节分别为
A5 64 BA BF、A7 F5 B9 74。saved_role_names_目前默认全零，
新游戏和未恢复扩展时的默认姓名初始化仍有既有缺口；本批验证已恢复姓名的借用。
完整指标、敌方进度、帧、返回和实际续玩仍待完成。
