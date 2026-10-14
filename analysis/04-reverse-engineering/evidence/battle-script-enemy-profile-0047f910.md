# 战斗脚本复制敌人坐标时读取实际角色参数

状态：`pending`。

源码：`1f4e6749626cb9ee35f45753d0f631c9b7cf3590`。
脚本50这一处读取已回收，脚本、行动和最终角色父协议仍pending。

## 指令与实际记录

叶`0047F910..0047F917`只有`mov ax,[ecx+2A0C]`与`retn`。
它读取WORD，只替换EAX低WORD；ECX、EDX、flags及EAX高WORD保持。
没有业务成功值或回调错误合同。

脚本50的`0046CD3C..0046CDCD`先MOVSX选择WORD并计算敌人地址，
`0046CD58`读参数，`0046CD6C`写共享`53CE92`即packed_value_a高WORD。
随后`0046CD72`读取敌人0坐标，`0046CDA8`向所选敌人发布坐标和8 DWORD，
最后推进游标4并清共享高WORD。低WORD始终保留，不能把所选敌人当坐标来源。

本批直接借`startup.group_b_lifecycle[index].action_execution.profile_value`。
既有reset/raw镜像和frame snapshot均把同一字段映射到`+2A0C`，
没有新增记录副本、查询端口或包装函数。脚本47F910枚举及模拟回复已删除。

后继坐标查询的入口EAX来自实际参数和选择地址算术，EDX为选择乘345，
flags保持地址算术结果。旧测试注入的任意寄存器回复不能保留。
有效选择0..7的地址算术EAX高WORD为0；参数8000/FFFF仍只是WORD，不能符号扩展。

选择越界保持既有group_b_actor_typed_stop。生命周期存储允许缺失，
角色也已有profile_value_read_accessible访问标记；两种失败在首个实际读取
报告同一既有状态，保留地址前缀并阻断共享WORD与坐标后缀。
没有新增状态类型、访问模拟字段或默认成功值。

## 验证

- 新增60次执行：8槽乘6种WORD值的48组、两次实时重读、6种非法选择、
  2种operand短读、缺失存储和既有字段访问失败。
- 全部有效槽均从敌人0复制坐标；所选参数、相邻记录及共享低WORD保持。
  成功后才清高WORD，实际参数改写在下次执行立即可见。
- 原坐标读取六类逐访问失败和坐标发布失败改观测实际参数与共享高WORD，
  保留部分坐标、游标及未清除参数的失败前缀。
- 首轮测试通过后复核发现未消费既有访问标记；补齐后重跑最终门禁，
  首轮结果不代替最终源码的验证。
- 最终core角色帧1/1通过，26.07秒；ASan同目标1/1通过，26.89秒。
  SDL实际重新链接，三份源码/测试身份一致，最终无警告或错误。

日志：`build/tmp/runtime/script-enemy-profile-{core,asan,sdl}-final.log`。
身份：`script-enemy-profile-formatted-source.sha256`；
指令与正反向审查：`script-enemy-profile-lst-analysis.md`及
`script-enemy-profile-{source,tests}-review-final.diff`。
清单重复生成、发布身份与顺序核对见`script-enemy-profile-ledger-check.log`。
首次清单检查的宽正则误把thiscall声明计为调用；改为只匹配带机器码的call行，
六个实际地址逐项核对，不以放宽预期数量消除报错。

## 未完成边界

LST共有六处物理调用：00454665、004546F1、00454736、0045AD2E、
00466AC8及本批0046CD58。行动和最终角色的通用查询尚未回收，
摘要所在完整父链仍需核对；不能由本批推断所有2A0C消费者已验收。
脚本其他操作枚举、参数包、寄存器报告、轨迹、计数、Port及资源协议仍在。
全仓范围完整性、B11、Workpack 316和实际游戏差分未验收，保留分类数不增加。
