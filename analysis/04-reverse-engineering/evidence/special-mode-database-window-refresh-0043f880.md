# 标准模式数据库窗口刷新 `0x0043F880`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。范围`0x0043F880..0x0043F93C`，98行；六个caller为DD20、DDF0、DED0、DFA0、E080、E170。callee B980、B9A0、BB40、BC90、F160、F940均已关闭；44D5D0继续保留分配边界。

## 控制流

入口EAX严格为`interaction_phase-1`。不为0时不调用任何helper并直接返回其32位结果。phase1顺序为：

1. `direction_selection`（FCAB8）为0/1时调用F940边界，源分别为FCBA8/FCC58，第三参数按32位环绕计算`window_offset+list_selection`；大于1时跳过。
2. forward head为空时调用44D5D0分配边界并写回。
3. B980统计完整forward count。
4. F160按u16 key排序，并保持相同key前插导致的反转语义。
5. B9A0按window offset得到current head。
6. BC90以16为上限写visible count和尾节点。
7. BB40归一化local cursor/window offset，并直接传播其路径相关EAX。

新增`refresh_legacy_standard_mode_database_window`结果合同，记录ignored/refreshed路径、helper数、F940调用和空head分配。F940现直接接收32位环绕绝对索引并传播typed-stop；44D5D0由独立分配port承载。FCAB8复用既有`direction_selection` owner，与E250/E310及E800保持同一真值，没有新增平行状态。

六caller均已从collapsed port调用改为直接typed helper调用；随后仍按原顺序执行F1E0及sample。UT覆盖phase非1早退、source0重建、source>1跳过、空head分配、完整count/sort/window链和BB40返回；既有page及DA30用例修正为观察F160相同key反转和F880后续归一化。

定向测试通过。workpack双生成稳定为`64/227`，SHA256均为`64f495976602fee78a768203f323533d2e0ada917ef76e9b97851096b4e89381`；下一单元`0x0043F940`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
