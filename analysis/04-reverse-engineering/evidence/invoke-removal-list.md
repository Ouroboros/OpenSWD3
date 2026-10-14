# INVOKE 逐项迁移清单

## 使用规则

- 这是逐项执行清单。定义及行号基于建表快照；后续进展记录在对应条目。固定ID不随排序变化。
- 方框只在实际调用链完成迁移、LST复核、相关测试和构建通过、提交推送后勾选。
- 每次只实现一个可独立验证批次。子切片发布只能更新该条断点，不能勾选整条。
- 未命中关键词不等于完成。纯转发、继承协议、资源所有权仍须逐项人工复核。
- 平台边界和合法业务操作不能仅按名字删除；保留必须填写具体理由与证据。
- 旧台账的verified/excluded单独保留。新快照未复验，不抹去历史，也不冒充当前通过。
- 下列文件路径是定义/实现范围。逐个类型的所有生产与测试符号引用在[Port清单](invoke-removal-ports.md)。
- 符号引用是调用方导航，包含声明和同名引用；还需核实虚调用、继承和别名接线。
- 每次发布更新本条：已删协议、剩余协议、测试日志、提交、远端验证。
- 自动生成器只用于建立快照；已开始人工更新后不得直接覆盖勾选和验收记录。

## 建表基线

- HEAD：`d16d3e675e5f5c996e3357846fb6d2d92974ad88`；包含未提交行查询改动，该批未验证。
- 生产文件：874；每个文件均列入清单及SHA256快照。
- 旧台账：324族；状态分布：`{'pending': 293, 'verified': 27, 'excluded': 4}`。
- 本清单：537条，其中213条补充覆盖旧台账之外的生产文件。
- Port/Ports定义：345；含Dispatch/Adapter共358个类型定义。
- 上述数值分别是族、文件和类型的数量，不能相加或作为同一完成率分母。
- 全项目语义覆盖尚未验收。所有新清单方框保持未勾选。
- 当前顺序：行查询批次f21aa0b1已推送并核对远端，沿角色列表条目继续剩余协议。

## 逐项 LIST

- [ ] **CHAIN-e0f6819f24dc** `battle/legacy_battle_actor_list_query`
  - 本批：普通资源行查询直接写实际名称、flags和数量，删除request、寄存器副本及诊断计数；core/ASan定向门及SDL构建通过。
  - 证据：[行查询及唯一普通网格调用方](battle-actor-resource-list-query-00470910.md)。提交f21aa0b1已推送，远端main一致。
  - 本批选择：删除Request、寄存器/输出回包、调用计数和脚本开关，三个caller直接写实际DWORD/WORD；纠正4708C0错接、profile影子来源、分类位和重复flags_49。
  - 验证：真实资源/MON夹具、独立边界、四项core/ASan、SDL构建及源码身份检查通过；源码4b163e92已推送并核对远端main一致。见[资源选择](battle-actor-resource-selection-00470ac0.md)。
  - 剩余：同族状态、资料、选择诊断转发及资源生产；整链未验收，不勾选。
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_list_query.hpp](../../../include/openswd3/battle/legacy_battle_actor_list_query.hpp)；[src/battle/legacy_battle_actor_list_query.cpp](../../../src/battle/legacy_battle_actor_list_query.cpp)。
  - 类型：[PORT-e7050ee9a7d7](invoke-removal-ports.md#port-e7050ee9a7d7), [PORT-395776ccb4e9](invoke-removal-ports.md#port-395776ccb4e9)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_actor_list_query.hpp](../../../include/openswd3/battle/legacy_battle_actor_list_query.hpp)：95, 96, 120, 121, 163, 220, 221, 270, 274, 275, 287
  - `register` [include/openswd3/battle/legacy_battle_actor_list_query.hpp](../../../include/openswd3/battle/legacy_battle_actor_list_query.hpp)：73, 74, 102, 103, 104, 111, 112, 129, 130, 131, 138, 139, 148, 149, 150, 154, 155, 168, 169, 170, 174, 175, 176, 195, 196, 209, 210, 228, 229, 230, 277, 278, 279, 291, 292, 293, 331, 332
  - `counter` [src/battle/legacy_battle_actor_list_query.cpp](../../../src/battle/legacy_battle_actor_list_query.cpp)：73, 146, 232, 289, 363, 495, 523, 1222, 1300, 1442, 1463, 1468
  - `register` [src/battle/legacy_battle_actor_list_query.cpp](../../../src/battle/legacy_battle_actor_list_query.cpp)：64, 65, 66, 71, 74, 75, 76, 100, 124, 130, 131, 132, 151, 152, 153, 176, 177, 178, 181, 182, 183, 197, 198, 199, 201, 230, 254, 276, 339, 341, 355, 356, 357, 361, 364, 387, 404, 412, 413, 416, 417, 420, 425, 445, 458, 459, 460, 467, 468, 493, 496, 497, 498, 521, 524, 525, 526, 1114, 1115, 1116, 1135, 1204, 1236, 1261, 1279, 1333, 1428, 1429, 1430, 1432, 1440, 1443, 1484, 1501, 1532, 1543, 1547, 1551, 1561, 1569, 1574

- [ ] **CHAIN-cdfe45119708** `battle/legacy_battle_frame_coordinator`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)；[src/battle/legacy_battle_frame_coordinator.cpp](../../../src/battle/legacy_battle_frame_coordinator.cpp)。
  - 类型：[PORT-e59fcf9dbbd4](invoke-removal-ports.md#port-e59fcf9dbbd4), [PORT-c19e599fde1a](invoke-removal-ports.md#port-c19e599fde1a)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：246, 414, 433, 559, 618, 631, 633, 634, 635, 636, 687, 752, 807, 856, 988, 989, 991, 996, 1085, 1086, 1088, 1093, 1148, 1149, 1151, 1156, 1182, 1199, 1229, 1230, 1232, 1236, 1274, 1309, 1310, 1312, 1317, 1374, 1375, 1377, 1386, 1432, 1482, 1531, 1579, 1633, 1634, 1636, 1640, 1681, 1698, 1699, 1701, 1706, 1760, 1763, 1764, 1765, 1766, 1803, 1809, 1810, 1824
  - `counter` [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：1988, 1989, 1992, 1994, 1996, 1998, 2000, 2002, 2004, 2006, 2008, 2010, 2012, 2014, 2017, 2019, 2021, 2023, 2025, 2027, 2029, 2031, 2039, 2045, 2050, 2052, 2055, 2057, 2058, 2059, 2061
  - `invoke` [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：405, 412, 423, 429, 446, 556, 578, 615, 625, 628, 644, 684, 709, 749, 775, 804, 826, 853, 874, 993, 1047, 1090, 1125, 1136, 1145, 1153, 1179, 1196, 1211, 1234, 1272, 1286, 1314, 1339, 1383, 1405, 1429, 1448, 1479, 1503, 1528, 1550, 1577, 1612, 1638, 1678, 1693, 1703, 1718, 1758, 1787, 1801, 1821
  - `register` [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：247, 248, 249, 268, 269, 270, 415, 416, 417, 419, 434, 435, 436, 439, 440, 441, 560, 561, 562, 565, 566, 567, 575, 619, 620, 621, 623, 638, 639, 640, 642, 688, 689, 690, 696, 697, 698, 753, 754, 755, 761, 762, 763, 808, 809, 810, 816, 817, 818, 857, 858, 859, 865, 866, 867, 997, 998, 999, 1002, 1003, 1004, 1094, 1095, 1096, 1101, 1102, 1103, 1123, 1127, 1128, 1129, 1131, 1134, 1138, 1139, 1140, 1142, 1157, 1158, 1159, 1162, 1163, 1164, 1174, 1175, 1176, 1183, 1184, 1185, 1187, 1190, 1191, 1192, 1204, 1205, 1206, 1208, 1237, 1238, 1239, 1247, 1248, 1249, 1266, 1267, 1268, 1279, 1280, 1281, 1283, 1318, 1319, 1320, 1323, 1324, 1325, 1364, 1365, 1366, 1387, 1388, 1389, 1394, 1395, 1396, 1433, 1434, 1435, 1440, 1441, 1442, 1483, 1484, 1485, 1490, 1491, 1492, 1532, 1533, 1534, 1539, 1540, 1541, 1580, 1581, 1582, 1593, 1597, 1598, 1599, 1641, 1642, 1643, 1653, 1654, 1655, 1672, 1673, 1674, 1686, 1687, 1688, 1690, 1707, 1708, 1709, 1712, 1713, 1714, 1768, 1769, 1770, 1773, 1774, 1775, 1812, 1813, 1814, 1816, 1830, 1831, 1832, 1834
  - `counter` [src/battle/legacy_battle_frame_coordinator.cpp](../../../src/battle/legacy_battle_frame_coordinator.cpp)：164, 221, 235, 282, 297, 298, 362, 381, 418, 450, 468, 491, 507, 540, 580, 583, 596, 635, 650, 689, 754, 783, 826, 848, 868, 905, 928, 959, 981, 990, 1004, 1013, 1021, 1046
  - `register` [src/battle/legacy_battle_frame_coordinator.cpp](../../../src/battle/legacy_battle_frame_coordinator.cpp)：213, 214, 215, 232, 233, 253, 568, 569, 603, 606, 610, 749, 750

- [ ] **CHAIN-80c81fe30361** `battle/legacy_battle_grid_frame`
  - 本批：删除行查询操作转发和测试脚本回包，名称与数字共享40字节存储；core/ASan定向门及SDL构建通过。
  - 证据：[普通网格实际行查询](battle-grid-frame-004659c0.md)。提交f21aa0b1已推送，远端main一致。
  - 未决：字体及绘制Port、寄存器request/reply和其他诊断计数尚未迁移，整项不能勾选。
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_grid_frame.hpp](../../../include/openswd3/battle/legacy_battle_grid_frame.hpp)；[src/battle/legacy_battle_grid_frame.cpp](../../../src/battle/legacy_battle_grid_frame.cpp)。
  - 类型：[PORT-da997d756887](invoke-removal-ports.md#port-da997d756887)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_grid_frame.hpp](../../../include/openswd3/battle/legacy_battle_grid_frame.hpp)：32
  - `counter` [include/openswd3/battle/legacy_battle_grid_frame.hpp](../../../include/openswd3/battle/legacy_battle_grid_frame.hpp)：149, 150, 151, 152, 153, 161, 162, 163, 164
  - `invoke` [include/openswd3/battle/legacy_battle_grid_frame.hpp](../../../include/openswd3/battle/legacy_battle_grid_frame.hpp)：61
  - `register` [include/openswd3/battle/legacy_battle_grid_frame.hpp](../../../include/openswd3/battle/legacy_battle_grid_frame.hpp)：33, 34, 35, 42, 43, 44, 97, 98, 99, 107, 108, 109, 146, 147, 148
  - `arguments` [src/battle/legacy_battle_grid_frame.cpp](../../../src/battle/legacy_battle_grid_frame.cpp)：581, 599
  - `counter` [src/battle/legacy_battle_grid_frame.cpp](../../../src/battle/legacy_battle_grid_frame.cpp)：327, 354, 375, 393, 419, 478, 505, 543, 552, 586, 607
  - `invoke` [src/battle/legacy_battle_grid_frame.cpp](../../../src/battle/legacy_battle_grid_frame.cpp)：123, 124, 125, 243, 261, 279, 289, 362, 363, 450, 458, 558, 570, 571, 578, 579, 589, 597
  - `register` [src/battle/legacy_battle_grid_frame.cpp](../../../src/battle/legacy_battle_grid_frame.cpp)：99, 100, 227, 228, 394, 395, 396, 420, 421, 422, 564, 565, 566, 572, 573, 574, 582, 583, 584, 600, 601, 602, 611, 612, 613

- [ ] **CHAIN-617fcacf39ae** `battle/legacy_battle_selection_frame`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_selection_frame.hpp](../../../include/openswd3/battle/legacy_battle_selection_frame.hpp)；[src/battle/legacy_battle_selection_frame.cpp](../../../src/battle/legacy_battle_selection_frame.cpp)。
  - 类型：[PORT-62023efc7570](invoke-removal-ports.md#port-62023efc7570), [PORT-982d98cdcbdd](invoke-removal-ports.md#port-982d98cdcbdd)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_selection_frame.hpp](../../../include/openswd3/battle/legacy_battle_selection_frame.hpp)：109
  - `counter` [include/openswd3/battle/legacy_battle_selection_frame.hpp](../../../include/openswd3/battle/legacy_battle_selection_frame.hpp)：232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 259
  - `invoke` [include/openswd3/battle/legacy_battle_selection_frame.hpp](../../../include/openswd3/battle/legacy_battle_selection_frame.hpp)：142
  - `register` [include/openswd3/battle/legacy_battle_selection_frame.hpp](../../../include/openswd3/battle/legacy_battle_selection_frame.hpp)：110, 111, 112, 116, 117, 118, 175, 176, 177, 229, 230, 231
  - `arguments` [src/battle/legacy_battle_selection_frame.cpp](../../../src/battle/legacy_battle_selection_frame.cpp)：223, 228, 242, 260, 268, 275, 283, 297, 305, 320
  - `counter` [src/battle/legacy_battle_selection_frame.cpp](../../../src/battle/legacy_battle_selection_frame.cpp)：261, 276, 298, 321, 349, 468, 553, 725, 781, 839, 886, 936, 974, 1019, 1062, 1290, 1496, 1533, 1606
  - `invoke` [src/battle/legacy_battle_selection_frame.cpp](../../../src/battle/legacy_battle_selection_frame.cpp)：96, 108, 118, 220, 225, 239, 260, 265, 275, 280, 297, 302, 320, 482, 505, 597, 638, 640, 648, 655, 1347, 1360, 1398, 1412, 1460, 1509, 1521, 1550, 1620, 1636
  - `register` [src/battle/legacy_battle_selection_frame.cpp](../../../src/battle/legacy_battle_selection_frame.cpp)：79, 80, 197, 198, 199, 229, 230, 231, 233, 234, 235, 343, 344, 345, 350, 351, 352, 353, 377, 378, 382, 383, 384, 436, 437, 438, 441, 442, 443, 469, 470, 471, 548, 549, 550, 554, 555, 556, 706, 707, 708, 726, 727, 728, 748, 749, 750, 782, 783, 784, 804, 805, 806, 840, 841, 842, 862, 863, 864, 887, 888, 889, 905, 906, 907, 937, 938, 939, 953, 954, 955, 975, 976, 977, 993, 994, 995, 1020, 1021, 1022, 1038, 1039, 1040, 1063, 1064, 1065, 1310, 1330, 1331, 1332, 1578, 1579, 1580, 1607, 1608, 1609

- [ ] **CHAIN-afc88b98bee9** `app/display_lifecycle`
  - 当前残留信号：interface, operation。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/app/display_lifecycle.hpp](../../../include/openswd3/app/display_lifecycle.hpp)；[src/app/display_lifecycle.cpp](../../../src/app/display_lifecycle.cpp)。
  - 类型：[PORT-637969f4ac5a](invoke-removal-ports.md#port-637969f4ac5a), [PORT-b7e9d84e2af1](invoke-removal-ports.md#port-b7e9d84e2af1)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `operation` [include/openswd3/app/display_lifecycle.hpp](../../../include/openswd3/app/display_lifecycle.hpp)：102, 105, 107
  - `operation` [src/app/display_lifecycle.cpp](../../../src/app/display_lifecycle.cpp)：62, 63, 65, 66, 67

- [ ] **CHAIN-8ec4c462be34** `app/frame_preparation`
  - 当前残留信号：interface, operation。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/app/frame_preparation.hpp](../../../include/openswd3/app/frame_preparation.hpp)；[src/app/frame_preparation.cpp](../../../src/app/frame_preparation.cpp)。
  - 类型：[PORT-3e47878555c7](invoke-removal-ports.md#port-3e47878555c7)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `operation` [include/openswd3/app/frame_preparation.hpp](../../../include/openswd3/app/frame_preparation.hpp)：58
  - `operation` [src/app/frame_preparation.cpp](../../../src/app/frame_preparation.cpp)：29, 37

- [ ] **CHAIN-42cefff77119** `asset_runtime/legacy_action_record`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/asset_runtime/legacy_action_record.hpp](../../../include/openswd3/asset_runtime/legacy_action_record.hpp)；[src/asset_runtime/legacy_action_record.cpp](../../../src/asset_runtime/legacy_action_record.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/asset_runtime/legacy_action_record.hpp](../../../include/openswd3/asset_runtime/legacy_action_record.hpp)：114, 153, 165
  - `register` [src/asset_runtime/legacy_action_record.cpp](../../../src/asset_runtime/legacy_action_record.cpp)：130, 132, 133, 138, 148, 161, 165, 169, 173, 177, 181, 182, 183, 189, 193, 197, 201, 207, 211, 215, 219, 224, 230, 234, 238, 242, 255, 327, 330, 365, 369, 391, 392, 393, 403, 404, 439, 685

- [ ] **CHAIN-f7167be89c4d** `asset_runtime/legacy_ani_directional_effect`
  - 当前残留信号：counter, interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/asset_runtime/legacy_ani_directional_effect.hpp](../../../include/openswd3/asset_runtime/legacy_ani_directional_effect.hpp)；[src/asset_runtime/legacy_ani_directional_effect.cpp](../../../src/asset_runtime/legacy_ani_directional_effect.cpp)。
  - 类型：[PORT-dc01c6327a93](invoke-removal-ports.md#port-dc01c6327a93), [PORT-596e8c6acc5d](invoke-removal-ports.md#port-596e8c6acc5d), [PORT-e923f0c8070f](invoke-removal-ports.md#port-e923f0c8070f)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/asset_runtime/legacy_ani_directional_effect.hpp](../../../include/openswd3/asset_runtime/legacy_ani_directional_effect.hpp)：95, 176
  - `counter` [src/asset_runtime/legacy_ani_directional_effect.cpp](../../../src/asset_runtime/legacy_ani_directional_effect.cpp)：96, 105

- [ ] **CHAIN-d6dcaff802fb** `asset_runtime/legacy_ani_role_particle_effect`
  - 当前残留信号：counter, interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/asset_runtime/legacy_ani_role_particle_effect.hpp](../../../include/openswd3/asset_runtime/legacy_ani_role_particle_effect.hpp)；[src/asset_runtime/legacy_ani_role_particle_effect.cpp](../../../src/asset_runtime/legacy_ani_role_particle_effect.cpp)。
  - 类型：[PORT-50014e7449e8](invoke-removal-ports.md#port-50014e7449e8), [PORT-4ef90f0541e5](invoke-removal-ports.md#port-4ef90f0541e5), [PORT-9a093152f449](invoke-removal-ports.md#port-9a093152f449)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/asset_runtime/legacy_ani_role_particle_effect.hpp](../../../include/openswd3/asset_runtime/legacy_ani_role_particle_effect.hpp)：167
  - `counter` [src/asset_runtime/legacy_ani_role_particle_effect.cpp](../../../src/asset_runtime/legacy_ani_role_particle_effect.cpp)：83

- [ ] **CHAIN-5792d4774a40** `battle/legacy_battle_action_dispatch`
  - 当前残留信号：arguments, callee, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)；[include/openswd3/battle/legacy_battle_deformation.hpp](../../../include/openswd3/battle/legacy_battle_deformation.hpp)；[src/battle/legacy_battle_action_dispatch.cpp](../../../src/battle/legacy_battle_action_dispatch.cpp)；[src/battle/legacy_battle_action_dispatch_cases_high.cpp](../../../src/battle/legacy_battle_action_dispatch_cases_high.cpp)；[src/battle/legacy_battle_action_dispatch_cases_low.cpp](../../../src/battle/legacy_battle_action_dispatch_cases_low.cpp)；[src/battle/legacy_battle_action_dispatch_effects.cpp](../../../src/battle/legacy_battle_action_dispatch_effects.cpp)；[src/battle/legacy_battle_action_dispatch_internal.hpp](../../../src/battle/legacy_battle_action_dispatch_internal.hpp)；[src/battle/legacy_battle_action_dispatch_runner.cpp](../../../src/battle/legacy_battle_action_dispatch_runner.cpp)；[src/battle/legacy_battle_action_dispatch_special.cpp](../../../src/battle/legacy_battle_action_dispatch_special.cpp)；[src/battle/legacy_battle_deformation.cpp](../../../src/battle/legacy_battle_deformation.cpp)。
  - 类型：[PORT-77b3544a479a](invoke-removal-ports.md#port-77b3544a479a), [PORT-e3865412f2ac](invoke-removal-ports.md#port-e3865412f2ac), [PORT-3113c5c46779](invoke-removal-ports.md#port-3113c5c46779)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)：98, 108, 330, 331, 333, 358, 359, 361, 427
  - `callee` [include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)：97, 107, 191, 193, 325, 353, 423
  - `counter` [include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)：490, 491, 545, 547, 551, 554, 555, 559, 616, 617, 619, 622, 623, 630, 631, 632, 696, 697, 699, 700, 701, 702, 727, 728, 766, 767, 768, 769, 770, 803, 804, 806, 807, 808, 809, 810, 811, 812, 813, 849, 850, 852, 853, 854, 855, 880, 881, 883, 920, 921, 922, 923, 924, 926, 928, 929, 930, 931, 932, 968, 969, 970, 971, 972, 973, 974, 975, 976, 977, 1012, 1013, 1014, 1015, 1016, 1054, 1055, 1056, 1058, 1061, 1062, 1063, 1064, 1065, 1066, 1101, 1102, 1104, 1106, 1107, 1108, 1109, 1110, 1125, 1155, 1156, 1158, 1161, 1162, 1163, 1164, 1165, 1193, 1195, 1196, 1197, 1209, 1210, 1212, 1213, 1238, 1279, 1280, 1281, 1282, 1283, 1284, 1285, 1290, 1619, 1624, 1626, 1627, 1629, 1660, 1663, 1667, 1669, 1670, 1676, 1678, 1683, 1685, 1687, 1689, 1691, 1693, 1697, 1699, 1701, 1703, 1705, 1707, 1709, 1711, 1713, 1715, 1717, 1719, 1721, 1723, 1725, 1727, 1729, 1731, 1733, 1735, 1737, 1739, 1741, 1743, 1745, 1746, 1747, 1748, 1750, 1753, 1755, 1758, 1761
  - `invoke` [include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)：165, 188, 203, 208, 212, 217, 221, 226, 230, 235, 238, 243, 247, 252, 256, 261, 265, 270, 274, 287, 291, 296, 300, 305, 309, 316, 320, 336, 340, 345, 349, 363, 367, 372, 376, 389, 393, 406, 410, 411, 415, 419, 431
  - `register` [include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)：101, 102, 103, 109, 110, 111, 117, 123, 124, 130, 131, 132, 326, 327, 328, 354, 355, 356, 428, 429, 430, 432, 495, 509, 510, 511, 512, 513, 514, 515, 516, 560, 561, 562, 563, 564, 565, 566, 567, 577, 578, 579, 596, 597, 598, 633, 634, 635, 646, 647, 648, 660, 661, 662, 678, 679, 680, 704, 705, 706, 712, 713, 714, 730, 731, 732, 744, 749, 750, 751, 772, 773, 774, 782, 783, 784, 818, 819, 820, 831, 832, 833, 857, 858, 859, 865, 866, 867, 885, 886, 887, 895, 896, 897, 939, 940, 941, 949, 950, 951, 982, 983, 984, 992, 993, 994, 1020, 1021, 1022, 1032, 1033, 1034, 1071, 1072, 1073, 1079, 1080, 1081, 1115, 1116, 1117, 1130, 1131, 1132, 1140, 1141, 1142, 1167, 1168, 1169, 1177, 1178, 1179, 1199, 1200, 1201, 1217, 1218, 1219, 1224, 1225, 1226, 1241, 1242, 1243, 1261, 1262, 1263, 1292, 1293, 1294
  - `arguments` [src/battle/legacy_battle_action_dispatch.cpp](../../../src/battle/legacy_battle_action_dispatch.cpp)：11, 15, 26, 149, 189, 325, 328, 573, 576, 761, 911, 914, 1221, 1224, 1515, 1518, 1724, 1727, 1822, 1825, 2003, 2006
  - `callee` [src/battle/legacy_battle_action_dispatch.cpp](../../../src/battle/legacy_battle_action_dispatch.cpp)：14, 25, 148, 188, 324, 327, 572, 575, 760, 910, 913, 1220, 1223, 1514, 1517, 1723, 1726, 1821, 1824, 2002, 2005
  - `counter` [src/battle/legacy_battle_action_dispatch.cpp](../../../src/battle/legacy_battle_action_dispatch.cpp)：146, 186, 289, 290, 349, 439, 492, 506, 594, 602, 642, 653, 661, 747, 766, 796, 822, 931, 941, 983, 984, 1023, 1062, 1063, 1132, 1147, 1153, 1241, 1251, 1274, 1275, 1314, 1354, 1355, 1425, 1440, 1447, 1533, 1541, 1600, 1644, 1648, 1664, 1677, 1751, 1755, 1844, 1852, 1906, 1911, 1921, 1933, 2025, 2033, 2098, 2140, 2147, 2164, 2177, 2194, 2211, 2230, 2236, 2253, 2264
  - `invoke` [src/battle/legacy_battle_action_dispatch.cpp](../../../src/battle/legacy_battle_action_dispatch.cpp)：7, 35, 147, 187, 326, 437, 491, 574, 655, 663, 682, 759, 912, 1149, 1155, 1175, 1222, 1442, 1449, 1469, 1516, 1646, 1652, 1657, 1665, 1678, 1725, 1752, 1756, 1823, 1908, 1913, 1922, 1934, 2004, 2142, 2152, 2157, 2165, 2178, 2212, 2231, 2238, 2254, 2265
  - `register` [src/battle/legacy_battle_action_dispatch.cpp](../../../src/battle/legacy_battle_action_dispatch.cpp)：37, 38, 39, 67, 100, 101, 109, 150, 176, 180, 191, 195, 200, 209, 227, 232, 233, 234, 235, 236, 237, 238, 239, 243, 244, 247, 251, 252, 254, 256, 257, 259, 261, 262, 267, 268, 269, 270, 271, 272, 273, 291, 292, 293, 294, 295, 296, 297, 315, 329, 330, 331, 333, 334, 335, 340, 343, 361, 366, 367, 368, 378, 379, 393, 394, 395, 396, 397, 402, 403, 404, 405, 407, 408, 409, 410, 411, 426, 427, 428, 431, 432, 434, 435, 438, 441, 484, 485, 486, 493, 509, 517, 518, 519, 526, 527, 528, 530, 531, 532, 533, 534, 535, 537, 553, 554, 555, 564, 565, 566, 568, 569, 570, 577, 578, 579, 581, 582, 583, 597, 609, 651, 652, 655, 676, 677, 678, 693, 694, 695, 715, 716, 717, 762, 763, 764, 776, 781, 878, 891, 892, 893, 902, 903, 904, 906, 907, 908, 915, 916, 917, 919, 920, 921, 934, 935, 936, 948, 949, 950, 982, 1002, 1003, 1004, 1009, 1010, 1011, 1035, 1040, 1041, 1042, 1047, 1048, 1049, 1080, 1081, 1082, 1087, 1088, 1089, 1092, 1094, 1096, 1099, 1101, 1103, 1107, 1108, 1134, 1135, 1146, 1149, 1169, 1170, 1171, 1186, 1187, 1188, 1201, 1202, 1203, 1212, 1213, 1214, 1216, 1217, 1218, 1225, 1226, 1227, 1229, 1230, 1231, 1244, 1245, 1246, 1258, 1259, 1260, 1273, 1293, 1294, 1295, 1300, 1301, 1302, 1331, 1332, 1333, 1338, 1339, 1340, 1366, 1372, 1373, 1374, 1379, 1380, 1381, 1384, 1385, 1387, 1389, 1394, 1396, 1400, 1401, 1427, 1428, 1439, 1442, 1463, 1464, 1465, 1480, 1481, 1482, 1495, 1496, 1497, 1506, 1507, 1508, 1510, 1511, 1512, 1519, 1520, 1521, 1523, 1524, 1525, 1536, 1548, 1562, 1573, 1615, 1616, 1617, 1622, 1623, 1624, 1642, 1643, 1646, 1650, 1652, 1655, 1657, 1690, 1691, 1692, 1704, 1705, 1706, 1715, 1716, 1717, 1719, 1720, 1721, 1728, 1729, 1730, 1732, 1733, 1734, 1738, 1740, 1741, 1750, 1753, 1758, 1759, 1763, 1764, 1765, 1766, 1771, 1774, 1775, 1776, 1790, 1802, 1803, 1804, 1813, 1814, 1815, 1817, 1818, 1819, 1826, 1827, 1828, 1830, 1831, 1832, 1847, 1859, 1904, 1905, 1908, 1910, 1913, 1950, 1951, 1952, 1953, 1957, 1958, 1959, 1960, 1967, 1968, 1969, 1970, 1983, 1984, 1985, 1994, 1995, 1996, 1998, 1999, 2000, 2007, 2008, 2009, 2011, 2012, 2013, 2028, 2040, 2113, 2114, 2115, 2120, 2121, 2122, 2138, 2139, 2142, 2149, 2150, 2152, 2155, 2157, 2193, 2205, 2206, 2207, 2210, 2221, 2229, 2235, 2243, 2244, 2245, 2252, 2278, 2279, 2280, 2288, 2289, 2290
  - `counter` [src/battle/legacy_battle_action_dispatch_cases_high.cpp](../../../src/battle/legacy_battle_action_dispatch_cases_high.cpp)：55, 99, 138, 199, 232, 234, 449, 469, 591, 694, 789, 830, 876, 1007, 1102, 1164, 1194, 1225, 1311, 1343, 1391
  - `invoke` [src/battle/legacy_battle_action_dispatch_cases_high.cpp](../../../src/battle/legacy_battle_action_dispatch_cases_high.cpp)：30, 34, 286, 303, 342, 359, 392, 510, 525, 611, 621, 651, 657, 659, 864, 885, 942, 944, 963, 974, 981, 1029, 1114, 1120, 1220, 1231, 1257, 1263, 1265, 1326, 1338, 1346, 1401, 1409, 1415, 1416, 1424, 1430, 1431, 1436, 1442, 1443
  - `register` [src/battle/legacy_battle_action_dispatch_cases_high.cpp](../../../src/battle/legacy_battle_action_dispatch_cases_high.cpp)：38, 40, 41, 61, 145, 217, 239, 243, 273, 292, 309, 318, 319, 320, 327, 348, 365, 374, 375, 376, 383, 413, 456, 477, 543, 600, 617, 629, 648, 665, 701, 722, 729, 782, 783, 784, 800, 841, 911, 969, 1018, 1035, 1109, 1114, 1140, 1176, 1240, 1254, 1271, 1322, 1329, 1341, 1344, 1353, 1354, 1398, 1407
  - `counter` [src/battle/legacy_battle_action_dispatch_cases_low.cpp](../../../src/battle/legacy_battle_action_dispatch_cases_low.cpp)：94, 191, 340, 371, 585, 633, 714, 910, 985
  - `invoke` [src/battle/legacy_battle_action_dispatch_cases_low.cpp](../../../src/battle/legacy_battle_action_dispatch_cases_low.cpp)：25, 108, 112, 149, 155, 161, 162, 172, 250, 256, 394, 454, 511, 849, 924
  - `register` [src/battle/legacy_battle_action_dispatch_cases_low.cpp](../../../src/battle/legacy_battle_action_dispatch_cases_low.cpp)：31, 103, 108, 112, 144, 200, 250, 256, 382, 383, 384, 385, 400, 478, 592, 616, 617, 618, 619, 621, 622, 646, 647, 721, 728, 789, 790, 797, 826, 855, 917, 992
  - `arguments` [src/battle/legacy_battle_action_dispatch_effects.cpp](../../../src/battle/legacy_battle_action_dispatch_effects.cpp)：39, 42, 122, 159, 241, 276, 371, 374, 488, 526, 669, 706, 860, 863, 1012, 1015, 1161, 1403, 1564, 1680, 1683
  - `callee` [src/battle/legacy_battle_action_dispatch_effects.cpp](../../../src/battle/legacy_battle_action_dispatch_effects.cpp)：38, 41, 96, 121, 158, 240, 275, 370, 373, 487, 525, 668, 705, 859, 862, 1011, 1014, 1160, 1402, 1563, 1679, 1682
  - `counter` [src/battle/legacy_battle_action_dispatch_effects.cpp](../../../src/battle/legacy_battle_action_dispatch_effects.cpp)：59, 93, 118, 155, 237, 262, 266, 272, 317, 397, 410, 424, 484, 522, 557, 605, 632, 665, 701, 736, 908, 918, 937, 962, 967, 1070, 1087, 1101, 1157, 1193, 1231, 1309, 1322, 1340, 1398, 1449, 1470, 1474, 1560, 1706, 1714, 1722, 1785, 1804
  - `invoke` [src/battle/legacy_battle_action_dispatch_effects.cpp](../../../src/battle/legacy_battle_action_dispatch_effects.cpp)：40, 94, 119, 156, 238, 264, 268, 273, 372, 411, 485, 523, 666, 703, 861, 910, 939, 964, 968, 1013, 1088, 1158, 1324, 1400, 1471, 1477, 1482, 1561, 1681, 1724, 1786, 1805
  - `register` [src/battle/legacy_battle_action_dispatch_effects.cpp](../../../src/battle/legacy_battle_action_dispatch_effects.cpp)：13, 14, 15, 24, 25, 26, 28, 29, 30, 33, 34, 35, 43, 44, 45, 47, 48, 49, 79, 80, 81, 97, 98, 99, 106, 107, 108, 127, 128, 129, 137, 138, 139, 164, 165, 166, 170, 171, 172, 173, 184, 185, 186, 203, 204, 205, 253, 254, 255, 259, 260, 261, 281, 282, 283, 287, 288, 289, 290, 304, 305, 306, 322, 325, 326, 327, 341, 342, 343, 351, 356, 357, 358, 360, 361, 362, 365, 366, 367, 375, 376, 377, 379, 380, 381, 434, 435, 436, 445, 446, 447, 493, 494, 495, 503, 504, 505, 531, 532, 533, 537, 538, 539, 540, 558, 569, 570, 571, 606, 607, 608, 680, 681, 682, 690, 691, 692, 693, 713, 714, 715, 723, 724, 725, 737, 748, 749, 750, 828, 829, 830, 841, 842, 843, 851, 852, 853, 855, 856, 857, 864, 865, 866, 868, 869, 870, 875, 876, 877, 903, 905, 906, 907, 911, 917, 929, 930, 931, 934, 935, 941, 942, 944, 945, 946, 949, 950, 952, 953, 956, 957, 958, 959, 960, 961, 964, 966, 982, 983, 984, 992, 997, 998, 999, 1001, 1002, 1003, 1006, 1007, 1008, 1016, 1017, 1018, 1020, 1021, 1022, 1111, 1112, 1113, 1122, 1123, 1124, 1166, 1167, 1168, 1176, 1177, 1178, 1195, 1232, 1243, 1244, 1245, 1311, 1320, 1321, 1324, 1361, 1362, 1363, 1368, 1369, 1370, 1373, 1374, 1376, 1378, 1382, 1384, 1388, 1389, 1412, 1413, 1414, 1419, 1420, 1421, 1452, 1469, 1472, 1476, 1478, 1481, 1483, 1571, 1572, 1573, 1581, 1582, 1583, 1645, 1646, 1647, 1661, 1662, 1663, 1671, 1672, 1673, 1675, 1676, 1677, 1684, 1685, 1686, 1688, 1689, 1690, 1709, 1721, 1724, 1738, 1739, 1740, 1775, 1776, 1777, 1798, 1799, 1800, 1814, 1815, 1816
  - `arguments` [src/battle/legacy_battle_action_dispatch_internal.hpp](../../../src/battle/legacy_battle_action_dispatch_internal.hpp)：327, 330, 428, 429, 473, 485, 486, 487, 488, 489
  - `callee` [src/battle/legacy_battle_action_dispatch_internal.hpp](../../../src/battle/legacy_battle_action_dispatch_internal.hpp)：326, 330, 394, 408, 423, 435
  - `counter` [src/battle/legacy_battle_action_dispatch_internal.hpp](../../../src/battle/legacy_battle_action_dispatch_internal.hpp)：492, 511, 596
  - `invoke` [src/battle/legacy_battle_action_dispatch_internal.hpp](../../../src/battle/legacy_battle_action_dispatch_internal.hpp)：323, 330, 391, 430
  - `register` [src/battle/legacy_battle_action_dispatch_internal.hpp](../../../src/battle/legacy_battle_action_dispatch_internal.hpp)：221, 222, 235, 236, 257, 258, 271, 272, 293, 294, 295, 308, 309, 310, 354, 355, 370, 371, 381, 399, 400, 401, 414, 415, 416, 424, 425, 426, 432, 433, 434, 568, 744, 745, 750, 751, 752
  - `counter` [src/battle/legacy_battle_action_dispatch_runner.cpp](../../../src/battle/legacy_battle_action_dispatch_runner.cpp)：53, 150, 185, 387, 580, 616, 647, 668
  - `invoke` [src/battle/legacy_battle_action_dispatch_runner.cpp](../../../src/battle/legacy_battle_action_dispatch_runner.cpp)：100, 110, 209, 275, 328, 336, 339, 352, 355, 410, 478, 529, 532, 544, 547, 693, 709, 799, 862, 890
  - `register` [src/battle/legacy_battle_action_dispatch_runner.cpp](../../../src/battle/legacy_battle_action_dispatch_runner.cpp)：64, 116, 157, 158, 159, 196, 197, 198, 200, 215, 301, 334, 398, 399, 400, 401, 416, 510, 600, 627, 654, 675, 699, 822, 863, 880, 881
  - `arguments` [src/battle/legacy_battle_action_dispatch_special.cpp](../../../src/battle/legacy_battle_action_dispatch_special.cpp)：35, 38, 355, 390, 508, 511, 705, 913, 916, 1169, 1253, 1342, 1345, 1370, 1426
  - `callee` [src/battle/legacy_battle_action_dispatch_special.cpp](../../../src/battle/legacy_battle_action_dispatch_special.cpp)：34, 37, 354, 389, 507, 510, 704, 912, 915, 1168, 1252, 1341, 1344, 1369, 1425
  - `counter` [src/battle/legacy_battle_action_dispatch_special.cpp](../../../src/battle/legacy_battle_action_dispatch_special.cpp)：56, 64, 123, 131, 147, 159, 182, 192, 250, 282, 351, 386, 436, 529, 538, 586, 593, 610, 622, 640, 662, 701, 733, 750, 764, 769, 779, 803, 811, 815, 821, 845, 850, 854, 864, 933, 942, 992, 1012, 1018, 1035, 1068, 1079, 1112, 1118, 1142, 1165, 1195, 1212, 1231, 1236, 1249, 1365, 1395, 1422, 1451
  - `invoke` [src/battle/legacy_battle_action_dispatch_special.cpp](../../../src/battle/legacy_battle_action_dispatch_special.cpp)：36, 125, 135, 140, 148, 160, 283, 352, 387, 509, 588, 598, 603, 611, 623, 702, 751, 765, 771, 804, 813, 817, 846, 852, 856, 865, 914, 993, 1014, 1023, 1028, 1036, 1114, 1123, 1128, 1143, 1166, 1213, 1232, 1237, 1250, 1343, 1367, 1423, 1452
  - `register` [src/battle/legacy_battle_action_dispatch_special.cpp](../../../src/battle/legacy_battle_action_dispatch_special.cpp)：15, 16, 17, 26, 27, 28, 30, 31, 32, 39, 40, 41, 43, 44, 45, 59, 71, 121, 122, 125, 130, 133, 135, 138, 140, 173, 174, 175, 185, 186, 187, 199, 200, 201, 236, 265, 266, 267, 272, 273, 274, 296, 297, 298, 305, 306, 307, 318, 319, 320, 329, 330, 331, 333, 334, 335, 339, 340, 341, 360, 361, 362, 368, 385, 395, 396, 397, 402, 417, 418, 419, 437, 438, 439, 445, 446, 447, 458, 459, 460, 464, 465, 466, 473, 474, 475, 488, 489, 490, 499, 500, 501, 503, 504, 505, 512, 513, 514, 516, 517, 518, 531, 533, 545, 583, 584, 585, 588, 595, 596, 598, 601, 603, 654, 655, 656, 674, 677, 678, 679, 684, 685, 686, 695, 696, 697, 716, 717, 718, 722, 723, 724, 725, 726, 727, 728, 732, 744, 745, 746, 749, 755, 763, 777, 778, 786, 787, 795, 796, 797, 800, 801, 802, 820, 828, 829, 837, 838, 839, 842, 843, 844, 859, 879, 880, 881, 893, 894, 895, 904, 905, 906, 908, 909, 910, 917, 918, 919, 921, 922, 923, 935, 937, 949, 1009, 1010, 1011, 1014, 1020, 1021, 1023, 1026, 1028, 1070, 1072, 1073, 1074, 1086, 1109, 1110, 1111, 1114, 1120, 1121, 1123, 1126, 1128, 1184, 1185, 1186, 1190, 1191, 1192, 1193, 1194, 1206, 1207, 1208, 1211, 1222, 1230, 1260, 1261, 1262, 1266, 1267, 1268, 1269, 1275, 1276, 1277, 1288, 1289, 1290, 1300, 1301, 1302, 1321, 1322, 1323, 1325, 1326, 1327, 1330, 1331, 1332, 1336, 1337, 1338, 1346, 1347, 1348, 1350, 1351, 1352, 1359, 1360, 1361, 1362, 1375, 1376, 1377, 1385, 1386, 1387, 1392, 1393, 1394, 1404, 1407, 1408, 1409, 1414, 1415, 1416, 1419, 1420, 1421, 1432, 1433, 1434, 1438, 1439, 1440, 1443, 1444, 1446, 1448, 1449, 1450, 1460, 1466, 1467, 1478, 1479, 1496, 1497, 1500, 1501, 1502

- [ ] **CHAIN-478ade7bd4a6** `battle/legacy_battle_action_frame_draw`
  - 当前残留信号：counter, interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_action_frame_draw.hpp](../../../include/openswd3/battle/legacy_battle_action_frame_draw.hpp)；[src/battle/legacy_battle_action_frame_draw.cpp](../../../src/battle/legacy_battle_action_frame_draw.cpp)。
  - 类型：[PORT-1cc3568ae19a](invoke-removal-ports.md#port-1cc3568ae19a)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_action_frame_draw.hpp](../../../include/openswd3/battle/legacy_battle_action_frame_draw.hpp)：39, 40, 41, 87, 88, 135, 136, 167, 168, 188, 189
  - `counter` [src/battle/legacy_battle_action_frame_draw.cpp](../../../src/battle/legacy_battle_action_frame_draw.cpp)：110, 153, 190, 222, 290, 309, 363, 404, 456, 476, 530, 553

- [ ] **CHAIN-bf5907716775** `battle/legacy_battle_action_mode_refresh`
  - 当前残留信号：interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_action_mode_refresh.hpp](../../../include/openswd3/battle/legacy_battle_action_mode_refresh.hpp)；[src/battle/legacy_battle_action_mode_refresh.cpp](../../../src/battle/legacy_battle_action_mode_refresh.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_action_mode_refresh.hpp](../../../include/openswd3/battle/legacy_battle_action_mode_refresh.hpp)：31, 32, 33, 50, 51, 52
  - `invoke` [src/battle/legacy_battle_action_mode_refresh.cpp](../../../src/battle/legacy_battle_action_mode_refresh.cpp)：132, 237, 254, 271
  - `register` [src/battle/legacy_battle_action_mode_refresh.cpp](../../../src/battle/legacy_battle_action_mode_refresh.cpp)：116, 117, 118, 119, 122, 123, 124, 134, 135, 136, 138, 139, 140, 143, 144, 145, 147, 149, 152, 157, 160, 161, 183, 185, 190, 196, 197, 203, 205, 206, 207, 208, 214, 215, 216, 223, 226, 227, 228, 229, 230, 231, 239, 240, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 256, 257, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 273, 277, 278, 279, 280, 282, 283, 285, 288, 289

- [ ] **CHAIN-539261d5f092** `battle/legacy_battle_action_rotation_cache`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_action_rotation_cache.hpp](../../../include/openswd3/battle/legacy_battle_action_rotation_cache.hpp)；[src/battle/legacy_battle_action_rotation_cache.cpp](../../../src/battle/legacy_battle_action_rotation_cache.cpp)。
  - 类型：[PORT-4d636c2d3ddb](invoke-removal-ports.md#port-4d636c2d3ddb), [PORT-2b5d57e26962](invoke-removal-ports.md#port-2b5d57e26962), [PORT-646f55206409](invoke-removal-ports.md#port-646f55206409), [PORT-8bbb503d4e27](invoke-removal-ports.md#port-8bbb503d4e27)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_action_rotation_cache.hpp](../../../include/openswd3/battle/legacy_battle_action_rotation_cache.hpp)：105, 107, 108, 111, 138, 140, 141, 143, 144, 169, 170, 179, 180
  - `register` [include/openswd3/battle/legacy_battle_action_rotation_cache.hpp](../../../include/openswd3/battle/legacy_battle_action_rotation_cache.hpp)：16, 17
  - `counter` [src/battle/legacy_battle_action_rotation_cache.cpp](../../../src/battle/legacy_battle_action_rotation_cache.cpp)：121, 191, 257, 271, 279, 307, 313, 343, 385, 418, 424, 497, 535, 550, 555, 564
  - `register` [src/battle/legacy_battle_action_rotation_cache.cpp](../../../src/battle/legacy_battle_action_rotation_cache.cpp)：89, 90, 128, 142, 143, 160, 180, 186, 286, 431, 445, 446, 571

- [ ] **CHAIN-ac312291d668** `battle/legacy_battle_action_summary`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_action_summary.hpp](../../../include/openswd3/battle/legacy_battle_action_summary.hpp)；[src/battle/legacy_battle_action_summary.cpp](../../../src/battle/legacy_battle_action_summary.cpp)。
  - 类型：[PORT-aeaaca3cd801](invoke-removal-ports.md#port-aeaaca3cd801), [PORT-ef20d3a58313](invoke-removal-ports.md#port-ef20d3a58313)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_action_summary.hpp](../../../include/openswd3/battle/legacy_battle_action_summary.hpp)：26
  - `counter` [include/openswd3/battle/legacy_battle_action_summary.hpp](../../../include/openswd3/battle/legacy_battle_action_summary.hpp)：79, 80, 81, 87
  - `invoke` [include/openswd3/battle/legacy_battle_action_summary.hpp](../../../include/openswd3/battle/legacy_battle_action_summary.hpp)：44
  - `register` [include/openswd3/battle/legacy_battle_action_summary.hpp](../../../include/openswd3/battle/legacy_battle_action_summary.hpp)：27, 28, 29, 33, 34, 35, 60, 61, 62, 76, 77, 78
  - `arguments` [src/battle/legacy_battle_action_summary.cpp](../../../src/battle/legacy_battle_action_summary.cpp)：147, 156
  - `counter` [src/battle/legacy_battle_action_summary.cpp](../../../src/battle/legacy_battle_action_summary.cpp)：163, 165, 167, 222
  - `invoke` [src/battle/legacy_battle_action_summary.cpp](../../../src/battle/legacy_battle_action_summary.cpp)：91, 107, 158, 176, 178, 200, 246, 270, 276, 289, 312, 333, 352, 358, 371, 381
  - `register` [src/battle/legacy_battle_action_summary.cpp](../../../src/battle/legacy_battle_action_summary.cpp)：109, 110, 111, 112, 114, 131, 132, 133, 136, 137, 138, 151, 152, 153, 159, 160, 161, 171, 175, 177, 180, 181, 182, 183, 187, 188, 199, 200, 202, 220, 223, 224, 225, 233, 234, 235, 236, 237, 243, 244, 245, 251, 264, 265, 266, 269, 272, 273, 274, 275, 281, 288, 293, 294, 295, 301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312, 317, 320, 321, 322, 323, 325, 326, 329, 330, 332, 347, 348, 349, 350, 351, 354, 355, 356, 357, 364, 370, 377, 380

- [ ] **CHAIN-dea2c7adfe36** `battle/legacy_battle_actor_action_candidate_availability`
  - 当前残留信号：counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_action_candidate_availability.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_candidate_availability.hpp)；[src/battle/legacy_battle_actor_action_candidate_availability.cpp](../../../src/battle/legacy_battle_actor_action_candidate_availability.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_actor_action_candidate_availability.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_candidate_availability.hpp)：33
  - `register` [include/openswd3/battle/legacy_battle_actor_action_candidate_availability.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_candidate_availability.hpp)：14, 15, 16, 29, 30, 31
  - `counter` [src/battle/legacy_battle_actor_action_candidate_availability.cpp](../../../src/battle/legacy_battle_actor_action_candidate_availability.cpp)：70
  - `invoke` [src/battle/legacy_battle_actor_action_candidate_availability.cpp](../../../src/battle/legacy_battle_actor_action_candidate_availability.cpp)：64
  - `register` [src/battle/legacy_battle_actor_action_candidate_availability.cpp](../../../src/battle/legacy_battle_actor_action_candidate_availability.cpp)：23, 24, 25, 29, 30, 31, 35, 39, 50, 51, 52, 53, 54, 55, 56, 57, 66, 67, 68, 71, 72, 73, 77, 79, 80, 81

- [ ] **CHAIN-a0757fbf1074** `battle/legacy_battle_actor_action_commit`
  - 当前残留信号：counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_action_commit.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_commit.hpp)；[src/battle/legacy_battle_actor_action_commit.cpp](../../../src/battle/legacy_battle_actor_action_commit.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_actor_action_commit.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_commit.hpp)：36
  - `register` [include/openswd3/battle/legacy_battle_actor_action_commit.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_commit.hpp)：17, 18, 19, 32, 33, 34
  - `counter` [src/battle/legacy_battle_actor_action_commit.cpp](../../../src/battle/legacy_battle_actor_action_commit.cpp)：78
  - `invoke` [src/battle/legacy_battle_actor_action_commit.cpp](../../../src/battle/legacy_battle_actor_action_commit.cpp)：72
  - `register` [src/battle/legacy_battle_actor_action_commit.cpp](../../../src/battle/legacy_battle_actor_action_commit.cpp)：20, 21, 22, 27, 28, 29, 33, 40, 44, 45, 46, 61, 62, 63, 64, 65, 66, 74, 75, 76, 79, 80, 81, 82, 83, 85, 92, 94, 95

- [ ] **CHAIN-2116e0bdf84b** `battle/legacy_battle_actor_action_cycle`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_action_cycle.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_cycle.hpp)；[src/battle/legacy_battle_actor_action_cycle.cpp](../../../src/battle/legacy_battle_actor_action_cycle.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_actor_action_cycle.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_cycle.hpp)：35, 36
  - `register` [include/openswd3/battle/legacy_battle_actor_action_cycle.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_cycle.hpp)：16, 17, 18, 32, 33, 34
  - `counter` [src/battle/legacy_battle_actor_action_cycle.cpp](../../../src/battle/legacy_battle_actor_action_cycle.cpp)：48, 76
  - `register` [src/battle/legacy_battle_actor_action_cycle.cpp](../../../src/battle/legacy_battle_actor_action_cycle.cpp)：22, 23, 24, 27, 28, 29, 30, 31, 42, 43, 44, 45, 49, 50, 51, 56, 57, 58, 71, 72, 73, 74, 77, 78, 79

- [ ] **CHAIN-38fc7331ff2a** `battle/legacy_battle_actor_action_presentation`
  - 当前残留信号：arguments, callee, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_action_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_presentation.hpp)；[src/battle/legacy_battle_actor_action_presentation.cpp](../../../src/battle/legacy_battle_actor_action_presentation.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_actor_action_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_presentation.hpp)：106, 107
  - `callee` [include/openswd3/battle/legacy_battle_actor_action_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_presentation.hpp)：105
  - `counter` [include/openswd3/battle/legacy_battle_actor_action_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_presentation.hpp)：173, 174
  - `register` [include/openswd3/battle/legacy_battle_actor_action_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_presentation.hpp)：82, 83, 84, 85, 86, 87, 88, 108, 109, 110, 111, 112, 113, 150, 151, 152, 153, 154, 155, 156, 157, 257, 258
  - `arguments` [src/battle/legacy_battle_actor_action_presentation.cpp](../../../src/battle/legacy_battle_actor_action_presentation.cpp)：442, 456, 457, 467, 471, 478
  - `callee` [src/battle/legacy_battle_actor_action_presentation.cpp](../../../src/battle/legacy_battle_actor_action_presentation.cpp)：441, 455, 466, 471, 476
  - `counter` [src/battle/legacy_battle_actor_action_presentation.cpp](../../../src/battle/legacy_battle_actor_action_presentation.cpp)：444, 447, 451, 452, 496, 512, 598, 1785
  - `invoke` [src/battle/legacy_battle_actor_action_presentation.cpp](../../../src/battle/legacy_battle_actor_action_presentation.cpp)：483, 634, 735, 1049, 1065, 1206, 1217, 1231, 1242, 1330, 1383, 1445, 1534, 1547, 1600, 1658, 1716, 1796, 1810, 1837, 1855, 1919, 1933, 1973, 2041, 2054, 2058, 2080
  - `register` [src/battle/legacy_battle_actor_action_presentation.cpp](../../../src/battle/legacy_battle_actor_action_presentation.cpp)：27, 28, 29, 93, 219, 220, 221, 222, 223, 224, 225, 226, 233, 234, 235, 236, 237, 238, 239, 240, 246, 247, 248, 249, 250, 251, 252, 253, 458, 459, 460, 480, 481, 482, 485, 486, 487, 491, 492, 493, 497, 498, 499, 513, 514, 515, 521, 522, 523, 524, 525, 531, 535, 541, 572, 585, 586, 587, 599, 600, 601, 602, 603, 604, 610, 639, 733, 738, 740, 744, 774, 805, 1047, 1049, 1052, 1062, 1063, 1066, 1070, 1078, 1117, 1133, 1169, 1185, 1204, 1215, 1229, 1240, 1273, 1307, 1315, 1357, 1365, 1418, 1427, 1465, 1509, 1518, 1545, 1575, 1583, 1633, 1641, 1691, 1699, 1786, 1787, 1835, 1838, 1842, 1852, 1853, 1856, 1858, 1867, 1890, 1899, 1970, 1971, 1977, 1980, 1989, 2013, 2022, 2052, 2057, 2079, 2098, 2099, 2112, 2113

- [ ] **CHAIN-d673136b85b5** `battle/legacy_battle_actor_action_reverse_cycle`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_action_reverse_cycle.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_reverse_cycle.hpp)；[src/battle/legacy_battle_actor_action_reverse_cycle.cpp](../../../src/battle/legacy_battle_actor_action_reverse_cycle.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_actor_action_reverse_cycle.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_reverse_cycle.hpp)：35, 36
  - `register` [include/openswd3/battle/legacy_battle_actor_action_reverse_cycle.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_reverse_cycle.hpp)：16, 17, 18, 32, 33, 34
  - `counter` [src/battle/legacy_battle_actor_action_reverse_cycle.cpp](../../../src/battle/legacy_battle_actor_action_reverse_cycle.cpp)：49, 77
  - `register` [src/battle/legacy_battle_actor_action_reverse_cycle.cpp](../../../src/battle/legacy_battle_actor_action_reverse_cycle.cpp)：23, 24, 25, 28, 29, 30, 31, 32, 43, 44, 45, 46, 50, 51, 52, 57, 58, 59, 72, 73, 74, 75, 78, 79, 80

- [ ] **CHAIN-f459ef9b8986** `battle/legacy_battle_actor_action_target`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_action_target.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_target.hpp)；[src/battle/legacy_battle_actor_action_target.cpp](../../../src/battle/legacy_battle_actor_action_target.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_actor_action_target.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_target.hpp)：50, 51, 52, 64, 65, 66, 67
  - `register` [src/battle/legacy_battle_actor_action_target.cpp](../../../src/battle/legacy_battle_actor_action_target.cpp)：89, 90, 91, 92, 105, 106, 118

- [ ] **CHAIN-82611cce9bf0** `battle/legacy_battle_actor_action_target_clear`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_action_target_clear.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_target_clear.hpp)；[src/battle/legacy_battle_actor_action_target_clear.cpp](../../../src/battle/legacy_battle_actor_action_target_clear.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_actor_action_target_clear.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_target_clear.hpp)：49, 50, 51, 69, 70, 71, 72, 107, 108
  - `register` [src/battle/legacy_battle_actor_action_target_clear.cpp](../../../src/battle/legacy_battle_actor_action_target_clear.cpp)：22, 23, 24, 25, 49, 62, 63, 76, 77

- [ ] **CHAIN-afa8d87b84a3** `battle/legacy_battle_actor_base_coordinates`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_base_coordinates.hpp](../../../include/openswd3/battle/legacy_battle_actor_base_coordinates.hpp)；[src/battle/legacy_battle_actor_base_coordinates.cpp](../../../src/battle/legacy_battle_actor_base_coordinates.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_actor_base_coordinates.hpp](../../../include/openswd3/battle/legacy_battle_actor_base_coordinates.hpp)：23, 24, 36, 37, 38
  - `register` [src/battle/legacy_battle_actor_base_coordinates.cpp](../../../src/battle/legacy_battle_actor_base_coordinates.cpp)：58, 59, 60, 70, 78, 91, 108, 122, 130

- [ ] **CHAIN-cf7612c5e6d6** `battle/legacy_battle_actor_binary_state_toggle`
  - 当前残留信号：arguments, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_binary_state_toggle.hpp](../../../include/openswd3/battle/legacy_battle_actor_binary_state_toggle.hpp)；[src/battle/legacy_battle_actor_binary_state_toggle.cpp](../../../src/battle/legacy_battle_actor_binary_state_toggle.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_actor_binary_state_toggle.hpp](../../../include/openswd3/battle/legacy_battle_actor_binary_state_toggle.hpp)：114
  - `register` [include/openswd3/battle/legacy_battle_actor_binary_state_toggle.hpp](../../../include/openswd3/battle/legacy_battle_actor_binary_state_toggle.hpp)：64, 65, 66, 77, 78, 79, 80, 136, 137
  - `arguments` [src/battle/legacy_battle_actor_binary_state_toggle.cpp](../../../src/battle/legacy_battle_actor_binary_state_toggle.cpp)：212
  - `register` [src/battle/legacy_battle_actor_binary_state_toggle.cpp](../../../src/battle/legacy_battle_actor_binary_state_toggle.cpp)：117, 118, 119, 120, 122, 134, 141, 143, 168, 171, 183, 184, 200, 201

- [ ] **CHAIN-20015b63f11a** `battle/legacy_battle_actor_coordinate_adjustment`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_coordinate_adjustment.hpp](../../../include/openswd3/battle/legacy_battle_actor_coordinate_adjustment.hpp)；[src/battle/legacy_battle_actor_coordinate_adjustment.cpp](../../../src/battle/legacy_battle_actor_coordinate_adjustment.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_actor_coordinate_adjustment.hpp](../../../include/openswd3/battle/legacy_battle_actor_coordinate_adjustment.hpp)：20, 21, 31, 32, 33
  - `register` [src/battle/legacy_battle_actor_coordinate_adjustment.cpp](../../../src/battle/legacy_battle_actor_coordinate_adjustment.cpp)：56, 57, 58, 68, 77

- [ ] **CHAIN-753e7eb0bf68** `battle/legacy_battle_actor_coordinate_publication`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_coordinate_publication.hpp](../../../include/openswd3/battle/legacy_battle_actor_coordinate_publication.hpp)；[src/battle/legacy_battle_actor_coordinate_publication.cpp](../../../src/battle/legacy_battle_actor_coordinate_publication.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_actor_coordinate_publication.hpp](../../../include/openswd3/battle/legacy_battle_actor_coordinate_publication.hpp)：24, 25, 26, 27, 28, 42, 43, 44, 45, 46
  - `register` [src/battle/legacy_battle_actor_coordinate_publication.cpp](../../../src/battle/legacy_battle_actor_coordinate_publication.cpp)：30, 31, 32, 33, 34, 44, 52, 83, 84, 85, 114, 115, 116, 126, 134

- [ ] **CHAIN-33432079663b** `battle/legacy_battle_actor_coordinates`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_coordinates.hpp](../../../include/openswd3/battle/legacy_battle_actor_coordinates.hpp)；[src/battle/legacy_battle_actor_coordinates.cpp](../../../src/battle/legacy_battle_actor_coordinates.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_actor_coordinates.hpp](../../../include/openswd3/battle/legacy_battle_actor_coordinates.hpp)：188, 189, 201, 202, 203, 240, 241, 253, 254, 255
  - `register` [src/battle/legacy_battle_actor_coordinates.cpp](../../../src/battle/legacy_battle_actor_coordinates.cpp)：95, 96, 97, 128, 138, 157, 164, 180, 187, 202, 209, 229, 230, 231, 240, 249, 266, 274

- [ ] **CHAIN-31fb23ba6f32** `battle/legacy_battle_actor_effect_resource_slot_write`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_effect_resource_slot_write.hpp](../../../include/openswd3/battle/legacy_battle_actor_effect_resource_slot_write.hpp)；[src/battle/legacy_battle_actor_effect_resource_slot_write.cpp](../../../src/battle/legacy_battle_actor_effect_resource_slot_write.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_actor_effect_resource_slot_write.hpp](../../../include/openswd3/battle/legacy_battle_actor_effect_resource_slot_write.hpp)：88, 89, 90, 102, 103, 104, 105, 170, 171, 185, 186
  - `register` [src/battle/legacy_battle_actor_effect_resource_slot_write.cpp](../../../src/battle/legacy_battle_actor_effect_resource_slot_write.cpp)：60, 61, 77, 78, 175, 176, 177, 178, 179, 181, 193, 199, 211, 239, 243, 255, 256, 271, 272, 287, 288, 303, 304

- [ ] **CHAIN-8beb02a8e3d0** `battle/legacy_battle_actor_field_26b8_high_bit_query`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_field_26b8_high_bit_query.hpp](../../../include/openswd3/battle/legacy_battle_actor_field_26b8_high_bit_query.hpp)；[src/battle/legacy_battle_actor_field_26b8_high_bit_query.cpp](../../../src/battle/legacy_battle_actor_field_26b8_high_bit_query.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_actor_field_26b8_high_bit_query.hpp](../../../include/openswd3/battle/legacy_battle_actor_field_26b8_high_bit_query.hpp)：54, 55, 56, 68, 69, 70, 71
  - `register` [src/battle/legacy_battle_actor_field_26b8_high_bit_query.cpp](../../../src/battle/legacy_battle_actor_field_26b8_high_bit_query.cpp)：94, 95, 96, 97, 99, 111, 117, 119, 129, 133

- [ ] **CHAIN-67177b537f21** `battle/legacy_battle_actor_field_26b8_high_bit_set`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_field_26b8_high_bit_set.hpp](../../../include/openswd3/battle/legacy_battle_actor_field_26b8_high_bit_set.hpp)；[src/battle/legacy_battle_actor_field_26b8_high_bit_set.cpp](../../../src/battle/legacy_battle_actor_field_26b8_high_bit_set.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_actor_field_26b8_high_bit_set.hpp](../../../include/openswd3/battle/legacy_battle_actor_field_26b8_high_bit_set.hpp)：91, 92, 93, 104, 105, 106, 107, 174, 175, 194, 195
  - `register` [src/battle/legacy_battle_actor_field_26b8_high_bit_set.cpp](../../../src/battle/legacy_battle_actor_field_26b8_high_bit_set.cpp)：154, 155, 156, 157, 164, 193, 204, 237, 238, 245, 246, 259, 263, 288, 289, 296, 297, 340, 341, 348, 349

- [ ] **CHAIN-147801fed7a7** `battle/legacy_battle_actor_frame_host_heap`
  - 当前残留信号：interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_frame_host_heap.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_host_heap.hpp)；[src/battle/legacy_battle_actor_frame_host_heap.cpp](../../../src/battle/legacy_battle_actor_frame_host_heap.cpp)。
  - 类型：[PORT-4020b11af967](invoke-removal-ports.md#port-4020b11af967)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_actor_frame_host_heap.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_host_heap.hpp)：22, 23, 24
  - `register` [src/battle/legacy_battle_actor_frame_host_heap.cpp](../../../src/battle/legacy_battle_actor_frame_host_heap.cpp)：24, 25, 57, 58, 59

- [ ] **CHAIN-7625a6dd0bd8** `battle/legacy_battle_actor_frame_presentation`
  - 当前残留信号：arguments, callee, counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp)；[src/battle/legacy_battle_actor_frame_decoder_commands.hpp](../../../src/battle/legacy_battle_actor_frame_decoder_commands.hpp)；[src/battle/legacy_battle_actor_frame_decoder_heap_stats.hpp](../../../src/battle/legacy_battle_actor_frame_decoder_heap_stats.hpp)；[src/battle/legacy_battle_actor_frame_io_helpers.hpp](../../../src/battle/legacy_battle_actor_frame_io_helpers.hpp)；[src/battle/legacy_battle_actor_frame_presentation.cpp](../../../src/battle/legacy_battle_actor_frame_presentation.cpp)；[src/battle/legacy_battle_actor_frame_presentation_case_five.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_case_five.cpp)；[src/battle/legacy_battle_actor_frame_presentation_decoder.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_decoder.cpp)；[src/battle/legacy_battle_actor_frame_presentation_decoder_continuations.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_decoder_continuations.cpp)；[src/battle/legacy_battle_actor_frame_presentation_draw_arguments.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_draw_arguments.cpp)；[src/battle/legacy_battle_actor_frame_presentation_draw_rectangles.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_draw_rectangles.cpp)；[src/battle/legacy_battle_actor_frame_presentation_late_cases.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_late_cases.cpp)；[src/battle/legacy_battle_actor_frame_presentation_late_decoder.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_late_decoder.cpp)；[src/battle/legacy_battle_actor_frame_presentation_rectangle_phases.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_rectangle_phases.cpp)；[src/battle/legacy_battle_actor_frame_presentation_resource_pipeline.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_resource_pipeline.cpp)；[src/battle/legacy_battle_actor_frame_presentation_selector_audio.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_selector_audio.cpp)；[src/battle/legacy_battle_actor_frame_route_internal.hpp](../../../src/battle/legacy_battle_actor_frame_route_internal.hpp)；[src/battle/legacy_battle_actor_frame_route_phase_1.cpp](../../../src/battle/legacy_battle_actor_frame_route_phase_1.cpp)；[src/battle/legacy_battle_actor_frame_route_phase_2.cpp](../../../src/battle/legacy_battle_actor_frame_route_phase_2.cpp)；[src/battle/legacy_battle_actor_frame_route_phase_3.cpp](../../../src/battle/legacy_battle_actor_frame_route_phase_3.cpp)；[src/battle/legacy_battle_actor_frame_route_phase_4.cpp](../../../src/battle/legacy_battle_actor_frame_route_phase_4.cpp)；[src/battle/legacy_battle_actor_frame_route_phase_5.cpp](../../../src/battle/legacy_battle_actor_frame_route_phase_5.cpp)；[src/battle/legacy_battle_actor_frame_route_phase_6.cpp](../../../src/battle/legacy_battle_actor_frame_route_phase_6.cpp)。
  - 类型：[PORT-44e623b4acb6](invoke-removal-ports.md#port-44e623b4acb6), [PORT-7a299cc4c0e6](invoke-removal-ports.md#port-7a299cc4c0e6), [PORT-bce4a6a79a59](invoke-removal-ports.md#port-bce4a6a79a59), [PORT-72f3184c4566](invoke-removal-ports.md#port-72f3184c4566), [PORT-0711ed9a27e9](invoke-removal-ports.md#port-0711ed9a27e9), [PORT-116407424685](invoke-removal-ports.md#port-116407424685), [PORT-2580ef150bce](invoke-removal-ports.md#port-2580ef150bce), [PORT-6a1f12d149d4](invoke-removal-ports.md#port-6a1f12d149d4), [PORT-393f36387a26](invoke-removal-ports.md#port-393f36387a26), [PORT-8a3cdbb661bb](invoke-removal-ports.md#port-8a3cdbb661bb), [PORT-0e88be68f775](invoke-removal-ports.md#port-0e88be68f775), [PORT-1fa5cc29946f](invoke-removal-ports.md#port-1fa5cc29946f), [PORT-323464d4f75c](invoke-removal-ports.md#port-323464d4f75c)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp)：770, 885, 902
  - `counter` [include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp)：984, 985, 986, 987, 988, 989, 990, 991, 996, 997, 2255
  - `register` [include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp)：538, 539, 540, 551, 552, 553, 560, 561, 562, 579, 580, 581, 587, 588, 589, 590, 591, 592, 593, 727, 728, 729, 751, 752, 753, 771, 772, 773, 780, 781, 782, 797, 798, 799, 815, 816, 817, 827, 828, 829, 840, 841, 842, 874, 875, 876, 886, 887, 888, 905, 906, 907, 924, 925, 926, 931, 932, 933, 941, 942, 943, 944, 945, 946, 947, 948, 1065, 1097, 1098, 1099, 1100, 1113, 1115, 1116, 1117
  - `register` [src/battle/legacy_battle_actor_frame_decoder_commands.hpp](../../../src/battle/legacy_battle_actor_frame_decoder_commands.hpp)：45, 46, 53, 59, 63, 65, 69, 71, 75, 77, 81, 83, 91, 95, 96, 97, 127, 131, 154, 171, 172, 174, 175, 176, 178, 179, 180, 185, 186, 198, 200, 201, 203, 204, 234, 236, 244, 254, 255, 261, 263, 265, 268, 270, 304, 308, 310, 321, 323, 328, 335, 344, 351, 353, 354, 356, 357, 380, 384, 396, 398, 399, 401, 402, 407, 408, 431, 461, 471, 484, 485, 487, 489, 490, 492, 493, 502, 514, 515, 517, 518, 521, 534
  - `register` [src/battle/legacy_battle_actor_frame_decoder_heap_stats.hpp](../../../src/battle/legacy_battle_actor_frame_decoder_heap_stats.hpp)：82, 85, 89, 90, 105, 149, 154, 158, 159, 165, 171, 184, 190, 197, 217, 221, 223, 232, 237, 239, 246, 251, 253, 270, 274, 276, 282, 286, 289, 291, 293, 296, 298, 302, 303, 306, 308, 310, 311, 314, 316, 319, 321, 324, 326, 329, 331, 334, 336, 339, 341, 344, 346, 364, 368, 391, 392, 395, 397, 401, 402, 403, 422, 423, 424, 425, 427, 430, 434, 444, 446, 449, 450, 451, 452, 453, 456, 458, 462, 467, 468, 470, 471, 492, 496, 497, 507, 510, 514, 517, 521, 523, 524, 528, 550, 551, 552, 554, 558, 560, 564, 565, 586, 589, 590, 592, 596, 598, 602, 613, 615, 617, 620, 621, 622, 623, 624, 627, 629, 633, 634, 635, 636, 644, 651, 657, 663, 664, 671, 672, 674, 675, 676, 679, 681, 682, 694, 696, 702, 707, 708, 710, 713, 716, 722, 728, 729, 737, 739, 743, 745, 749, 753, 759, 761, 762, 765, 767, 769, 772, 795, 796, 799, 801, 805, 806, 807, 829, 831, 832, 833, 836, 838, 842, 844, 848, 859, 861, 863, 866, 867, 868, 869, 870, 873, 875, 879, 884, 885, 887, 888, 889, 892, 894, 895, 911, 914, 920, 924, 926, 928, 929, 933, 939, 944, 945, 946, 953, 963, 965, 969, 971, 975, 979, 985, 987, 988, 994, 1004, 1006, 1010, 1011, 1013, 1023, 1025, 1029, 1032, 1034, 1038, 1041, 1043, 1047, 1048, 1050, 1060, 1062, 1066, 1070, 1076, 1078, 1079, 1085, 1092, 1093, 1099, 1109, 1111, 1115, 1122, 1132, 1134, 1138, 1142, 1148, 1150, 1151, 1153, 1163, 1165, 1169, 1173, 1179, 1181, 1182, 1190, 1197, 1228
  - `callee` [src/battle/legacy_battle_actor_frame_io_helpers.hpp](../../../src/battle/legacy_battle_actor_frame_io_helpers.hpp)：337, 341, 342, 343, 345, 346, 347, 348, 356, 360, 362, 365, 368, 369, 370, 374, 375, 379, 380, 381, 384, 387, 391, 393, 395, 396, 397, 398, 401, 402, 403, 406, 409, 413, 415, 416, 417, 421, 422, 423, 424, 426, 430, 433, 434, 435, 436, 440, 441, 442, 443, 446, 447, 448, 449, 450, 451, 454, 456, 458, 459, 462, 466, 468, 469, 470, 474, 475, 476, 477, 479, 482, 483, 486, 488, 490, 492, 495, 496, 497, 498, 499, 502, 506, 508, 509, 510, 514, 520, 525, 527, 528, 529, 530, 533, 536, 540, 542, 543, 544, 547, 548, 549, 552, 554, 556, 557, 558, 561, 562, 565, 566, 569, 570, 573, 574, 575, 578, 579, 580, 583, 584, 585, 586, 588, 591, 592, 595, 596, 598, 599, 600, 605, 608, 609, 610, 613, 614, 617, 620, 624, 626, 627, 628, 632, 633, 635, 637, 638, 639, 640, 643, 644, 646, 647, 654, 655, 659, 660, 661, 665, 666, 667, 671, 672, 673, 674, 675, 676, 677, 678, 682, 683, 684, 688, 689, 690, 696, 698, 702, 703, 704, 705, 706, 707, 708, 712, 713, 714, 718, 722, 724, 727, 728, 732, 733, 734, 738, 739, 740, 744, 745, 746, 747, 748, 749, 750, 751, 752, 756, 757, 758, 762, 763, 764, 765, 766, 773, 777, 778, 780, 784, 785, 787, 788, 789, 790, 791, 792, 795, 797, 803, 807, 808, 809, 813, 814, 818, 819, 821, 823, 825, 826, 836, 840, 841, 845, 847, 848, 853, 854, 855, 857, 865, 869, 870, 871, 873, 874, 875, 876
  - `register` [src/battle/legacy_battle_actor_frame_io_helpers.hpp](../../../src/battle/legacy_battle_actor_frame_io_helpers.hpp)：52, 55, 57, 59, 60, 62, 67, 70, 72, 85, 92, 93, 96, 99, 100, 104, 108, 109, 111, 112, 115, 116, 117, 122, 125, 128, 132, 133, 136, 141, 142, 143, 144, 145, 146, 147, 148, 149, 152, 172, 173, 176, 179, 199, 200, 203, 206, 209, 212, 215, 220, 223, 226, 229, 243, 246, 250, 252, 253, 254, 255, 256, 259, 260, 261, 262, 263, 273, 295, 299, 302, 303, 304, 307, 309, 312, 315, 316, 319, 322, 323, 324, 346, 374, 380, 384, 387, 393, 396, 397, 398, 401, 402, 403, 422, 423, 433, 434, 435, 441, 443, 446, 447, 448, 450, 451, 456, 475, 476, 477, 479, 482, 483, 488, 492, 495, 496, 497, 520, 525, 530, 547, 549, 561, 562, 565, 566, 569, 570, 573, 574, 575, 578, 579, 580, 583, 584, 585, 588, 599, 605, 608, 609, 610, 613, 614, 635, 638, 654, 655, 659, 660, 661, 665, 666, 667, 671, 672, 673, 675, 678, 682, 683, 684, 688, 689, 690, 698, 702, 703, 704, 705, 706, 707, 708, 712, 713, 714, 724, 728, 732, 733, 734, 738, 739, 740, 744, 745, 746, 748, 749, 750, 751, 752, 756, 757, 758, 762, 763, 764, 766, 773, 777, 778, 780, 784, 785, 787, 788, 790, 791, 797, 803, 807, 808, 809, 813, 814, 845, 847, 855, 874, 895, 898, 911, 915, 916, 917, 922, 929, 940, 944, 960, 962, 963, 964, 966, 1010, 1011, 1026
  - `counter` [src/battle/legacy_battle_actor_frame_presentation.cpp](../../../src/battle/legacy_battle_actor_frame_presentation.cpp)：712, 913
  - `register` [src/battle/legacy_battle_actor_frame_presentation.cpp](../../../src/battle/legacy_battle_actor_frame_presentation.cpp)：28, 29, 30, 31, 32, 33, 34, 40, 41, 42, 43, 44, 45, 46, 47, 153, 163, 171, 178, 184, 186, 191, 192, 212, 213, 239, 240, 253, 254, 255, 281, 286, 296, 305, 321, 323, 326, 360, 397, 398, 399, 402, 403, 405, 410, 411, 415, 417, 421, 423, 425, 426, 428, 436, 437, 438, 442, 444, 448, 449, 453, 459, 535, 538, 539, 552, 555, 558, 564, 567, 572, 574, 576, 577, 578, 582, 585, 586, 613, 617, 665, 666, 667, 668, 671, 700, 721, 722, 723, 724, 725, 726, 727, 728, 741, 742, 743, 744, 745, 746, 747, 748, 799, 804, 816, 817, 820, 832, 833, 834, 835, 872, 882, 891, 897, 902, 903, 904, 905, 918, 923, 924, 927, 930, 933, 934, 937, 953, 955, 968, 970, 981, 1001, 1002, 1003, 1006, 1013, 1014, 1015, 1016, 1023, 1028, 1031, 1034, 1035, 1036, 1037, 1040, 1041, 1046, 1051
  - `arguments` [src/battle/legacy_battle_actor_frame_presentation_case_five.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_case_five.cpp)：483, 492, 1007, 1016
  - `callee` [src/battle/legacy_battle_actor_frame_presentation_case_five.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_case_five.cpp)：470, 471, 472, 478, 479, 482, 496, 992, 993, 994, 1002, 1003, 1006, 1020
  - `counter` [src/battle/legacy_battle_actor_frame_presentation_case_five.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_case_five.cpp)：469, 991, 1152
  - `register` [src/battle/legacy_battle_actor_frame_presentation_case_five.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_case_five.cpp)：57, 59, 63, 68, 73, 74, 75, 86, 87, 92, 93, 94, 96, 102, 171, 173, 177, 178, 188, 189, 190, 200, 201, 206, 216, 291, 292, 300, 310, 318, 327, 333, 336, 337, 348, 349, 354, 362, 363, 368, 372, 378, 382, 388, 389, 390, 394, 402, 403, 404, 405, 451, 454, 467, 492, 500, 501, 502, 503, 508, 514, 519, 520, 522, 565, 575, 580, 583, 589, 595, 598, 599, 602, 605, 613, 618, 651, 652, 665, 666, 667, 683, 736, 737, 812, 813, 821, 831, 844, 853, 862, 871, 880, 881, 892, 895, 903, 904, 909, 910, 914, 923, 931, 932, 933, 936, 937, 938, 973, 976, 989, 1016, 1024, 1025, 1026, 1027, 1073, 1074, 1077, 1090, 1134, 1137, 1150, 1218, 1219, 1229, 1239, 1257, 1266, 1275, 1284, 1293, 1294, 1305, 1306, 1309, 1312, 1313, 1315, 1324, 1325, 1330, 1334, 1343, 1351, 1352, 1353, 1356, 1357, 1358, 1359, 1381, 1387, 1395, 1401, 1406, 1433, 1439, 1442, 1443, 1444, 1447, 1450, 1453, 1454, 1462, 1467, 1581, 1585, 1586, 1587, 1632, 1635, 1639, 1640, 1651, 1654, 1655, 1656, 1662, 1665, 1668, 1669, 1672, 1673, 1679, 1687, 1688, 1690, 1695, 1703, 1704, 1706, 1708, 1709, 1710, 1711, 1719, 1727, 1728, 1731, 1732, 1740, 1748, 1749, 1750, 1753, 1754, 1762, 1767, 1856, 1859, 1887, 1888, 1890, 1901, 1907, 1911, 1914, 1917, 1920, 1924, 1925, 1933, 1938
  - `arguments` [src/battle/legacy_battle_actor_frame_presentation_decoder.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_decoder.cpp)：2329, 2336
  - `counter` [src/battle/legacy_battle_actor_frame_presentation_decoder.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_decoder.cpp)：81
  - `register` [src/battle/legacy_battle_actor_frame_presentation_decoder.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_decoder.cpp)：60, 63, 76, 90, 96, 111, 112, 115, 129, 130, 131, 140, 171, 177, 180, 193, 197, 201, 203, 221, 227, 230, 231, 232, 236, 241, 252, 257, 260, 261, 262, 308, 310, 314, 316, 320, 324, 326, 337, 342, 347, 354, 359, 371, 376, 397, 400, 403, 407, 408, 409, 411, 415, 449, 454, 455, 460, 464, 468, 480, 481, 482, 485, 487, 488, 489, 493, 498, 506, 507, 510, 528, 529, 530, 540, 545, 546, 549, 550, 553, 554, 578, 579, 580, 581, 582, 583, 585, 587, 590, 591, 594, 595, 597, 598, 599, 600, 601, 602, 620, 646, 650, 653, 655, 656, 659, 672, 674, 676, 689, 699, 701, 703, 705, 707, 709, 711, 714, 716, 718, 720, 722, 741, 744, 745, 746, 748, 752, 760, 765, 766, 767, 772, 774, 775, 779, 783, 784, 789, 793, 794, 797, 799, 803, 804, 809, 815, 819, 820, 822, 826, 827, 830, 836, 840, 841, 842, 844, 846, 852, 856, 859, 860, 862, 875, 878, 880, 889, 895, 901, 905, 906, 907, 910, 912, 913, 916, 918, 920, 925, 927, 947, 950, 951, 953, 954, 959, 964, 966, 968, 976, 980, 982, 983, 985, 989, 990, 991, 994, 996, 997, 1000, 1002, 1006, 1007, 1009, 1011, 1012, 1017, 1023, 1033, 1037, 1039, 1040, 1043, 1045, 1046, 1051, 1052, 1056, 1058, 1062, 1063, 1064, 1073, 1075, 1076, 1079, 1081, 1082, 1087, 1152, 1155, 1158, 1159, 1160, 1172, 1179, 1184, 1188, 1192, 1193, 1194, 1209, 1210, 1211, 1212, 1218, 1219, 1220, 1221, 1224, 1226, 1227, 1232, 1241, 1243, 1249, 1251, 1255, 1263, 1268, 1269, 1271, 1272, 1275, 1283, 1289, 1297, 1299, 1304, 1308, 1315, 1317, 1318, 1320, 1328, 1330, 1335, 1339, 1346, 1348, 1349, 1356, 1361, 1372, 1379, 1394, 1409, 1410, 1411, 1412, 1413, 1415, 1416, 1421, 1423, 1424, 1425, 1426, 1428, 1431, 1432, 1433, 1435, 1442, 1443, 1446, 1448, 1456, 1458, 1459, 1460, 1476, 1485, 1487, 1488, 1489, 1505, 1514, 1518, 1521, 1541, 1542, 1543, 1544, 1545, 1546, 1547, 1552, 1554, 1555, 1556, 1557, 1559, 1562, 1563, 1564, 1566, 1573, 1574, 1580, 1581, 1589, 1591, 1592, 1593, 1606, 1614, 1616, 1617, 1618, 1631, 1639, 1643, 1646, 1656, 1664, 1672, 1677, 1679, 1684, 1686, 1691, 1692, 1694, 1703, 1705, 1710, 1713, 1718, 1720, 1721, 1727, 1730, 1736, 1747, 1756, 1766, 1770, 1780, 1782, 1784, 1789, 1790, 1794, 1795, 1800, 1810, 1813, 1817, 1820, 1832, 1837, 1838, 1839, 1841, 1846, 1852, 1873, 1874, 1875, 1876, 1883, 1884, 1885, 1888, 1889, 1890, 1891, 1893, 1895, 1898, 1902, 1905, 1907, 1912, 1916, 1923, 1925, 1926, 1927, 1928, 1946, 1950, 1951, 1968, 1972, 2008, 2010, 2014, 2017, 2019, 2021, 2024, 2026, 2028, 2031, 2033, 2036, 2040, 2042, 2045, 2048, 2052, 2054, 2056, 2059, 2061, 2063, 2067, 2090, 2091, 2094, 2096, 2100, 2101, 2102, 2123, 2124, 2125, 2126, 2128, 2131, 2135, 2145, 2147, 2150, 2151, 2152, 2153, 2154, 2157, 2159, 2163, 2168, 2169, 2171, 2172, 2194, 2198, 2199, 2210, 2212, 2215, 2219, 2223, 2229, 2231, 2232, 2236, 2259, 2260, 2263, 2265, 2269, 2271, 2275, 2276, 2336, 2358, 2359, 2360, 2361, 2362, 2363, 2364
  - `arguments` [src/battle/legacy_battle_actor_frame_presentation_decoder_continuations.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_decoder_continuations.cpp)：2089, 2098
  - `callee` [src/battle/legacy_battle_actor_frame_presentation_decoder_continuations.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_decoder_continuations.cpp)：1088, 1089, 1092, 1095, 1098, 1099, 1100, 1104, 1105, 1109, 1110, 1111, 1115, 1119, 1122, 1123, 1124, 1126, 1127, 1128, 1129, 1131, 1132, 1133, 1134, 1137, 1140, 1142, 1143, 1144, 1145, 1147, 1151, 1162, 1281, 1282, 1283, 1289, 1290, 1291, 1294, 1295, 1306, 1667, 1668, 1669, 1674, 1675, 1676, 1679, 1680, 1691, 2077, 2078, 2079, 2084, 2085, 2088, 2102
  - `counter` [src/battle/legacy_battle_actor_frame_presentation_decoder_continuations.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_decoder_continuations.cpp)：920, 953, 1080, 1280, 1402, 1666, 2076
  - `register` [src/battle/legacy_battle_actor_frame_presentation_decoder_continuations.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_decoder_continuations.cpp)：76, 84, 91, 93, 108, 121, 124, 139, 141, 230, 232, 269, 271, 281, 284, 286, 290, 298, 302, 308, 316, 379, 381, 395, 397, 431, 439, 440, 441, 445, 455, 463, 464, 465, 467, 471, 480, 491, 499, 500, 501, 504, 507, 508, 511, 512, 513, 514, 515, 519, 526, 532, 537, 545, 594, 596, 605, 606, 626, 628, 632, 635, 636, 637, 638, 639, 640, 647, 649, 659, 660, 663, 664, 719, 729, 741, 743, 753, 756, 759, 768, 773, 775, 776, 777, 781, 795, 808, 809, 818, 886, 896, 912, 922, 932, 933, 936, 937, 938, 939, 942, 945, 946, 955, 965, 966, 969, 970, 971, 972, 975, 978, 979, 1009, 1010, 1012, 1025, 1029, 1032, 1033, 1063, 1076, 1104, 1110, 1122, 1123, 1126, 1127, 1128, 1131, 1132, 1133, 1143, 1155, 1156, 1157, 1158, 1165, 1166, 1167, 1168, 1197, 1203, 1221, 1223, 1236, 1240, 1265, 1278, 1282, 1298, 1299, 1300, 1301, 1310, 1311, 1312, 1313, 1314, 1316, 1343, 1350, 1356, 1375, 1388, 1406, 1407, 1408, 1409, 1410, 1422, 1423, 1424, 1425, 1428, 1430, 1434, 1452, 1458, 1472, 1478, 1481, 1482, 1483, 1486, 1489, 1492, 1493, 1501, 1506, 1527, 1533, 1539, 1557, 1562, 1591, 1612, 1614, 1627, 1631, 1651, 1664, 1668, 1683, 1684, 1685, 1686, 1695, 1696, 1697, 1698, 1699, 1701, 1739, 1741, 1745, 1746, 1756, 1757, 1758, 1765, 1766, 1767, 1773, 1774, 1784, 1789, 1790, 1794, 1798, 1800, 1801, 1802, 1803, 1804, 1805, 1816, 1826, 1850, 1880, 1882, 1911, 1921, 1934, 1943, 1951, 1954, 1957, 1958, 1962, 1965, 1969, 1973, 1977, 1981, 1985, 1987, 1991, 2000, 2006, 2010, 2016, 2017, 2021, 2029, 2030, 2031, 2058, 2061, 2074, 2098, 2106, 2107, 2108, 2109, 2110, 2112, 2144, 2165, 2168, 2216, 2222, 2228, 2239, 2245, 2251, 2283, 2292, 2325, 2326, 2327, 2328, 2334, 2341, 2342, 2343, 2344, 2387, 2413, 2420, 2423, 2424, 2426, 2427, 2428, 2429, 2434, 2439, 2440, 2445, 2446, 2452, 2453, 2470, 2471, 2472, 2473, 2474, 2475, 2476, 2510, 2512, 2525, 2529
  - `arguments` [src/battle/legacy_battle_actor_frame_presentation_draw_arguments.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_draw_arguments.cpp)：488, 497, 1489, 1493, 1508, 1510
  - `callee` [src/battle/legacy_battle_actor_frame_presentation_draw_arguments.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_draw_arguments.cpp)：442, 443, 444, 483, 484, 487, 501
  - `counter` [src/battle/legacy_battle_actor_frame_presentation_draw_arguments.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_draw_arguments.cpp)：441
  - `register` [src/battle/legacy_battle_actor_frame_presentation_draw_arguments.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_draw_arguments.cpp)：84, 85, 99, 100, 101, 106, 110, 119, 129, 140, 148, 150, 151, 152, 156, 165, 174, 178, 193, 202, 209, 210, 213, 214, 215, 223, 227, 242, 252, 256, 257, 263, 271, 272, 274, 275, 277, 278, 281, 282, 284, 285, 291, 297, 409, 412, 425, 497, 505, 506, 507, 508, 618, 619, 644, 653, 662, 671, 680, 690, 698, 700, 701, 704, 711, 712, 714, 715, 716, 717, 721, 727, 731, 737, 744, 745, 746, 747, 748, 749, 750, 751, 752, 756, 762, 763, 764, 765, 766, 767, 771, 779, 780, 781, 782, 783, 784, 785, 849, 850, 865, 866, 867, 876, 885, 894, 902, 908, 917, 923, 928, 935, 942, 950, 951, 952, 953, 954, 955, 956, 957, 958, 959, 960, 961, 964, 965, 969, 977, 978, 979, 980, 981, 982, 983, 987, 995, 996, 997, 998, 999, 1000, 1001, 1002, 1066, 1067, 1082, 1083, 1084, 1093, 1102, 1111, 1119, 1124, 1133, 1139, 1144, 1151, 1158, 1164, 1165, 1169, 1177, 1178, 1179, 1180, 1181, 1184, 1185, 1186, 1187, 1188, 1189, 1190, 1191, 1195, 1201, 1202, 1203, 1207, 1215, 1216, 1217, 1218, 1219, 1220, 1221, 1222, 1223, 1224, 1225, 1226, 1290, 1291, 1305, 1306, 1307, 1312, 1316, 1325, 1333, 1342, 1348, 1350, 1351, 1352, 1355, 1358, 1369, 1373, 1376, 1387, 1393, 1397, 1403, 1404, 1405, 1409, 1415, 1418, 1419, 1420, 1421, 1422, 1450, 1452, 1454, 1457, 1494, 1507, 1538, 1540, 1541, 1552, 1558, 1565, 1566, 1572, 1573, 1579, 1580, 1586, 1587, 1591, 1592, 1602, 1607
  - `arguments` [src/battle/legacy_battle_actor_frame_presentation_draw_rectangles.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_draw_rectangles.cpp)：1200, 1204, 1219, 1221
  - `register` [src/battle/legacy_battle_actor_frame_presentation_draw_rectangles.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_draw_rectangles.cpp)：88, 89, 104, 105, 106, 115, 124, 135, 146, 152, 154, 156, 157, 164, 168, 175, 184, 193, 201, 207, 214, 225, 227, 233, 234, 240, 248, 249, 252, 253, 254, 255, 257, 258, 263, 267, 268, 273, 281, 290, 296, 301, 307, 309, 310, 311, 313, 317, 323, 328, 335, 343, 344, 347, 348, 349, 350, 352, 355, 356, 360, 369, 375, 376, 377, 378, 379, 452, 453, 468, 469, 470, 479, 488, 497, 505, 511, 518, 524, 529, 536, 542, 543, 547, 555, 556, 557, 558, 559, 560, 564, 572, 573, 574, 575, 576, 580, 586, 587, 588, 589, 590, 594, 602, 603, 604, 605, 606, 607, 671, 672, 698, 707, 716, 725, 734, 740, 742, 743, 746, 748, 754, 759, 765, 766, 767, 771, 777, 778, 781, 782, 783, 784, 788, 790, 797, 805, 806, 807, 808, 809, 810, 811, 814, 818, 824, 825, 826, 827, 828, 829, 833, 841, 842, 843, 912, 913, 928, 929, 930, 939, 948, 957, 966, 972, 974, 975, 976, 979, 982, 987, 993, 998, 1002, 1006, 1015, 1019, 1023, 1031, 1032, 1036, 1042, 1043, 1044, 1045, 1048, 1049, 1050, 1057, 1066, 1072, 1074, 1075, 1076, 1079, 1083, 1089, 1091, 1096, 1103, 1110, 1114, 1118, 1127, 1135, 1136, 1137, 1138, 1139, 1140, 1141, 1142, 1166, 1169, 1196, 1197, 1200, 1205, 1218, 1288, 1289, 1297, 1306, 1315, 1324, 1335, 1336, 1337, 1342, 1343, 1347, 1353, 1354, 1355, 1359, 1365, 1366, 1367, 1368, 1371, 1376, 1377, 1378, 1383, 1384, 1385, 1394, 1403, 1407, 1415, 1416, 1417, 1418, 1419, 1420, 1421, 1422, 1423, 1426, 1427, 1428, 1492, 1493, 1501, 1510, 1519, 1528, 1538, 1544, 1545, 1546, 1548, 1549, 1552, 1553, 1554, 1555, 1557, 1558, 1559, 1560, 1564, 1573, 1574, 1575, 1580, 1581, 1585, 1593, 1594, 1595, 1596, 1597, 1598, 1599, 1600, 1601, 1602, 1603, 1604, 1668, 1669, 1677, 1686, 1694, 1703, 1709, 1714, 1715, 1716, 1721, 1722, 1723, 1725, 1727, 1728, 1731, 1735, 1743, 1748, 1749, 1750, 1755, 1756, 1760, 1766, 1767, 1768, 1772, 1779, 1787, 1788, 1791, 1792, 1793, 1794, 1796, 1797, 1798, 1801, 1802, 1803
  - `arguments` [src/battle/legacy_battle_actor_frame_presentation_late_cases.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_late_cases.cpp)：610, 618, 2398, 2407
  - `callee` [src/battle/legacy_battle_actor_frame_presentation_late_cases.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_late_cases.cpp)：560, 561, 563, 564, 565, 568, 569, 571, 573, 575, 576, 577, 580, 581, 582, 585, 586, 592, 596, 598, 599, 600, 601, 603, 609, 627, 1026, 1027, 1028, 1033, 1034, 1035, 1038, 1039, 1050, 1542, 1543, 1544, 1552, 1553, 1554, 1557, 1558, 1569, 2376, 2377, 2378, 2393, 2394, 2397, 2411
  - `counter` [src/battle/legacy_battle_actor_frame_presentation_late_cases.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_late_cases.cpp)：1025, 1541, 2375
  - `register` [src/battle/legacy_battle_actor_frame_presentation_late_cases.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_late_cases.cpp)：33, 39, 44, 45, 46, 53, 107, 109, 139, 147, 156, 157, 169, 173, 176, 180, 186, 191, 192, 194, 196, 197, 202, 207, 213, 218, 221, 222, 223, 225, 227, 229, 283, 285, 289, 294, 295, 300, 309, 313, 318, 320, 334, 339, 404, 405, 413, 422, 436, 445, 452, 453, 458, 459, 464, 465, 470, 471, 475, 484, 490, 493, 494, 498, 506, 507, 508, 509, 542, 545, 558, 563, 565, 568, 581, 585, 621, 622, 623, 630, 631, 632, 633, 661, 668, 693, 695, 733, 745, 751, 754, 755, 756, 757, 760, 761, 769, 774, 795, 801, 806, 814, 819, 845, 846, 851, 857, 862, 867, 868, 871, 877, 882, 884, 886, 887, 889, 908, 946, 948, 952, 958, 960, 974, 976, 986, 990, 1010, 1023, 1027, 1041, 1042, 1043, 1044, 1045, 1054, 1055, 1056, 1057, 1058, 1060, 1106, 1108, 1112, 1113, 1123, 1124, 1126, 1133, 1134, 1139, 1140, 1150, 1155, 1157, 1161, 1164, 1166, 1176, 1201, 1239, 1240, 1253, 1263, 1271, 1280, 1288, 1291, 1294, 1295, 1299, 1302, 1310, 1311, 1316, 1317, 1324, 1325, 1330, 1331, 1335, 1344, 1360, 1365, 1370, 1371, 1375, 1384, 1385, 1386, 1387, 1391, 1399, 1400, 1401, 1432, 1459, 1461, 1474, 1482, 1524, 1537, 1543, 1561, 1562, 1563, 1564, 1573, 1574, 1575, 1576, 1577, 1579, 1709, 1711, 1715, 1722, 1723, 1728, 1743, 1749, 1751, 1756, 1760, 1763, 1764, 1771, 1820, 1822, 1826, 1833, 1834, 1839, 1849, 1854, 1856, 1861, 1865, 1868, 1869, 1879, 1928, 1930, 1959, 1961, 1965, 1966, 1976, 1977, 1978, 1986, 1987, 1992, 1999, 2001, 2008, 2010, 2017, 2019, 2026, 2063, 2110, 2111, 2119, 2129, 2142, 2151, 2159, 2162, 2165, 2166, 2171, 2181, 2182, 2183, 2189, 2196, 2199, 2200, 2207, 2208, 2213, 2218, 2219, 2224, 2228, 2234, 2235, 2239, 2248, 2256, 2257, 2258, 2259, 2260, 2261, 2352, 2355, 2368, 2407, 2415, 2416, 2417, 2418, 2431, 2433, 2596, 2598, 2613, 2617, 2627, 2628, 2629, 2631, 2638, 2640, 2647, 2649, 2656, 2704, 2739, 2740, 2753, 2763, 2771, 2780, 2788, 2791, 2794, 2795, 2799, 2802, 2810, 2811, 2816, 2817, 2824, 2825, 2830, 2831, 2835, 2844, 2850, 2854, 2863, 2864, 2868, 2874, 2878, 2886, 2887, 2888, 2889, 2890, 2891, 2918, 2953, 2954, 2967, 2977, 2985, 2994, 3002, 3005, 3008, 3009, 3013, 3016, 3024, 3025, 3030, 3031, 3038, 3039, 3044, 3045, 3049, 3058, 3065, 3068, 3069, 3070, 3074, 3083, 3091, 3092, 3093, 3097, 3105, 3106, 3107, 3152, 3155, 3168, 3186, 3197, 3203, 3208, 3209, 3213, 3214, 3218, 3219, 3225, 3226, 3230, 3231, 3240, 3245
  - `callee` [src/battle/legacy_battle_actor_frame_presentation_late_decoder.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_late_decoder.cpp)：345, 347, 349, 354, 355, 356, 359, 360, 361, 372, 1636, 1638, 1640, 1642, 1648
  - `counter` [src/battle/legacy_battle_actor_frame_presentation_late_decoder.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_late_decoder.cpp)：344, 1364, 1397, 1633
  - `register` [src/battle/legacy_battle_actor_frame_presentation_late_decoder.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_late_decoder.cpp)：32, 34, 40, 48, 86, 92, 102, 103, 105, 115, 160, 166, 176, 184, 210, 216, 219, 220, 221, 224, 227, 230, 231, 239, 244, 281, 283, 296, 300, 329, 342, 347, 364, 365, 366, 367, 376, 377, 378, 379, 380, 382, 434, 437, 440, 441, 445, 446, 447, 452, 454, 455, 460, 464, 475, 479, 486, 488, 490, 542, 544, 548, 557, 570, 574, 575, 578, 581, 582, 589, 591, 595, 596, 599, 605, 606, 607, 611, 617, 618, 624, 630, 631, 634, 635, 641, 642, 643, 644, 669, 678, 690, 691, 693, 694, 697, 700, 701, 705, 706, 709, 712, 715, 716, 719, 720, 724, 747, 748, 749, 782, 783, 794, 799, 800, 803, 809, 814, 840, 846, 849, 850, 853, 856, 859, 860, 868, 873, 895, 901, 907, 931, 944, 949, 951, 952, 953, 958, 964, 969, 970, 975, 976, 982, 987, 988, 1038, 1040, 1041, 1044, 1045, 1047, 1051, 1053, 1054, 1058, 1059, 1063, 1064, 1067, 1070, 1072, 1073, 1077, 1078, 1082, 1105, 1114, 1131, 1132, 1134, 1139, 1145, 1150, 1151, 1152, 1153, 1158, 1163, 1171, 1172, 1196, 1205, 1230, 1231, 1246, 1247, 1253, 1259, 1260, 1262, 1304, 1305, 1318, 1319, 1332, 1338, 1344, 1376, 1389, 1399, 1406, 1407, 1410, 1412, 1413, 1414, 1422, 1425, 1426, 1427, 1430, 1433, 1434, 1454, 1460, 1494, 1503, 1524, 1534, 1535, 1542, 1545, 1547, 1548, 1549, 1556, 1577, 1583, 1588, 1589, 1590, 1611, 1613, 1626, 1644, 1652, 1653, 1654, 1655, 1656, 1658, 1691, 1697, 1702, 1712, 1713, 1720, 1721, 1725, 1731, 1736, 1738, 1740, 1741, 1742, 1749, 1750, 1754, 1812, 1813, 1830, 1831, 1832, 1854, 1858, 1867, 1876, 1885, 1895, 1905, 1913, 1914, 1917, 1920, 1921, 1925, 1926, 1930, 1938, 1939, 1940, 1941, 1942, 1945, 1950, 1959, 1962, 1963, 1966, 1970, 1978, 1979, 1980, 1981, 1982, 1983, 1984, 1985, 1986, 1990, 1999, 2000, 2001, 2002, 2003
  - `arguments` [src/battle/legacy_battle_actor_frame_presentation_rectangle_phases.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_rectangle_phases.cpp)：2810, 2826, 2828
  - `counter` [src/battle/legacy_battle_actor_frame_presentation_rectangle_phases.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_rectangle_phases.cpp)：1345
  - `register` [src/battle/legacy_battle_actor_frame_presentation_rectangle_phases.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_rectangle_phases.cpp)：79, 80, 88, 97, 106, 115, 124, 130, 131, 132, 133, 137, 142, 143, 144, 145, 149, 154, 158, 162, 166, 174, 179, 180, 181, 186, 187, 188, 192, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 212, 217, 218, 219, 220, 221, 285, 286, 294, 303, 311, 320, 326, 328, 330, 331, 334, 336, 342, 343, 344, 349, 350, 351, 356, 357, 358, 363, 364, 365, 369, 375, 379, 385, 386, 390, 397, 405, 406, 409, 410, 411, 412, 414, 415, 416, 419, 420, 421, 485, 486, 494, 503, 512, 521, 527, 532, 533, 534, 539, 540, 544, 550, 551, 555, 563, 564, 565, 568, 573, 574, 575, 580, 581, 585, 593, 594, 597, 598, 599, 600, 602, 603, 604, 605, 606, 607, 608, 609, 610, 611, 612, 616, 622, 623, 624, 625, 626, 690, 691, 699, 708, 716, 725, 731, 733, 735, 736, 739, 742, 747, 748, 749, 754, 755, 756, 761, 762, 763, 768, 769, 770, 774, 783, 787, 791, 797, 798, 802, 810, 811, 814, 815, 816, 817, 819, 820, 821, 824, 825, 826, 877, 879, 885, 887, 889, 906, 908, 920, 991, 992, 1009, 1010, 1011, 1033, 1037, 1046, 1055, 1063, 1064, 1068, 1072, 1081, 1092, 1099, 1102, 1103, 1104, 1108, 1117, 1118, 1122, 1130, 1131, 1132, 1135, 1140, 1147, 1150, 1151, 1152, 1155, 1158, 1159, 1162, 1163, 1164, 1165, 1166, 1167, 1168, 1169, 1170, 1174, 1183, 1184, 1185, 1309, 1312, 1325, 1429, 1435, 1440, 1441, 1442, 1449, 1476, 1482, 1487, 1488, 1489, 1498, 1524, 1530, 1535, 1603, 1605, 1609, 1614, 1615, 1620, 1629, 1633, 1634, 1638, 1639, 1640, 1709, 1710, 1725, 1726, 1727, 1736, 1745, 1754, 1762, 1768, 1777, 1787, 1796, 1797, 1800, 1803, 1804, 1807, 1808, 1812, 1820, 1821, 1822, 1823, 1832, 1836, 1842, 1845, 1850, 1860, 1865, 1866, 1867, 1868, 1869, 1870, 1871, 1872, 1873, 1874, 1875, 1876, 1880, 1889, 1890, 1891, 1892, 1893, 1914, 1927, 1928, 1941, 1942, 1944, 1945, 1946, 1948, 1971, 2034, 2035, 2047, 2048, 2049, 2054, 2058, 2067, 2075, 2084, 2092, 2095, 2098, 2099, 2103, 2106, 2117, 2120, 2134, 2140, 2144, 2153, 2154, 2158, 2164, 2167, 2168, 2169, 2170, 2171, 2172, 2240, 2241, 2272, 2277, 2286, 2295, 2304, 2305, 2308, 2309, 2312, 2314, 2324, 2328, 2337, 2346, 2347, 2348, 2350, 2351, 2352, 2356, 2364, 2365, 2368, 2371, 2372, 2377, 2379, 2389, 2390, 2391, 2392, 2393, 2397, 2405, 2406, 2407, 2411, 2420, 2421, 2425, 2433, 2434, 2435, 2439, 2440, 2441, 2442, 2443, 2444, 2485, 2552, 2553, 2568, 2569, 2570, 2579, 2588, 2601, 2610, 2618, 2624, 2633, 2636, 2639, 2640, 2643, 2644, 2647, 2652, 2661, 2671, 2680, 2681, 2682, 2686, 2694, 2695, 2699, 2704, 2705, 2706, 2707, 2732, 2740, 2750, 2751, 2752, 2754, 2756, 2758, 2759, 2763, 2780, 2810, 2812, 2825, 2895, 2896, 2911, 2912, 2913, 2922, 2931, 2940, 2948, 2954, 2963, 2973, 2982, 2983, 2986, 2989, 2990, 2993, 2994, 2995, 2999, 3007, 3008, 3009, 3010, 3011, 3014, 3019, 3029, 3034, 3038, 3043, 3047, 3055, 3056, 3057, 3058, 3059, 3060, 3061, 3062, 3063, 3067, 3076, 3077, 3078, 3079, 3080, 3144, 3145, 3171, 3180, 3189, 3198, 3207, 3216, 3219, 3220, 3223, 3225, 3234, 3239, 3247, 3248, 3249, 3253, 3261, 3262, 3265, 3268, 3269, 3270, 3274, 3276, 3286, 3294, 3295, 3296, 3297, 3298, 3299, 3300, 3304, 3313, 3314, 3317, 3320, 3321, 3324, 3325, 3329, 3337, 3338, 3339, 3340, 3341, 3342, 3343, 3407, 3408, 3423, 3424, 3425, 3434, 3443, 3451, 3460, 3468, 3471, 3474, 3475, 3479, 3482, 3487, 3496, 3501, 3508, 3512, 3521, 3528, 3532, 3540, 3541, 3545, 3553, 3554, 3557, 3560, 3561, 3562, 3564, 3565, 3566, 3569, 3570, 3571
  - `arguments` [src/battle/legacy_battle_actor_frame_presentation_resource_pipeline.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_resource_pipeline.cpp)：2093, 2102
  - `callee` [src/battle/legacy_battle_actor_frame_presentation_resource_pipeline.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_resource_pipeline.cpp)：94, 96, 98, 100, 106, 1558, 1559, 1560, 1563, 1564, 1566, 1571, 1572, 1584, 2080, 2081, 2082, 2088, 2089, 2092, 2106, 2438, 2440, 2442, 2444, 2450
  - `counter` [src/battle/legacy_battle_actor_frame_presentation_resource_pipeline.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_resource_pipeline.cpp)：91, 249, 553, 1211, 1557, 2079, 2435
  - `register` [src/battle/legacy_battle_actor_frame_presentation_resource_pipeline.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_resource_pipeline.cpp)：47, 56, 57, 64, 69, 70, 72, 84, 102, 109, 110, 111, 112, 115, 117, 118, 119, 120, 131, 137, 140, 141, 142, 143, 152, 157, 159, 162, 163, 164, 165, 168, 169, 176, 181, 217, 227, 236, 242, 246, 251, 255, 256, 261, 266, 267, 268, 276, 285, 287, 288, 289, 290, 298, 304, 305, 306, 313, 319, 324, 330, 344, 353, 357, 358, 359, 360, 365, 371, 376, 378, 379, 382, 383, 384, 400, 403, 404, 407, 410, 411, 415, 422, 425, 426, 427, 428, 431, 432, 437, 442, 479, 489, 507, 509, 522, 524, 539, 540, 555, 573, 574, 575, 589, 594, 605, 615, 618, 619, 622, 624, 625, 626, 676, 678, 693, 695, 713, 716, 717, 718, 721, 725, 729, 730, 747, 753, 754, 755, 758, 761, 762, 765, 768, 769, 772, 773, 825, 827, 841, 843, 852, 880, 887, 901, 907, 925, 926, 928, 929, 931, 937, 946, 951, 952, 953, 954, 956, 964, 1000, 1006, 1009, 1010, 1011, 1014, 1016, 1019, 1020, 1028, 1033, 1053, 1059, 1065, 1105, 1111, 1123, 1124, 1127, 1128, 1131, 1132, 1135, 1141, 1146, 1148, 1150, 1151, 1153, 1201, 1219, 1220, 1221, 1222, 1223, 1224, 1225, 1226, 1239, 1240, 1241, 1242, 1243, 1244, 1245, 1246, 1286, 1300, 1302, 1313, 1316, 1317, 1318, 1322, 1323, 1324, 1328, 1329, 1330, 1339, 1345, 1352, 1353, 1359, 1360, 1366, 1367, 1373, 1374, 1378, 1380, 1387, 1392, 1436, 1446, 1452, 1466, 1467, 1479, 1481, 1485, 1486, 1487, 1490, 1511, 1512, 1519, 1524, 1542, 1555, 1559, 1575, 1576, 1577, 1578, 1590, 1591, 1592, 1595, 1596, 1598, 1642, 1658, 1660, 1667, 1671, 1672, 1673, 1674, 1675, 1677, 1678, 1679, 1681, 1682, 1684, 1687, 1688, 1691, 1694, 1695, 1696, 1708, 1710, 1754, 1756, 1766, 1776, 1787, 1796, 1797, 1802, 1803, 1804, 1814, 1818, 1823, 1824, 1825, 1826, 1841, 1842, 1846, 1889, 1891, 1899, 1909, 1917, 1926, 1933, 1942, 1943, 1948, 1949, 1957, 1958, 1963, 1964, 1968, 1987, 1988, 1991, 1992, 1993, 1997, 2009, 2010, 2014, 2026, 2027, 2028, 2029, 2030, 2061, 2064, 2077, 2102, 2109, 2110, 2111, 2112, 2133, 2136, 2149, 2179, 2181, 2182, 2193, 2199, 2202, 2203, 2204, 2205, 2208, 2209, 2217, 2222, 2242, 2248, 2254, 2264, 2270, 2276, 2306, 2312, 2317, 2327, 2328, 2334, 2335, 2338, 2344, 2349, 2351, 2353, 2354, 2364, 2369, 2370, 2371, 2388, 2413, 2415, 2428, 2446, 2453, 2454, 2455, 2456, 2459, 2461, 2462, 2488, 2491, 2492, 2503, 2509, 2514, 2516, 2518, 2519, 2521, 2554, 2555, 2558, 2559, 2560, 2561, 2572, 2573, 2581, 2586, 2588, 2590, 2591, 2639, 2649, 2654, 2655, 2656, 2657, 2664, 2666, 2670, 2671, 2672, 2683, 2684, 2689, 2690
  - `callee` [src/battle/legacy_battle_actor_frame_presentation_selector_audio.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_selector_audio.cpp)：898, 899, 900, 905, 906, 907, 910, 911, 922, 1910, 1914, 1915, 1934, 1935, 1936, 1939, 1940, 1941, 1954
  - `counter` [src/battle/legacy_battle_actor_frame_presentation_selector_audio.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_selector_audio.cpp)：897, 1479, 1909
  - `register` [src/battle/legacy_battle_actor_frame_presentation_selector_audio.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_selector_audio.cpp)：36, 42, 48, 59, 65, 70, 149, 155, 161, 186, 192, 197, 198, 200, 210, 257, 259, 263, 264, 271, 272, 281, 282, 293, 294, 299, 348, 350, 367, 368, 369, 402, 408, 410, 412, 418, 422, 428, 435, 441, 446, 452, 458, 473, 475, 479, 484, 486, 490, 491, 492, 493, 494, 496, 499, 546, 556, 563, 565, 571, 577, 580, 589, 594, 595, 597, 604, 616, 620, 679, 689, 694, 695, 696, 697, 704, 706, 710, 715, 717, 722, 731, 732, 737, 738, 739, 806, 808, 812, 818, 820, 824, 829, 831, 845, 847, 857, 861, 882, 895, 899, 913, 914, 915, 916, 917, 926, 927, 928, 929, 930, 932, 979, 981, 985, 990, 995, 996, 1019, 1028, 1040, 1046, 1052, 1056, 1057, 1058, 1059, 1060, 1065, 1071, 1085, 1086, 1087, 1089, 1099, 1103, 1106, 1124, 1137, 1171, 1174, 1226, 1241, 1258, 1267, 1285, 1312, 1318, 1321, 1322, 1323, 1326, 1329, 1332, 1333, 1341, 1346, 1376, 1382, 1387, 1397, 1398, 1404, 1405, 1408, 1414, 1419, 1421, 1423, 1424, 1426, 1469, 1487, 1488, 1489, 1490, 1491, 1492, 1493, 1494, 1507, 1508, 1509, 1510, 1511, 1512, 1513, 1514, 1566, 1567, 1577, 1580, 1581, 1582, 1590, 1596, 1599, 1600, 1601, 1602, 1605, 1607, 1615, 1620, 1641, 1647, 1652, 1653, 1654, 1661, 1683, 1689, 1694, 1695, 1696, 1705, 1769, 1771, 1773, 1776, 1789, 1802, 1803, 1804, 1887, 1900, 1911, 1912, 1913, 1944, 1945, 1946, 1947, 1948, 1949, 1958, 1959, 1960, 1961, 1962, 1964, 2119, 2121, 2125, 2127, 2129, 2139, 2140, 2145, 2155, 2218, 2219, 2233, 2234, 2235, 2240, 2244, 2253, 2262, 2271, 2289, 2299, 2307, 2308, 2309, 2313, 2319, 2320, 2321, 2322, 2326, 2332, 2335, 2346, 2352, 2353, 2354, 2355, 2356, 2357, 2358, 2359, 2360, 2361, 2362, 2363, 2364, 2365, 2368, 2369, 2370, 2539, 2545, 2547, 2553, 2554, 2555, 2558, 2559, 2560, 2561, 2562, 2563, 2581, 2602, 2607
  - `register` [src/battle/legacy_battle_actor_frame_route_phase_1.cpp](../../../src/battle/legacy_battle_actor_frame_route_phase_1.cpp)：49
  - `counter` [src/battle/legacy_battle_actor_frame_route_phase_3.cpp](../../../src/battle/legacy_battle_actor_frame_route_phase_3.cpp)：289

- [ ] **CHAIN-e486ef011045** `battle/legacy_battle_actor_frame_resource`
  - 当前残留信号：counter, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_frame_resource.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_resource.hpp)；[src/battle/legacy_battle_actor_frame_resource.cpp](../../../src/battle/legacy_battle_actor_frame_resource.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_actor_frame_resource.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_resource.hpp)：147, 148
  - `register` [include/openswd3/battle/legacy_battle_actor_frame_resource.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_resource.hpp)：79, 80, 81, 82, 83, 84, 85, 126, 127, 128, 129, 130, 131, 132
  - `counter` [src/battle/legacy_battle_actor_frame_resource.cpp](../../../src/battle/legacy_battle_actor_frame_resource.cpp)：338, 445
  - `register` [src/battle/legacy_battle_actor_frame_resource.cpp](../../../src/battle/legacy_battle_actor_frame_resource.cpp)：156, 157, 158, 159, 160, 161, 162, 184, 200, 210, 211, 219, 220, 228, 229, 237, 243, 246, 254, 261, 262, 263, 264, 304, 305, 306, 335, 336, 337, 340, 343, 344, 347, 351, 352, 355, 356, 372, 394, 395, 411, 412, 417, 425, 442, 443, 444, 449, 450, 451, 454, 458, 459, 471, 489

- [ ] **CHAIN-f4333b3d2b8a** `battle/legacy_battle_actor_frame_sequence`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_frame_sequence.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_sequence.hpp)；[src/battle/legacy_battle_actor_frame_sequence.cpp](../../../src/battle/legacy_battle_actor_frame_sequence.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_actor_frame_sequence.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_sequence.hpp)：32, 33
  - `counter` [src/battle/legacy_battle_actor_frame_sequence.cpp](../../../src/battle/legacy_battle_actor_frame_sequence.cpp)：103, 111
  - `register` [src/battle/legacy_battle_actor_frame_sequence.cpp](../../../src/battle/legacy_battle_actor_frame_sequence.cpp)：47, 48, 49, 54, 59, 62, 65, 66, 73, 83, 89, 116, 122, 127, 128

- [ ] **CHAIN-2bba77448872** `battle/legacy_battle_actor_frame_snapshot`
  - 当前残留信号：counter, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_frame_snapshot.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_snapshot.hpp)；[src/battle/legacy_battle_actor_frame_snapshot.cpp](../../../src/battle/legacy_battle_actor_frame_snapshot.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_actor_frame_snapshot.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_snapshot.hpp)：114, 115
  - `register` [include/openswd3/battle/legacy_battle_actor_frame_snapshot.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_snapshot.hpp)：73, 102, 103, 104
  - `counter` [src/battle/legacy_battle_actor_frame_snapshot.cpp](../../../src/battle/legacy_battle_actor_frame_snapshot.cpp)：261, 291
  - `register` [src/battle/legacy_battle_actor_frame_snapshot.cpp](../../../src/battle/legacy_battle_actor_frame_snapshot.cpp)：187, 188, 189, 202, 204, 206, 216, 218, 219, 230, 231, 233, 234, 239, 240, 241, 249, 251, 252, 253, 256, 257, 263, 264, 265, 277, 288, 298, 299, 300, 309, 311, 313, 325, 334, 336, 337, 338, 339, 340, 342, 343, 351, 353, 354, 355, 362, 384, 390, 392, 393, 394, 408, 410, 412, 418, 420, 433, 435, 437, 443, 445

- [ ] **CHAIN-0fd4aadf86b5** `battle/legacy_battle_actor_frame_snapshot_clear`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_frame_snapshot_clear.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_snapshot_clear.hpp)；[src/battle/legacy_battle_actor_frame_snapshot_clear.cpp](../../../src/battle/legacy_battle_actor_frame_snapshot_clear.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_actor_frame_snapshot_clear.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_snapshot_clear.hpp)：50, 51, 52, 53, 74, 75, 76, 77, 78
  - `register` [src/battle/legacy_battle_actor_frame_snapshot_clear.cpp](../../../src/battle/legacy_battle_actor_frame_snapshot_clear.cpp)：88, 89, 90, 91, 92, 112, 113, 116, 118, 119, 121, 131, 153, 154, 155, 158, 166, 167, 168, 178

- [ ] **CHAIN-653cf2b14319** `battle/legacy_battle_actor_frame_tsw_lookup`
  - 当前残留信号：interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_frame_tsw_lookup.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_tsw_lookup.hpp)；[src/battle/legacy_battle_actor_frame_tsw_lookup.cpp](../../../src/battle/legacy_battle_actor_frame_tsw_lookup.cpp)。
  - 类型：[PORT-777564b100e8](invoke-removal-ports.md#port-777564b100e8)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_actor_frame_tsw_lookup.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_tsw_lookup.hpp)：29, 30, 31, 37, 38, 39
  - `register` [src/battle/legacy_battle_actor_frame_tsw_lookup.cpp](../../../src/battle/legacy_battle_actor_frame_tsw_lookup.cpp)：37, 38, 39, 47, 48, 49, 58, 59, 60, 67, 98, 99, 100, 103

- [ ] **CHAIN-4c5bc28be50c** `battle/legacy_battle_actor_gate_decay`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_gate_decay.hpp](../../../include/openswd3/battle/legacy_battle_actor_gate_decay.hpp)；[src/battle/legacy_battle_actor_gate_decay.cpp](../../../src/battle/legacy_battle_actor_gate_decay.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_actor_gate_decay.hpp](../../../include/openswd3/battle/legacy_battle_actor_gate_decay.hpp)：88, 89, 90, 118, 119, 120, 121, 159, 160
  - `register` [src/battle/legacy_battle_actor_gate_decay.cpp](../../../src/battle/legacy_battle_actor_gate_decay.cpp)：143, 144, 145, 146, 163, 164, 165, 170, 171, 172, 174, 209, 216, 217, 219, 221, 250, 262, 275, 276, 289, 290

- [ ] **CHAIN-bbb2f304bbe9** `battle/legacy_battle_actor_list_index_commit`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_list_index_commit.hpp](../../../include/openswd3/battle/legacy_battle_actor_list_index_commit.hpp)；[src/battle/legacy_battle_actor_list_index_commit.cpp](../../../src/battle/legacy_battle_actor_list_index_commit.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_actor_list_index_commit.hpp](../../../include/openswd3/battle/legacy_battle_actor_list_index_commit.hpp)：9, 10, 22, 23, 24
  - `register` [src/battle/legacy_battle_actor_list_index_commit.cpp](../../../src/battle/legacy_battle_actor_list_index_commit.cpp)：11, 12, 13, 20, 21

- [ ] **CHAIN-c99bebd888c2** `battle/legacy_battle_actor_metrics`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_metrics.hpp](../../../include/openswd3/battle/legacy_battle_actor_metrics.hpp)；[src/battle/legacy_battle_actor_metrics.cpp](../../../src/battle/legacy_battle_actor_metrics.cpp)。
  - 类型：[PORT-a356b5049ace](invoke-removal-ports.md#port-a356b5049ace), [PORT-4b8940cc4813](invoke-removal-ports.md#port-4b8940cc4813)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_actor_metrics.hpp](../../../include/openswd3/battle/legacy_battle_actor_metrics.hpp)：156
  - `register` [include/openswd3/battle/legacy_battle_actor_metrics.hpp](../../../include/openswd3/battle/legacy_battle_actor_metrics.hpp)：34, 35, 36
  - `counter` [src/battle/legacy_battle_actor_metrics.cpp](../../../src/battle/legacy_battle_actor_metrics.cpp)：123, 186
  - `register` [src/battle/legacy_battle_actor_metrics.cpp](../../../src/battle/legacy_battle_actor_metrics.cpp)：26, 27, 28, 96, 97, 100, 101, 102, 108, 109, 111, 117, 118, 119, 128, 129, 130, 132, 133, 134, 138, 139, 143, 144, 145, 147, 148, 149, 153, 164, 165, 166, 168, 169, 174, 180, 181, 182, 191, 192, 193, 195, 196, 197, 201, 202, 206, 207, 208, 210, 211, 212, 216, 219, 220, 221, 230, 233, 235, 236, 237, 258, 259, 498, 499, 500

- [ ] **CHAIN-4cc6579f220c** `battle/legacy_battle_actor_presentation_activation`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_presentation_activation.hpp](../../../include/openswd3/battle/legacy_battle_actor_presentation_activation.hpp)；[src/battle/legacy_battle_actor_presentation_activation.cpp](../../../src/battle/legacy_battle_actor_presentation_activation.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_actor_presentation_activation.hpp](../../../include/openswd3/battle/legacy_battle_actor_presentation_activation.hpp)：97, 98, 99, 110, 111, 112, 113, 182, 183
  - `register` [src/battle/legacy_battle_actor_presentation_activation.cpp](../../../src/battle/legacy_battle_actor_presentation_activation.cpp)：150, 151, 152, 153, 154, 162, 173, 179, 201, 264, 273, 300, 304, 316, 317, 332, 333

- [ ] **CHAIN-49d2c1101b5b** `battle/legacy_battle_actor_priority`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_priority.hpp](../../../include/openswd3/battle/legacy_battle_actor_priority.hpp)；[src/battle/legacy_battle_actor_priority.cpp](../../../src/battle/legacy_battle_actor_priority.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_actor_priority.hpp](../../../include/openswd3/battle/legacy_battle_actor_priority.hpp)：28, 33, 35
  - `counter` [src/battle/legacy_battle_actor_priority.cpp](../../../src/battle/legacy_battle_actor_priority.cpp)：201, 202, 239, 240, 447
  - `register` [src/battle/legacy_battle_actor_priority.cpp](../../../src/battle/legacy_battle_actor_priority.cpp)：129, 130, 131, 133, 134, 135, 136, 137, 138, 151, 154, 159, 160, 162, 168, 174, 177, 189, 190, 210, 211, 212, 216, 217, 218, 227, 228, 248, 249, 250, 254, 255, 256

- [ ] **CHAIN-7af6c6794458** `battle/legacy_battle_actor_profile_preparation`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_profile_preparation.hpp](../../../include/openswd3/battle/legacy_battle_actor_profile_preparation.hpp)；[src/battle/legacy_battle_actor_profile_preparation.cpp](../../../src/battle/legacy_battle_actor_profile_preparation.cpp)。
  - 类型：[PORT-f13d46e6d951](invoke-removal-ports.md#port-f13d46e6d951)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_actor_profile_preparation.hpp](../../../include/openswd3/battle/legacy_battle_actor_profile_preparation.hpp)：46, 47, 48, 49
  - `register` [include/openswd3/battle/legacy_battle_actor_profile_preparation.hpp](../../../include/openswd3/battle/legacy_battle_actor_profile_preparation.hpp)：30, 31, 54, 55, 56
  - `counter` [src/battle/legacy_battle_actor_profile_preparation.cpp](../../../src/battle/legacy_battle_actor_profile_preparation.cpp)：55, 75, 103, 115
  - `register` [src/battle/legacy_battle_actor_profile_preparation.cpp](../../../src/battle/legacy_battle_actor_profile_preparation.cpp)：42, 43, 44

- [ ] **CHAIN-d1bd77e4111a** `battle/legacy_battle_actor_progress`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_progress.hpp](../../../include/openswd3/battle/legacy_battle_actor_progress.hpp)；[src/battle/legacy_battle_actor_progress.cpp](../../../src/battle/legacy_battle_actor_progress.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_actor_progress.hpp](../../../include/openswd3/battle/legacy_battle_actor_progress.hpp)：85, 86, 87, 102, 103, 104, 152
  - `register` [src/battle/legacy_battle_actor_progress.cpp](../../../src/battle/legacy_battle_actor_progress.cpp)：146, 170, 182, 230, 233, 234, 246, 247, 257, 259, 271, 280, 284, 289, 297

- [ ] **CHAIN-463adb563e64** `battle/legacy_battle_actor_ready`
  - 当前残留信号：interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_ready.hpp](../../../include/openswd3/battle/legacy_battle_actor_ready.hpp)；[src/battle/legacy_battle_actor_ready.cpp](../../../src/battle/legacy_battle_actor_ready.cpp)。
  - 类型：[PORT-f36643bb9d23](invoke-removal-ports.md#port-f36643bb9d23)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_actor_ready.hpp](../../../include/openswd3/battle/legacy_battle_actor_ready.hpp)：14, 15, 16
  - `register` [src/battle/legacy_battle_actor_ready.cpp](../../../src/battle/legacy_battle_actor_ready.cpp)：31, 32, 33

- [ ] **CHAIN-21d2fd219caa** `battle/legacy_battle_actor_record_selection`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_record_selection.hpp](../../../include/openswd3/battle/legacy_battle_actor_record_selection.hpp)；[src/battle/legacy_battle_actor_record_selection.cpp](../../../src/battle/legacy_battle_actor_record_selection.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_actor_record_selection.hpp](../../../include/openswd3/battle/legacy_battle_actor_record_selection.hpp)：34, 35, 36, 48, 49, 50, 51
  - `register` [src/battle/legacy_battle_actor_record_selection.cpp](../../../src/battle/legacy_battle_actor_record_selection.cpp)：46, 47, 48, 49, 61, 78, 89, 95, 103, 107, 124, 128, 140

- [ ] **CHAIN-66a7ac0efec7** `battle/legacy_battle_actor_render_offsets`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_render_offsets.hpp](../../../include/openswd3/battle/legacy_battle_actor_render_offsets.hpp)；[src/battle/legacy_battle_actor_render_offsets.cpp](../../../src/battle/legacy_battle_actor_render_offsets.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_actor_render_offsets.hpp](../../../include/openswd3/battle/legacy_battle_actor_render_offsets.hpp)：83, 84, 85, 101, 102, 103, 104
  - `register` [src/battle/legacy_battle_actor_render_offsets.cpp](../../../src/battle/legacy_battle_actor_render_offsets.cpp)：197, 198, 199, 200, 209, 218, 234, 243, 282, 299, 316, 317, 319, 331, 345, 348, 356, 360, 361, 363

- [ ] **CHAIN-41fc056f3e2d** `battle/legacy_battle_actor_retreat_ready`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_retreat_ready.hpp](../../../include/openswd3/battle/legacy_battle_actor_retreat_ready.hpp)；[src/battle/legacy_battle_actor_retreat_ready.cpp](../../../src/battle/legacy_battle_actor_retreat_ready.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_actor_retreat_ready.hpp](../../../include/openswd3/battle/legacy_battle_actor_retreat_ready.hpp)：10, 11, 23, 24, 25
  - `register` [src/battle/legacy_battle_actor_retreat_ready.cpp](../../../src/battle/legacy_battle_actor_retreat_ready.cpp)：10, 11, 12, 20, 22

- [ ] **CHAIN-53e96bd91692** `battle/legacy_battle_actor_runtime_reset`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_runtime_reset.hpp](../../../include/openswd3/battle/legacy_battle_actor_runtime_reset.hpp)；[src/battle/legacy_battle_actor_runtime_reset.cpp](../../../src/battle/legacy_battle_actor_runtime_reset.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_actor_runtime_reset.hpp](../../../include/openswd3/battle/legacy_battle_actor_runtime_reset.hpp)：175
  - `register` [include/openswd3/battle/legacy_battle_actor_runtime_reset.hpp](../../../include/openswd3/battle/legacy_battle_actor_runtime_reset.hpp)：131, 132, 133, 134, 135, 136, 137, 151, 152, 153, 154, 155, 156, 157, 158, 293, 294
  - `counter` [src/battle/legacy_battle_actor_runtime_reset.cpp](../../../src/battle/legacy_battle_actor_runtime_reset.cpp)：1686
  - `register` [src/battle/legacy_battle_actor_runtime_reset.cpp](../../../src/battle/legacy_battle_actor_runtime_reset.cpp)：1242, 1243, 1244, 1245, 1246, 1247, 1248, 1249, 1257, 1258, 1259, 1260, 1261, 1262, 1263, 1264, 1270, 1271, 1272, 1273, 1274, 1275, 1276, 1277, 1358, 1367, 1375, 1381, 1457, 1460, 1461, 1462, 1464, 1467, 1468, 1475, 1476, 1477, 1478, 1479, 1480, 1481, 1482, 1483, 1486, 1487, 1490, 1491, 1492, 1495, 1497, 1498, 1499, 1500, 1501, 1502, 1504, 1505, 1507, 1508, 1509, 1510, 1511, 1512, 1513, 1514, 1515, 1516, 1517, 1518, 1521, 1530, 1537, 1538, 1539, 1543, 1544, 1546, 1555, 1556, 1560, 1563, 1566, 1568, 1571, 1572, 1573, 1574, 1575, 1582, 1583, 1584, 1587, 1594, 1595, 1596, 1599, 1600, 1606, 1607, 1608, 1609, 1610, 1614, 1615, 1619, 1620, 1624, 1625, 1629, 1630, 1634, 1635, 1636, 1637, 1638, 1639, 1640, 1641, 1644, 1645, 1646, 1650, 1651, 1655, 1656, 1657, 1660, 1661, 1662, 1663, 1666, 1669, 1687, 1688, 1689, 1690, 1691, 1692, 1693, 1695, 1700, 1701, 1707, 1712, 1724, 1725, 1739, 1740

- [ ] **CHAIN-ae2b124ad29b** `battle/legacy_battle_actor_target_preparation`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_target_preparation.hpp](../../../include/openswd3/battle/legacy_battle_actor_target_preparation.hpp)；[src/battle/legacy_battle_actor_target_preparation.cpp](../../../src/battle/legacy_battle_actor_target_preparation.cpp)。
  - 类型：[PORT-e6008042ae5e](invoke-removal-ports.md#port-e6008042ae5e)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_actor_target_preparation.hpp](../../../include/openswd3/battle/legacy_battle_actor_target_preparation.hpp)：25
  - `counter` [include/openswd3/battle/legacy_battle_actor_target_preparation.hpp](../../../include/openswd3/battle/legacy_battle_actor_target_preparation.hpp)：82
  - `invoke` [include/openswd3/battle/legacy_battle_actor_target_preparation.hpp](../../../include/openswd3/battle/legacy_battle_actor_target_preparation.hpp)：42
  - `register` [include/openswd3/battle/legacy_battle_actor_target_preparation.hpp](../../../include/openswd3/battle/legacy_battle_actor_target_preparation.hpp)：26, 27, 28, 32, 33, 34, 60, 61, 62, 77, 78, 79
  - `arguments` [src/battle/legacy_battle_actor_target_preparation.cpp](../../../src/battle/legacy_battle_actor_target_preparation.cpp)：46, 50
  - `counter` [src/battle/legacy_battle_actor_target_preparation.cpp](../../../src/battle/legacy_battle_actor_target_preparation.cpp)：118
  - `invoke` [src/battle/legacy_battle_actor_target_preparation.cpp](../../../src/battle/legacy_battle_actor_target_preparation.cpp)：47, 68
  - `register` [src/battle/legacy_battle_actor_target_preparation.cpp](../../../src/battle/legacy_battle_actor_target_preparation.cpp)：33, 34, 35, 36, 39, 40, 41, 51, 52, 53, 55, 56, 57, 60, 61, 62, 70, 77, 80, 81, 89, 90, 93, 98, 103, 111, 112, 113, 117, 119, 120, 121, 124, 129, 130, 131, 132, 133, 134, 135, 137, 138, 139, 142, 145

- [ ] **CHAIN-db4c66ba032d** `battle/legacy_battle_actor_target_selection`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_target_selection.hpp](../../../include/openswd3/battle/legacy_battle_actor_target_selection.hpp)；[src/battle/legacy_battle_actor_target_selection.cpp](../../../src/battle/legacy_battle_actor_target_selection.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_actor_target_selection.hpp](../../../include/openswd3/battle/legacy_battle_actor_target_selection.hpp)：80, 81, 82, 167, 168, 169, 170, 278, 279
  - `register` [src/battle/legacy_battle_actor_target_selection.cpp](../../../src/battle/legacy_battle_actor_target_selection.cpp)：117, 118, 119, 120, 135, 140, 223, 228, 242, 243, 257, 258

- [ ] **CHAIN-590118663f07** `battle/legacy_battle_alternate_grid_frame`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_alternate_grid_frame.hpp](../../../include/openswd3/battle/legacy_battle_alternate_grid_frame.hpp)；[src/battle/legacy_battle_alternate_grid_frame.cpp](../../../src/battle/legacy_battle_alternate_grid_frame.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_alternate_grid_frame.hpp](../../../include/openswd3/battle/legacy_battle_alternate_grid_frame.hpp)：83, 84, 85, 86, 92, 93
  - `register` [include/openswd3/battle/legacy_battle_alternate_grid_frame.hpp](../../../include/openswd3/battle/legacy_battle_alternate_grid_frame.hpp)：42, 43, 44, 80, 81, 82
  - `arguments` [src/battle/legacy_battle_alternate_grid_frame.cpp](../../../src/battle/legacy_battle_alternate_grid_frame.cpp)：419, 440
  - `counter` [src/battle/legacy_battle_alternate_grid_frame.cpp](../../../src/battle/legacy_battle_alternate_grid_frame.cpp)：200, 218, 273, 389, 424, 448
  - `invoke` [src/battle/legacy_battle_alternate_grid_frame.cpp](../../../src/battle/legacy_battle_alternate_grid_frame.cpp)：103, 104, 105, 171, 176, 289, 314, 338, 358, 369, 408, 409, 416, 417, 427, 438
  - `register` [src/battle/legacy_battle_alternate_grid_frame.cpp](../../../src/battle/legacy_battle_alternate_grid_frame.cpp)：82, 83, 219, 220, 221, 275, 276, 277, 334, 335, 390, 391, 392, 410, 411, 412, 420, 421, 422, 441, 442, 443, 452, 453, 454

- [ ] **CHAIN-011f3ed382c2** `battle/legacy_battle_animation_collision`
  - 当前残留信号：counter, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_animation_collision.hpp](../../../include/openswd3/battle/legacy_battle_animation_collision.hpp)；[src/battle/legacy_battle_animation_collision.cpp](../../../src/battle/legacy_battle_animation_collision.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_animation_collision.hpp](../../../include/openswd3/battle/legacy_battle_animation_collision.hpp)：41
  - `register` [include/openswd3/battle/legacy_battle_animation_collision.hpp](../../../include/openswd3/battle/legacy_battle_animation_collision.hpp)：38, 39, 40
  - `counter` [src/battle/legacy_battle_animation_collision.cpp](../../../src/battle/legacy_battle_animation_collision.cpp)：59
  - `register` [src/battle/legacy_battle_animation_collision.cpp](../../../src/battle/legacy_battle_animation_collision.cpp)：85, 86, 91, 95

- [ ] **CHAIN-3d0483b00497** `battle/legacy_battle_available_actor_cycle`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_available_actor_cycle.hpp](../../../include/openswd3/battle/legacy_battle_available_actor_cycle.hpp)；[src/battle/legacy_battle_available_actor_cycle.cpp](../../../src/battle/legacy_battle_available_actor_cycle.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_available_actor_cycle.hpp](../../../include/openswd3/battle/legacy_battle_available_actor_cycle.hpp)：47
  - `register` [include/openswd3/battle/legacy_battle_available_actor_cycle.hpp](../../../include/openswd3/battle/legacy_battle_available_actor_cycle.hpp)：28, 29, 30, 43, 44, 45
  - `counter` [src/battle/legacy_battle_available_actor_cycle.cpp](../../../src/battle/legacy_battle_available_actor_cycle.cpp)：29, 30
  - `register` [src/battle/legacy_battle_available_actor_cycle.cpp](../../../src/battle/legacy_battle_available_actor_cycle.cpp)：16, 17, 18, 23, 24, 40, 41, 42, 46, 47, 48, 61, 62, 63, 66, 67, 68, 69, 79, 80, 81, 87, 88, 89, 92, 93, 94, 95

- [ ] **CHAIN-bd85e7067dd7** `battle/legacy_battle_available_actor_reverse_cycle`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_available_actor_reverse_cycle.hpp](../../../include/openswd3/battle/legacy_battle_available_actor_reverse_cycle.hpp)；[src/battle/legacy_battle_available_actor_reverse_cycle.cpp](../../../src/battle/legacy_battle_available_actor_reverse_cycle.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [src/battle/legacy_battle_available_actor_reverse_cycle.cpp](../../../src/battle/legacy_battle_available_actor_reverse_cycle.cpp)：31, 32
  - `register` [src/battle/legacy_battle_available_actor_reverse_cycle.cpp](../../../src/battle/legacy_battle_available_actor_reverse_cycle.cpp)：18, 19, 20, 25, 26, 42, 43, 44, 48, 49, 50, 68, 69, 70, 73, 74, 75, 76, 82, 83, 84

- [ ] **CHAIN-c5c49164db7f** `battle/legacy_battle_background_initialization`
  - 当前残留信号：counter, interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_background_initialization.hpp](../../../include/openswd3/battle/legacy_battle_background_initialization.hpp)；[src/battle/legacy_battle_background_initialization.cpp](../../../src/battle/legacy_battle_background_initialization.cpp)。
  - 类型：[PORT-57b6e1832ed1](invoke-removal-ports.md#port-57b6e1832ed1), [PORT-a5f5ee600cd5](invoke-removal-ports.md#port-a5f5ee600cd5)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_background_initialization.hpp](../../../include/openswd3/battle/legacy_battle_background_initialization.hpp)：102
  - `counter` [src/battle/legacy_battle_background_initialization.cpp](../../../src/battle/legacy_battle_background_initialization.cpp)：162

- [ ] **CHAIN-94ff52774a09** `battle/legacy_battle_border_panel`
  - 当前残留信号：counter。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_border_panel.hpp](../../../include/openswd3/battle/legacy_battle_border_panel.hpp)；[src/battle/legacy_battle_border_panel.cpp](../../../src/battle/legacy_battle_border_panel.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_border_panel.hpp](../../../include/openswd3/battle/legacy_battle_border_panel.hpp)：37, 38, 39
  - `counter` [src/battle/legacy_battle_border_panel.cpp](../../../src/battle/legacy_battle_border_panel.cpp)：83, 120, 152

- [ ] **CHAIN-a95bc6a54a42** `battle/legacy_battle_color_accumulation`
  - 当前残留信号：interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_color_accumulation.hpp](../../../include/openswd3/battle/legacy_battle_color_accumulation.hpp)；[src/battle/legacy_battle_color_accumulation.cpp](../../../src/battle/legacy_battle_color_accumulation.cpp)。
  - 类型：[PORT-16725e22a897](invoke-removal-ports.md#port-16725e22a897)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_color_accumulation.hpp](../../../include/openswd3/battle/legacy_battle_color_accumulation.hpp)：54, 55, 56
  - `register` [src/battle/legacy_battle_color_accumulation.cpp](../../../src/battle/legacy_battle_color_accumulation.cpp)：105, 106, 107

- [ ] **CHAIN-f73da72fa6ad** `battle/legacy_battle_context_prompt`
  - 当前残留信号：counter, interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_context_prompt.hpp](../../../include/openswd3/battle/legacy_battle_context_prompt.hpp)；[src/battle/legacy_battle_context_prompt.cpp](../../../src/battle/legacy_battle_context_prompt.cpp)。
  - 类型：[PORT-9c69fd704d9f](invoke-removal-ports.md#port-9c69fd704d9f)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_context_prompt.hpp](../../../include/openswd3/battle/legacy_battle_context_prompt.hpp)：63
  - `counter` [src/battle/legacy_battle_context_prompt.cpp](../../../src/battle/legacy_battle_context_prompt.cpp)：87

- [ ] **CHAIN-e557def7f934** `battle/legacy_battle_control_panel_frame`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_control_panel_frame.hpp](../../../include/openswd3/battle/legacy_battle_control_panel_frame.hpp)；[src/battle/legacy_battle_control_panel_frame.cpp](../../../src/battle/legacy_battle_control_panel_frame.cpp)。
  - 类型：[PORT-3af2858d579f](invoke-removal-ports.md#port-3af2858d579f)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_control_panel_frame.hpp](../../../include/openswd3/battle/legacy_battle_control_panel_frame.hpp)：46
  - `counter` [include/openswd3/battle/legacy_battle_control_panel_frame.hpp](../../../include/openswd3/battle/legacy_battle_control_panel_frame.hpp)：145, 147, 148, 149, 150, 152
  - `invoke` [include/openswd3/battle/legacy_battle_control_panel_frame.hpp](../../../include/openswd3/battle/legacy_battle_control_panel_frame.hpp)：71
  - `register` [include/openswd3/battle/legacy_battle_control_panel_frame.hpp](../../../include/openswd3/battle/legacy_battle_control_panel_frame.hpp)：47, 48, 49, 56, 57, 58, 102, 103, 104, 111, 112, 113, 142, 143, 144
  - `arguments` [src/battle/legacy_battle_control_panel_frame.cpp](../../../src/battle/legacy_battle_control_panel_frame.cpp)：203, 215, 235
  - `counter` [src/battle/legacy_battle_control_panel_frame.cpp](../../../src/battle/legacy_battle_control_panel_frame.cpp)：144, 183, 208, 220, 250, 358, 439
  - `invoke` [src/battle/legacy_battle_control_panel_frame.cpp](../../../src/battle/legacy_battle_control_panel_frame.cpp)：80, 81, 200, 212, 232
  - `register` [src/battle/legacy_battle_control_panel_frame.cpp](../../../src/battle/legacy_battle_control_panel_frame.cpp)：48, 49, 73, 74, 75, 82, 83, 84, 194, 195, 196, 204, 205, 206, 216, 217, 218, 244, 245, 246, 353, 354, 355, 359, 360, 361, 434, 435, 436, 440, 441, 442

- [ ] **CHAIN-518e36931a12** `battle/legacy_battle_database_shutdown`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_database_shutdown.hpp](../../../include/openswd3/battle/legacy_battle_database_shutdown.hpp)；[src/battle/legacy_battle_database_shutdown.cpp](../../../src/battle/legacy_battle_database_shutdown.cpp)。
  - 类型：[PORT-f52d60a7b4cd](invoke-removal-ports.md#port-f52d60a7b4cd)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_database_shutdown.hpp](../../../include/openswd3/battle/legacy_battle_database_shutdown.hpp)：44
  - `register` [include/openswd3/battle/legacy_battle_database_shutdown.hpp](../../../include/openswd3/battle/legacy_battle_database_shutdown.hpp)：10, 11, 12, 16, 17, 18, 34, 35, 36, 45, 46, 47
  - `counter` [src/battle/legacy_battle_database_shutdown.cpp](../../../src/battle/legacy_battle_database_shutdown.cpp)：31, 47
  - `register` [src/battle/legacy_battle_database_shutdown.cpp](../../../src/battle/legacy_battle_database_shutdown.cpp)：16, 17, 18, 19, 21, 22, 23, 24, 26, 27, 28, 34, 35, 37, 38, 39, 40, 42, 43, 44, 55, 56, 57

- [ ] **CHAIN-b6d378f1418c** `battle/legacy_battle_debug_actor_value`
  - 当前残留信号：arguments, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[src/battle/legacy_battle_debug_actor_value.cpp](../../../src/battle/legacy_battle_debug_actor_value.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [src/battle/legacy_battle_debug_actor_value.cpp](../../../src/battle/legacy_battle_debug_actor_value.cpp)：147, 240, 242
  - `register` [src/battle/legacy_battle_debug_actor_value.cpp](../../../src/battle/legacy_battle_debug_actor_value.cpp)：126, 127, 128, 132

- [ ] **CHAIN-c4b7cc6bf979** `battle/legacy_battle_debug_hotkeys`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_debug_hotkeys.hpp](../../../include/openswd3/battle/legacy_battle_debug_hotkeys.hpp)；[src/battle/legacy_battle_debug_hotkeys.cpp](../../../src/battle/legacy_battle_debug_hotkeys.cpp)。
  - 类型：[PORT-55bac89def47](invoke-removal-ports.md#port-55bac89def47), [PORT-3d40ca273a58](invoke-removal-ports.md#port-3d40ca273a58), [PORT-7f69a3107b73](invoke-removal-ports.md#port-7f69a3107b73)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_debug_hotkeys.hpp](../../../include/openswd3/battle/legacy_battle_debug_hotkeys.hpp)：39
  - `counter` [include/openswd3/battle/legacy_battle_debug_hotkeys.hpp](../../../include/openswd3/battle/legacy_battle_debug_hotkeys.hpp)：156, 160, 164, 168
  - `invoke` [include/openswd3/battle/legacy_battle_debug_hotkeys.hpp](../../../include/openswd3/battle/legacy_battle_debug_hotkeys.hpp)：66, 89
  - `register` [include/openswd3/battle/legacy_battle_debug_hotkeys.hpp](../../../include/openswd3/battle/legacy_battle_debug_hotkeys.hpp)：40, 41, 42, 46, 47, 48
  - `arguments` [src/battle/legacy_battle_debug_hotkeys.cpp](../../../src/battle/legacy_battle_debug_hotkeys.cpp)：112, 190, 196, 351, 389, 399
  - `counter` [src/battle/legacy_battle_debug_hotkeys.cpp](../../../src/battle/legacy_battle_debug_hotkeys.cpp)：157, 174, 284, 723
  - `invoke` [src/battle/legacy_battle_debug_hotkeys.cpp](../../../src/battle/legacy_battle_debug_hotkeys.cpp)：105, 108, 187, 198, 303, 505, 540, 551, 570, 603, 638, 661, 800, 808, 843, 861
  - `register` [src/battle/legacy_battle_debug_hotkeys.cpp](../../../src/battle/legacy_battle_debug_hotkeys.cpp)：113, 114, 115, 127, 128, 129, 255, 275, 276, 291, 311, 312, 313, 470, 471, 478, 479, 488, 708, 729, 733, 759, 850

- [ ] **CHAIN-7da922b66066** `battle/legacy_battle_debug_overlay`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_debug_overlay.hpp](../../../include/openswd3/battle/legacy_battle_debug_overlay.hpp)；[src/battle/legacy_battle_debug_overlay.cpp](../../../src/battle/legacy_battle_debug_overlay.cpp)。
  - 类型：[PORT-1054cd126300](invoke-removal-ports.md#port-1054cd126300)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_debug_overlay.hpp](../../../include/openswd3/battle/legacy_battle_debug_overlay.hpp)：34, 35
  - `counter` [include/openswd3/battle/legacy_battle_debug_overlay.hpp](../../../include/openswd3/battle/legacy_battle_debug_overlay.hpp)：150
  - `invoke` [include/openswd3/battle/legacy_battle_debug_overlay.hpp](../../../include/openswd3/battle/legacy_battle_debug_overlay.hpp)：63
  - `register` [include/openswd3/battle/legacy_battle_debug_overlay.hpp](../../../include/openswd3/battle/legacy_battle_debug_overlay.hpp)：39, 40, 41, 136, 137
  - `arguments` [src/battle/legacy_battle_debug_overlay.cpp](../../../src/battle/legacy_battle_debug_overlay.cpp)：106, 109, 126, 127, 132, 133
  - `counter` [src/battle/legacy_battle_debug_overlay.cpp](../../../src/battle/legacy_battle_debug_overlay.cpp)：453
  - `invoke` [src/battle/legacy_battle_debug_overlay.cpp](../../../src/battle/legacy_battle_debug_overlay.cpp)：129
  - `register` [src/battle/legacy_battle_debug_overlay.cpp](../../../src/battle/legacy_battle_debug_overlay.cpp)：201, 211, 245, 246, 267, 268, 282, 308, 309, 328, 329, 341, 438, 465, 466, 467, 588, 589, 590

- [ ] **CHAIN-a099d2f7bc27** `battle/legacy_battle_debug_status_panel`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_debug_status_panel.hpp](../../../include/openswd3/battle/legacy_battle_debug_status_panel.hpp)；[src/battle/legacy_battle_debug_status_panel.cpp](../../../src/battle/legacy_battle_debug_status_panel.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_debug_status_panel.hpp](../../../include/openswd3/battle/legacy_battle_debug_status_panel.hpp)：98, 102, 107, 109, 110, 111
  - `register` [include/openswd3/battle/legacy_battle_debug_status_panel.hpp](../../../include/openswd3/battle/legacy_battle_debug_status_panel.hpp)：38, 39, 40
  - `arguments` [src/battle/legacy_battle_debug_status_panel.cpp](../../../src/battle/legacy_battle_debug_status_panel.cpp)：117, 124
  - `counter` [src/battle/legacy_battle_debug_status_panel.cpp](../../../src/battle/legacy_battle_debug_status_panel.cpp)：129, 146, 166, 195, 246, 259, 368
  - `invoke` [src/battle/legacy_battle_debug_status_panel.cpp](../../../src/battle/legacy_battle_debug_status_panel.cpp)：122, 210, 333
  - `register` [src/battle/legacy_battle_debug_status_panel.cpp](../../../src/battle/legacy_battle_debug_status_panel.cpp)：110, 111, 112, 114, 118, 119, 120, 125, 126, 127, 130, 131, 132, 167, 168, 169, 176, 196, 197, 198, 221, 226, 260, 261, 262, 269, 351, 352, 369, 370, 371, 378, 379

- [ ] **CHAIN-ef9d752bb9d1** `battle/legacy_battle_defeat_panel`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_defeat_panel.hpp](../../../include/openswd3/battle/legacy_battle_defeat_panel.hpp)；[src/battle/legacy_battle_defeat_panel.cpp](../../../src/battle/legacy_battle_defeat_panel.cpp)。
  - 类型：[PORT-75dcd66e85f6](invoke-removal-ports.md#port-75dcd66e85f6)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_defeat_panel.hpp](../../../include/openswd3/battle/legacy_battle_defeat_panel.hpp)：42
  - `counter` [include/openswd3/battle/legacy_battle_defeat_panel.hpp](../../../include/openswd3/battle/legacy_battle_defeat_panel.hpp)：93, 94, 95, 96, 97, 98, 99
  - `invoke` [include/openswd3/battle/legacy_battle_defeat_panel.hpp](../../../include/openswd3/battle/legacy_battle_defeat_panel.hpp)：63
  - `register` [include/openswd3/battle/legacy_battle_defeat_panel.hpp](../../../include/openswd3/battle/legacy_battle_defeat_panel.hpp)：43, 44, 45, 51, 52, 53, 64, 69, 70, 71, 90, 91, 92
  - `arguments` [src/battle/legacy_battle_defeat_panel.cpp](../../../src/battle/legacy_battle_defeat_panel.cpp)：60, 66
  - `counter` [src/battle/legacy_battle_defeat_panel.cpp](../../../src/battle/legacy_battle_defeat_panel.cpp)：118, 149, 175, 200, 218, 226, 256
  - `invoke` [src/battle/legacy_battle_defeat_panel.cpp](../../../src/battle/legacy_battle_defeat_panel.cpp)：57, 77, 162, 213, 243
  - `register` [src/battle/legacy_battle_defeat_panel.cpp](../../../src/battle/legacy_battle_defeat_panel.cpp)：44, 45, 67, 68, 69, 79, 80, 81, 92, 96, 97, 98, 102, 119, 120, 121, 132, 150, 151, 152, 183, 201, 202, 203, 227, 228, 229, 261, 262, 263

- [ ] **CHAIN-894cb5a26940** `battle/legacy_battle_dialog_text`
  - 当前残留信号：counter, interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_dialog_text.hpp](../../../include/openswd3/battle/legacy_battle_dialog_text.hpp)；[src/battle/legacy_battle_dialog_text.cpp](../../../src/battle/legacy_battle_dialog_text.cpp)。
  - 类型：[PORT-04aa251baab4](invoke-removal-ports.md#port-04aa251baab4)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_dialog_text.hpp](../../../include/openswd3/battle/legacy_battle_dialog_text.hpp)：33, 34
  - `counter` [src/battle/legacy_battle_dialog_text.cpp](../../../src/battle/legacy_battle_dialog_text.cpp)：42, 151

- [ ] **CHAIN-7d2f92a65436** `battle/legacy_battle_display_surfaces`
  - 当前残留信号：counter, interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_display_surfaces.hpp](../../../include/openswd3/battle/legacy_battle_display_surfaces.hpp)。
  - 类型：[PORT-80b3c5932ebf](invoke-removal-ports.md#port-80b3c5932ebf)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_display_surfaces.hpp](../../../include/openswd3/battle/legacy_battle_display_surfaces.hpp)：28, 34

- [ ] **CHAIN-72de3d964089** `battle/legacy_battle_effect_coordinator`
  - 当前残留信号：arguments, callee, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_effect_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_effect_coordinator.hpp)；[src/battle/legacy_battle_effect_coordinator.cpp](../../../src/battle/legacy_battle_effect_coordinator.cpp)。
  - 类型：[PORT-6d055401c423](invoke-removal-ports.md#port-6d055401c423)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_effect_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_effect_coordinator.hpp)：118, 121, 122, 123, 124, 125, 127, 128, 129, 133
  - `arguments` [src/battle/legacy_battle_effect_coordinator.cpp](../../../src/battle/legacy_battle_effect_coordinator.cpp)：129, 140
  - `callee` [src/battle/legacy_battle_effect_coordinator.cpp](../../../src/battle/legacy_battle_effect_coordinator.cpp)：128, 135
  - `counter` [src/battle/legacy_battle_effect_coordinator.cpp](../../../src/battle/legacy_battle_effect_coordinator.cpp)：203, 205, 206, 221, 238, 287, 329, 361, 387, 442, 495
  - `invoke` [src/battle/legacy_battle_effect_coordinator.cpp](../../../src/battle/legacy_battle_effect_coordinator.cpp)：127, 142, 223, 330, 418, 478, 479
  - `register` [src/battle/legacy_battle_effect_coordinator.cpp](../../../src/battle/legacy_battle_effect_coordinator.cpp)：130, 131, 132, 136, 137, 138, 181, 182, 189, 190, 211, 215, 226, 264, 312, 330, 419, 439, 449, 453, 454, 465, 466, 475, 626, 1025, 1091, 1094

- [ ] **CHAIN-423fcabc2295** `battle/legacy_battle_effect_frame`
  - 当前残留信号：arguments, callee, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/asset_runtime/legacy_frame_deformation.hpp](../../../include/openswd3/asset_runtime/legacy_frame_deformation.hpp)；[include/openswd3/battle/legacy_battle_effect_frame.hpp](../../../include/openswd3/battle/legacy_battle_effect_frame.hpp)；[src/asset_runtime/legacy_frame_deformation.cpp](../../../src/asset_runtime/legacy_frame_deformation.cpp)；[src/battle/legacy_battle_effect_frame.cpp](../../../src/battle/legacy_battle_effect_frame.cpp)。
  - 类型：[PORT-9244ea9fae5b](invoke-removal-ports.md#port-9244ea9fae5b)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_effect_frame.hpp](../../../include/openswd3/battle/legacy_battle_effect_frame.hpp)：35
  - `callee` [include/openswd3/battle/legacy_battle_effect_frame.hpp](../../../include/openswd3/battle/legacy_battle_effect_frame.hpp)：34
  - `counter` [include/openswd3/battle/legacy_battle_effect_frame.hpp](../../../include/openswd3/battle/legacy_battle_effect_frame.hpp)：126, 208, 212, 214, 216, 223
  - `invoke` [include/openswd3/battle/legacy_battle_effect_frame.hpp](../../../include/openswd3/battle/legacy_battle_effect_frame.hpp)：63
  - `register` [include/openswd3/battle/legacy_battle_effect_frame.hpp](../../../include/openswd3/battle/legacy_battle_effect_frame.hpp)：36, 37, 38, 42, 43, 44, 206, 207
  - `arguments` [src/battle/legacy_battle_effect_frame.cpp](../../../src/battle/legacy_battle_effect_frame.cpp)：144, 148
  - `callee` [src/battle/legacy_battle_effect_frame.cpp](../../../src/battle/legacy_battle_effect_frame.cpp)：143, 147
  - `counter` [src/battle/legacy_battle_effect_frame.cpp](../../../src/battle/legacy_battle_effect_frame.cpp)：231, 320, 354, 612, 648, 768, 937
  - `invoke` [src/battle/legacy_battle_effect_frame.cpp](../../../src/battle/legacy_battle_effect_frame.cpp)：141, 149, 296, 363, 474, 481, 483, 534, 542, 698, 706, 714, 721, 782, 794, 798, 822, 834, 843, 851, 875, 886, 889, 910, 916, 922, 1010, 1014, 1026, 1040, 1056, 1062, 1098, 1106, 1112, 1149, 1154, 1160
  - `register` [src/battle/legacy_battle_effect_frame.cpp](../../../src/battle/legacy_battle_effect_frame.cpp)：165, 225, 226, 240, 241, 302, 303, 329, 362, 444, 481, 483, 535, 605, 606, 607, 625, 640, 641, 704, 727, 736, 738, 775, 823, 840, 849, 889, 910, 916, 922, 938, 957, 966, 967, 968, 969, 970, 973, 991, 1000, 1001, 1002, 1003, 1004, 1007, 1010, 1014, 1045, 1077, 1078, 1086, 1087, 1088, 1089, 1090, 1094, 1095, 1096, 1128, 1129, 1137, 1138, 1139, 1140, 1141, 1145, 1146, 1147, 1238, 1239

- [ ] **CHAIN-9b2b71604410** `battle/legacy_battle_effect_shift`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_effect_shift.hpp](../../../include/openswd3/battle/legacy_battle_effect_shift.hpp)；[src/battle/legacy_battle_effect_shift.cpp](../../../src/battle/legacy_battle_effect_shift.cpp)。
  - 类型：[PORT-f22ae89410fc](invoke-removal-ports.md#port-f22ae89410fc)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_effect_shift.hpp](../../../include/openswd3/battle/legacy_battle_effect_shift.hpp)：82, 84
  - `register` [include/openswd3/battle/legacy_battle_effect_shift.hpp](../../../include/openswd3/battle/legacy_battle_effect_shift.hpp)：94, 95
  - `counter` [src/battle/legacy_battle_effect_shift.cpp](../../../src/battle/legacy_battle_effect_shift.cpp)：145, 185
  - `register` [src/battle/legacy_battle_effect_shift.cpp](../../../src/battle/legacy_battle_effect_shift.cpp)：128, 129, 161, 162, 163, 164, 177, 178, 179, 180, 181, 186, 194, 195, 250, 251, 258, 259, 300, 307, 354, 369

- [ ] **CHAIN-9dc8e9fe9891** `battle/legacy_battle_final_actor_step`
  - 当前残留信号：arguments, callee, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_final_actor_step.hpp](../../../include/openswd3/battle/legacy_battle_final_actor_step.hpp)；[src/battle/legacy_battle_final_actor_step.cpp](../../../src/battle/legacy_battle_final_actor_step.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [src/battle/legacy_battle_final_actor_step.cpp](../../../src/battle/legacy_battle_final_actor_step.cpp)：55, 57
  - `callee` [src/battle/legacy_battle_final_actor_step.cpp](../../../src/battle/legacy_battle_final_actor_step.cpp)：54, 57
  - `counter` [src/battle/legacy_battle_final_actor_step.cpp](../../../src/battle/legacy_battle_final_actor_step.cpp)：312
  - `invoke` [src/battle/legacy_battle_final_actor_step.cpp](../../../src/battle/legacy_battle_final_actor_step.cpp)：52, 57, 60, 72, 245, 398, 452, 606, 654
  - `register` [src/battle/legacy_battle_final_actor_step.cpp](../../../src/battle/legacy_battle_final_actor_step.cpp)：92, 93, 94, 107, 114, 116, 117, 118, 182, 256, 398, 449, 463, 467, 496, 497, 545, 547, 548, 549, 569, 574, 576, 577, 578, 588, 598, 607, 608, 657

- [ ] **CHAIN-7c582e1f9b46** `battle/legacy_battle_frame_draw`
  - 当前残留信号：counter。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_frame_draw.hpp](../../../include/openswd3/battle/legacy_battle_frame_draw.hpp)；[src/battle/legacy_battle_frame_draw.cpp](../../../src/battle/legacy_battle_frame_draw.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_frame_draw.hpp](../../../include/openswd3/battle/legacy_battle_frame_draw.hpp)：29, 30, 60, 61, 143, 144, 155, 156
  - `counter` [src/battle/legacy_battle_frame_draw.cpp](../../../src/battle/legacy_battle_frame_draw.cpp)：45, 76, 107, 148, 383, 384, 434, 435, 457, 458, 545, 580, 607, 648

- [ ] **CHAIN-55a1598efdbb** `battle/legacy_battle_frame_effect`
  - 当前残留信号：counter, interface, invoke。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_frame_effect.hpp](../../../include/openswd3/battle/legacy_battle_frame_effect.hpp)；[src/battle/legacy_battle_frame_effect.cpp](../../../src/battle/legacy_battle_frame_effect.cpp)。
  - 类型：[PORT-0bc8fbaacbda](invoke-removal-ports.md#port-0bc8fbaacbda), [PORT-0fbfe71d8507](invoke-removal-ports.md#port-0fbfe71d8507), [PORT-6707630e445a](invoke-removal-ports.md#port-6707630e445a)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_frame_effect.hpp](../../../include/openswd3/battle/legacy_battle_frame_effect.hpp)：124, 125, 126, 127, 128, 129, 130, 132
  - `counter` [src/battle/legacy_battle_frame_effect.cpp](../../../src/battle/legacy_battle_frame_effect.cpp)：94, 131, 164, 197, 218, 243, 253, 263, 305, 326, 344
  - `invoke` [src/battle/legacy_battle_frame_effect.cpp](../../../src/battle/legacy_battle_frame_effect.cpp)：309, 481, 557

- [ ] **CHAIN-42f7ef802832** `battle/legacy_battle_frame_input_resolution`
  - 当前残留信号：arguments, counter, interface, invoke, operation, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_frame_input_resolution.hpp](../../../include/openswd3/battle/legacy_battle_frame_input_resolution.hpp)；[src/battle/legacy_battle_frame_input_resolution.cpp](../../../src/battle/legacy_battle_frame_input_resolution.cpp)。
  - 类型：[PORT-168328af62c0](invoke-removal-ports.md#port-168328af62c0)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_frame_input_resolution.hpp](../../../include/openswd3/battle/legacy_battle_frame_input_resolution.hpp)：42
  - `counter` [include/openswd3/battle/legacy_battle_frame_input_resolution.hpp](../../../include/openswd3/battle/legacy_battle_frame_input_resolution.hpp)：227
  - `invoke` [include/openswd3/battle/legacy_battle_frame_input_resolution.hpp](../../../include/openswd3/battle/legacy_battle_frame_input_resolution.hpp)：64
  - `register` [include/openswd3/battle/legacy_battle_frame_input_resolution.hpp](../../../include/openswd3/battle/legacy_battle_frame_input_resolution.hpp)：43, 44, 45, 49, 50, 51, 83, 84, 85, 119, 130, 204, 205, 206, 217, 224, 225, 226
  - `arguments` [src/battle/legacy_battle_frame_input_resolution.cpp](../../../src/battle/legacy_battle_frame_input_resolution.cpp)：285, 289
  - `counter` [src/battle/legacy_battle_frame_input_resolution.cpp](../../../src/battle/legacy_battle_frame_input_resolution.cpp)：314
  - `invoke` [src/battle/legacy_battle_frame_input_resolution.cpp](../../../src/battle/legacy_battle_frame_input_resolution.cpp)：286, 300
  - `operation` [src/battle/legacy_battle_frame_input_resolution.cpp](../../../src/battle/legacy_battle_frame_input_resolution.cpp)：283, 287
  - `register` [src/battle/legacy_battle_frame_input_resolution.cpp](../../../src/battle/legacy_battle_frame_input_resolution.cpp)：85, 86, 87, 88, 89, 95, 108, 120, 124, 199, 203, 204, 207, 208, 215, 216, 218, 228, 231, 233, 234, 235, 237, 238, 242, 243, 246, 259, 260, 261, 264, 265, 266, 270, 275, 290, 291, 292, 294, 295, 296, 303, 304, 305, 310, 312, 315, 316, 317, 323, 328, 330, 336, 341, 342, 348, 354, 355, 357, 359, 377, 378, 379, 380, 381, 382, 386, 432, 462, 463, 464, 476, 477, 478, 479, 480, 481, 488, 508, 509, 510, 524, 563, 579, 607, 615, 616, 627, 629, 630, 631, 635, 689, 696, 697, 700, 702, 704, 705, 711, 725, 726, 727, 728, 729, 730, 734, 735, 795, 796, 797, 798, 799, 800, 804, 805, 806, 980, 982, 983, 984, 985, 986, 987, 988, 992, 993, 994, 995, 1007, 1009, 1043, 1065, 1095, 1096, 1100, 1101, 1102, 1111, 1177, 1262, 1331

- [ ] **CHAIN-0df90b693548** `battle/legacy_battle_frame_surface`
  - 当前残留信号：counter, interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_frame_surface.hpp](../../../include/openswd3/battle/legacy_battle_frame_surface.hpp)；[src/battle/legacy_battle_frame_surface.cpp](../../../src/battle/legacy_battle_frame_surface.cpp)。
  - 类型：[PORT-d4ae5c34c93e](invoke-removal-ports.md#port-d4ae5c34c93e)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_frame_surface.hpp](../../../include/openswd3/battle/legacy_battle_frame_surface.hpp)：34, 35
  - `counter` [src/battle/legacy_battle_frame_surface.cpp](../../../src/battle/legacy_battle_frame_surface.cpp)：25, 34

- [ ] **CHAIN-6c01bb7057b5** `battle/legacy_battle_full_frame_darkening`
  - 当前残留信号：counter。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_full_frame_darkening.hpp](../../../include/openswd3/battle/legacy_battle_full_frame_darkening.hpp)；[src/battle/legacy_battle_full_frame_darkening.cpp](../../../src/battle/legacy_battle_full_frame_darkening.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_full_frame_darkening.hpp](../../../include/openswd3/battle/legacy_battle_full_frame_darkening.hpp)：33
  - `counter` [src/battle/legacy_battle_full_frame_darkening.cpp](../../../src/battle/legacy_battle_full_frame_darkening.cpp)：36, 48, 60

- [ ] **CHAIN-93d4a385839a** `battle/legacy_battle_global_reset`
  - 当前残留信号：counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)；[src/battle/legacy_battle_global_reset.cpp](../../../src/battle/legacy_battle_global_reset.cpp)。
  - 类型：[PORT-157307fdc89c](invoke-removal-ports.md#port-157307fdc89c)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：103
  - `invoke` [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：65
  - `register` [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：34, 35, 36, 108, 109
  - `counter` [src/battle/legacy_battle_global_reset.cpp](../../../src/battle/legacy_battle_global_reset.cpp)：577, 578
  - `invoke` [src/battle/legacy_battle_global_reset.cpp](../../../src/battle/legacy_battle_global_reset.cpp)：616, 664, 670, 680
  - `register` [src/battle/legacy_battle_global_reset.cpp](../../../src/battle/legacy_battle_global_reset.cpp)：688, 689, 690, 699, 700

- [ ] **CHAIN-9979b72f62fe** `battle/legacy_battle_group_a_action_execution`
  - 当前残留信号：arguments, callee, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_action_execution.hpp](../../../include/openswd3/battle/legacy_battle_group_a_action_execution.hpp)；[src/battle/legacy_battle_group_a_action_execution.cpp](../../../src/battle/legacy_battle_group_a_action_execution.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_group_a_action_execution.hpp](../../../include/openswd3/battle/legacy_battle_group_a_action_execution.hpp)：41, 42, 43
  - `register` [include/openswd3/battle/legacy_battle_group_a_action_execution.hpp](../../../include/openswd3/battle/legacy_battle_group_a_action_execution.hpp)：19, 20, 37, 38, 39
  - `arguments` [src/battle/legacy_battle_group_a_action_execution.cpp](../../../src/battle/legacy_battle_group_a_action_execution.cpp)：86, 92
  - `callee` [src/battle/legacy_battle_group_a_action_execution.cpp](../../../src/battle/legacy_battle_group_a_action_execution.cpp)：85, 91, 97
  - `counter` [src/battle/legacy_battle_group_a_action_execution.cpp](../../../src/battle/legacy_battle_group_a_action_execution.cpp)：233, 328, 448, 486, 528
  - `invoke` [src/battle/legacy_battle_group_a_action_execution.cpp](../../../src/battle/legacy_battle_group_a_action_execution.cpp)：98, 112, 159, 185, 371, 397, 469, 511
  - `register` [src/battle/legacy_battle_group_a_action_execution.cpp](../../../src/battle/legacy_battle_group_a_action_execution.cpp)：73, 74, 75, 82, 83, 84, 93, 94, 95, 101, 102, 103, 117, 119, 120, 121, 122, 124, 125, 126, 164, 166, 167, 168, 195, 197, 199, 200, 201, 202, 230, 231, 232, 242, 243, 244, 259, 260, 265, 266, 267, 270, 271, 272, 275, 276, 277, 286, 287, 288, 305, 306, 307, 325, 326, 327, 350, 351, 352, 383, 385, 386, 387, 388, 407, 409, 411, 412, 413, 425, 426, 427, 445, 446, 447, 479, 480, 481, 483, 484, 485, 506, 507, 508, 521, 523, 525, 526, 527, 566, 567, 568

- [ ] **CHAIN-aee0fa8ff50e** `battle/legacy_battle_group_a_actor_cleanup`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_actor_cleanup.hpp](../../../include/openswd3/battle/legacy_battle_group_a_actor_cleanup.hpp)；[src/battle/legacy_battle_group_a_actor_cleanup.cpp](../../../src/battle/legacy_battle_group_a_actor_cleanup.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_group_a_actor_cleanup.hpp](../../../include/openswd3/battle/legacy_battle_group_a_actor_cleanup.hpp)：32, 33, 34
  - `register` [src/battle/legacy_battle_group_a_actor_cleanup.cpp](../../../src/battle/legacy_battle_group_a_actor_cleanup.cpp)：36, 37, 58, 68, 123, 127, 146, 147

- [ ] **CHAIN-b718d4a63280** `battle/legacy_battle_group_a_attribute_aggregation`
  - 当前残留信号：counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_attribute_aggregation.hpp](../../../include/openswd3/battle/legacy_battle_group_a_attribute_aggregation.hpp)；[src/battle/legacy_battle_group_a_attribute_aggregation.cpp](../../../src/battle/legacy_battle_group_a_attribute_aggregation.cpp)。
  - 类型：[PORT-54e9aba5bfc3](invoke-removal-ports.md#port-54e9aba5bfc3)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_group_a_attribute_aggregation.hpp](../../../include/openswd3/battle/legacy_battle_group_a_attribute_aggregation.hpp)：100, 101
  - `invoke` [include/openswd3/battle/legacy_battle_group_a_attribute_aggregation.hpp](../../../include/openswd3/battle/legacy_battle_group_a_attribute_aggregation.hpp)：83
  - `register` [include/openswd3/battle/legacy_battle_group_a_attribute_aggregation.hpp](../../../include/openswd3/battle/legacy_battle_group_a_attribute_aggregation.hpp)：67, 68, 69, 73, 74, 75, 115, 116, 117
  - `counter` [src/battle/legacy_battle_group_a_attribute_aggregation.cpp](../../../src/battle/legacy_battle_group_a_attribute_aggregation.cpp)：272, 350
  - `invoke` [src/battle/legacy_battle_group_a_attribute_aggregation.cpp](../../../src/battle/legacy_battle_group_a_attribute_aggregation.cpp)：258
  - `register` [src/battle/legacy_battle_group_a_attribute_aggregation.cpp](../../../src/battle/legacy_battle_group_a_attribute_aggregation.cpp)：215, 231, 287, 288, 318, 346, 347, 351, 352, 353, 361, 366, 367, 381, 397, 402, 407

- [ ] **CHAIN-bbfe2d33a516** `battle/legacy_battle_group_a_attribute_effect`
  - 当前残留信号：arguments, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_attribute_effect.hpp](../../../include/openswd3/battle/legacy_battle_group_a_attribute_effect.hpp)；[src/battle/legacy_battle_group_a_attribute_effect.cpp](../../../src/battle/legacy_battle_group_a_attribute_effect.cpp)。
  - 类型：[PORT-554bddc7c40e](invoke-removal-ports.md#port-554bddc7c40e)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_group_a_attribute_effect.hpp](../../../include/openswd3/battle/legacy_battle_group_a_attribute_effect.hpp)：31
  - `invoke` [include/openswd3/battle/legacy_battle_group_a_attribute_effect.hpp](../../../include/openswd3/battle/legacy_battle_group_a_attribute_effect.hpp)：48
  - `register` [include/openswd3/battle/legacy_battle_group_a_attribute_effect.hpp](../../../include/openswd3/battle/legacy_battle_group_a_attribute_effect.hpp)：32, 33, 34, 38, 39, 40, 54, 55, 79, 80, 81
  - `arguments` [src/battle/legacy_battle_group_a_attribute_effect.cpp](../../../src/battle/legacy_battle_group_a_attribute_effect.cpp)：90, 95
  - `invoke` [src/battle/legacy_battle_group_a_attribute_effect.cpp](../../../src/battle/legacy_battle_group_a_attribute_effect.cpp)：91, 140, 150, 190, 195, 200
  - `register` [src/battle/legacy_battle_group_a_attribute_effect.cpp](../../../src/battle/legacy_battle_group_a_attribute_effect.cpp)：63, 64, 65, 73, 83, 84, 85, 96, 97, 98, 100, 101, 102, 107, 138, 139, 146, 147, 165, 166, 172, 173, 174, 177, 178, 179, 182, 183, 184, 186, 188, 218, 219, 220

- [ ] **CHAIN-d24d57834820** `battle/legacy_battle_group_a_configuration`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_configuration.hpp](../../../include/openswd3/battle/legacy_battle_group_a_configuration.hpp)；[src/battle/legacy_battle_group_a_configuration.cpp](../../../src/battle/legacy_battle_group_a_configuration.cpp)。
  - 类型：[PORT-ee048797c4f3](invoke-removal-ports.md#port-ee048797c4f3)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_group_a_configuration.hpp](../../../include/openswd3/battle/legacy_battle_group_a_configuration.hpp)：99
  - `register` [include/openswd3/battle/legacy_battle_group_a_configuration.hpp](../../../include/openswd3/battle/legacy_battle_group_a_configuration.hpp)：69, 70, 71, 100, 101, 102
  - `counter` [src/battle/legacy_battle_group_a_configuration.cpp](../../../src/battle/legacy_battle_group_a_configuration.cpp)：173
  - `register` [src/battle/legacy_battle_group_a_configuration.cpp](../../../src/battle/legacy_battle_group_a_configuration.cpp)：94, 95, 125, 126, 127, 133, 134, 135, 144, 145, 146, 163, 172, 180, 191, 192, 193

- [ ] **CHAIN-27a84b9319a3** `battle/legacy_battle_group_a_effect_reward_application`
  - 当前残留信号：arguments, callee, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_effect_reward_application.hpp](../../../include/openswd3/battle/legacy_battle_group_a_effect_reward_application.hpp)；[src/battle/legacy_battle_group_a_effect_reward_application.cpp](../../../src/battle/legacy_battle_group_a_effect_reward_application.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_group_a_effect_reward_application.hpp](../../../include/openswd3/battle/legacy_battle_group_a_effect_reward_application.hpp)：41
  - `register` [include/openswd3/battle/legacy_battle_group_a_effect_reward_application.hpp](../../../include/openswd3/battle/legacy_battle_group_a_effect_reward_application.hpp)：13, 14, 44, 45, 46
  - `arguments` [src/battle/legacy_battle_group_a_effect_reward_application.cpp](../../../src/battle/legacy_battle_group_a_effect_reward_application.cpp)：190
  - `callee` [src/battle/legacy_battle_group_a_effect_reward_application.cpp](../../../src/battle/legacy_battle_group_a_effect_reward_application.cpp)：189
  - `counter` [src/battle/legacy_battle_group_a_effect_reward_application.cpp](../../../src/battle/legacy_battle_group_a_effect_reward_application.cpp)：196
  - `invoke` [src/battle/legacy_battle_group_a_effect_reward_application.cpp](../../../src/battle/legacy_battle_group_a_effect_reward_application.cpp)：195
  - `register` [src/battle/legacy_battle_group_a_effect_reward_application.cpp](../../../src/battle/legacy_battle_group_a_effect_reward_application.cpp)：15, 16, 44, 51, 52, 83, 84, 86, 88, 89, 90, 102, 107, 112, 118, 119, 120, 123, 124, 127, 128, 129, 130, 131, 138, 139, 140, 147, 148, 151, 156, 157, 158, 177, 178, 182, 183, 184, 192, 193, 194, 197, 198, 199, 200, 201, 205, 206, 207, 216, 217, 218, 222, 226, 229, 230, 231, 244, 247, 248, 249

- [ ] **CHAIN-44a8a8e00a2d** `battle/legacy_battle_group_a_embedded_profile_application`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_embedded_profile_application.hpp](../../../include/openswd3/battle/legacy_battle_group_a_embedded_profile_application.hpp)；[src/battle/legacy_battle_group_a_embedded_profile_application.cpp](../../../src/battle/legacy_battle_group_a_embedded_profile_application.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_group_a_embedded_profile_application.hpp](../../../include/openswd3/battle/legacy_battle_group_a_embedded_profile_application.hpp)：19, 20, 43, 44, 45
  - `register` [src/battle/legacy_battle_group_a_embedded_profile_application.cpp](../../../src/battle/legacy_battle_group_a_embedded_profile_application.cpp)：98, 99, 100, 112, 152, 154, 174, 175, 176, 177, 184, 185, 186, 189, 190, 191, 196, 197, 198, 210, 211, 212, 219, 220, 221, 228, 229, 230, 232, 235, 236, 237, 259, 285, 286, 287, 298, 299, 300, 304, 305, 306

- [ ] **CHAIN-acdcfbe45427** `battle/legacy_battle_group_a_final_processing`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_final_processing.hpp](../../../include/openswd3/battle/legacy_battle_group_a_final_processing.hpp)；[src/battle/legacy_battle_group_a_final_processing.cpp](../../../src/battle/legacy_battle_group_a_final_processing.cpp)。
  - 类型：[PORT-e8a07778a7da](invoke-removal-ports.md#port-e8a07778a7da)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_group_a_final_processing.hpp](../../../include/openswd3/battle/legacy_battle_group_a_final_processing.hpp)：39, 40, 41
  - `register` [include/openswd3/battle/legacy_battle_group_a_final_processing.hpp](../../../include/openswd3/battle/legacy_battle_group_a_final_processing.hpp)：20, 21, 49, 50, 51, 69, 70, 71, 81
  - `counter` [src/battle/legacy_battle_group_a_final_processing.cpp](../../../src/battle/legacy_battle_group_a_final_processing.cpp)：165, 187, 209, 232
  - `register` [src/battle/legacy_battle_group_a_final_processing.cpp](../../../src/battle/legacy_battle_group_a_final_processing.cpp)：26, 29, 30, 31, 39, 62, 63, 83, 84, 85, 93, 94, 95, 97, 98, 99, 114, 117, 118, 124, 125, 132, 133, 138, 139, 140, 143, 144, 145, 147, 151, 155, 163, 166, 167, 168, 173, 185, 188, 189, 190, 195, 213, 219, 222, 223, 229, 236, 239, 240, 246, 247, 248, 249, 260, 261, 262, 263

- [ ] **CHAIN-de3c4b230f6d** `battle/legacy_battle_group_a_frame`
  - 当前残留信号：arguments, callee, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_frame.hpp](../../../include/openswd3/battle/legacy_battle_group_a_frame.hpp)；[src/battle/legacy_battle_group_a_frame.cpp](../../../src/battle/legacy_battle_group_a_frame.cpp)。
  - 类型：[PORT-e419a631f126](invoke-removal-ports.md#port-e419a631f126), [PORT-0e5c19ca16d1](invoke-removal-ports.md#port-0e5c19ca16d1), [PORT-a6765d0b70fb](invoke-removal-ports.md#port-a6765d0b70fb)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [src/battle/legacy_battle_group_a_frame.cpp](../../../src/battle/legacy_battle_group_a_frame.cpp)：132, 133, 135, 139, 166, 170, 175, 180, 181, 182, 187, 206, 245, 246, 248, 252, 404, 406, 618, 630, 631, 632, 633, 634, 902, 905
  - `callee` [src/battle/legacy_battle_group_a_frame.cpp](../../../src/battle/legacy_battle_group_a_frame.cpp)：138, 165, 174, 179, 186, 205, 228, 234, 238, 242, 251, 403, 406, 901, 904, 1302
  - `counter` [src/battle/legacy_battle_group_a_frame.cpp](../../../src/battle/legacy_battle_group_a_frame.cpp)：522, 524, 574, 604, 637, 651, 655, 656, 657, 658, 659, 662, 663, 746, 747, 848, 918, 957, 968, 1006, 1012, 1035, 1093, 1144, 1298, 1301, 1392, 1540, 2366, 3075, 3154, 3251
  - `invoke` [src/battle/legacy_battle_group_a_frame.cpp](../../../src/battle/legacy_battle_group_a_frame.cpp)：129, 137, 162, 185, 204, 225, 250, 401, 406, 780, 909, 919, 958, 969, 1008, 1013, 1145, 1328, 1424, 1436, 1448, 1500, 1510, 1576, 1595, 1616, 2094, 2142, 2163, 2215, 2399, 2629, 2694, 2713, 2715, 2857, 2859, 2908, 2921, 3049, 3101, 3104, 3162, 3189, 3210
  - `register` [src/battle/legacy_battle_group_a_frame.cpp](../../../src/battle/legacy_battle_group_a_frame.cpp)：140, 141, 142, 145, 146, 147, 188, 189, 190, 193, 194, 195, 202, 207, 208, 209, 211, 253, 254, 255, 258, 259, 260, 416, 417, 425, 426, 435, 446, 447, 463, 464, 474, 485, 486, 497, 498, 508, 529, 532, 781, 831, 832, 833, 835, 852, 856, 860, 863, 876, 877, 878, 890, 898, 899, 900, 906, 907, 908, 910, 911, 912, 920, 924, 927, 929, 930, 934, 944, 945, 946, 955, 956, 958, 959, 960, 961, 962, 966, 967, 969, 972, 988, 989, 990, 1005, 1010, 1045, 1046, 1072, 1073, 1074, 1090, 1091, 1092, 1100, 1101, 1102, 1103, 1104, 1108, 1109, 1110, 1115, 1116, 1117, 1123, 1124, 1125, 1130, 1131, 1132, 1155, 1156, 1157, 1208, 1209, 1223, 1224, 1234, 1293, 1321, 1341, 1344, 1348, 1384, 1385, 1407, 1429, 1443, 1472, 1511, 1535, 1536, 1537, 1553, 1579, 1596, 1607, 1616, 1797, 2069, 2097, 2113, 2147, 2163, 2220, 2231, 2250, 2256, 2257, 2258, 2265, 2403, 2404, 2434, 2455, 2456, 2495, 2504, 2531, 2545, 2630, 2634, 2635, 2639, 2673, 2714, 2774, 2858, 2862, 2890, 2909, 2910, 2912, 2926, 2942, 2943, 2944, 2956, 2958, 2972, 2973, 3040, 3049, 3071, 3080, 3083, 3101, 3105, 3150, 3151, 3161, 3247, 3256, 3261

- [ ] **CHAIN-9078735f2819** `battle/legacy_battle_group_a_growth_reward_selection`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_growth_reward_selection.hpp](../../../include/openswd3/battle/legacy_battle_group_a_growth_reward_selection.hpp)；[src/battle/legacy_battle_group_a_growth_reward_selection.cpp](../../../src/battle/legacy_battle_group_a_growth_reward_selection.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_group_a_growth_reward_selection.hpp](../../../include/openswd3/battle/legacy_battle_group_a_growth_reward_selection.hpp)：30, 31, 32
  - `register` [src/battle/legacy_battle_group_a_growth_reward_selection.cpp](../../../src/battle/legacy_battle_group_a_growth_reward_selection.cpp)：52, 53, 54, 56, 57, 58, 75, 76, 77, 80, 84, 85, 86, 99, 105, 112, 113, 116, 121, 122, 123, 130, 133, 134, 135

- [ ] **CHAIN-6d31c0e7f817** `battle/legacy_battle_group_a_item_effect_application`
  - 当前残留信号：counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_item_effect_application.hpp](../../../include/openswd3/battle/legacy_battle_group_a_item_effect_application.hpp)；[src/battle/legacy_battle_group_a_item_effect_application.cpp](../../../src/battle/legacy_battle_group_a_item_effect_application.cpp)。
  - 类型：[PORT-7075003e8754](invoke-removal-ports.md#port-7075003e8754)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_group_a_item_effect_application.hpp](../../../include/openswd3/battle/legacy_battle_group_a_item_effect_application.hpp)：80, 81, 82
  - `invoke` [include/openswd3/battle/legacy_battle_group_a_item_effect_application.hpp](../../../include/openswd3/battle/legacy_battle_group_a_item_effect_application.hpp)：58
  - `register` [include/openswd3/battle/legacy_battle_group_a_item_effect_application.hpp](../../../include/openswd3/battle/legacy_battle_group_a_item_effect_application.hpp)：40, 41, 42, 46, 47, 48, 65, 66, 91, 92, 93
  - `counter` [src/battle/legacy_battle_group_a_item_effect_application.cpp](../../../src/battle/legacy_battle_group_a_item_effect_application.cpp)：102, 131, 351
  - `invoke` [src/battle/legacy_battle_group_a_item_effect_application.cpp](../../../src/battle/legacy_battle_group_a_item_effect_application.cpp)：92, 121, 339
  - `register` [src/battle/legacy_battle_group_a_item_effect_application.cpp](../../../src/battle/legacy_battle_group_a_item_effect_application.cpp)：78, 79, 80, 88, 89, 90, 98, 99, 100, 103, 105, 106, 107, 112, 114, 115, 116, 127, 128, 129, 136, 137, 138, 143, 144, 148, 152, 155, 156, 159, 173, 174, 175, 176, 180, 184, 187, 188, 191, 214, 238, 240, 241, 242, 243, 244, 245, 246, 254, 260, 266, 269, 270, 271, 272, 273, 274, 275, 277, 281, 282, 283, 287, 288, 289, 290, 291, 292, 293, 294, 295, 296, 297, 298, 300, 304, 305, 306, 315, 316, 317, 318, 319, 320, 321, 322, 323, 325, 326, 327, 328, 329, 330, 347, 348, 349, 352, 353, 354

- [ ] **CHAIN-095fa71b8372** `battle/legacy_battle_group_a_npc_materialization`
  - 当前残留信号：counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_npc_materialization.hpp](../../../include/openswd3/battle/legacy_battle_group_a_npc_materialization.hpp)；[src/battle/legacy_battle_group_a_npc_materialization.cpp](../../../src/battle/legacy_battle_group_a_npc_materialization.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_group_a_npc_materialization.hpp](../../../include/openswd3/battle/legacy_battle_group_a_npc_materialization.hpp)：31, 32, 33, 34
  - `register` [include/openswd3/battle/legacy_battle_group_a_npc_materialization.hpp](../../../include/openswd3/battle/legacy_battle_group_a_npc_materialization.hpp)：41, 42, 43
  - `counter` [src/battle/legacy_battle_group_a_npc_materialization.cpp](../../../src/battle/legacy_battle_group_a_npc_materialization.cpp)：133, 168, 183, 219
  - `invoke` [src/battle/legacy_battle_group_a_npc_materialization.cpp](../../../src/battle/legacy_battle_group_a_npc_materialization.cpp)：104, 108, 126, 204
  - `register` [src/battle/legacy_battle_group_a_npc_materialization.cpp](../../../src/battle/legacy_battle_group_a_npc_materialization.cpp)：134, 140, 141, 142, 294, 296, 297

- [ ] **CHAIN-355231daf56c** `battle/legacy_battle_group_a_profile_mode_selection`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_profile_mode_selection.hpp](../../../include/openswd3/battle/legacy_battle_group_a_profile_mode_selection.hpp)；[src/battle/legacy_battle_group_a_profile_mode_selection.cpp](../../../src/battle/legacy_battle_group_a_profile_mode_selection.cpp)。
  - 类型：[PORT-73eb1218ea44](invoke-removal-ports.md#port-73eb1218ea44)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_group_a_profile_mode_selection.hpp](../../../include/openswd3/battle/legacy_battle_group_a_profile_mode_selection.hpp)：39
  - `register` [include/openswd3/battle/legacy_battle_group_a_profile_mode_selection.hpp](../../../include/openswd3/battle/legacy_battle_group_a_profile_mode_selection.hpp)：11, 12, 13, 21, 26, 27, 43, 44, 45
  - `counter` [src/battle/legacy_battle_group_a_profile_mode_selection.cpp](../../../src/battle/legacy_battle_group_a_profile_mode_selection.cpp)：83
  - `register` [src/battle/legacy_battle_group_a_profile_mode_selection.cpp](../../../src/battle/legacy_battle_group_a_profile_mode_selection.cpp)：21, 22, 23, 30, 31, 32, 34, 35, 36, 49, 50, 51, 57, 58, 59, 63, 64, 65, 69, 70, 72, 73, 74, 82, 84, 85, 86, 87, 94, 95, 97, 98, 99

- [ ] **CHAIN-c01236183e96** `battle/legacy_battle_group_a_resource_pair`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_resource_pair.hpp](../../../include/openswd3/battle/legacy_battle_group_a_resource_pair.hpp)；[src/battle/legacy_battle_group_a_resource_pair.cpp](../../../src/battle/legacy_battle_group_a_resource_pair.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_group_a_resource_pair.hpp](../../../include/openswd3/battle/legacy_battle_group_a_resource_pair.hpp)：27, 28, 29, 38, 46
  - `register` [src/battle/legacy_battle_group_a_resource_pair.cpp](../../../src/battle/legacy_battle_group_a_resource_pair.cpp)：10, 18, 27, 30, 31, 32

- [ ] **CHAIN-fb8b8a6b310f** `battle/legacy_battle_group_a_reward_profile_application`
  - 当前残留信号：arguments, callee, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_reward_profile_application.hpp](../../../include/openswd3/battle/legacy_battle_group_a_reward_profile_application.hpp)；[src/battle/legacy_battle_group_a_reward_profile_application.cpp](../../../src/battle/legacy_battle_group_a_reward_profile_application.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_group_a_reward_profile_application.hpp](../../../include/openswd3/battle/legacy_battle_group_a_reward_profile_application.hpp)：38
  - `register` [include/openswd3/battle/legacy_battle_group_a_reward_profile_application.hpp](../../../include/openswd3/battle/legacy_battle_group_a_reward_profile_application.hpp)：14, 15, 41, 42, 43
  - `arguments` [src/battle/legacy_battle_group_a_reward_profile_application.cpp](../../../src/battle/legacy_battle_group_a_reward_profile_application.cpp)：203
  - `callee` [src/battle/legacy_battle_group_a_reward_profile_application.cpp](../../../src/battle/legacy_battle_group_a_reward_profile_application.cpp)：202
  - `counter` [src/battle/legacy_battle_group_a_reward_profile_application.cpp](../../../src/battle/legacy_battle_group_a_reward_profile_application.cpp)：209
  - `invoke` [src/battle/legacy_battle_group_a_reward_profile_application.cpp](../../../src/battle/legacy_battle_group_a_reward_profile_application.cpp)：208
  - `register` [src/battle/legacy_battle_group_a_reward_profile_application.cpp](../../../src/battle/legacy_battle_group_a_reward_profile_application.cpp)：15, 16, 44, 50, 96, 125, 126, 127, 135, 136, 137, 144, 152, 153, 154, 160, 161, 164, 169, 170, 171, 191, 192, 195, 196, 197, 205, 206, 207, 210, 211, 212, 213, 214, 218, 219, 220, 230, 231, 232, 236, 240, 243, 244, 245, 255, 258, 259, 260

- [ ] **CHAIN-8441284c4758** `battle/legacy_battle_group_a_summon_materialization`
  - 当前残留信号：counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_summon_materialization.hpp](../../../include/openswd3/battle/legacy_battle_group_a_summon_materialization.hpp)；[src/battle/legacy_battle_group_a_summon_materialization.cpp](../../../src/battle/legacy_battle_group_a_summon_materialization.cpp)。
  - 类型：[PORT-f12f61c2ae44](invoke-removal-ports.md#port-f12f61c2ae44)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_group_a_summon_materialization.hpp](../../../include/openswd3/battle/legacy_battle_group_a_summon_materialization.hpp)：80, 81, 82, 83
  - `invoke` [include/openswd3/battle/legacy_battle_group_a_summon_materialization.hpp](../../../include/openswd3/battle/legacy_battle_group_a_summon_materialization.hpp)：61
  - `register` [include/openswd3/battle/legacy_battle_group_a_summon_materialization.hpp](../../../include/openswd3/battle/legacy_battle_group_a_summon_materialization.hpp)：49, 50, 51, 88, 89, 90
  - `counter` [src/battle/legacy_battle_group_a_summon_materialization.cpp](../../../src/battle/legacy_battle_group_a_summon_materialization.cpp)：98, 135, 150, 181
  - `invoke` [src/battle/legacy_battle_group_a_summon_materialization.cpp](../../../src/battle/legacy_battle_group_a_summon_materialization.cpp)：72, 76, 91, 166
  - `register` [src/battle/legacy_battle_group_a_summon_materialization.cpp](../../../src/battle/legacy_battle_group_a_summon_materialization.cpp)：99, 100, 101, 114, 209, 210, 212

- [ ] **CHAIN-335b8167d002** `battle/legacy_battle_group_a_target_cycle`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_target_cycle.hpp](../../../include/openswd3/battle/legacy_battle_group_a_target_cycle.hpp)；[src/battle/legacy_battle_group_a_target_cycle.cpp](../../../src/battle/legacy_battle_group_a_target_cycle.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_group_a_target_cycle.hpp](../../../include/openswd3/battle/legacy_battle_group_a_target_cycle.hpp)：18, 19, 20, 32, 33, 34
  - `register` [src/battle/legacy_battle_group_a_target_cycle.cpp](../../../src/battle/legacy_battle_group_a_target_cycle.cpp)：36, 37, 38, 40, 41, 42, 43, 44, 45, 46, 49, 51, 52, 54, 57, 58, 59, 63, 64, 69, 70, 71, 74, 75, 76

- [ ] **CHAIN-53c39dfdbf07** `battle/legacy_battle_group_a_value_pair`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_value_pair.hpp](../../../include/openswd3/battle/legacy_battle_group_a_value_pair.hpp)；[src/battle/legacy_battle_group_a_value_pair.cpp](../../../src/battle/legacy_battle_group_a_value_pair.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_group_a_value_pair.hpp](../../../include/openswd3/battle/legacy_battle_group_a_value_pair.hpp)：27, 28, 29, 38, 46
  - `register` [src/battle/legacy_battle_group_a_value_pair.cpp](../../../src/battle/legacy_battle_group_a_value_pair.cpp)：9, 17, 25, 28, 29, 30

- [ ] **CHAIN-adaf61fd616e** `battle/legacy_battle_group_a_workspace_reset`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_workspace_reset.hpp](../../../include/openswd3/battle/legacy_battle_group_a_workspace_reset.hpp)；[src/battle/legacy_battle_group_a_workspace_reset.cpp](../../../src/battle/legacy_battle_group_a_workspace_reset.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_group_a_workspace_reset.hpp](../../../include/openswd3/battle/legacy_battle_group_a_workspace_reset.hpp)：43, 44, 45
  - `register` [src/battle/legacy_battle_group_a_workspace_reset.cpp](../../../src/battle/legacy_battle_group_a_workspace_reset.cpp)：104

- [ ] **CHAIN-ea702adfb034** `battle/legacy_battle_group_b_action_composition`
  - 当前残留信号：arguments, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_action_composition.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_composition.hpp)；[src/battle/legacy_battle_group_b_action_composition.cpp](../../../src/battle/legacy_battle_group_b_action_composition.cpp)。
  - 类型：[PORT-8cc7663143f8](invoke-removal-ports.md#port-8cc7663143f8)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_group_b_action_composition.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_composition.hpp)：26
  - `invoke` [include/openswd3/battle/legacy_battle_group_b_action_composition.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_composition.hpp)：46
  - `register` [include/openswd3/battle/legacy_battle_group_b_action_composition.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_composition.hpp)：27, 28, 29, 33, 34, 35, 53, 54, 55, 77, 78, 79
  - `arguments` [src/battle/legacy_battle_group_b_action_composition.cpp](../../../src/battle/legacy_battle_group_b_action_composition.cpp)：89
  - `invoke` [src/battle/legacy_battle_group_b_action_composition.cpp](../../../src/battle/legacy_battle_group_b_action_composition.cpp)：87
  - `register` [src/battle/legacy_battle_group_b_action_composition.cpp](../../../src/battle/legacy_battle_group_b_action_composition.cpp)：40, 41, 42, 74, 75, 76, 94, 95, 96, 98, 99, 100, 124, 125, 126, 132, 148, 152, 167, 176

- [ ] **CHAIN-811265d139a4** `battle/legacy_battle_group_b_action_configuration`
  - 当前残留信号：interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_action_configuration.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_configuration.hpp)；[src/battle/legacy_battle_group_b_action_configuration.cpp](../../../src/battle/legacy_battle_group_b_action_configuration.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_group_b_action_configuration.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_configuration.hpp)：27, 28, 29
  - `register` [src/battle/legacy_battle_group_b_action_configuration.cpp](../../../src/battle/legacy_battle_group_b_action_configuration.cpp)：87, 88, 89, 140, 204, 205, 210, 211

- [ ] **CHAIN-2bfb0f9ee21d** `battle/legacy_battle_group_b_action_execution`
  - 当前残留信号：arguments, callee, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_action_execution.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_execution.hpp)；[src/battle/legacy_battle_group_b_action_execution.cpp](../../../src/battle/legacy_battle_group_b_action_execution.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_group_b_action_execution.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_execution.hpp)：46, 47, 48, 49, 50
  - `register` [include/openswd3/battle/legacy_battle_group_b_action_execution.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_execution.hpp)：31, 32, 56, 57, 58
  - `arguments` [src/battle/legacy_battle_group_b_action_execution.cpp](../../../src/battle/legacy_battle_group_b_action_execution.cpp)：152, 156, 163, 168, 219, 225, 233, 238
  - `callee` [src/battle/legacy_battle_group_b_action_execution.cpp](../../../src/battle/legacy_battle_group_b_action_execution.cpp)：151, 155, 162, 167, 216, 224, 237
  - `counter` [src/battle/legacy_battle_group_b_action_execution.cpp](../../../src/battle/legacy_battle_group_b_action_execution.cpp)：164, 221, 234, 350, 534
  - `invoke` [src/battle/legacy_battle_group_b_action_execution.cpp](../../../src/battle/legacy_battle_group_b_action_execution.cpp)：154, 165, 222, 235, 261, 265, 288, 317, 384, 397, 409, 480, 485, 492, 498, 502, 576, 601, 632, 703
  - `register` [src/battle/legacy_battle_group_b_action_execution.cpp](../../../src/battle/legacy_battle_group_b_action_execution.cpp)：39, 40, 41, 108, 109, 110, 125, 126, 127, 132, 133, 134, 138, 139, 143, 144, 145, 153, 157, 158, 159, 169, 170, 171, 187, 188, 201, 202, 207, 208, 209, 226, 227, 228, 239, 240, 241, 261, 264, 269, 286, 287, 294, 302, 305, 315, 316, 323, 326, 330, 364, 383, 387, 399, 405, 406, 407, 410, 421, 422, 428, 429, 430, 431, 432, 433, 439, 440, 441, 442, 443, 444, 452, 453, 459, 460, 461, 462, 463, 464, 470, 471, 472, 473, 474, 475, 491, 499, 500, 554, 559, 574, 575, 589, 598, 599, 600, 620, 623, 624, 630, 631, 666, 667, 668, 700, 701, 702, 704, 713, 723, 724

- [ ] **CHAIN-2c0f2f5c5e48** `battle/legacy_battle_group_b_action_item_option`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_action_item_option.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_item_option.hpp)；[src/battle/legacy_battle_group_b_action_item_option.cpp](../../../src/battle/legacy_battle_group_b_action_item_option.cpp)。
  - 类型：[PORT-911a059381af](invoke-removal-ports.md#port-911a059381af)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_group_b_action_item_option.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_item_option.hpp)：59, 60
  - `register` [include/openswd3/battle/legacy_battle_group_b_action_item_option.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_item_option.hpp)：12, 13, 14, 18, 19, 20, 50, 51, 52, 66, 67, 68
  - `counter` [src/battle/legacy_battle_group_b_action_item_option.cpp](../../../src/battle/legacy_battle_group_b_action_item_option.cpp)：109, 168
  - `register` [src/battle/legacy_battle_group_b_action_item_option.cpp](../../../src/battle/legacy_battle_group_b_action_item_option.cpp)：33, 34, 35, 38, 39, 40, 55, 59, 68, 69, 74, 76, 77, 82, 84, 85, 90, 94, 98, 117, 118, 119, 120, 126, 127, 129, 130, 131, 132, 138, 139, 141, 142, 143, 144, 150, 151, 164, 165, 166, 169, 170, 171, 194

- [ ] **CHAIN-76bc7cabdf1c** `battle/legacy_battle_group_b_action_item_selection`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_action_item_selection.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_item_selection.hpp)；[src/battle/legacy_battle_group_b_action_item_selection.cpp](../../../src/battle/legacy_battle_group_b_action_item_selection.cpp)。
  - 类型：[PORT-efe72baa43ee](invoke-removal-ports.md#port-efe72baa43ee)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_group_b_action_item_selection.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_item_selection.hpp)：36, 37
  - `register` [include/openswd3/battle/legacy_battle_group_b_action_item_selection.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_item_selection.hpp)：27, 28, 29, 47, 48, 49
  - `counter` [src/battle/legacy_battle_group_b_action_item_selection.cpp](../../../src/battle/legacy_battle_group_b_action_item_selection.cpp)：40, 190
  - `register` [src/battle/legacy_battle_group_b_action_item_selection.cpp](../../../src/battle/legacy_battle_group_b_action_item_selection.cpp)：35, 36, 37, 41, 42, 43, 76, 77, 78, 87, 91, 92, 93, 96, 97, 98, 99, 101, 102, 103, 110, 114, 115, 116, 123, 124, 125, 131, 133, 138, 139, 140, 141, 142, 143, 149, 150, 154, 155, 156, 161, 162, 164, 168, 169, 170, 175, 176, 178, 199, 201, 202, 207

- [ ] **CHAIN-81c2e06fa574** `battle/legacy_battle_group_b_action_item_special_option`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_action_item_special_option.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_item_special_option.hpp)；[src/battle/legacy_battle_group_b_action_item_special_option.cpp](../../../src/battle/legacy_battle_group_b_action_item_special_option.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_group_b_action_item_special_option.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_item_special_option.hpp)：30, 31
  - `register` [include/openswd3/battle/legacy_battle_group_b_action_item_special_option.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_item_special_option.hpp)：21, 22, 23, 36, 37, 38
  - `counter` [src/battle/legacy_battle_group_b_action_item_special_option.cpp](../../../src/battle/legacy_battle_group_b_action_item_special_option.cpp)：100, 120
  - `register` [src/battle/legacy_battle_group_b_action_item_special_option.cpp](../../../src/battle/legacy_battle_group_b_action_item_special_option.cpp)：32, 33, 34, 37, 38, 39, 54, 59, 64, 65, 69, 71, 76, 77, 81, 85, 89, 108, 110, 116, 117, 118, 121, 122, 123, 146

- [ ] **CHAIN-ef5cdb11d90c** `battle/legacy_battle_group_b_action_profile_flag`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_action_profile_flag.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_profile_flag.hpp)；[src/battle/legacy_battle_group_b_action_profile_flag.cpp](../../../src/battle/legacy_battle_group_b_action_profile_flag.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_group_b_action_profile_flag.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_profile_flag.hpp)：9, 10, 22, 23, 24
  - `register` [src/battle/legacy_battle_group_b_action_profile_flag.cpp](../../../src/battle/legacy_battle_group_b_action_profile_flag.cpp)：28, 29, 30, 40, 44

- [ ] **CHAIN-5952f6556d6b** `battle/legacy_battle_group_b_action_profile_mode`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_action_profile_mode.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_profile_mode.hpp)；[src/battle/legacy_battle_group_b_action_profile_mode.cpp](../../../src/battle/legacy_battle_group_b_action_profile_mode.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_group_b_action_profile_mode.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_profile_mode.hpp)：32
  - `register` [include/openswd3/battle/legacy_battle_group_b_action_profile_mode.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_profile_mode.hpp)：14, 15, 16, 37, 38, 39
  - `counter` [src/battle/legacy_battle_group_b_action_profile_mode.cpp](../../../src/battle/legacy_battle_group_b_action_profile_mode.cpp)：108
  - `register` [src/battle/legacy_battle_group_b_action_profile_mode.cpp](../../../src/battle/legacy_battle_group_b_action_profile_mode.cpp)：35, 36, 37, 49, 67, 76, 82, 89, 90, 91, 99, 120, 121, 123, 127, 142, 145, 151

- [ ] **CHAIN-da4c2b23a06e** `battle/legacy_battle_group_b_action_profile_selection`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_action_profile_selection.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_profile_selection.hpp)；[src/battle/legacy_battle_group_b_action_profile_selection.cpp](../../../src/battle/legacy_battle_group_b_action_profile_selection.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_group_b_action_profile_selection.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_profile_selection.hpp)：33, 36
  - `register` [include/openswd3/battle/legacy_battle_group_b_action_profile_selection.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_profile_selection.hpp)：40, 41, 42
  - `counter` [src/battle/legacy_battle_group_b_action_profile_selection.cpp](../../../src/battle/legacy_battle_group_b_action_profile_selection.cpp)：72, 105
  - `register` [src/battle/legacy_battle_group_b_action_profile_selection.cpp](../../../src/battle/legacy_battle_group_b_action_profile_selection.cpp)：36, 37, 38, 50, 52, 63, 84, 87, 90, 91, 93, 120, 129, 135, 153, 156

- [ ] **CHAIN-0371a7125163** `battle/legacy_battle_group_b_action_reconfiguration`
  - 当前残留信号：interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_action_reconfiguration.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_reconfiguration.hpp)；[src/battle/legacy_battle_group_b_action_reconfiguration.cpp](../../../src/battle/legacy_battle_group_b_action_reconfiguration.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_group_b_action_reconfiguration.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_reconfiguration.hpp)：11, 27, 28, 29
  - `register` [src/battle/legacy_battle_group_b_action_reconfiguration.cpp](../../../src/battle/legacy_battle_group_b_action_reconfiguration.cpp)：42, 43, 44, 67

- [ ] **CHAIN-43c1243c60ab** `battle/legacy_battle_group_b_action_seventeen_frame`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_action_seventeen_frame.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_seventeen_frame.hpp)；[src/battle/legacy_battle_group_b_action_seventeen_frame.cpp](../../../src/battle/legacy_battle_group_b_action_seventeen_frame.cpp)。
  - 类型：[PORT-5c656e4a608b](invoke-removal-ports.md#port-5c656e4a608b)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_group_b_action_seventeen_frame.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_seventeen_frame.hpp)：34
  - `counter` [include/openswd3/battle/legacy_battle_group_b_action_seventeen_frame.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_seventeen_frame.hpp)：92, 93, 94, 95, 96, 97, 100
  - `invoke` [include/openswd3/battle/legacy_battle_group_b_action_seventeen_frame.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_seventeen_frame.hpp)：52
  - `register` [include/openswd3/battle/legacy_battle_group_b_action_seventeen_frame.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_seventeen_frame.hpp)：35, 36, 37, 41, 42, 43, 83, 84, 85, 110, 111, 112
  - `arguments` [src/battle/legacy_battle_group_b_action_seventeen_frame.cpp](../../../src/battle/legacy_battle_group_b_action_seventeen_frame.cpp)：176, 179
  - `counter` [src/battle/legacy_battle_group_b_action_seventeen_frame.cpp](../../../src/battle/legacy_battle_group_b_action_seventeen_frame.cpp)：204, 215, 227, 247, 262, 307, 365, 433
  - `invoke` [src/battle/legacy_battle_group_b_action_seventeen_frame.cpp](../../../src/battle/legacy_battle_group_b_action_seventeen_frame.cpp)：177, 205, 248, 263
  - `register` [src/battle/legacy_battle_group_b_action_seventeen_frame.cpp](../../../src/battle/legacy_battle_group_b_action_seventeen_frame.cpp)：146, 147, 148, 157, 158, 159, 163, 164, 165, 168, 169, 170, 180, 181, 182, 184, 185, 186, 195, 196, 203, 217, 221, 235, 236, 237, 243, 246, 252, 254, 257, 259, 271, 273, 277, 279, 282, 293, 304, 305, 306, 317, 318, 338, 339, 340, 362, 363, 364, 372, 373, 374, 375, 376, 380, 381, 382, 393, 401, 428, 429, 430, 449

- [ ] **CHAIN-5cb6dd3ccb40** `battle/legacy_battle_group_b_action_six_target_availability`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_action_six_target_availability.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_six_target_availability.hpp)；[src/battle/legacy_battle_group_b_action_six_target_availability.cpp](../../../src/battle/legacy_battle_group_b_action_six_target_availability.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_group_b_action_six_target_availability.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_six_target_availability.hpp)：15, 16, 23, 24, 25
  - `register` [src/battle/legacy_battle_group_b_action_six_target_availability.cpp](../../../src/battle/legacy_battle_group_b_action_six_target_availability.cpp)：34, 35, 36, 39, 40, 41, 51, 52, 58, 59, 60, 61, 64, 65, 70

- [ ] **CHAIN-3a84a94e6e54** `battle/legacy_battle_group_b_coordinate_offsets`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_coordinate_offsets.hpp](../../../include/openswd3/battle/legacy_battle_group_b_coordinate_offsets.hpp)；[src/battle/legacy_battle_group_b_coordinate_offsets.cpp](../../../src/battle/legacy_battle_group_b_coordinate_offsets.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_group_b_coordinate_offsets.hpp](../../../include/openswd3/battle/legacy_battle_group_b_coordinate_offsets.hpp)：11, 12, 30, 31, 32
  - `register` [src/battle/legacy_battle_group_b_coordinate_offsets.cpp](../../../src/battle/legacy_battle_group_b_coordinate_offsets.cpp)：34, 35, 36, 44, 52, 53, 62, 63, 65, 66

- [ ] **CHAIN-cca5f02f354d** `battle/legacy_battle_group_b_frame`
  - 当前残留信号：arguments, callee, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_frame.hpp](../../../include/openswd3/battle/legacy_battle_group_b_frame.hpp)；[src/battle/legacy_battle_group_b_frame.cpp](../../../src/battle/legacy_battle_group_b_frame.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [src/battle/legacy_battle_group_b_frame.cpp](../../../src/battle/legacy_battle_group_b_frame.cpp)：145, 149, 279, 291, 292, 293, 294, 295
  - `callee` [src/battle/legacy_battle_group_b_frame.cpp](../../../src/battle/legacy_battle_group_b_frame.cpp)：144, 148, 2190
  - `counter` [src/battle/legacy_battle_group_b_frame.cpp](../../../src/battle/legacy_battle_group_b_frame.cpp)：261, 263, 298, 317, 331, 335, 336, 337, 338, 339, 342, 343, 382, 383, 932, 997, 1100, 1463, 1794, 2186, 2189
  - `invoke` [src/battle/legacy_battle_group_b_frame.cpp](../../../src/battle/legacy_battle_group_b_frame.cpp)：142, 150, 505, 509, 560, 583, 636, 639, 781, 836, 845, 858, 871, 882, 898, 911, 1271, 1286, 1301, 1324, 1339, 1356, 1376, 1495, 1524, 1629, 1753, 1885, 1981, 1983, 2032, 2079, 2085, 2095
  - `register` [src/battle/legacy_battle_group_b_frame.cpp](../../../src/battle/legacy_battle_group_b_frame.cpp)：160, 161, 169, 170, 179, 190, 191, 202, 203, 213, 224, 225, 236, 237, 247, 268, 271, 474, 475, 488, 489, 499, 505, 533, 541, 560, 579, 584, 601, 613, 637, 642, 663, 786, 811, 817, 818, 819, 836, 850, 863, 876, 887, 903, 916, 937, 940, 993, 994, 1003, 1006, 1009, 1010, 1012, 1052, 1069, 1091, 1092, 1093, 1115, 1119, 1121, 1194, 1276, 1291, 1306, 1311, 1312, 1313, 1329, 1344, 1359, 1364, 1365, 1366, 1403, 1404, 1405, 1439, 1441, 1459, 1460, 1469, 1473, 1500, 1505, 1506, 1510, 1527, 1532, 1533, 1534, 1635, 1636, 1637, 1638, 1671, 1695, 1696, 1721, 1729, 1756, 1757, 1886, 1887, 1889, 1891, 1939, 1982, 2037, 2047, 2048, 2051, 2056, 2067, 2068, 2076, 2103, 2181, 2209

- [ ] **CHAIN-d7c76b8b561e** `battle/legacy_battle_group_b_opponent_mode`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_opponent_mode.hpp](../../../include/openswd3/battle/legacy_battle_group_b_opponent_mode.hpp)；[src/battle/legacy_battle_group_b_opponent_mode.cpp](../../../src/battle/legacy_battle_group_b_opponent_mode.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_group_b_opponent_mode.hpp](../../../include/openswd3/battle/legacy_battle_group_b_opponent_mode.hpp)：18
  - `register` [include/openswd3/battle/legacy_battle_group_b_opponent_mode.hpp](../../../include/openswd3/battle/legacy_battle_group_b_opponent_mode.hpp)：21, 22, 23
  - `counter` [src/battle/legacy_battle_group_b_opponent_mode.cpp](../../../src/battle/legacy_battle_group_b_opponent_mode.cpp)：41
  - `register` [src/battle/legacy_battle_group_b_opponent_mode.cpp](../../../src/battle/legacy_battle_group_b_opponent_mode.cpp)：42, 43, 59, 60, 78, 81, 90, 95, 103, 107

- [ ] **CHAIN-153b340e473f** `battle/legacy_battle_group_b_opponent_wave_parameters`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_opponent_wave_parameters.hpp](../../../include/openswd3/battle/legacy_battle_group_b_opponent_wave_parameters.hpp)；[src/battle/legacy_battle_group_b_opponent_wave_parameters.cpp](../../../src/battle/legacy_battle_group_b_opponent_wave_parameters.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_group_b_opponent_wave_parameters.hpp](../../../include/openswd3/battle/legacy_battle_group_b_opponent_wave_parameters.hpp)：19, 20, 31, 32, 33
  - `register` [src/battle/legacy_battle_group_b_opponent_wave_parameters.cpp](../../../src/battle/legacy_battle_group_b_opponent_wave_parameters.cpp)：36, 37, 38, 48, 59, 60

- [ ] **CHAIN-82529089cbd1** `battle/legacy_battle_group_b_order`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[src/battle/legacy_battle_group_b_order.cpp](../../../src/battle/legacy_battle_group_b_order.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [src/battle/legacy_battle_group_b_order.cpp](../../../src/battle/legacy_battle_group_b_order.cpp)：24, 27, 35, 49, 52, 53, 54

- [ ] **CHAIN-33360134ef95** `battle/legacy_battle_group_b_reward_item_selection`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_reward_item_selection.hpp](../../../include/openswd3/battle/legacy_battle_group_b_reward_item_selection.hpp)；[src/battle/legacy_battle_group_b_reward_item_selection.cpp](../../../src/battle/legacy_battle_group_b_reward_item_selection.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_group_b_reward_item_selection.hpp](../../../include/openswd3/battle/legacy_battle_group_b_reward_item_selection.hpp)：24
  - `register` [include/openswd3/battle/legacy_battle_group_b_reward_item_selection.hpp](../../../include/openswd3/battle/legacy_battle_group_b_reward_item_selection.hpp)：10, 11, 28, 29, 30
  - `counter` [src/battle/legacy_battle_group_b_reward_item_selection.cpp](../../../src/battle/legacy_battle_group_b_reward_item_selection.cpp)：49
  - `register` [src/battle/legacy_battle_group_b_reward_item_selection.cpp](../../../src/battle/legacy_battle_group_b_reward_item_selection.cpp)：25, 26, 27, 35, 44, 50, 51, 52, 53, 61, 66

- [ ] **CHAIN-9d5420ea5211** `battle/legacy_battle_group_b_script_action_item_parameters`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_script_action_item_parameters.hpp](../../../include/openswd3/battle/legacy_battle_group_b_script_action_item_parameters.hpp)；[src/battle/legacy_battle_group_b_script_action_item_parameters.cpp](../../../src/battle/legacy_battle_group_b_script_action_item_parameters.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_group_b_script_action_item_parameters.hpp](../../../include/openswd3/battle/legacy_battle_group_b_script_action_item_parameters.hpp)：19, 20, 32, 33, 34
  - `register` [src/battle/legacy_battle_group_b_script_action_item_parameters.cpp](../../../src/battle/legacy_battle_group_b_script_action_item_parameters.cpp)：23, 24, 25, 31, 48, 50

- [ ] **CHAIN-90a55a716cc3** `battle/legacy_battle_group_b_script_resource_parameters`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_script_resource_parameters.hpp](../../../include/openswd3/battle/legacy_battle_group_b_script_resource_parameters.hpp)；[src/battle/legacy_battle_group_b_script_resource_parameters.cpp](../../../src/battle/legacy_battle_group_b_script_resource_parameters.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_group_b_script_resource_parameters.hpp](../../../include/openswd3/battle/legacy_battle_group_b_script_resource_parameters.hpp)：23, 34, 35, 36
  - `register` [src/battle/legacy_battle_group_b_script_resource_parameters.cpp](../../../src/battle/legacy_battle_group_b_script_resource_parameters.cpp)：25, 26, 27, 42, 44, 60

- [ ] **CHAIN-d8a48b6dcb8e** `battle/legacy_battle_group_b_script_special_action_item_parameters`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_script_special_action_item_parameters.hpp](../../../include/openswd3/battle/legacy_battle_group_b_script_special_action_item_parameters.hpp)；[src/battle/legacy_battle_group_b_script_special_action_item_parameters.cpp](../../../src/battle/legacy_battle_group_b_script_special_action_item_parameters.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_group_b_script_special_action_item_parameters.hpp](../../../include/openswd3/battle/legacy_battle_group_b_script_special_action_item_parameters.hpp)：20, 21, 33, 34, 35
  - `register` [src/battle/legacy_battle_group_b_script_special_action_item_parameters.cpp](../../../src/battle/legacy_battle_group_b_script_special_action_item_parameters.cpp)：29, 30, 31, 37, 55, 57

- [ ] **CHAIN-77325cac05c0** `battle/legacy_battle_group_b_status_action`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_status_action.hpp](../../../include/openswd3/battle/legacy_battle_group_b_status_action.hpp)；[src/battle/legacy_battle_group_b_status_action.cpp](../../../src/battle/legacy_battle_group_b_status_action.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_group_b_status_action.hpp](../../../include/openswd3/battle/legacy_battle_group_b_status_action.hpp)：25
  - `register` [include/openswd3/battle/legacy_battle_group_b_status_action.hpp](../../../include/openswd3/battle/legacy_battle_group_b_status_action.hpp)：17, 18, 33, 34, 35
  - `counter` [src/battle/legacy_battle_group_b_status_action.cpp](../../../src/battle/legacy_battle_group_b_status_action.cpp)：28, 68
  - `register` [src/battle/legacy_battle_group_b_status_action.cpp](../../../src/battle/legacy_battle_group_b_status_action.cpp)：29, 30, 31, 39, 51, 52, 53, 54, 63, 69, 70, 71, 75, 83, 85, 90, 109, 115, 124

- [ ] **CHAIN-b24cb45fefd0** `battle/legacy_battle_group_b_target_cycle`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_target_cycle.hpp](../../../include/openswd3/battle/legacy_battle_group_b_target_cycle.hpp)；[src/battle/legacy_battle_group_b_target_cycle.cpp](../../../src/battle/legacy_battle_group_b_target_cycle.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_group_b_target_cycle.hpp](../../../include/openswd3/battle/legacy_battle_group_b_target_cycle.hpp)：38
  - `register` [include/openswd3/battle/legacy_battle_group_b_target_cycle.hpp](../../../include/openswd3/battle/legacy_battle_group_b_target_cycle.hpp)：19, 20, 21, 34, 35, 36
  - `arguments` [src/battle/legacy_battle_group_b_target_cycle.cpp](../../../src/battle/legacy_battle_group_b_target_cycle.cpp)：104
  - `counter` [src/battle/legacy_battle_group_b_target_cycle.cpp](../../../src/battle/legacy_battle_group_b_target_cycle.cpp)：109
  - `invoke` [src/battle/legacy_battle_group_b_target_cycle.cpp](../../../src/battle/legacy_battle_group_b_target_cycle.cpp)：51, 86, 101, 122, 123
  - `register` [src/battle/legacy_battle_group_b_target_cycle.cpp](../../../src/battle/legacy_battle_group_b_target_cycle.cpp)：32, 33, 105, 106, 107, 110, 111, 112, 126, 127, 128, 131, 132, 133, 141, 142, 143

- [ ] **CHAIN-6c6269e4e288** `battle/legacy_battle_group_effect_frame`
  - 当前残留信号：arguments, callee, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_effect_frame.hpp](../../../include/openswd3/battle/legacy_battle_group_effect_frame.hpp)；[src/battle/legacy_battle_group_effect_frame.cpp](../../../src/battle/legacy_battle_group_effect_frame.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_group_effect_frame.hpp](../../../include/openswd3/battle/legacy_battle_group_effect_frame.hpp)：67, 73, 75, 77, 84
  - `register` [include/openswd3/battle/legacy_battle_group_effect_frame.hpp](../../../include/openswd3/battle/legacy_battle_group_effect_frame.hpp)：65, 66
  - `arguments` [src/battle/legacy_battle_group_effect_frame.cpp](../../../src/battle/legacy_battle_group_effect_frame.cpp)：199, 206
  - `callee` [src/battle/legacy_battle_group_effect_frame.cpp](../../../src/battle/legacy_battle_group_effect_frame.cpp)：198, 201
  - `counter` [src/battle/legacy_battle_group_effect_frame.cpp](../../../src/battle/legacy_battle_group_effect_frame.cpp)：360, 396, 511, 547, 695
  - `invoke` [src/battle/legacy_battle_group_effect_frame.cpp](../../../src/battle/legacy_battle_group_effect_frame.cpp)：208, 319, 325, 330, 488, 561, 570, 571, 592, 601, 607, 624, 633, 634, 670, 677, 718, 733, 764, 777, 790, 794, 824, 828, 831, 869, 872, 875, 906, 936, 953, 984
  - `register` [src/battle/legacy_battle_group_effect_frame.cpp](../../../src/battle/legacy_battle_group_effect_frame.cpp)：35, 36, 37, 197, 202, 203, 204, 209, 210, 211, 224, 225, 230, 231, 232, 252, 253, 258, 259, 260, 264, 265, 266, 281, 319, 328, 354, 355, 356, 361, 362, 363, 390, 391, 397, 398, 399, 491, 506, 507, 508, 512, 513, 514, 554, 580, 592, 605, 636, 646, 655, 657, 664, 673, 696, 697, 698, 707, 709, 718, 723, 726, 733, 738, 748, 781, 804, 820, 822, 837, 839, 847, 866, 882, 886, 904, 909, 910, 911, 915, 927, 936, 953, 974, 977, 984, 1003, 1035, 1042, 1051, 1052, 1053, 1055, 1056, 1057, 1097, 1103, 1110, 1118, 1119, 1120, 1122, 1123, 1124

- [ ] **CHAIN-5772565724bd** `battle/legacy_battle_growth_actor_selection`
  - 当前残留信号：arguments, counter, interface, invoke, operation, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_growth_actor_selection.hpp](../../../include/openswd3/battle/legacy_battle_growth_actor_selection.hpp)；[src/battle/legacy_battle_growth_actor_selection.cpp](../../../src/battle/legacy_battle_growth_actor_selection.cpp)。
  - 类型：[PORT-b407b49d5c4a](invoke-removal-ports.md#port-b407b49d5c4a), [PORT-9eb33b05d668](invoke-removal-ports.md#port-9eb33b05d668)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_growth_actor_selection.hpp](../../../include/openswd3/battle/legacy_battle_growth_actor_selection.hpp)：77
  - `counter` [include/openswd3/battle/legacy_battle_growth_actor_selection.hpp](../../../include/openswd3/battle/legacy_battle_growth_actor_selection.hpp)：152, 153, 154, 155, 156
  - `invoke` [include/openswd3/battle/legacy_battle_growth_actor_selection.hpp](../../../include/openswd3/battle/legacy_battle_growth_actor_selection.hpp)：107
  - `register` [include/openswd3/battle/legacy_battle_growth_actor_selection.hpp](../../../include/openswd3/battle/legacy_battle_growth_actor_selection.hpp)：78, 79, 80, 84, 85, 86, 111, 112, 113, 127, 128, 129, 149, 150, 151
  - `arguments` [src/battle/legacy_battle_growth_actor_selection.cpp](../../../src/battle/legacy_battle_growth_actor_selection.cpp)：84, 91
  - `counter` [src/battle/legacy_battle_growth_actor_selection.cpp](../../../src/battle/legacy_battle_growth_actor_selection.cpp)：132, 179, 220, 254, 288, 377, 408, 412
  - `invoke` [src/battle/legacy_battle_growth_actor_selection.cpp](../../../src/battle/legacy_battle_growth_actor_selection.cpp)：86
  - `operation` [src/battle/legacy_battle_growth_actor_selection.cpp](../../../src/battle/legacy_battle_growth_actor_selection.cpp)：80, 87, 96
  - `register` [src/battle/legacy_battle_growth_actor_selection.cpp](../../../src/battle/legacy_battle_growth_actor_selection.cpp)：48, 49, 92, 93, 94, 97, 98, 99, 501, 502, 503, 508, 509, 510

- [ ] **CHAIN-0a21d93eae41** `battle/legacy_battle_growth_caption`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_growth_caption.hpp](../../../include/openswd3/battle/legacy_battle_growth_caption.hpp)；[src/battle/legacy_battle_growth_caption.cpp](../../../src/battle/legacy_battle_growth_caption.cpp)。
  - 类型：[PORT-c72a604940f2](invoke-removal-ports.md#port-c72a604940f2)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_growth_caption.hpp](../../../include/openswd3/battle/legacy_battle_growth_caption.hpp)：41
  - `counter` [include/openswd3/battle/legacy_battle_growth_caption.hpp](../../../include/openswd3/battle/legacy_battle_growth_caption.hpp)：132, 133, 134, 135, 136, 137, 142
  - `invoke` [include/openswd3/battle/legacy_battle_growth_caption.hpp](../../../include/openswd3/battle/legacy_battle_growth_caption.hpp)：73
  - `register` [include/openswd3/battle/legacy_battle_growth_caption.hpp](../../../include/openswd3/battle/legacy_battle_growth_caption.hpp)：42, 43, 44, 50, 51, 52, 63, 64, 65, 75, 76, 77, 81, 91, 92, 93, 97, 106, 107, 108, 129, 130, 131
  - `arguments` [src/battle/legacy_battle_growth_caption.cpp](../../../src/battle/legacy_battle_growth_caption.cpp)：105, 110
  - `counter` [src/battle/legacy_battle_growth_caption.cpp](../../../src/battle/legacy_battle_growth_caption.cpp)：81, 175, 191, 256, 284, 299, 333, 372, 392, 419
  - `invoke` [src/battle/legacy_battle_growth_caption.cpp](../../../src/battle/legacy_battle_growth_caption.cpp)：103, 117, 166, 319, 363, 405
  - `register` [src/battle/legacy_battle_growth_caption.cpp](../../../src/battle/legacy_battle_growth_caption.cpp)：58, 59, 82, 83, 84, 111, 112, 113, 119, 120, 121, 257, 258, 259, 285, 286, 287, 300, 301, 302, 423, 424, 425

- [ ] **CHAIN-d41c2d546b04** `battle/legacy_battle_growth_item_completion_panel`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_growth_item_completion_panel.hpp](../../../include/openswd3/battle/legacy_battle_growth_item_completion_panel.hpp)；[src/battle/legacy_battle_growth_item_completion_panel.cpp](../../../src/battle/legacy_battle_growth_item_completion_panel.cpp)。
  - 类型：[PORT-7d32b439f489](invoke-removal-ports.md#port-7d32b439f489)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_growth_item_completion_panel.hpp](../../../include/openswd3/battle/legacy_battle_growth_item_completion_panel.hpp)：46
  - `counter` [include/openswd3/battle/legacy_battle_growth_item_completion_panel.hpp](../../../include/openswd3/battle/legacy_battle_growth_item_completion_panel.hpp)：119, 120, 121, 122, 123, 124, 125, 126
  - `invoke` [include/openswd3/battle/legacy_battle_growth_item_completion_panel.hpp](../../../include/openswd3/battle/legacy_battle_growth_item_completion_panel.hpp)：70
  - `register` [include/openswd3/battle/legacy_battle_growth_item_completion_panel.hpp](../../../include/openswd3/battle/legacy_battle_growth_item_completion_panel.hpp)：47, 48, 49, 55, 56, 57, 74, 75, 76, 79, 87, 95, 96, 97, 116, 117, 118
  - `arguments` [src/battle/legacy_battle_growth_item_completion_panel.cpp](../../../src/battle/legacy_battle_growth_item_completion_panel.cpp)：84, 89
  - `counter` [src/battle/legacy_battle_growth_item_completion_panel.cpp](../../../src/battle/legacy_battle_growth_item_completion_panel.cpp)：139, 176, 203, 236, 256, 265, 284, 302, 309
  - `invoke` [src/battle/legacy_battle_growth_item_completion_panel.cpp](../../../src/battle/legacy_battle_growth_item_completion_panel.cpp)：82, 96, 130, 169, 249, 280, 288, 305
  - `register` [src/battle/legacy_battle_growth_item_completion_panel.cpp](../../../src/battle/legacy_battle_growth_item_completion_panel.cpp)：55, 56, 90, 91, 92, 98, 99, 100, 204, 205, 206, 237, 238, 239, 266, 267, 268, 313, 314, 315

- [ ] **CHAIN-5805079db8f0** `battle/legacy_battle_growth_item_result_selection`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_growth_item_result_selection.hpp](../../../include/openswd3/battle/legacy_battle_growth_item_result_selection.hpp)；[src/battle/legacy_battle_growth_item_result_selection.cpp](../../../src/battle/legacy_battle_growth_item_result_selection.cpp)。
  - 类型：[PORT-fd5db5506d60](invoke-removal-ports.md#port-fd5db5506d60)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_growth_item_result_selection.hpp](../../../include/openswd3/battle/legacy_battle_growth_item_result_selection.hpp)：47
  - `counter` [include/openswd3/battle/legacy_battle_growth_item_result_selection.hpp](../../../include/openswd3/battle/legacy_battle_growth_item_result_selection.hpp)：115, 116, 117, 118, 119
  - `invoke` [include/openswd3/battle/legacy_battle_growth_item_result_selection.hpp](../../../include/openswd3/battle/legacy_battle_growth_item_result_selection.hpp)：77
  - `register` [include/openswd3/battle/legacy_battle_growth_item_result_selection.hpp](../../../include/openswd3/battle/legacy_battle_growth_item_result_selection.hpp)：48, 49, 50, 57, 58, 59, 81, 82, 83, 87, 94, 95, 96, 112, 113, 114
  - `arguments` [src/battle/legacy_battle_growth_item_result_selection.cpp](../../../src/battle/legacy_battle_growth_item_result_selection.cpp)：78, 90
  - `counter` [src/battle/legacy_battle_growth_item_result_selection.cpp](../../../src/battle/legacy_battle_growth_item_result_selection.cpp)：130, 143, 194, 227, 249, 281
  - `invoke` [src/battle/legacy_battle_growth_item_result_selection.cpp](../../../src/battle/legacy_battle_growth_item_result_selection.cpp)：71, 100, 126, 265
  - `register` [src/battle/legacy_battle_growth_item_result_selection.cpp](../../../src/battle/legacy_battle_growth_item_result_selection.cpp)：42, 43, 91, 92, 93, 102, 103, 104, 144, 145, 146, 286, 287, 288

- [ ] **CHAIN-1cb5d0ed9023** `battle/legacy_battle_guard_panel_frame`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_guard_panel_frame.hpp](../../../include/openswd3/battle/legacy_battle_guard_panel_frame.hpp)；[src/battle/legacy_battle_guard_panel_frame.cpp](../../../src/battle/legacy_battle_guard_panel_frame.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_guard_panel_frame.hpp](../../../include/openswd3/battle/legacy_battle_guard_panel_frame.hpp)：67, 68, 69, 70, 71, 72, 73
  - `register` [include/openswd3/battle/legacy_battle_guard_panel_frame.hpp](../../../include/openswd3/battle/legacy_battle_guard_panel_frame.hpp)：33, 34, 35, 64, 65, 66
  - `arguments` [src/battle/legacy_battle_guard_panel_frame.cpp](../../../src/battle/legacy_battle_guard_panel_frame.cpp)：169, 190
  - `counter` [src/battle/legacy_battle_guard_panel_frame.cpp](../../../src/battle/legacy_battle_guard_panel_frame.cpp)：105, 174, 196, 204, 222, 294, 328
  - `invoke` [src/battle/legacy_battle_guard_panel_frame.cpp](../../../src/battle/legacy_battle_guard_panel_frame.cpp)：89, 98, 114, 132, 145, 157, 158, 165, 166, 177, 187, 244
  - `register` [src/battle/legacy_battle_guard_panel_frame.cpp](../../../src/battle/legacy_battle_guard_panel_frame.cpp)：70, 71, 101, 102, 103, 111, 117, 151, 152, 153, 159, 160, 161, 170, 171, 172, 191, 192, 193, 223, 224, 225, 296, 297, 298, 329, 330, 331

- [ ] **CHAIN-0c077a92aac6** `battle/legacy_battle_hud_frame`
  - 当前残留信号：arguments, callee, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_hud_frame.hpp](../../../include/openswd3/battle/legacy_battle_hud_frame.hpp)；[src/battle/legacy_battle_hud_frame.cpp](../../../src/battle/legacy_battle_hud_frame.cpp)。
  - 类型：[PORT-37047085d1fd](invoke-removal-ports.md#port-37047085d1fd)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_hud_frame.hpp](../../../include/openswd3/battle/legacy_battle_hud_frame.hpp)：20, 61, 62, 63
  - `callee` [include/openswd3/battle/legacy_battle_hud_frame.hpp](../../../include/openswd3/battle/legacy_battle_hud_frame.hpp)：19, 44, 49, 52, 55, 59
  - `counter` [include/openswd3/battle/legacy_battle_hud_frame.hpp](../../../include/openswd3/battle/legacy_battle_hud_frame.hpp)：142
  - `invoke` [include/openswd3/battle/legacy_battle_hud_frame.hpp](../../../include/openswd3/battle/legacy_battle_hud_frame.hpp)：37, 41, 65
  - `register` [include/openswd3/battle/legacy_battle_hud_frame.hpp](../../../include/openswd3/battle/legacy_battle_hud_frame.hpp)：24, 25, 26, 67, 68, 69
  - `arguments` [src/battle/legacy_battle_hud_frame.cpp](../../../src/battle/legacy_battle_hud_frame.cpp)：149, 153
  - `callee` [src/battle/legacy_battle_hud_frame.cpp](../../../src/battle/legacy_battle_hud_frame.cpp)：148, 151
  - `counter` [src/battle/legacy_battle_hud_frame.cpp](../../../src/battle/legacy_battle_hud_frame.cpp)：225, 710
  - `invoke` [src/battle/legacy_battle_hud_frame.cpp](../../../src/battle/legacy_battle_hud_frame.cpp)：155, 194, 195, 206, 226, 231, 245, 246, 264, 290, 305, 338, 346, 353, 354, 366, 381, 394, 395, 400, 414, 422, 446, 447, 453, 472, 486, 494, 520, 530, 551, 565, 573, 596, 606, 627, 641, 649, 672
  - `register` [src/battle/legacy_battle_hud_frame.cpp](../../../src/battle/legacy_battle_hud_frame.cpp)：195, 206, 219, 220, 221, 245, 290, 339, 353, 394, 446, 704, 705, 706, 711

- [ ] **CHAIN-0c8411564b16** `battle/legacy_battle_input_dispatch`
  - 当前残留信号：arguments, counter, interface, invoke, operation, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_input_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_input_dispatch.hpp)；[src/battle/legacy_battle_input_dispatch.cpp](../../../src/battle/legacy_battle_input_dispatch.cpp)。
  - 类型：[PORT-61523c9cd1bf](invoke-removal-ports.md#port-61523c9cd1bf), [PORT-390b7e9ca4de](invoke-removal-ports.md#port-390b7e9ca4de), [PORT-e4c82312d72e](invoke-removal-ports.md#port-e4c82312d72e)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_input_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_input_dispatch.hpp)：138
  - `counter` [include/openswd3/battle/legacy_battle_input_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_input_dispatch.hpp)：263, 267, 269, 270, 271, 274, 275, 276, 277, 278, 279, 280, 281, 282, 283, 284
  - `invoke` [include/openswd3/battle/legacy_battle_input_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_input_dispatch.hpp)：162
  - `register` [include/openswd3/battle/legacy_battle_input_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_input_dispatch.hpp)：139, 140, 141, 145, 146, 147, 181, 182, 183, 187, 223, 224, 225, 257, 258, 259
  - `arguments` [src/battle/legacy_battle_input_dispatch.cpp](../../../src/battle/legacy_battle_input_dispatch.cpp)：41, 266, 269
  - `counter` [src/battle/legacy_battle_input_dispatch.cpp](../../../src/battle/legacy_battle_input_dispatch.cpp)：343, 384, 413, 436, 462, 487, 533, 534, 535, 536, 563, 586, 609, 633, 656, 893, 907, 923, 948
  - `invoke` [src/battle/legacy_battle_input_dispatch.cpp](../../../src/battle/legacy_battle_input_dispatch.cpp)：34, 37, 267, 820, 1057, 1220, 1223, 1270, 1276, 1309, 1315, 1343
  - `operation` [src/battle/legacy_battle_input_dispatch.cpp](../../../src/battle/legacy_battle_input_dispatch.cpp)：265, 268, 1325, 1343
  - `register` [src/battle/legacy_battle_input_dispatch.cpp](../../../src/battle/legacy_battle_input_dispatch.cpp)：42, 43, 44, 46, 252, 253, 254, 256, 257, 258, 270, 271, 272, 274, 275, 276, 281, 282, 283, 315, 316, 337, 338, 339, 349, 350, 351, 378, 379, 380, 390, 391, 392, 411, 414, 415, 416, 434, 437, 438, 439, 460, 463, 464, 465, 485, 488, 489, 490, 531, 537, 538, 539, 558, 559, 560, 561, 564, 565, 566, 584, 587, 588, 589, 607, 610, 611, 612, 631, 634, 635, 636, 654, 657, 658, 659, 721, 726, 727, 729, 770, 773, 786, 787, 808, 811, 812, 841, 844, 845, 856, 857, 861, 864, 866, 869, 870, 873, 874, 879, 888, 889, 890, 900, 901, 902, 903, 919, 925, 926, 927, 934, 938, 939, 940, 942, 943, 944, 959, 960, 969, 977, 992, 1006, 1007, 1033, 1034, 1037, 1048, 1049, 1050, 1054, 1081, 1092, 1103, 1114, 1116, 1117, 1118, 1119, 1122, 1125, 1138, 1139, 1140, 1141, 1143, 1167, 1168, 1169, 1182, 1213, 1214, 1215, 1217, 1252, 1253, 1254, 1256, 1260, 1261, 1291, 1292, 1293, 1295, 1298, 1299, 1340, 1341, 1342, 1353, 1354, 1355, 1357

- [ ] **CHAIN-643d307755d3** `battle/legacy_battle_input_record_priming`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_input_record_priming.hpp](../../../include/openswd3/battle/legacy_battle_input_record_priming.hpp)；[src/battle/legacy_battle_input_record_priming.cpp](../../../src/battle/legacy_battle_input_record_priming.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_input_record_priming.hpp](../../../include/openswd3/battle/legacy_battle_input_record_priming.hpp)：19, 20, 21, 29
  - `register` [src/battle/legacy_battle_input_record_priming.cpp](../../../src/battle/legacy_battle_input_record_priming.cpp)：7, 10

- [ ] **CHAIN-90f0ada85eae** `battle/legacy_battle_intensity_effect_frame`
  - 当前残留信号：arguments, callee, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[src/battle/legacy_battle_intensity_effect_frame.cpp](../../../src/battle/legacy_battle_intensity_effect_frame.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [src/battle/legacy_battle_intensity_effect_frame.cpp](../../../src/battle/legacy_battle_intensity_effect_frame.cpp)：91, 95
  - `callee` [src/battle/legacy_battle_intensity_effect_frame.cpp](../../../src/battle/legacy_battle_intensity_effect_frame.cpp)：90, 93
  - `counter` [src/battle/legacy_battle_intensity_effect_frame.cpp](../../../src/battle/legacy_battle_intensity_effect_frame.cpp)：117
  - `invoke` [src/battle/legacy_battle_intensity_effect_frame.cpp](../../../src/battle/legacy_battle_intensity_effect_frame.cpp)：97, 137, 149, 167
  - `register` [src/battle/legacy_battle_intensity_effect_frame.cpp](../../../src/battle/legacy_battle_intensity_effect_frame.cpp)：112, 113, 128, 138, 139, 140, 144, 146, 150, 153, 176

- [ ] **CHAIN-fa05ef690335** `battle/legacy_battle_level_advancement`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_level_advancement.hpp](../../../include/openswd3/battle/legacy_battle_level_advancement.hpp)；[src/battle/legacy_battle_level_advancement.cpp](../../../src/battle/legacy_battle_level_advancement.cpp)。
  - 类型：[PORT-9bdb1bd31ce7](invoke-removal-ports.md#port-9bdb1bd31ce7), [PORT-587465dbdb06](invoke-removal-ports.md#port-587465dbdb06)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_level_advancement.hpp](../../../include/openswd3/battle/legacy_battle_level_advancement.hpp)：66
  - `counter` [include/openswd3/battle/legacy_battle_level_advancement.hpp](../../../include/openswd3/battle/legacy_battle_level_advancement.hpp)：179, 180, 181, 182
  - `invoke` [include/openswd3/battle/legacy_battle_level_advancement.hpp](../../../include/openswd3/battle/legacy_battle_level_advancement.hpp)：97
  - `register` [include/openswd3/battle/legacy_battle_level_advancement.hpp](../../../include/openswd3/battle/legacy_battle_level_advancement.hpp)：67, 68, 69, 73, 74, 75, 85, 86, 87, 101, 102, 103, 109, 111, 116, 117, 118, 122, 138, 139, 140, 176, 177, 178
  - `arguments` [src/battle/legacy_battle_level_advancement.cpp](../../../src/battle/legacy_battle_level_advancement.cpp)：85, 89
  - `counter` [src/battle/legacy_battle_level_advancement.cpp](../../../src/battle/legacy_battle_level_advancement.cpp)：166, 230, 272, 317, 330
  - `invoke` [src/battle/legacy_battle_level_advancement.cpp](../../../src/battle/legacy_battle_level_advancement.cpp)：83, 87
  - `register` [src/battle/legacy_battle_level_advancement.cpp](../../../src/battle/legacy_battle_level_advancement.cpp)：52, 53, 90, 91, 92, 95, 96, 97, 160, 161, 162, 167, 168, 169, 218, 219, 220, 231, 232, 233, 260, 261, 262, 273, 274, 275, 318, 319, 320, 331, 332, 333, 372, 373, 374

- [ ] **CHAIN-2e37951a2779** `battle/legacy_battle_level_growth_panel`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_level_growth_panel.hpp](../../../include/openswd3/battle/legacy_battle_level_growth_panel.hpp)；[src/battle/legacy_battle_level_growth_panel.cpp](../../../src/battle/legacy_battle_level_growth_panel.cpp)。
  - 类型：[PORT-4da57ddbeb5f](invoke-removal-ports.md#port-4da57ddbeb5f)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_level_growth_panel.hpp](../../../include/openswd3/battle/legacy_battle_level_growth_panel.hpp)：45
  - `counter` [include/openswd3/battle/legacy_battle_level_growth_panel.hpp](../../../include/openswd3/battle/legacy_battle_level_growth_panel.hpp)：134, 135, 136, 137, 138, 141
  - `invoke` [include/openswd3/battle/legacy_battle_level_growth_panel.hpp](../../../include/openswd3/battle/legacy_battle_level_growth_panel.hpp)：77
  - `register` [include/openswd3/battle/legacy_battle_level_growth_panel.hpp](../../../include/openswd3/battle/legacy_battle_level_growth_panel.hpp)：46, 47, 48, 54, 55, 56, 67, 68, 69, 81, 82, 83, 86, 96, 97, 98, 102, 107, 108, 109, 131, 132, 133
  - `arguments` [src/battle/legacy_battle_level_growth_panel.cpp](../../../src/battle/legacy_battle_level_growth_panel.cpp)：152, 157
  - `counter` [src/battle/legacy_battle_level_growth_panel.cpp](../../../src/battle/legacy_battle_level_growth_panel.cpp)：205, 232, 273, 295, 311, 418, 453, 577, 610
  - `invoke` [src/battle/legacy_battle_level_growth_panel.cpp](../../../src/battle/legacy_battle_level_growth_panel.cpp)：150, 164, 262, 409, 439, 565
  - `register` [src/battle/legacy_battle_level_growth_panel.cpp](../../../src/battle/legacy_battle_level_growth_panel.cpp)：123, 124, 158, 159, 160, 166, 167, 168, 206, 207, 208, 233, 234, 235, 296, 297, 298, 312, 313, 314, 611, 612, 613, 697, 698, 699

- [ ] **CHAIN-73f0a211d567** `battle/legacy_battle_level_profile`
  - 当前残留信号：counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_level_profile.hpp](../../../include/openswd3/battle/legacy_battle_level_profile.hpp)；[src/battle/legacy_battle_level_profile.cpp](../../../src/battle/legacy_battle_level_profile.cpp)。
  - 类型：[PORT-f7ecb1eddb59](invoke-removal-ports.md#port-f7ecb1eddb59)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_level_profile.hpp](../../../include/openswd3/battle/legacy_battle_level_profile.hpp)：158, 159, 160, 161, 162, 163, 164, 165
  - `invoke` [include/openswd3/battle/legacy_battle_level_profile.hpp](../../../include/openswd3/battle/legacy_battle_level_profile.hpp)：70
  - `register` [include/openswd3/battle/legacy_battle_level_profile.hpp](../../../include/openswd3/battle/legacy_battle_level_profile.hpp)：41, 42, 43, 47, 48, 49, 72, 73, 74, 82, 107, 108, 109, 172, 173, 174
  - `counter` [src/battle/legacy_battle_level_profile.cpp](../../../src/battle/legacy_battle_level_profile.cpp)：328, 382, 436, 553, 590, 606, 623, 635, 659, 672, 711
  - `invoke` [src/battle/legacy_battle_level_profile.cpp](../../../src/battle/legacy_battle_level_profile.cpp)：317, 375, 537, 577, 594, 610, 625, 647, 662, 701
  - `register` [src/battle/legacy_battle_level_profile.cpp](../../../src/battle/legacy_battle_level_profile.cpp)：60, 61, 62, 67, 68, 103, 107, 111, 324, 325, 326, 329, 330, 331, 378, 379, 380, 385, 532, 533, 534, 547, 548, 549, 554, 556, 557, 558, 561, 562, 563, 569, 584, 585, 586, 600, 601, 602, 617, 618, 619, 629, 630, 631, 636, 640, 641, 642, 653, 654, 655, 666, 667, 668, 673, 674, 675, 688, 689, 690, 694, 695, 696, 705, 706, 707, 712, 713, 714

- [ ] **CHAIN-11ed14ba0a87** `battle/legacy_battle_level_requirement`
  - 当前残留信号：counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_level_requirement.hpp](../../../include/openswd3/battle/legacy_battle_level_requirement.hpp)；[src/battle/legacy_battle_level_requirement.cpp](../../../src/battle/legacy_battle_level_requirement.cpp)。
  - 类型：[PORT-c7528372200b](invoke-removal-ports.md#port-c7528372200b)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_level_requirement.hpp](../../../include/openswd3/battle/legacy_battle_level_requirement.hpp)：121, 122, 123, 124, 125
  - `invoke` [include/openswd3/battle/legacy_battle_level_requirement.hpp](../../../include/openswd3/battle/legacy_battle_level_requirement.hpp)：66
  - `register` [include/openswd3/battle/legacy_battle_level_requirement.hpp](../../../include/openswd3/battle/legacy_battle_level_requirement.hpp)：46, 47, 48, 52, 53, 54, 83, 84, 85, 129, 130, 131
  - `counter` [src/battle/legacy_battle_level_requirement.cpp](../../../src/battle/legacy_battle_level_requirement.cpp)：215, 252, 268, 285, 297, 321, 334, 369
  - `invoke` [src/battle/legacy_battle_level_requirement.cpp](../../../src/battle/legacy_battle_level_requirement.cpp)：173, 199, 239, 256, 272, 287, 309, 324, 359
  - `register` [src/battle/legacy_battle_level_requirement.cpp](../../../src/battle/legacy_battle_level_requirement.cpp)：45, 50, 83, 87, 91, 179, 181, 194, 195, 196, 209, 210, 211, 216, 218, 219, 220, 223, 224, 225, 231, 246, 247, 248, 262, 263, 264, 279, 280, 281, 291, 292, 293, 298, 302, 303, 304, 315, 316, 317, 328, 329, 330, 335, 336, 337, 347, 352, 353, 354, 363, 364, 365, 371, 372, 373

- [ ] **CHAIN-069c1ce92b5d** `battle/legacy_battle_level_up_panel`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_level_up_panel.hpp](../../../include/openswd3/battle/legacy_battle_level_up_panel.hpp)；[src/battle/legacy_battle_level_up_panel.cpp](../../../src/battle/legacy_battle_level_up_panel.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_level_up_panel.hpp](../../../include/openswd3/battle/legacy_battle_level_up_panel.hpp)：72, 73, 74, 75
  - `register` [include/openswd3/battle/legacy_battle_level_up_panel.hpp](../../../include/openswd3/battle/legacy_battle_level_up_panel.hpp)：45, 46, 47, 69, 70, 71
  - `arguments` [src/battle/legacy_battle_level_up_panel.cpp](../../../src/battle/legacy_battle_level_up_panel.cpp)：63, 69
  - `counter` [src/battle/legacy_battle_level_up_panel.cpp](../../../src/battle/legacy_battle_level_up_panel.cpp)：114, 142, 168, 189, 205, 291
  - `invoke` [src/battle/legacy_battle_level_up_panel.cpp](../../../src/battle/legacy_battle_level_up_panel.cpp)：60, 76, 155, 254, 278
  - `register` [src/battle/legacy_battle_level_up_panel.cpp](../../../src/battle/legacy_battle_level_up_panel.cpp)：35, 36, 70, 71, 72, 78, 79, 80, 115, 116, 117, 143, 144, 145, 190, 191, 192, 206, 207, 208, 295, 296, 297

- [ ] **CHAIN-2ef6f1108945** `battle/legacy_battle_list_contents`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_list_contents.hpp](../../../include/openswd3/battle/legacy_battle_list_contents.hpp)；[src/battle/legacy_battle_list_contents.cpp](../../../src/battle/legacy_battle_list_contents.cpp)。
  - 类型：[PORT-5273e66ab0e8](invoke-removal-ports.md#port-5273e66ab0e8)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_list_contents.hpp](../../../include/openswd3/battle/legacy_battle_list_contents.hpp)：28
  - `counter` [include/openswd3/battle/legacy_battle_list_contents.hpp](../../../include/openswd3/battle/legacy_battle_list_contents.hpp)：147, 148, 149, 150, 151, 152, 153, 154, 155
  - `invoke` [include/openswd3/battle/legacy_battle_list_contents.hpp](../../../include/openswd3/battle/legacy_battle_list_contents.hpp)：56
  - `register` [include/openswd3/battle/legacy_battle_list_contents.hpp](../../../include/openswd3/battle/legacy_battle_list_contents.hpp)：29, 30, 31, 38, 39, 40, 91, 92, 93, 101, 102, 103, 144, 145, 146
  - `arguments` [src/battle/legacy_battle_list_contents.cpp](../../../src/battle/legacy_battle_list_contents.cpp)：122, 129, 136, 156, 193, 237, 255, 492, 518, 555
  - `counter` [src/battle/legacy_battle_list_contents.cpp](../../../src/battle/legacy_battle_list_contents.cpp)：162, 177, 205, 224, 266, 293, 410, 434, 469, 479, 563
  - `invoke` [src/battle/legacy_battle_list_contents.cpp](../../../src/battle/legacy_battle_list_contents.cpp)：120, 127, 134, 153, 170, 190, 217, 234, 252, 325, 357, 385, 487, 490, 516, 528, 529, 545, 553
  - `register` [src/battle/legacy_battle_list_contents.cpp](../../../src/battle/legacy_battle_list_contents.cpp)：106, 107, 123, 124, 125, 130, 131, 132, 137, 138, 139, 158, 159, 160, 173, 174, 175, 181, 201, 202, 203, 209, 220, 221, 222, 242, 243, 244, 260, 261, 262, 274, 298, 299, 300, 308, 309, 344, 345, 367, 368, 493, 494, 495, 513, 519, 520, 521, 530, 531, 532, 556, 557, 558, 567, 568, 569

- [ ] **CHAIN-d5bd6a91f7f3** `battle/legacy_battle_list_frame`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_list_frame.hpp](../../../include/openswd3/battle/legacy_battle_list_frame.hpp)；[src/battle/legacy_battle_list_frame.cpp](../../../src/battle/legacy_battle_list_frame.cpp)。
  - 类型：[PORT-7fd8a5281cc0](invoke-removal-ports.md#port-7fd8a5281cc0)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_list_frame.hpp](../../../include/openswd3/battle/legacy_battle_list_frame.hpp)：21
  - `counter` [include/openswd3/battle/legacy_battle_list_frame.hpp](../../../include/openswd3/battle/legacy_battle_list_frame.hpp)：86, 87, 88, 89, 90
  - `invoke` [include/openswd3/battle/legacy_battle_list_frame.hpp](../../../include/openswd3/battle/legacy_battle_list_frame.hpp)：39
  - `register` [include/openswd3/battle/legacy_battle_list_frame.hpp](../../../include/openswd3/battle/legacy_battle_list_frame.hpp)：22, 23, 24, 28, 29, 30, 56, 57, 58, 64, 65, 66, 83, 84, 85
  - `arguments` [src/battle/legacy_battle_list_frame.cpp](../../../src/battle/legacy_battle_list_frame.cpp)：113, 124
  - `counter` [src/battle/legacy_battle_list_frame.cpp](../../../src/battle/legacy_battle_list_frame.cpp)：64, 98, 118, 129, 142, 159, 189
  - `invoke` [src/battle/legacy_battle_list_frame.cpp](../../../src/battle/legacy_battle_list_frame.cpp)：111, 122
  - `register` [src/battle/legacy_battle_list_frame.cpp](../../../src/battle/legacy_battle_list_frame.cpp)：39, 40, 41, 65, 66, 67, 72, 73, 74, 99, 100, 101, 105, 106, 107, 114, 115, 116, 119, 120, 121, 125, 126, 127, 130, 131, 132, 160, 161, 162, 165, 166, 167, 171, 178, 190, 191, 192, 196, 197, 198, 202, 203, 204, 207, 208, 209

- [ ] **CHAIN-e0aeaae7830b** `battle/legacy_battle_menu_context_advance`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_menu_context_advance.hpp](../../../include/openswd3/battle/legacy_battle_menu_context_advance.hpp)；[src/battle/legacy_battle_menu_context_advance.cpp](../../../src/battle/legacy_battle_menu_context_advance.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_menu_context_advance.hpp](../../../include/openswd3/battle/legacy_battle_menu_context_advance.hpp)：39
  - `register` [include/openswd3/battle/legacy_battle_menu_context_advance.hpp](../../../include/openswd3/battle/legacy_battle_menu_context_advance.hpp)：17, 18, 19, 33, 34, 35
  - `counter` [src/battle/legacy_battle_menu_context_advance.cpp](../../../src/battle/legacy_battle_menu_context_advance.cpp)：63
  - `register` [src/battle/legacy_battle_menu_context_advance.cpp](../../../src/battle/legacy_battle_menu_context_advance.cpp)：45, 46, 47, 50, 51, 52, 61, 64, 65, 66, 71, 72, 73, 74, 75, 76, 77, 78, 81, 86, 88, 90, 91, 93, 98, 99, 100, 103, 110, 111, 112, 113, 114, 116, 117, 123, 125, 131, 133, 134, 139, 140, 141, 144

- [ ] **CHAIN-fd6faa2ccab3** `battle/legacy_battle_menu_context_retreat`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_menu_context_retreat.hpp](../../../include/openswd3/battle/legacy_battle_menu_context_retreat.hpp)；[src/battle/legacy_battle_menu_context_retreat.cpp](../../../src/battle/legacy_battle_menu_context_retreat.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_menu_context_retreat.hpp](../../../include/openswd3/battle/legacy_battle_menu_context_retreat.hpp)：39
  - `register` [include/openswd3/battle/legacy_battle_menu_context_retreat.hpp](../../../include/openswd3/battle/legacy_battle_menu_context_retreat.hpp)：17, 18, 19, 33, 34, 35
  - `counter` [src/battle/legacy_battle_menu_context_retreat.cpp](../../../src/battle/legacy_battle_menu_context_retreat.cpp)：63
  - `register` [src/battle/legacy_battle_menu_context_retreat.cpp](../../../src/battle/legacy_battle_menu_context_retreat.cpp)：45, 46, 47, 50, 51, 52, 61, 64, 65, 66, 71, 72, 73, 74, 75, 76, 79, 84, 86, 88, 89, 91, 95, 96, 97, 98, 99, 100, 101, 103, 109, 110, 111, 112, 113, 115, 116, 122, 124, 130, 132, 133, 138, 139, 140, 143

- [ ] **CHAIN-2b6b9ac32978** `battle/legacy_battle_menu_input_finalize`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_menu_input_finalize.hpp](../../../include/openswd3/battle/legacy_battle_menu_input_finalize.hpp)；[src/battle/legacy_battle_menu_input_finalize.cpp](../../../src/battle/legacy_battle_menu_input_finalize.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_menu_input_finalize.hpp](../../../include/openswd3/battle/legacy_battle_menu_input_finalize.hpp)：42, 43
  - `register` [include/openswd3/battle/legacy_battle_menu_input_finalize.hpp](../../../include/openswd3/battle/legacy_battle_menu_input_finalize.hpp)：19, 20, 21, 38, 39, 40
  - `arguments` [src/battle/legacy_battle_menu_input_finalize.cpp](../../../src/battle/legacy_battle_menu_input_finalize.cpp)：56
  - `counter` [src/battle/legacy_battle_menu_input_finalize.cpp](../../../src/battle/legacy_battle_menu_input_finalize.cpp)：92, 162, 243, 251
  - `invoke` [src/battle/legacy_battle_menu_input_finalize.cpp](../../../src/battle/legacy_battle_menu_input_finalize.cpp)：54, 161, 240, 250
  - `register` [src/battle/legacy_battle_menu_input_finalize.cpp](../../../src/battle/legacy_battle_menu_input_finalize.cpp)：43, 44, 45, 48, 49, 50, 57, 58, 59, 61, 62, 63, 93, 94, 95, 107, 109, 111, 122, 123, 124, 149, 153, 154, 163, 165, 167, 171, 175, 176, 178, 180, 182, 187, 191, 192, 193, 194, 195, 196, 197, 198, 199, 201, 214, 222, 229, 234, 244, 249, 260, 262, 265, 272, 284, 285, 289, 293, 294, 301, 315, 342, 343, 347, 348, 349, 351, 359, 360, 361, 362, 363, 364, 365, 367, 369, 371, 375, 383, 384, 385, 386, 388

- [ ] **CHAIN-0da52a9f3292** `battle/legacy_battle_menu_page_advance`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_menu_page_advance.hpp](../../../include/openswd3/battle/legacy_battle_menu_page_advance.hpp)；[src/battle/legacy_battle_menu_page_advance.cpp](../../../src/battle/legacy_battle_menu_page_advance.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_menu_page_advance.hpp](../../../include/openswd3/battle/legacy_battle_menu_page_advance.hpp)：34
  - `register` [include/openswd3/battle/legacy_battle_menu_page_advance.hpp](../../../include/openswd3/battle/legacy_battle_menu_page_advance.hpp)：16, 17, 18, 31, 32, 33
  - `counter` [src/battle/legacy_battle_menu_page_advance.cpp](../../../src/battle/legacy_battle_menu_page_advance.cpp)：47
  - `register` [src/battle/legacy_battle_menu_page_advance.cpp](../../../src/battle/legacy_battle_menu_page_advance.cpp)：32, 33, 34, 37, 38, 39, 43, 45, 48, 49, 50, 53, 56, 57, 62, 67, 68, 69, 71, 72, 73, 75, 85, 87, 89, 93, 94, 99, 100, 101, 102, 103, 104, 105, 107, 109, 115, 116, 118, 119, 120, 127, 128, 129, 130, 131, 132, 133, 135, 136, 137, 138, 139, 141, 143, 153, 154, 159, 160, 167, 168, 169, 170, 171, 172, 173, 174, 176, 178

- [ ] **CHAIN-917eea2c08d9** `battle/legacy_battle_menu_page_retreat`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_menu_page_retreat.hpp](../../../include/openswd3/battle/legacy_battle_menu_page_retreat.hpp)；[src/battle/legacy_battle_menu_page_retreat.cpp](../../../src/battle/legacy_battle_menu_page_retreat.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_menu_page_retreat.hpp](../../../include/openswd3/battle/legacy_battle_menu_page_retreat.hpp)：34
  - `register` [include/openswd3/battle/legacy_battle_menu_page_retreat.hpp](../../../include/openswd3/battle/legacy_battle_menu_page_retreat.hpp)：16, 17, 18, 31, 32, 33
  - `counter` [src/battle/legacy_battle_menu_page_retreat.cpp](../../../src/battle/legacy_battle_menu_page_retreat.cpp)：46
  - `register` [src/battle/legacy_battle_menu_page_retreat.cpp](../../../src/battle/legacy_battle_menu_page_retreat.cpp)：27, 28, 29, 32, 33, 34, 42, 44, 47, 48, 49, 52, 54, 56, 57, 58, 59, 62, 63, 64, 67, 71, 72, 74, 75, 76, 77, 78, 79, 82, 83, 84, 87, 88, 89, 90, 96, 97, 102, 106, 107, 112, 113, 114, 115, 118, 119, 120, 123

- [ ] **CHAIN-74678d5d70b2** `battle/legacy_battle_menu_selection_advance`
  - 当前残留信号：arguments, counter, interface, invoke, operation, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_menu_selection_advance.hpp](../../../include/openswd3/battle/legacy_battle_menu_selection_advance.hpp)；[src/battle/legacy_battle_menu_selection_advance.cpp](../../../src/battle/legacy_battle_menu_selection_advance.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_menu_selection_advance.hpp](../../../include/openswd3/battle/legacy_battle_menu_selection_advance.hpp)：49
  - `register` [include/openswd3/battle/legacy_battle_menu_selection_advance.hpp](../../../include/openswd3/battle/legacy_battle_menu_selection_advance.hpp)：22, 23, 24, 46, 47, 48
  - `arguments` [src/battle/legacy_battle_menu_selection_advance.cpp](../../../src/battle/legacy_battle_menu_selection_advance.cpp)：79, 83
  - `counter` [src/battle/legacy_battle_menu_selection_advance.cpp](../../../src/battle/legacy_battle_menu_selection_advance.cpp)：98
  - `invoke` [src/battle/legacy_battle_menu_selection_advance.cpp](../../../src/battle/legacy_battle_menu_selection_advance.cpp)：81
  - `operation` [src/battle/legacy_battle_menu_selection_advance.cpp](../../../src/battle/legacy_battle_menu_selection_advance.cpp)：77, 82
  - `register` [src/battle/legacy_battle_menu_selection_advance.cpp](../../../src/battle/legacy_battle_menu_selection_advance.cpp)：63, 64, 65, 68, 69, 70, 80, 84, 85, 86, 88, 89, 90, 94, 96, 99, 100, 101, 112, 113, 114, 115, 118, 119, 123, 126, 127, 128, 129, 132, 133, 134, 139, 140, 141, 144, 160, 161, 162, 173, 181, 185, 186, 191, 192, 194, 199, 200, 201, 202, 204, 205, 206, 208, 214, 220, 224, 225, 226, 227, 229, 232, 233, 234, 235, 236, 242, 245, 246, 253, 254, 267, 268, 269, 270, 271, 272, 274, 280, 291, 293, 299, 302, 304, 305, 315, 318, 326, 331, 332, 336, 337, 338, 339, 340, 342, 343, 344, 345, 346, 347, 350, 356, 380, 382, 387, 388, 389, 392, 393, 394, 396, 397, 398, 399, 400, 405, 408, 410, 411, 414, 415, 416, 417, 418, 419, 420, 422, 423, 424, 426, 432, 439, 444, 446, 454, 485, 493, 494, 495, 496, 497, 498, 499, 501, 502, 503, 508, 509, 510, 515, 516, 517, 518, 524, 525, 531, 537, 538, 539, 547, 548, 549, 550, 555, 559, 560, 563, 564, 565, 571, 572, 573, 574, 575, 576, 577, 579, 580, 581, 587, 588, 589, 596, 597, 598

- [ ] **CHAIN-064b88a5f449** `battle/legacy_battle_menu_selection_retreat`
  - 当前残留信号：arguments, counter, interface, invoke, operation, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_menu_selection_retreat.hpp](../../../include/openswd3/battle/legacy_battle_menu_selection_retreat.hpp)；[src/battle/legacy_battle_menu_selection_retreat.cpp](../../../src/battle/legacy_battle_menu_selection_retreat.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_menu_selection_retreat.hpp](../../../include/openswd3/battle/legacy_battle_menu_selection_retreat.hpp)：49
  - `register` [include/openswd3/battle/legacy_battle_menu_selection_retreat.hpp](../../../include/openswd3/battle/legacy_battle_menu_selection_retreat.hpp)：22, 23, 24, 46, 47, 48
  - `arguments` [src/battle/legacy_battle_menu_selection_retreat.cpp](../../../src/battle/legacy_battle_menu_selection_retreat.cpp)：79, 83
  - `counter` [src/battle/legacy_battle_menu_selection_retreat.cpp](../../../src/battle/legacy_battle_menu_selection_retreat.cpp)：98
  - `invoke` [src/battle/legacy_battle_menu_selection_retreat.cpp](../../../src/battle/legacy_battle_menu_selection_retreat.cpp)：81
  - `operation` [src/battle/legacy_battle_menu_selection_retreat.cpp](../../../src/battle/legacy_battle_menu_selection_retreat.cpp)：77, 82
  - `register` [src/battle/legacy_battle_menu_selection_retreat.cpp](../../../src/battle/legacy_battle_menu_selection_retreat.cpp)：63, 64, 65, 68, 69, 70, 80, 84, 85, 86, 88, 89, 90, 94, 96, 99, 100, 101, 112, 113, 114, 115, 118, 119, 123, 126, 127, 128, 129, 132, 133, 134, 139, 140, 141, 144, 160, 161, 162, 173, 181, 185, 186, 191, 192, 194, 199, 200, 202, 204, 205, 206, 210, 216, 222, 226, 227, 228, 232, 233, 236, 237, 238, 247, 248, 261, 262, 263, 264, 265, 267, 273, 284, 286, 292, 295, 297, 298, 308, 311, 319, 324, 325, 329, 330, 331, 332, 333, 335, 336, 337, 338, 339, 340, 343, 349, 373, 375, 380, 381, 382, 383, 384, 387, 390, 391, 392, 397, 400, 402, 403, 406, 407, 408, 409, 410, 411, 413, 414, 415, 417, 423, 430, 435, 437, 445, 476, 484, 485, 486, 488, 489, 490, 495, 496, 497, 499, 505, 506, 512, 517, 518, 519, 527, 528, 529, 530, 531, 535, 539, 540, 541, 542, 545, 550, 551, 552, 556, 557, 558, 566, 567, 568

- [ ] **CHAIN-29fa07afafff** `battle/legacy_battle_message_phase`
  - 当前残留信号：arguments, callee, counter, interface, invoke, operation, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_message_phase.hpp](../../../include/openswd3/battle/legacy_battle_message_phase.hpp)；[src/battle/legacy_battle_message_phase.cpp](../../../src/battle/legacy_battle_message_phase.cpp)。
  - 类型：[PORT-13fadaf7ce03](invoke-removal-ports.md#port-13fadaf7ce03), [PORT-d4160261067a](invoke-removal-ports.md#port-d4160261067a), [PORT-7b41058901bc](invoke-removal-ports.md#port-7b41058901bc)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_message_phase.hpp](../../../include/openswd3/battle/legacy_battle_message_phase.hpp)：105, 180
  - `callee` [include/openswd3/battle/legacy_battle_message_phase.hpp](../../../include/openswd3/battle/legacy_battle_message_phase.hpp)：177
  - `counter` [include/openswd3/battle/legacy_battle_message_phase.hpp](../../../include/openswd3/battle/legacy_battle_message_phase.hpp)：273, 274, 275, 276, 278, 282, 283, 286, 288, 289, 290, 291, 292, 293, 294, 295, 296, 297, 309, 311, 313, 315, 318
  - `invoke` [include/openswd3/battle/legacy_battle_message_phase.hpp](../../../include/openswd3/battle/legacy_battle_message_phase.hpp)：166, 175, 176
  - `register` [include/openswd3/battle/legacy_battle_message_phase.hpp](../../../include/openswd3/battle/legacy_battle_message_phase.hpp)：106, 107, 108, 112, 113, 114, 168, 169, 170, 181, 182, 183, 185, 214, 215, 270, 271, 272
  - `arguments` [src/battle/legacy_battle_message_phase.cpp](../../../src/battle/legacy_battle_message_phase.cpp)：257, 264
  - `counter` [src/battle/legacy_battle_message_phase.cpp](../../../src/battle/legacy_battle_message_phase.cpp)：397, 434, 439, 464, 477, 517, 534, 698, 718, 777, 803, 835, 876, 954, 1013, 1034, 1059, 1118, 1153, 1229, 1265, 1308, 1353
  - `invoke` [src/battle/legacy_battle_message_phase.cpp](../../../src/battle/legacy_battle_message_phase.cpp)：261
  - `operation` [src/battle/legacy_battle_message_phase.cpp](../../../src/battle/legacy_battle_message_phase.cpp)：255, 259, 262
  - `register` [src/battle/legacy_battle_message_phase.cpp](../../../src/battle/legacy_battle_message_phase.cpp)：134, 187, 188, 189, 200, 201, 202, 265, 266, 267, 287, 288, 289, 293, 294, 295, 302, 303, 314, 315, 321, 322, 323, 327, 328, 329, 392, 393, 394, 398, 399, 400, 475, 478, 479, 480, 536, 537, 538, 693, 694, 695, 699, 700, 701, 772, 773, 774, 778, 779, 780, 797, 798, 799, 804, 805, 806, 815, 816, 817, 836, 837, 838, 859, 860, 861, 877, 878, 879, 943, 944, 945, 955, 956, 957, 1001, 1002, 1003, 1014, 1015, 1016, 1035, 1036, 1037, 1054, 1055, 1056, 1060, 1061, 1062, 1106, 1107, 1108, 1119, 1120, 1121, 1137, 1138, 1139, 1154, 1155, 1156, 1211, 1212, 1213, 1230, 1231, 1232, 1247, 1248, 1249, 1266, 1267, 1268, 1290, 1291, 1292, 1309, 1310, 1311, 1336, 1337, 1338, 1354, 1355, 1356

- [ ] **CHAIN-2d45943f1275** `battle/legacy_battle_mode_grid_frame`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_mode_grid_frame.hpp](../../../include/openswd3/battle/legacy_battle_mode_grid_frame.hpp)；[src/battle/legacy_battle_mode_grid_frame.cpp](../../../src/battle/legacy_battle_mode_grid_frame.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_mode_grid_frame.hpp](../../../include/openswd3/battle/legacy_battle_mode_grid_frame.hpp)：85, 86, 87, 88, 89, 90, 91
  - `register` [include/openswd3/battle/legacy_battle_mode_grid_frame.hpp](../../../include/openswd3/battle/legacy_battle_mode_grid_frame.hpp)：44, 45, 46, 82, 83, 84
  - `arguments` [src/battle/legacy_battle_mode_grid_frame.cpp](../../../src/battle/legacy_battle_mode_grid_frame.cpp)：430, 450
  - `counter` [src/battle/legacy_battle_mode_grid_frame.cpp](../../../src/battle/legacy_battle_mode_grid_frame.cpp)：181, 199, 255, 331, 381, 435, 457
  - `invoke` [src/battle/legacy_battle_mode_grid_frame.cpp](../../../src/battle/legacy_battle_mode_grid_frame.cpp)：79, 80, 81, 152, 269, 347, 357, 393, 403, 419, 420, 427, 428, 438, 448
  - `register` [src/battle/legacy_battle_mode_grid_frame.cpp](../../../src/battle/legacy_battle_mode_grid_frame.cpp)：60, 61, 200, 201, 202, 257, 258, 259, 382, 383, 384, 421, 422, 423, 431, 432, 433, 451, 452, 453, 461, 462, 463

- [ ] **CHAIN-c7dfac7d2431** `battle/legacy_battle_mon_definition`
  - 当前残留信号：counter, interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_mon_definition.hpp](../../../include/openswd3/battle/legacy_battle_mon_definition.hpp)；[src/battle/legacy_battle_mon_definition.cpp](../../../src/battle/legacy_battle_mon_definition.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_mon_definition.hpp](../../../include/openswd3/battle/legacy_battle_mon_definition.hpp)：70, 71, 72, 73, 74, 75, 76, 77
  - `counter` [src/battle/legacy_battle_mon_definition.cpp](../../../src/battle/legacy_battle_mon_definition.cpp)：433, 551, 554, 563, 578, 582, 593, 597, 606, 609, 635, 638, 658

- [ ] **CHAIN-3b951da93626** `battle/legacy_battle_mon_definition_text_release`
  - 当前残留信号：counter, interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_mon_definition_text_release.hpp](../../../include/openswd3/battle/legacy_battle_mon_definition_text_release.hpp)；[src/battle/legacy_battle_mon_definition_text_release.cpp](../../../src/battle/legacy_battle_mon_definition_text_release.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_mon_definition_text_release.hpp](../../../include/openswd3/battle/legacy_battle_mon_definition_text_release.hpp)：34
  - `counter` [src/battle/legacy_battle_mon_definition_text_release.cpp](../../../src/battle/legacy_battle_mon_definition_text_release.cpp)：47

- [ ] **CHAIN-34218da1eccf** `battle/legacy_battle_mon_profile`
  - 当前残留信号：counter, interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_mon_profile.hpp](../../../include/openswd3/battle/legacy_battle_mon_profile.hpp)；[src/battle/legacy_battle_mon_profile.cpp](../../../src/battle/legacy_battle_mon_profile.cpp)。
  - 类型：[PORT-4fbbe55430e0](invoke-removal-ports.md#port-4fbbe55430e0)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_mon_profile.hpp](../../../include/openswd3/battle/legacy_battle_mon_profile.hpp)：127, 128, 129, 130, 131
  - `counter` [src/battle/legacy_battle_mon_profile.cpp](../../../src/battle/legacy_battle_mon_profile.cpp)：495, 510, 514, 525, 528, 537, 541, 567, 571, 583

- [ ] **CHAIN-32f8f1beabaf** `battle/legacy_battle_narrow_grid_frame`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_narrow_grid_frame.hpp](../../../include/openswd3/battle/legacy_battle_narrow_grid_frame.hpp)；[src/battle/legacy_battle_narrow_grid_frame.cpp](../../../src/battle/legacy_battle_narrow_grid_frame.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_narrow_grid_frame.hpp](../../../include/openswd3/battle/legacy_battle_narrow_grid_frame.hpp)：80, 81, 82, 83, 84, 85, 86, 87, 88
  - `register` [include/openswd3/battle/legacy_battle_narrow_grid_frame.hpp](../../../include/openswd3/battle/legacy_battle_narrow_grid_frame.hpp)：43, 44, 45, 77, 78, 79
  - `arguments` [src/battle/legacy_battle_narrow_grid_frame.cpp](../../../src/battle/legacy_battle_narrow_grid_frame.cpp)：115, 136, 204, 225
  - `counter` [src/battle/legacy_battle_narrow_grid_frame.cpp](../../../src/battle/legacy_battle_narrow_grid_frame.cpp)：120, 150, 209, 232, 243, 261, 316, 354, 413
  - `invoke` [src/battle/legacy_battle_narrow_grid_frame.cpp](../../../src/battle/legacy_battle_narrow_grid_frame.cpp)：98, 99, 100, 112, 133, 179, 192, 193, 200, 201, 212, 222, 331, 347, 368, 386, 424
  - `register` [src/battle/legacy_battle_narrow_grid_frame.cpp](../../../src/battle/legacy_battle_narrow_grid_frame.cpp)：82, 83, 116, 117, 118, 144, 145, 146, 186, 187, 188, 194, 195, 196, 205, 206, 207, 226, 227, 228, 262, 263, 264, 318, 319, 320, 350, 351, 352, 414, 415, 416

- [ ] **CHAIN-3f5fcc511853** `battle/legacy_battle_object_reset`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_object_reset.hpp](../../../include/openswd3/battle/legacy_battle_object_reset.hpp)；[src/battle/legacy_battle_object_reset.cpp](../../../src/battle/legacy_battle_object_reset.cpp)。
  - 类型：[PORT-70edce35e328](invoke-removal-ports.md#port-70edce35e328)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_object_reset.hpp](../../../include/openswd3/battle/legacy_battle_object_reset.hpp)：42, 43
  - `register` [include/openswd3/battle/legacy_battle_object_reset.hpp](../../../include/openswd3/battle/legacy_battle_object_reset.hpp)：18, 19, 20, 25, 26, 27, 45, 46
  - `counter` [src/battle/legacy_battle_object_reset.cpp](../../../src/battle/legacy_battle_object_reset.cpp)：57, 73
  - `register` [src/battle/legacy_battle_object_reset.cpp](../../../src/battle/legacy_battle_object_reset.cpp)：53, 54, 55, 69, 70, 71, 75, 76, 77

- [ ] **CHAIN-bbc0964ed5ca** `battle/legacy_battle_opponent_action_dispatch`
  - 当前残留信号：arguments, callee, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_opponent_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_opponent_action_dispatch.hpp)；[src/battle/legacy_battle_opponent_action_dispatch.cpp](../../../src/battle/legacy_battle_opponent_action_dispatch.cpp)。
  - 类型：[PORT-055a317c51be](invoke-removal-ports.md#port-055a317c51be)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `invoke` [include/openswd3/battle/legacy_battle_opponent_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_opponent_action_dispatch.hpp)：12
  - `arguments` [src/battle/legacy_battle_opponent_action_dispatch.cpp](../../../src/battle/legacy_battle_opponent_action_dispatch.cpp)：102, 109, 283
  - `callee` [src/battle/legacy_battle_opponent_action_dispatch.cpp](../../../src/battle/legacy_battle_opponent_action_dispatch.cpp)：101, 108, 265, 271, 282
  - `counter` [src/battle/legacy_battle_opponent_action_dispatch.cpp](../../../src/battle/legacy_battle_opponent_action_dispatch.cpp)：192, 397, 432, 519, 653, 881, 1038
  - `invoke` [src/battle/legacy_battle_opponent_action_dispatch.cpp](../../../src/battle/legacy_battle_opponent_action_dispatch.cpp)：98, 107, 140, 143, 261, 281, 336, 348, 434, 548, 578, 583, 712, 821, 826, 831, 841, 848, 896, 977, 1091
  - `register` [src/battle/legacy_battle_opponent_action_dispatch.cpp](../../../src/battle/legacy_battle_opponent_action_dispatch.cpp)：103, 104, 105, 110, 111, 112, 235, 284, 285, 286, 289, 290, 291, 328, 354, 386, 387, 417, 443, 508, 509, 539, 554, 578, 583, 633, 634, 635, 636, 637, 638, 639, 669, 670, 679, 718, 754, 755, 762, 789, 827, 841, 877, 878, 1033, 1034, 1035, 1044, 1047, 1064, 1072, 1097, 1101

- [ ] **CHAIN-e6a9433cccfd** `battle/legacy_battle_outcome_finalization`
  - 当前残留信号：counter, interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_outcome_finalization.hpp](../../../include/openswd3/battle/legacy_battle_outcome_finalization.hpp)；[src/battle/legacy_battle_outcome_finalization.cpp](../../../src/battle/legacy_battle_outcome_finalization.cpp)。
  - 类型：[PORT-8993a2dfa5cd](invoke-removal-ports.md#port-8993a2dfa5cd), [PORT-b64661945895](invoke-removal-ports.md#port-b64661945895)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_outcome_finalization.hpp](../../../include/openswd3/battle/legacy_battle_outcome_finalization.hpp)：65, 66, 67
  - `counter` [src/battle/legacy_battle_outcome_finalization.cpp](../../../src/battle/legacy_battle_outcome_finalization.cpp)：27, 29, 31
  - `register` [src/battle/legacy_battle_outcome_finalization.cpp](../../../src/battle/legacy_battle_outcome_finalization.cpp)：22, 33, 43, 56, 59, 61, 65, 68, 74, 76, 84, 88, 91, 97

- [ ] **CHAIN-787c19720c2e** `battle/legacy_battle_outcome_resolution`
  - 当前残留信号：counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_outcome_resolution.hpp](../../../include/openswd3/battle/legacy_battle_outcome_resolution.hpp)；[src/battle/legacy_battle_outcome_resolution.cpp](../../../src/battle/legacy_battle_outcome_resolution.cpp)。
  - 类型：[PORT-d418321a615f](invoke-removal-ports.md#port-d418321a615f)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_outcome_resolution.hpp](../../../include/openswd3/battle/legacy_battle_outcome_resolution.hpp)：62, 63, 64
  - `invoke` [include/openswd3/battle/legacy_battle_outcome_resolution.hpp](../../../include/openswd3/battle/legacy_battle_outcome_resolution.hpp)：31
  - `register` [include/openswd3/battle/legacy_battle_outcome_resolution.hpp](../../../include/openswd3/battle/legacy_battle_outcome_resolution.hpp)：19, 20, 21
  - `counter` [src/battle/legacy_battle_outcome_resolution.cpp](../../../src/battle/legacy_battle_outcome_resolution.cpp)：24, 65, 69, 120
  - `invoke` [src/battle/legacy_battle_outcome_resolution.cpp](../../../src/battle/legacy_battle_outcome_resolution.cpp)：62
  - `register` [src/battle/legacy_battle_outcome_resolution.cpp](../../../src/battle/legacy_battle_outcome_resolution.cpp)：67

- [ ] **CHAIN-2fd4c6deabf1** `battle/legacy_battle_pair_transition`
  - 当前残留信号：arguments, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_pair_transition.hpp](../../../include/openswd3/battle/legacy_battle_pair_transition.hpp)；[src/battle/legacy_battle_pair_transition.cpp](../../../src/battle/legacy_battle_pair_transition.cpp)。
  - 类型：[PORT-297df14b6866](invoke-removal-ports.md#port-297df14b6866), [PORT-a0ff1255e5e7](invoke-removal-ports.md#port-a0ff1255e5e7)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_pair_transition.hpp](../../../include/openswd3/battle/legacy_battle_pair_transition.hpp)：24
  - `invoke` [include/openswd3/battle/legacy_battle_pair_transition.hpp](../../../include/openswd3/battle/legacy_battle_pair_transition.hpp)：80
  - `register` [include/openswd3/battle/legacy_battle_pair_transition.hpp](../../../include/openswd3/battle/legacy_battle_pair_transition.hpp)：25, 26, 27, 31, 32, 33, 91, 92, 93, 109, 110, 111
  - `arguments` [src/battle/legacy_battle_pair_transition.cpp](../../../src/battle/legacy_battle_pair_transition.cpp)：43, 52
  - `invoke` [src/battle/legacy_battle_pair_transition.cpp](../../../src/battle/legacy_battle_pair_transition.cpp)：40, 54, 144, 161, 173, 179, 188, 206, 226, 232, 247, 265, 285, 291
  - `register` [src/battle/legacy_battle_pair_transition.cpp](../../../src/battle/legacy_battle_pair_transition.cpp)：30, 31, 32, 48, 49, 50, 56, 57, 58, 76, 79, 83, 84, 104, 108, 109, 110, 125, 126, 133, 134, 135, 155, 156, 157, 164

- [ ] **CHAIN-7b0d5dec4e4e** `battle/legacy_battle_party_item_definition`
  - 当前残留信号：counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_party_item_definition.hpp](../../../include/openswd3/battle/legacy_battle_party_item_definition.hpp)；[src/battle/legacy_battle_party_item_definition.cpp](../../../src/battle/legacy_battle_party_item_definition.cpp)。
  - 类型：[PORT-770177f05785](invoke-removal-ports.md#port-770177f05785)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_party_item_definition.hpp](../../../include/openswd3/battle/legacy_battle_party_item_definition.hpp)：124, 125, 126, 127
  - `invoke` [include/openswd3/battle/legacy_battle_party_item_definition.hpp](../../../include/openswd3/battle/legacy_battle_party_item_definition.hpp)：64
  - `register` [include/openswd3/battle/legacy_battle_party_item_definition.hpp](../../../include/openswd3/battle/legacy_battle_party_item_definition.hpp)：43, 44, 45, 49, 50, 51, 79, 80, 81, 138, 139, 140
  - `counter` [src/battle/legacy_battle_party_item_definition.cpp](../../../src/battle/legacy_battle_party_item_definition.cpp)：180, 304, 460, 484
  - `invoke` [src/battle/legacy_battle_party_item_definition.cpp](../../../src/battle/legacy_battle_party_item_definition.cpp)：169, 297, 476
  - `register` [src/battle/legacy_battle_party_item_definition.cpp](../../../src/battle/legacy_battle_party_item_definition.cpp)：121, 122, 123, 129, 130, 131, 147, 148, 149, 151, 152, 153, 162, 163, 164, 176, 177, 178, 181, 182, 183, 191, 202, 203, 204, 210, 212, 213, 214, 220, 225, 227, 228, 229, 246, 247, 248, 252, 253, 255, 258, 261, 267, 269, 270, 271, 284, 293, 300, 301, 302, 305, 306, 307, 308, 316, 324, 325, 326, 330, 332, 333, 334, 368, 369, 370, 386, 387, 388, 394, 397, 398, 403, 405, 406, 407, 411, 413, 415, 421, 423, 424, 425, 433, 435, 436, 438, 453, 475, 479, 480, 481, 482, 485, 486, 487, 505, 506, 507, 530

- [ ] **CHAIN-e9dcec824822** `battle/legacy_battle_party_item_order`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_party_item_order.hpp](../../../include/openswd3/battle/legacy_battle_party_item_order.hpp)；[src/battle/legacy_battle_party_item_order.cpp](../../../src/battle/legacy_battle_party_item_order.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_party_item_order.hpp](../../../include/openswd3/battle/legacy_battle_party_item_order.hpp)：18, 31
  - `register` [src/battle/legacy_battle_party_item_order.cpp](../../../src/battle/legacy_battle_party_item_order.cpp)：38, 41, 67, 104

- [ ] **CHAIN-167f4b70ca29** `battle/legacy_battle_pending_action_commit`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_pending_action_commit.hpp](../../../include/openswd3/battle/legacy_battle_pending_action_commit.hpp)；[src/battle/legacy_battle_pending_action_commit.cpp](../../../src/battle/legacy_battle_pending_action_commit.cpp)。
  - 类型：[PORT-4c2f8535575a](invoke-removal-ports.md#port-4c2f8535575a)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_pending_action_commit.hpp](../../../include/openswd3/battle/legacy_battle_pending_action_commit.hpp)：34
  - `counter` [include/openswd3/battle/legacy_battle_pending_action_commit.hpp](../../../include/openswd3/battle/legacy_battle_pending_action_commit.hpp)：85, 86, 88
  - `invoke` [include/openswd3/battle/legacy_battle_pending_action_commit.hpp](../../../include/openswd3/battle/legacy_battle_pending_action_commit.hpp)：54
  - `register` [include/openswd3/battle/legacy_battle_pending_action_commit.hpp](../../../include/openswd3/battle/legacy_battle_pending_action_commit.hpp)：35, 36, 37, 41, 42, 43
  - `counter` [src/battle/legacy_battle_pending_action_commit.cpp](../../../src/battle/legacy_battle_pending_action_commit.cpp)：88, 102, 159
  - `invoke` [src/battle/legacy_battle_pending_action_commit.cpp](../../../src/battle/legacy_battle_pending_action_commit.cpp)：20, 25, 74, 145
  - `register` [src/battle/legacy_battle_pending_action_commit.cpp](../../../src/battle/legacy_battle_pending_action_commit.cpp)：26, 27, 28, 83, 84, 85, 96, 154, 155, 156, 161, 167, 168

- [ ] **CHAIN-94683e04fd08** `battle/legacy_battle_player_item_order`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_player_item_order.hpp](../../../include/openswd3/battle/legacy_battle_player_item_order.hpp)；[src/battle/legacy_battle_player_item_order.cpp](../../../src/battle/legacy_battle_player_item_order.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_player_item_order.hpp](../../../include/openswd3/battle/legacy_battle_player_item_order.hpp)：17
  - `register` [src/battle/legacy_battle_player_item_order.cpp](../../../src/battle/legacy_battle_player_item_order.cpp)：38, 74

- [ ] **CHAIN-7cf951e1f47b** `battle/legacy_battle_post_action`
  - 当前残留信号：arguments, callee, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_post_action.hpp](../../../include/openswd3/battle/legacy_battle_post_action.hpp)；[src/battle/legacy_battle_post_action.cpp](../../../src/battle/legacy_battle_post_action.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [src/battle/legacy_battle_post_action.cpp](../../../src/battle/legacy_battle_post_action.cpp)：67, 69
  - `callee` [src/battle/legacy_battle_post_action.cpp](../../../src/battle/legacy_battle_post_action.cpp)：66, 69
  - `counter` [src/battle/legacy_battle_post_action.cpp](../../../src/battle/legacy_battle_post_action.cpp)：244, 246
  - `invoke` [src/battle/legacy_battle_post_action.cpp](../../../src/battle/legacy_battle_post_action.cpp)：64, 69, 267
  - `register` [src/battle/legacy_battle_post_action.cpp](../../../src/battle/legacy_battle_post_action.cpp)：100, 101, 108, 109, 121, 136, 137, 147, 154, 155, 164, 165, 175, 182, 183, 192, 193, 204, 218, 231, 232, 251, 255, 270, 275, 276, 277, 297, 305, 306, 307, 308, 309, 310, 322, 394, 420, 425

- [ ] **CHAIN-18e899d89e6a** `battle/legacy_battle_pre_frame`
  - 当前残留信号：interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_pre_frame.hpp](../../../include/openswd3/battle/legacy_battle_pre_frame.hpp)；[src/battle/legacy_battle_pre_frame.cpp](../../../src/battle/legacy_battle_pre_frame.cpp)。
  - 类型：[PORT-d98289cbb7b2](invoke-removal-ports.md#port-d98289cbb7b2)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `invoke` [include/openswd3/battle/legacy_battle_pre_frame.hpp](../../../include/openswd3/battle/legacy_battle_pre_frame.hpp)：47, 59
  - `register` [include/openswd3/battle/legacy_battle_pre_frame.hpp](../../../include/openswd3/battle/legacy_battle_pre_frame.hpp)：26, 27, 31, 32, 33, 73, 74, 90, 107, 108
  - `invoke` [src/battle/legacy_battle_pre_frame.cpp](../../../src/battle/legacy_battle_pre_frame.cpp)：39, 139, 232, 253, 276, 288
  - `register` [src/battle/legacy_battle_pre_frame.cpp](../../../src/battle/legacy_battle_pre_frame.cpp)：44, 45, 46, 55, 59, 63, 83, 84, 85, 89, 102, 103, 106, 107, 122, 123, 126, 127, 128, 130, 131, 132, 138, 143, 144, 155, 156, 157, 173, 174, 184, 188, 189, 193, 194, 195, 199, 200, 202, 212, 216, 224, 225, 226, 231, 233, 238, 241, 242, 243, 249, 251, 252, 263, 264, 265, 270, 275, 280, 284, 286, 296, 301, 303, 309, 313, 317

- [ ] **CHAIN-3c37f21ee250** `battle/legacy_battle_retreat_commit`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_retreat_commit.hpp](../../../include/openswd3/battle/legacy_battle_retreat_commit.hpp)；[src/battle/legacy_battle_retreat_commit.cpp](../../../src/battle/legacy_battle_retreat_commit.cpp)。
  - 类型：[PORT-ac77a2188211](invoke-removal-ports.md#port-ac77a2188211), [PORT-01cef6293059](invoke-removal-ports.md#port-01cef6293059)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_retreat_commit.hpp](../../../include/openswd3/battle/legacy_battle_retreat_commit.hpp)：61, 97
  - `counter` [include/openswd3/battle/legacy_battle_retreat_commit.hpp](../../../include/openswd3/battle/legacy_battle_retreat_commit.hpp)：141
  - `invoke` [include/openswd3/battle/legacy_battle_retreat_commit.hpp](../../../include/openswd3/battle/legacy_battle_retreat_commit.hpp)：85, 90, 93
  - `register` [include/openswd3/battle/legacy_battle_retreat_commit.hpp](../../../include/openswd3/battle/legacy_battle_retreat_commit.hpp)：62, 63, 64, 68, 69, 70, 98, 99, 100, 102
  - `arguments` [src/battle/legacy_battle_retreat_commit.cpp](../../../src/battle/legacy_battle_retreat_commit.cpp)：94
  - `counter` [src/battle/legacy_battle_retreat_commit.cpp](../../../src/battle/legacy_battle_retreat_commit.cpp)：45
  - `invoke` [src/battle/legacy_battle_retreat_commit.cpp](../../../src/battle/legacy_battle_retreat_commit.cpp)：23, 92
  - `register` [src/battle/legacy_battle_retreat_commit.cpp](../../../src/battle/legacy_battle_retreat_commit.cpp)：27, 28, 29, 30, 41, 42, 50, 51, 52, 58, 77, 78, 79, 87, 88, 89, 99, 100, 101, 122, 123

- [ ] **CHAIN-71b68f76445f** `battle/legacy_battle_reward_item_slot`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_reward_item_slot.hpp](../../../include/openswd3/battle/legacy_battle_reward_item_slot.hpp)；[src/battle/legacy_battle_reward_item_slot.cpp](../../../src/battle/legacy_battle_reward_item_slot.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_reward_item_slot.hpp](../../../include/openswd3/battle/legacy_battle_reward_item_slot.hpp)：20, 21, 22, 31, 32
  - `register` [src/battle/legacy_battle_reward_item_slot.cpp](../../../src/battle/legacy_battle_reward_item_slot.cpp)：9, 10, 15, 16, 17, 18

- [ ] **CHAIN-8866d59c1416** `battle/legacy_battle_reward_scale`
  - 当前残留信号：arguments, callee, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_reward_scale.hpp](../../../include/openswd3/battle/legacy_battle_reward_scale.hpp)；[src/battle/legacy_battle_reward_scale.cpp](../../../src/battle/legacy_battle_reward_scale.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_reward_scale.hpp](../../../include/openswd3/battle/legacy_battle_reward_scale.hpp)：30, 31
  - `register` [include/openswd3/battle/legacy_battle_reward_scale.hpp](../../../include/openswd3/battle/legacy_battle_reward_scale.hpp)：15, 16, 17, 33, 34, 35
  - `arguments` [src/battle/legacy_battle_reward_scale.cpp](../../../src/battle/legacy_battle_reward_scale.cpp)：36, 48
  - `callee` [src/battle/legacy_battle_reward_scale.cpp](../../../src/battle/legacy_battle_reward_scale.cpp)：35, 47
  - `counter` [src/battle/legacy_battle_reward_scale.cpp](../../../src/battle/legacy_battle_reward_scale.cpp)：33, 45
  - `invoke` [src/battle/legacy_battle_reward_scale.cpp](../../../src/battle/legacy_battle_reward_scale.cpp)：34, 46
  - `register` [src/battle/legacy_battle_reward_scale.cpp](../../../src/battle/legacy_battle_reward_scale.cpp)：20, 21, 22, 29, 37, 38, 39, 41, 42, 43, 49, 50, 51, 53, 54, 55, 69

- [ ] **CHAIN-7a6d8766a638** `battle/legacy_battle_scale_fill_panel`
  - 当前残留信号：counter。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_scale_fill_panel.hpp](../../../include/openswd3/battle/legacy_battle_scale_fill_panel.hpp)；[src/battle/legacy_battle_scale_fill_panel.cpp](../../../src/battle/legacy_battle_scale_fill_panel.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_scale_fill_panel.hpp](../../../include/openswd3/battle/legacy_battle_scale_fill_panel.hpp)：31, 32, 33
  - `counter` [src/battle/legacy_battle_scale_fill_panel.cpp](../../../src/battle/legacy_battle_scale_fill_panel.cpp)：101, 153, 212, 248

- [ ] **CHAIN-1f1ccf8500a9** `battle/legacy_battle_scale_scan`
  - 当前残留信号：counter。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_scale_scan.hpp](../../../include/openswd3/battle/legacy_battle_scale_scan.hpp)；[src/battle/legacy_battle_scale_scan.cpp](../../../src/battle/legacy_battle_scale_scan.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_scale_scan.hpp](../../../include/openswd3/battle/legacy_battle_scale_scan.hpp)：36, 37, 38
  - `counter` [src/battle/legacy_battle_scale_scan.cpp](../../../src/battle/legacy_battle_scale_scan.cpp)：101, 129, 181, 235, 252

- [ ] **CHAIN-4041ca368c58** `battle/legacy_battle_script_dispatch`
  - 当前残留信号：arguments, counter, interface, invoke, operation, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_script_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_script_dispatch.hpp)；[src/battle/legacy_battle_script_dispatch.cpp](../../../src/battle/legacy_battle_script_dispatch.cpp)；[src/battle/legacy_battle_script_dispatch_cases_fifty_five_to_eighty_three.cpp](../../../src/battle/legacy_battle_script_dispatch_cases_fifty_five_to_eighty_three.cpp)；[src/battle/legacy_battle_script_dispatch_cases_one_to_twenty_seven.cpp](../../../src/battle/legacy_battle_script_dispatch_cases_one_to_twenty_seven.cpp)；[src/battle/legacy_battle_script_dispatch_cases_twenty_eight_to_fifty_four.cpp](../../../src/battle/legacy_battle_script_dispatch_cases_twenty_eight_to_fifty_four.cpp)；[src/battle/legacy_battle_script_dispatch_internal.hpp](../../../src/battle/legacy_battle_script_dispatch_internal.hpp)。
  - 类型：[PORT-4139fa9e1174](invoke-removal-ports.md#port-4139fa9e1174), [PORT-8b216ea963fc](invoke-removal-ports.md#port-8b216ea963fc), [PORT-243b0f85d63c](invoke-removal-ports.md#port-243b0f85d63c)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_script_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_script_dispatch.hpp)：273, 274
  - `counter` [include/openswd3/battle/legacy_battle_script_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_script_dispatch.hpp)：411, 413, 417, 419, 421, 424, 427, 430, 433, 435
  - `invoke` [include/openswd3/battle/legacy_battle_script_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_script_dispatch.hpp)：300
  - `register` [include/openswd3/battle/legacy_battle_script_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_script_dispatch.hpp)：275, 276, 277, 283, 284, 285, 306, 307, 308, 365, 366, 367, 368, 369, 403, 404, 405
  - `counter` [src/battle/legacy_battle_script_dispatch_cases_fifty_five_to_eighty_three.cpp](../../../src/battle/legacy_battle_script_dispatch_cases_fifty_five_to_eighty_three.cpp)：62, 833, 900
  - `invoke` [src/battle/legacy_battle_script_dispatch_cases_fifty_five_to_eighty_three.cpp](../../../src/battle/legacy_battle_script_dispatch_cases_fifty_five_to_eighty_three.cpp)：82, 218, 219, 220, 331, 385, 430, 455, 478, 506, 531, 618, 647, 795, 931, 966, 968, 973, 1140, 1201, 1206, 1220, 1303
  - `register` [src/battle/legacy_battle_script_dispatch_cases_fifty_five_to_eighty_three.cpp](../../../src/battle/legacy_battle_script_dispatch_cases_fifty_five_to_eighty_three.cpp)：58, 59, 63, 64, 65, 830, 834, 835, 836, 896, 897, 901, 902, 903, 1158, 1159, 1160, 1168, 1169, 1170, 1187, 1188, 1189, 1197, 1198, 1199, 1268, 1271, 1272, 1273
  - `counter` [src/battle/legacy_battle_script_dispatch_cases_one_to_twenty_seven.cpp](../../../src/battle/legacy_battle_script_dispatch_cases_one_to_twenty_seven.cpp)：1325
  - `invoke` [src/battle/legacy_battle_script_dispatch_cases_one_to_twenty_seven.cpp](../../../src/battle/legacy_battle_script_dispatch_cases_one_to_twenty_seven.cpp)：11, 56, 72, 103, 115, 164, 284, 355, 370, 435, 790, 839, 851, 891, 919, 959, 1004, 1011, 1207, 1283, 1359, 1364
  - `register` [src/battle/legacy_battle_script_dispatch_cases_one_to_twenty_seven.cpp](../../../src/battle/legacy_battle_script_dispatch_cases_one_to_twenty_seven.cpp)：1318, 1319, 1320, 1331, 1332, 1333
  - `arguments` [src/battle/legacy_battle_script_dispatch_cases_twenty_eight_to_fifty_four.cpp](../../../src/battle/legacy_battle_script_dispatch_cases_twenty_eight_to_fifty_four.cpp)：9, 13, 18, 31, 1075, 1079, 1084, 1096
  - `counter` [src/battle/legacy_battle_script_dispatch_cases_twenty_eight_to_fifty_four.cpp](../../../src/battle/legacy_battle_script_dispatch_cases_twenty_eight_to_fifty_four.cpp)：1210, 1280
  - `invoke` [src/battle/legacy_battle_script_dispatch_cases_twenty_eight_to_fifty_four.cpp](../../../src/battle/legacy_battle_script_dispatch_cases_twenty_eight_to_fifty_four.cpp)：28, 34, 49, 103, 134, 147, 171, 188, 246, 263, 273, 294, 295, 309, 313, 351, 387, 392, 397, 539, 695, 709, 735, 737, 897, 911, 925, 937, 953, 1093, 1173
  - `operation` [src/battle/legacy_battle_script_dispatch_cases_twenty_eight_to_fifty_four.cpp](../../../src/battle/legacy_battle_script_dispatch_cases_twenty_eight_to_fifty_four.cpp)：345, 347, 354, 369, 371, 373
  - `register` [src/battle/legacy_battle_script_dispatch_cases_twenty_eight_to_fifty_four.cpp](../../../src/battle/legacy_battle_script_dispatch_cases_twenty_eight_to_fifty_four.cpp)：1216, 1217, 1218, 1275, 1276, 1277, 1281, 1282, 1283
  - `arguments` [src/battle/legacy_battle_script_dispatch_internal.hpp](../../../src/battle/legacy_battle_script_dispatch_internal.hpp)：771, 782, 784, 785, 786, 842, 848, 849, 890, 891, 896, 902, 907, 908, 914, 915, 916, 923, 929
  - `counter` [src/battle/legacy_battle_script_dispatch_internal.hpp](../../../src/battle/legacy_battle_script_dispatch_internal.hpp)：500, 528, 627, 668, 675, 712
  - `invoke` [src/battle/legacy_battle_script_dispatch_internal.hpp](../../../src/battle/legacy_battle_script_dispatch_internal.hpp)：768, 792, 814, 852, 874, 877, 885, 932, 951, 954, 962, 1186, 1203, 1533, 1553, 1565, 1608, 1620, 1663
  - `operation` [src/battle/legacy_battle_script_dispatch_internal.hpp](../../../src/battle/legacy_battle_script_dispatch_internal.hpp)：769, 774, 790, 798, 836, 840, 850, 888, 894, 906, 912, 921, 930
  - `register` [src/battle/legacy_battle_script_dispatch_internal.hpp](../../../src/battle/legacy_battle_script_dispatch_internal.hpp)：206, 207, 208, 209, 398, 399, 400, 401, 496, 497, 507, 508, 534, 535, 536, 553, 554, 568, 569, 622, 623, 628, 629, 630, 645, 646, 647, 648, 660, 661, 662, 663, 664, 669, 670, 671, 707, 708, 713, 714, 715, 776, 777, 778, 793, 794, 795, 824, 825, 826, 841, 843, 844, 845, 853, 854, 855, 857, 858, 859, 924, 925, 926, 933, 934, 935, 937, 938, 939, 967, 968, 979, 980, 986, 987, 988, 997, 998, 999, 1006, 1007, 1021, 1022, 1027, 1028, 1029, 1037, 1038, 1039, 1047, 1048, 1064, 1065, 1069, 1070, 1071, 1080, 1081, 1082

- [ ] **CHAIN-6e903b612999** `battle/legacy_battle_script_file`
  - 当前残留信号：interface, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_script_file.hpp](../../../include/openswd3/battle/legacy_battle_script_file.hpp)；[src/battle/legacy_battle_script_file.cpp](../../../src/battle/legacy_battle_script_file.cpp)。
  - 类型：[PORT-9607f06747d3](invoke-removal-ports.md#port-9607f06747d3)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_script_file.hpp](../../../include/openswd3/battle/legacy_battle_script_file.hpp)：23, 83
  - `register` [src/battle/legacy_battle_script_file.cpp](../../../src/battle/legacy_battle_script_file.cpp)：69, 92, 154

- [ ] **CHAIN-61c71eda5533** `battle/legacy_battle_selection_hint_frame`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_selection_hint_frame.hpp](../../../include/openswd3/battle/legacy_battle_selection_hint_frame.hpp)；[src/battle/legacy_battle_selection_hint_frame.cpp](../../../src/battle/legacy_battle_selection_hint_frame.cpp)。
  - 类型：[PORT-19075230a180](invoke-removal-ports.md#port-19075230a180)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_selection_hint_frame.hpp](../../../include/openswd3/battle/legacy_battle_selection_hint_frame.hpp)：46
  - `counter` [include/openswd3/battle/legacy_battle_selection_hint_frame.hpp](../../../include/openswd3/battle/legacy_battle_selection_hint_frame.hpp)：136, 137, 138, 140, 142, 143, 145, 149, 151
  - `invoke` [include/openswd3/battle/legacy_battle_selection_hint_frame.hpp](../../../include/openswd3/battle/legacy_battle_selection_hint_frame.hpp)：71
  - `register` [include/openswd3/battle/legacy_battle_selection_hint_frame.hpp](../../../include/openswd3/battle/legacy_battle_selection_hint_frame.hpp)：47, 48, 49, 56, 57, 58, 97, 98, 99, 105, 106, 107, 133, 134, 135
  - `arguments` [src/battle/legacy_battle_selection_hint_frame.cpp](../../../src/battle/legacy_battle_selection_hint_frame.cpp)：231, 252, 346, 450
  - `counter` [src/battle/legacy_battle_selection_hint_frame.cpp](../../../src/battle/legacy_battle_selection_hint_frame.cpp)：145, 170, 189, 215, 236, 268, 352, 455, 474
  - `invoke` [src/battle/legacy_battle_selection_hint_frame.cpp](../../../src/battle/legacy_battle_selection_hint_frame.cpp)：110, 111, 138, 228, 249, 343, 448
  - `register` [src/battle/legacy_battle_selection_hint_frame.cpp](../../../src/battle/legacy_battle_selection_hint_frame.cpp)：71, 72, 104, 105, 106, 112, 113, 114, 141, 142, 143, 146, 190, 191, 192, 216, 217, 218, 232, 233, 234, 261, 262, 263, 348, 349, 350, 451, 452, 453, 456, 476, 477, 478

- [ ] **CHAIN-31aa9d773cbd** `battle/legacy_battle_startup`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_startup.hpp](../../../include/openswd3/battle/legacy_battle_startup.hpp)；[src/battle/legacy_battle_startup.cpp](../../../src/battle/legacy_battle_startup.cpp)。
  - 类型：[PORT-a844c6f3c5c1](invoke-removal-ports.md#port-a844c6f3c5c1), [PORT-d9653246e88e](invoke-removal-ports.md#port-d9653246e88e), [PORT-04502dd7fe77](invoke-removal-ports.md#port-04502dd7fe77), [PORT-8a7773b9eefb](invoke-removal-ports.md#port-8a7773b9eefb), [PORT-89ec329a0143](invoke-removal-ports.md#port-89ec329a0143), [PORT-46589e75e09f](invoke-removal-ports.md#port-46589e75e09f)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_startup.hpp](../../../include/openswd3/battle/legacy_battle_startup.hpp)：120
  - `counter` [include/openswd3/battle/legacy_battle_startup.hpp](../../../include/openswd3/battle/legacy_battle_startup.hpp)：474, 477, 507, 510, 576, 577, 581, 584, 586, 590, 596, 599, 600, 604
  - `invoke` [include/openswd3/battle/legacy_battle_startup.hpp](../../../include/openswd3/battle/legacy_battle_startup.hpp)：161
  - `register` [include/openswd3/battle/legacy_battle_startup.hpp](../../../include/openswd3/battle/legacy_battle_startup.hpp)：121, 122, 123
  - `arguments` [src/battle/legacy_battle_startup.cpp](../../../src/battle/legacy_battle_startup.cpp)：30, 33, 87, 91, 103, 208, 221, 266, 622, 642
  - `counter` [src/battle/legacy_battle_startup.cpp](../../../src/battle/legacy_battle_startup.cpp)：666, 681, 739, 747, 832, 893, 1158, 1259, 1266, 1284, 1475, 1534, 1550, 1554, 1579, 1580, 1583, 1584, 1593, 1596, 1597
  - `invoke` [src/battle/legacy_battle_startup.cpp](../../../src/battle/legacy_battle_startup.cpp)：27, 32, 46, 67, 74, 83, 111, 199, 239, 263, 620, 628, 633, 640, 1176, 1190, 1198, 1205, 1211, 1218, 1220, 1226, 1233, 1252, 1270, 1285, 1299, 1333, 1422, 1482
  - `register` [src/battle/legacy_battle_startup.cpp](../../../src/battle/legacy_battle_startup.cpp)：113, 114, 115, 214, 227, 234, 235, 236, 241, 242, 243, 273, 276, 277, 278, 527, 544, 842, 882, 891, 902, 1079, 1108

- [ ] **CHAIN-8a5e3e61646d** `battle/legacy_battle_status_indicator`
  - 当前残留信号：counter, interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_status_indicator.hpp](../../../include/openswd3/battle/legacy_battle_status_indicator.hpp)；[src/battle/legacy_battle_status_indicator.cpp](../../../src/battle/legacy_battle_status_indicator.cpp)。
  - 类型：[PORT-3e60c2b3f4f5](invoke-removal-ports.md#port-3e60c2b3f4f5), [PORT-dcddf6f7982f](invoke-removal-ports.md#port-dcddf6f7982f)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_status_indicator.hpp](../../../include/openswd3/battle/legacy_battle_status_indicator.hpp)：49, 50, 51, 52, 53
  - `counter` [src/battle/legacy_battle_status_indicator.cpp](../../../src/battle/legacy_battle_status_indicator.cpp)：58, 66, 77, 104, 153

- [ ] **CHAIN-b68b7ffe5b48** `battle/legacy_battle_surface_blend`
  - 当前残留信号：counter, interface, operation。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_surface_blend.hpp](../../../include/openswd3/battle/legacy_battle_surface_blend.hpp)；[src/battle/legacy_battle_surface_blend.cpp](../../../src/battle/legacy_battle_surface_blend.cpp)。
  - 类型：[PORT-5a1ba6935260](invoke-removal-ports.md#port-5a1ba6935260)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_surface_blend.hpp](../../../include/openswd3/battle/legacy_battle_surface_blend.hpp)：92, 95, 96, 98, 99, 100
  - `operation` [include/openswd3/battle/legacy_battle_surface_blend.hpp](../../../include/openswd3/battle/legacy_battle_surface_blend.hpp)：59
  - `counter` [src/battle/legacy_battle_surface_blend.cpp](../../../src/battle/legacy_battle_surface_blend.cpp)：30, 89, 121, 128, 147, 151
  - `operation` [src/battle/legacy_battle_surface_blend.cpp](../../../src/battle/legacy_battle_surface_blend.cpp)：36, 38

- [ ] **CHAIN-1f80e4f825c5** `battle/legacy_battle_talisman_result_panel`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_talisman_result_panel.hpp](../../../include/openswd3/battle/legacy_battle_talisman_result_panel.hpp)；[src/battle/legacy_battle_talisman_result_panel.cpp](../../../src/battle/legacy_battle_talisman_result_panel.cpp)。
  - 类型：[PORT-7932e6ddad59](invoke-removal-ports.md#port-7932e6ddad59)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_talisman_result_panel.hpp](../../../include/openswd3/battle/legacy_battle_talisman_result_panel.hpp)：54
  - `counter` [include/openswd3/battle/legacy_battle_talisman_result_panel.hpp](../../../include/openswd3/battle/legacy_battle_talisman_result_panel.hpp)：126, 127, 128, 129, 130, 131, 132
  - `invoke` [include/openswd3/battle/legacy_battle_talisman_result_panel.hpp](../../../include/openswd3/battle/legacy_battle_talisman_result_panel.hpp)：81
  - `register` [include/openswd3/battle/legacy_battle_talisman_result_panel.hpp](../../../include/openswd3/battle/legacy_battle_talisman_result_panel.hpp)：56, 57, 58, 64, 65, 66, 85, 86, 87, 91, 99, 100, 101, 123, 124, 125
  - `arguments` [src/battle/legacy_battle_talisman_result_panel.cpp](../../../src/battle/legacy_battle_talisman_result_panel.cpp)：92, 99
  - `counter` [src/battle/legacy_battle_talisman_result_panel.cpp](../../../src/battle/legacy_battle_talisman_result_panel.cpp)：157, 188, 222, 255, 273, 304, 349, 368, 369
  - `invoke` [src/battle/legacy_battle_talisman_result_panel.cpp](../../../src/battle/legacy_battle_talisman_result_panel.cpp)：89, 111, 241, 262, 335
  - `register` [src/battle/legacy_battle_talisman_result_panel.cpp](../../../src/battle/legacy_battle_talisman_result_panel.cpp)：71, 72, 101, 102, 103, 113, 114, 115, 131, 135, 136, 137, 141, 158, 159, 160, 171, 189, 190, 191, 205, 223, 224, 225, 305, 306, 307, 373, 374, 375

- [ ] **CHAIN-56d856b16838** `battle/legacy_battle_target_ready`
  - 当前残留信号：arguments, callee, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_target_ready.hpp](../../../include/openswd3/battle/legacy_battle_target_ready.hpp)；[src/battle/legacy_battle_target_ready.cpp](../../../src/battle/legacy_battle_target_ready.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_target_ready.hpp](../../../include/openswd3/battle/legacy_battle_target_ready.hpp)：39, 40, 41, 42, 43, 45, 46, 47, 48, 49
  - `register` [include/openswd3/battle/legacy_battle_target_ready.hpp](../../../include/openswd3/battle/legacy_battle_target_ready.hpp)：19, 20, 21, 59, 60, 61
  - `arguments` [src/battle/legacy_battle_target_ready.cpp](../../../src/battle/legacy_battle_target_ready.cpp)：101, 104, 282, 287, 306
  - `callee` [src/battle/legacy_battle_target_ready.cpp](../../../src/battle/legacy_battle_target_ready.cpp)：100, 103, 286, 305
  - `counter` [src/battle/legacy_battle_target_ready.cpp](../../../src/battle/legacy_battle_target_ready.cpp)：121, 128, 177, 188, 206, 225, 247, 284, 296, 302, 341, 397
  - `invoke` [src/battle/legacy_battle_target_ready.cpp](../../../src/battle/legacy_battle_target_ready.cpp)：102, 179, 192, 196, 207, 226, 285, 297, 303, 399
  - `register` [src/battle/legacy_battle_target_ready.cpp](../../../src/battle/legacy_battle_target_ready.cpp)：72, 73, 74, 82, 83, 84, 86, 87, 88, 91, 92, 93, 96, 105, 106, 107, 109, 110, 111, 116, 117, 119, 123, 173, 174, 175, 176, 179, 186, 187, 190, 192, 195, 196, 203, 204, 205, 222, 223, 224, 238, 262, 263, 269, 270, 271, 283, 288, 289, 290, 292, 293, 294, 295, 301, 307, 308, 309, 313, 314, 315, 316, 319, 321, 323, 324, 340, 350, 351, 356, 357, 358, 359, 360, 361, 367, 368, 369, 370, 371, 372, 377, 379, 380, 396, 411, 412

- [ ] **CHAIN-a7bb1f3447c3** `battle/legacy_battle_target_selection_entry`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_target_selection_entry.hpp](../../../include/openswd3/battle/legacy_battle_target_selection_entry.hpp)；[src/battle/legacy_battle_target_selection_entry.cpp](../../../src/battle/legacy_battle_target_selection_entry.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_target_selection_entry.hpp](../../../include/openswd3/battle/legacy_battle_target_selection_entry.hpp)：64, 65, 67, 70, 72, 73
  - `register` [include/openswd3/battle/legacy_battle_target_selection_entry.hpp](../../../include/openswd3/battle/legacy_battle_target_selection_entry.hpp)：39, 40, 41, 61, 62, 63
  - `arguments` [src/battle/legacy_battle_target_selection_entry.cpp](../../../src/battle/legacy_battle_target_selection_entry.cpp)：106, 109
  - `counter` [src/battle/legacy_battle_target_selection_entry.cpp](../../../src/battle/legacy_battle_target_selection_entry.cpp)：135, 176, 277, 309, 359, 404
  - `invoke` [src/battle/legacy_battle_target_selection_entry.cpp](../../../src/battle/legacy_battle_target_selection_entry.cpp)：107, 268
  - `register` [src/battle/legacy_battle_target_selection_entry.cpp](../../../src/battle/legacy_battle_target_selection_entry.cpp)：95, 96, 97, 100, 101, 102, 110, 111, 112, 114, 115, 116, 133, 136, 137, 138, 148, 149, 150, 177, 178, 179, 188, 189, 195, 196, 202, 203, 204, 211, 212, 217, 218, 220, 221, 224, 225, 226, 232, 233, 234, 238, 244, 245, 255, 256, 262, 269, 273, 275, 278, 279, 280, 283, 294, 297, 298, 303, 310, 311, 312, 324, 325, 326, 327, 351, 354, 355, 356, 360, 361, 362, 377, 397, 399, 400, 401, 405, 406, 407, 422

- [ ] **CHAIN-720a76165b91** `battle/legacy_battle_target_selection_refresh`
  - 本批：三个资源选择点及相邻动作4完成点改接实际函数；删除两个通用操作、选择兼容开关及子寄存器/计数，验证见[资源选择](battle-actor-resource-selection-00470ac0.md)。源码4b163e92已推送并核对远端main一致。
  - 剩余：诊断转发、其他操作/参数包/寄存器/计数/Port及完整父路径；整链不勾选。
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_target_selection_refresh.hpp](../../../include/openswd3/battle/legacy_battle_target_selection_refresh.hpp)；[src/battle/legacy_battle_target_selection_refresh.cpp](../../../src/battle/legacy_battle_target_selection_refresh.cpp)。
  - 类型：[PORT-afd331603c89](invoke-removal-ports.md#port-afd331603c89), [PORT-f9d6f88adc33](invoke-removal-ports.md#port-f9d6f88adc33)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_target_selection_refresh.hpp](../../../include/openswd3/battle/legacy_battle_target_selection_refresh.hpp)：119, 127, 128, 129, 132, 136, 138, 141, 143
  - `register` [include/openswd3/battle/legacy_battle_target_selection_refresh.hpp](../../../include/openswd3/battle/legacy_battle_target_selection_refresh.hpp)：39, 40, 41, 75, 76, 77, 84, 85, 86, 98, 99, 100, 107, 108, 109, 116, 117, 118
  - `arguments` [src/battle/legacy_battle_target_selection_refresh.cpp](../../../src/battle/legacy_battle_target_selection_refresh.cpp)：45, 65, 297, 302, 337, 367, 397, 406, 457, 463
  - `counter` [src/battle/legacy_battle_target_selection_refresh.cpp](../../../src/battle/legacy_battle_target_selection_refresh.cpp)：317, 658, 682, 712, 755, 987, 1450, 1710, 1890
  - `invoke` [src/battle/legacy_battle_target_selection_refresh.cpp](../../../src/battle/legacy_battle_target_selection_refresh.cpp)：42, 58, 61, 294, 299, 334, 367, 394, 406, 454, 463, 937, 965, 969, 1154, 1323, 1335, 1487, 1515, 1524, 1543, 1557, 1626, 1667, 1816, 1854, 1954, 2009, 2056, 2173, 2177, 2243, 2258
  - `register` [src/battle/legacy_battle_target_selection_refresh.cpp](../../../src/battle/legacy_battle_target_selection_refresh.cpp)：66, 67, 68, 70, 103, 104, 190, 191, 192, 303, 304, 305, 307, 308, 309, 318, 319, 320, 425, 443, 444, 445, 660, 661, 662, 680, 684, 685, 686, 710, 714, 715, 716, 752, 753, 756, 757, 758, 984, 989, 990, 991, 1451, 1452, 1453, 1711, 1712, 1713, 1891, 1892, 1893, 2307, 2308, 2309, 2312, 2313, 2314, 2321, 2329, 2330, 2331, 2334, 2335, 2336, 2343

- [ ] **CHAIN-3a258ab4081f** `battle/legacy_battle_target_selection_runtime`
  - 当前残留信号：arguments, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_target_selection_runtime.hpp](../../../include/openswd3/battle/legacy_battle_target_selection_runtime.hpp)。
  - 类型：[PORT-129eeaecd845](invoke-removal-ports.md#port-129eeaecd845), [PORT-886bcc672f06](invoke-removal-ports.md#port-886bcc672f06)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_target_selection_runtime.hpp](../../../include/openswd3/battle/legacy_battle_target_selection_runtime.hpp)：96
  - `invoke` [include/openswd3/battle/legacy_battle_target_selection_runtime.hpp](../../../include/openswd3/battle/legacy_battle_target_selection_runtime.hpp)：115
  - `register` [include/openswd3/battle/legacy_battle_target_selection_runtime.hpp](../../../include/openswd3/battle/legacy_battle_target_selection_runtime.hpp)：97, 98, 99, 103, 104, 105, 119, 120, 121

- [ ] **CHAIN-300885b47a2f** `battle/legacy_battle_text_message`
  - 当前残留信号：counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_text_message.hpp](../../../include/openswd3/battle/legacy_battle_text_message.hpp)；[src/battle/legacy_battle_text_message.cpp](../../../src/battle/legacy_battle_text_message.cpp)。
  - 类型：[PORT-215eeb64554b](invoke-removal-ports.md#port-215eeb64554b)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_text_message.hpp](../../../include/openswd3/battle/legacy_battle_text_message.hpp)：98, 99
  - `invoke` [include/openswd3/battle/legacy_battle_text_message.hpp](../../../include/openswd3/battle/legacy_battle_text_message.hpp)：73
  - `register` [include/openswd3/battle/legacy_battle_text_message.hpp](../../../include/openswd3/battle/legacy_battle_text_message.hpp)：42, 43, 44, 55, 56, 57, 61, 62, 63
  - `counter` [src/battle/legacy_battle_text_message.cpp](../../../src/battle/legacy_battle_text_message.cpp)：41, 76
  - `invoke` [src/battle/legacy_battle_text_message.cpp](../../../src/battle/legacy_battle_text_message.cpp)：34, 69
  - `register` [src/battle/legacy_battle_text_message.cpp](../../../src/battle/legacy_battle_text_message.cpp)：26, 27, 28, 30, 37, 38, 39, 47, 48, 49, 50, 51, 72, 73, 74, 82, 83, 84, 89, 91, 92, 93, 94, 99, 100, 101, 102, 104, 107, 111, 114, 118, 121, 129

- [ ] **CHAIN-68eba0a9b15a** `battle/legacy_battle_text_message_frame`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_text_message_frame.hpp](../../../include/openswd3/battle/legacy_battle_text_message_frame.hpp)；[src/battle/legacy_battle_text_message_frame.cpp](../../../src/battle/legacy_battle_text_message_frame.cpp)。
  - 类型：[PORT-99c11857a4d1](invoke-removal-ports.md#port-99c11857a4d1)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_text_message_frame.hpp](../../../include/openswd3/battle/legacy_battle_text_message_frame.hpp)：31
  - `counter` [include/openswd3/battle/legacy_battle_text_message_frame.hpp](../../../include/openswd3/battle/legacy_battle_text_message_frame.hpp)：92, 93, 94, 95, 96, 97
  - `invoke` [include/openswd3/battle/legacy_battle_text_message_frame.hpp](../../../include/openswd3/battle/legacy_battle_text_message_frame.hpp)：48
  - `register` [include/openswd3/battle/legacy_battle_text_message_frame.hpp](../../../include/openswd3/battle/legacy_battle_text_message_frame.hpp)：32, 33, 34, 38, 39, 40
  - `arguments` [src/battle/legacy_battle_text_message_frame.cpp](../../../src/battle/legacy_battle_text_message_frame.cpp)：100, 374
  - `counter` [src/battle/legacy_battle_text_message_frame.cpp](../../../src/battle/legacy_battle_text_message_frame.cpp)：115, 135, 171, 189, 216, 240, 379
  - `invoke` [src/battle/legacy_battle_text_message_frame.cpp](../../../src/battle/legacy_battle_text_message_frame.cpp)：98, 372
  - `register` [src/battle/legacy_battle_text_message_frame.cpp](../../../src/battle/legacy_battle_text_message_frame.cpp)：84, 85, 86, 88, 94, 111, 112, 113, 116, 117, 118, 139, 140, 141, 172, 190, 191, 192, 198, 205, 217, 218, 219, 244, 245, 246, 375, 376, 377, 381, 382, 383

- [ ] **CHAIN-d15861ec306f** `battle/legacy_battle_text_panel`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_text_panel.hpp](../../../include/openswd3/battle/legacy_battle_text_panel.hpp)；[src/battle/legacy_battle_text_panel.cpp](../../../src/battle/legacy_battle_text_panel.cpp)。
  - 类型：[PORT-2781b9333f3e](invoke-removal-ports.md#port-2781b9333f3e)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_text_panel.hpp](../../../include/openswd3/battle/legacy_battle_text_panel.hpp)：32
  - `counter` [include/openswd3/battle/legacy_battle_text_panel.hpp](../../../include/openswd3/battle/legacy_battle_text_panel.hpp)：73, 74, 75, 76
  - `invoke` [include/openswd3/battle/legacy_battle_text_panel.hpp](../../../include/openswd3/battle/legacy_battle_text_panel.hpp)：51
  - `register` [include/openswd3/battle/legacy_battle_text_panel.hpp](../../../include/openswd3/battle/legacy_battle_text_panel.hpp)：18, 19, 20, 33, 34, 35, 39, 40, 41, 52
  - `arguments` [src/battle/legacy_battle_text_panel.cpp](../../../src/battle/legacy_battle_text_panel.cpp)：41, 45
  - `counter` [src/battle/legacy_battle_text_panel.cpp](../../../src/battle/legacy_battle_text_panel.cpp)：65, 80, 104, 133
  - `invoke` [src/battle/legacy_battle_text_panel.cpp](../../../src/battle/legacy_battle_text_panel.cpp)：43, 61, 69, 93, 122
  - `register` [src/battle/legacy_battle_text_panel.cpp](../../../src/battle/legacy_battle_text_panel.cpp)：36, 37, 38, 46, 47, 48, 50, 51, 52, 60, 67, 68, 86, 89, 90, 91, 92, 95, 106, 107, 112, 113, 117, 118, 119, 121, 135

- [ ] **CHAIN-055f37abbb95** `battle/legacy_battle_transition`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_transition.hpp](../../../include/openswd3/battle/legacy_battle_transition.hpp)；[src/battle/legacy_battle_transition.cpp](../../../src/battle/legacy_battle_transition.cpp)。
  - 类型：[PORT-4240acb37dfc](invoke-removal-ports.md#port-4240acb37dfc), [PORT-9aab8c7b9bdf](invoke-removal-ports.md#port-9aab8c7b9bdf), [PORT-190abae1c145](invoke-removal-ports.md#port-190abae1c145), [PORT-48f4497cc0d5](invoke-removal-ports.md#port-48f4497cc0d5)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_transition.hpp](../../../include/openswd3/battle/legacy_battle_transition.hpp)：99
  - `counter` [include/openswd3/battle/legacy_battle_transition.hpp](../../../include/openswd3/battle/legacy_battle_transition.hpp)：203, 204, 206, 209, 210, 211, 212, 213, 215, 217, 219, 220, 223, 225, 231, 237
  - `invoke` [include/openswd3/battle/legacy_battle_transition.hpp](../../../include/openswd3/battle/legacy_battle_transition.hpp)：121
  - `register` [include/openswd3/battle/legacy_battle_transition.hpp](../../../include/openswd3/battle/legacy_battle_transition.hpp)：104, 105
  - `arguments` [src/battle/legacy_battle_transition.cpp](../../../src/battle/legacy_battle_transition.cpp)：24, 27, 43
  - `counter` [src/battle/legacy_battle_transition.cpp](../../../src/battle/legacy_battle_transition.cpp)：71, 150, 157, 182, 202, 221, 227, 240, 252, 253, 279, 308, 323, 338, 342, 446, 478, 509, 658, 694, 871
  - `invoke` [src/battle/legacy_battle_transition.cpp](../../../src/battle/legacy_battle_transition.cpp)：21, 26, 36, 39, 158, 171, 177, 192, 211, 408, 421, 464, 469, 470, 487, 575, 580, 621, 630, 640, 646, 649, 668, 675, 678, 714, 748, 754, 775, 802, 838, 895
  - `register` [src/battle/legacy_battle_transition.cpp](../../../src/battle/legacy_battle_transition.cpp)：45, 321, 377, 378, 384, 394, 395, 404, 811, 817, 872, 904, 910

- [ ] **CHAIN-932a8d3ec57f** `battle/legacy_battle_transition_control_selection`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_transition_control_selection.hpp](../../../include/openswd3/battle/legacy_battle_transition_control_selection.hpp)；[src/battle/legacy_battle_transition_control_selection.cpp](../../../src/battle/legacy_battle_transition_control_selection.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_transition_control_selection.hpp](../../../include/openswd3/battle/legacy_battle_transition_control_selection.hpp)：22, 23, 24, 28, 29, 30
  - `register` [src/battle/legacy_battle_transition_control_selection.cpp](../../../src/battle/legacy_battle_transition_control_selection.cpp)：54, 55, 56, 79, 80, 81, 84, 89, 110

- [ ] **CHAIN-7cf7e5bc22e6** `battle/legacy_battle_transition_stage_advance`
  - 当前残留信号：register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_transition_stage_advance.hpp](../../../include/openswd3/battle/legacy_battle_transition_stage_advance.hpp)；[src/battle/legacy_battle_transition_stage_advance.cpp](../../../src/battle/legacy_battle_transition_stage_advance.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `register` [include/openswd3/battle/legacy_battle_transition_stage_advance.hpp](../../../include/openswd3/battle/legacy_battle_transition_stage_advance.hpp)：23, 24, 25
  - `register` [src/battle/legacy_battle_transition_stage_advance.cpp](../../../src/battle/legacy_battle_transition_stage_advance.cpp)：30, 31, 32, 51, 52, 53

- [ ] **CHAIN-13b8a50d65ef** `battle/legacy_battle_vertical_panel`
  - 当前残留信号：counter, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_vertical_panel.hpp](../../../include/openswd3/battle/legacy_battle_vertical_panel.hpp)；[src/battle/legacy_battle_vertical_panel.cpp](../../../src/battle/legacy_battle_vertical_panel.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_vertical_panel.hpp](../../../include/openswd3/battle/legacy_battle_vertical_panel.hpp)：57, 58, 59, 60, 67
  - `register` [include/openswd3/battle/legacy_battle_vertical_panel.hpp](../../../include/openswd3/battle/legacy_battle_vertical_panel.hpp)：12, 13, 14
  - `counter` [src/battle/legacy_battle_vertical_panel.cpp](../../../src/battle/legacy_battle_vertical_panel.cpp)：145, 166, 210, 211, 388, 423
  - `register` [src/battle/legacy_battle_vertical_panel.cpp](../../../src/battle/legacy_battle_vertical_panel.cpp)：153, 155

- [ ] **CHAIN-ede924de1ab6** `battle/legacy_battle_vertical_shift`
  - 当前残留信号：counter, interface, operation。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_vertical_shift.hpp](../../../include/openswd3/battle/legacy_battle_vertical_shift.hpp)；[src/battle/legacy_battle_vertical_shift.cpp](../../../src/battle/legacy_battle_vertical_shift.cpp)。
  - 类型：[PORT-b990ae590803](invoke-removal-ports.md#port-b990ae590803), [PORT-c85bfdf9142a](invoke-removal-ports.md#port-c85bfdf9142a)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/battle/legacy_battle_vertical_shift.hpp](../../../include/openswd3/battle/legacy_battle_vertical_shift.hpp)：67, 68
  - `operation` [include/openswd3/battle/legacy_battle_vertical_shift.hpp](../../../include/openswd3/battle/legacy_battle_vertical_shift.hpp)：49
  - `counter` [src/battle/legacy_battle_vertical_shift.cpp](../../../src/battle/legacy_battle_vertical_shift.cpp)：72, 89

- [ ] **CHAIN-487eecd6b011** `battle/legacy_battle_victory_item_list_panel`
  - 当前残留信号：arguments, counter, interface, invoke, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_victory_item_list_panel.hpp](../../../include/openswd3/battle/legacy_battle_victory_item_list_panel.hpp)；[src/battle/legacy_battle_victory_item_list_panel.cpp](../../../src/battle/legacy_battle_victory_item_list_panel.cpp)。
  - 类型：[PORT-e99bc2bc4419](invoke-removal-ports.md#port-e99bc2bc4419)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_victory_item_list_panel.hpp](../../../include/openswd3/battle/legacy_battle_victory_item_list_panel.hpp)：48
  - `counter` [include/openswd3/battle/legacy_battle_victory_item_list_panel.hpp](../../../include/openswd3/battle/legacy_battle_victory_item_list_panel.hpp)：120, 121, 122, 123, 124, 125, 126, 128
  - `invoke` [include/openswd3/battle/legacy_battle_victory_item_list_panel.hpp](../../../include/openswd3/battle/legacy_battle_victory_item_list_panel.hpp)：74
  - `register` [include/openswd3/battle/legacy_battle_victory_item_list_panel.hpp](../../../include/openswd3/battle/legacy_battle_victory_item_list_panel.hpp)：51, 52, 53, 59, 60, 61, 78, 79, 80, 84, 92, 93, 94, 117, 118, 119
  - `arguments` [src/battle/legacy_battle_victory_item_list_panel.cpp](../../../src/battle/legacy_battle_victory_item_list_panel.cpp)：70, 78
  - `counter` [src/battle/legacy_battle_victory_item_list_panel.cpp](../../../src/battle/legacy_battle_victory_item_list_panel.cpp)：117, 158, 189, 216, 241, 261, 323, 364
  - `invoke` [src/battle/legacy_battle_victory_item_list_panel.cpp](../../../src/battle/legacy_battle_victory_item_list_panel.cpp)：67, 92, 112, 201, 308, 349
  - `register` [src/battle/legacy_battle_victory_item_list_panel.cpp](../../../src/battle/legacy_battle_victory_item_list_panel.cpp)：45, 46, 81, 82, 83, 94, 95, 96, 132, 136, 137, 138, 142, 159, 160, 161, 172, 190, 191, 192, 224, 242, 243, 244, 262, 263, 264, 370, 371, 372

- [ ] **CHAIN-55f77acece7b** `battle/legacy_battle_victory_rewards`
  - 当前残留信号：arguments, counter, interface, invoke, operation, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_victory_rewards.hpp](../../../include/openswd3/battle/legacy_battle_victory_rewards.hpp)；[src/battle/legacy_battle_victory_rewards.cpp](../../../src/battle/legacy_battle_victory_rewards.cpp)。
  - 类型：[PORT-83b19a82edf3](invoke-removal-ports.md#port-83b19a82edf3), [PORT-4172aa8df321](invoke-removal-ports.md#port-4172aa8df321)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/battle/legacy_battle_victory_rewards.hpp](../../../include/openswd3/battle/legacy_battle_victory_rewards.hpp)：88
  - `counter` [include/openswd3/battle/legacy_battle_victory_rewards.hpp](../../../include/openswd3/battle/legacy_battle_victory_rewards.hpp)：204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214
  - `invoke` [include/openswd3/battle/legacy_battle_victory_rewards.hpp](../../../include/openswd3/battle/legacy_battle_victory_rewards.hpp)：131
  - `register` [include/openswd3/battle/legacy_battle_victory_rewards.hpp](../../../include/openswd3/battle/legacy_battle_victory_rewards.hpp)：89, 90, 91, 97, 98, 99, 117, 118, 119, 133, 134, 135, 140, 141, 145, 146, 169, 170, 171, 201, 202, 203
  - `arguments` [src/battle/legacy_battle_victory_rewards.cpp](../../../src/battle/legacy_battle_victory_rewards.cpp)：583, 610, 613, 614, 619
  - `counter` [src/battle/legacy_battle_victory_rewards.cpp](../../../src/battle/legacy_battle_victory_rewards.cpp)：186, 214, 256, 269, 275, 288, 319, 358, 388, 452, 511, 599
  - `invoke` [src/battle/legacy_battle_victory_rewards.cpp](../../../src/battle/legacy_battle_victory_rewards.cpp)：598, 624
  - `operation` [src/battle/legacy_battle_victory_rewards.cpp](../../../src/battle/legacy_battle_victory_rewards.cpp)：608, 617, 625
  - `register` [src/battle/legacy_battle_victory_rewards.cpp](../../../src/battle/legacy_battle_victory_rewards.cpp)：124, 125, 187, 188, 189, 215, 216, 217, 257, 258, 259, 270, 271, 272, 276, 277, 278, 289, 290, 291, 315, 316, 320, 321, 322, 448, 449, 453, 454, 455, 512, 513, 514, 592, 593, 594, 601, 602, 603, 620, 621, 622, 626, 627, 628, 652, 653, 654

- [ ] **CHAIN-edf68a3c3d1c** `platform/legacy_command_line`
  - 当前残留信号：arguments。
  - 历史状态：`excluded`；本快照：待复核。
  - 文件：[src/platform/sdl3/legacy_command_line.cpp](../../../src/platform/sdl3/legacy_command_line.cpp)；[src/platform/sdl3/legacy_command_line.hpp](../../../src/platform/sdl3/legacy_command_line.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [src/platform/sdl3/legacy_command_line.cpp](../../../src/platform/sdl3/legacy_command_line.cpp)：49, 50, 54, 55, 64, 68, 69
  - `arguments` [src/platform/sdl3/legacy_command_line.hpp](../../../src/platform/sdl3/legacy_command_line.hpp)：13

- [ ] **CHAIN-22c4bfa65c1e** `platform/legacy_sample_backend_sdl3`
  - 当前残留信号：operation。
  - 历史状态：`excluded`；本快照：待复核。
  - 文件：[src/platform/sdl3/legacy_sample_backend_sdl3.cpp](../../../src/platform/sdl3/legacy_sample_backend_sdl3.cpp)；[src/platform/sdl3/legacy_sample_backend_sdl3.hpp](../../../src/platform/sdl3/legacy_sample_backend_sdl3.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `operation` [src/platform/sdl3/legacy_sample_backend_sdl3.cpp](../../../src/platform/sdl3/legacy_sample_backend_sdl3.cpp)：475, 476
  - `operation` [src/platform/sdl3/legacy_sample_backend_sdl3.hpp](../../../src/platform/sdl3/legacy_sample_backend_sdl3.hpp)：116

- [ ] **CHAIN-0fe1c548029f** `platform/main`
  - 当前残留信号：arguments, counter, interface, invoke, operation, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)。
  - 类型：[PORT-7a54eff2cc37](invoke-removal-ports.md#port-7a54eff2cc37), [PORT-e9d262541f52](invoke-removal-ports.md#port-e9d262541f52), [PORT-845dde81be82](invoke-removal-ports.md#port-845dde81be82), [PORT-a77d845d4b85](invoke-removal-ports.md#port-a77d845d4b85), [PORT-70256fcd1195](invoke-removal-ports.md#port-70256fcd1195), [PORT-9100b117e286](invoke-removal-ports.md#port-9100b117e286), [PORT-03702411c458](invoke-removal-ports.md#port-03702411c458), [PORT-90a0dc6df60b](invoke-removal-ports.md#port-90a0dc6df60b), [PORT-6dd594fe2948](invoke-removal-ports.md#port-6dd594fe2948), [PORT-d4cc14c65328](invoke-removal-ports.md#port-d4cc14c65328), [PORT-f68cf6e0ed1e](invoke-removal-ports.md#port-f68cf6e0ed1e), [PORT-989ea10cefb9](invoke-removal-ports.md#port-989ea10cefb9), [PORT-c66d1579fe8c](invoke-removal-ports.md#port-c66d1579fe8c), [PORT-37479b034ea1](invoke-removal-ports.md#port-37479b034ea1), [PORT-d1f9bc8a7ebc](invoke-removal-ports.md#port-d1f9bc8a7ebc), [PORT-04a280de8386](invoke-removal-ports.md#port-04a280de8386), [PORT-154fc1309069](invoke-removal-ports.md#port-154fc1309069), [PORT-ede30e3e1fab](invoke-removal-ports.md#port-ede30e3e1fab), [PORT-8aa8e1888b1b](invoke-removal-ports.md#port-8aa8e1888b1b), [PORT-6f083a3b2e59](invoke-removal-ports.md#port-6f083a3b2e59), [PORT-e72684e65fc8](invoke-removal-ports.md#port-e72684e65fc8), [PORT-c25c01f66a08](invoke-removal-ports.md#port-c25c01f66a08), [PORT-f737d3106b78](invoke-removal-ports.md#port-f737d3106b78), [PORT-9cbf879ca236](invoke-removal-ports.md#port-9cbf879ca236), [PORT-c9ee92f626c0](invoke-removal-ports.md#port-c9ee92f626c0), [PORT-62d050835231](invoke-removal-ports.md#port-62d050835231), [PORT-2ed5441b3267](invoke-removal-ports.md#port-2ed5441b3267), [PORT-3f1ccdbb192c](invoke-removal-ports.md#port-3f1ccdbb192c), [PORT-af1a87092cd0](invoke-removal-ports.md#port-af1a87092cd0), [PORT-570f1316ada1](invoke-removal-ports.md#port-570f1316ada1), [PORT-4e1c3ee0b25e](invoke-removal-ports.md#port-4e1c3ee0b25e), [PORT-e54d86864b22](invoke-removal-ports.md#port-e54d86864b22), [PORT-62d8ed0cf5ce](invoke-removal-ports.md#port-62d8ed0cf5ce)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：3467, 3478, 3483, 3581, 3585, 3586, 3602, 3603, 3604, 3635, 3643, 3652, 3654, 3657, 3659, 3662, 3711, 3713, 3721, 3723, 9373, 9402, 9404, 9405, 9445, 9446
  - `counter` [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：4191
  - `invoke` [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：2220, 2294, 2748, 2759, 3297, 3406, 3431, 3446, 3524, 6056, 6061, 7257, 7345, 7549, 7554
  - `operation` [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：213, 216, 1127, 1128, 1216, 1219, 1221, 1223, 1225, 1229, 1233, 1245, 1254, 1263, 1272, 1281, 1287, 1292, 1297, 2594, 2596, 2602, 2609, 2616, 2623, 5992, 6023
  - `register` [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：2228, 2315, 2317, 2318, 2327, 2328, 2329, 2340, 2341, 2342, 2349, 2350, 2351, 2355, 2357, 3290, 3303, 3411, 3412, 3413, 3450, 3451, 3453, 3454, 3455, 3461, 3472, 3478, 3491, 3531, 3532, 3533, 3622, 3631, 3635, 3645, 3652, 3657, 3679, 3727, 3732, 3737, 3777, 4067, 4081, 4238, 4274, 4338

- [ ] **CHAIN-2c6602652dfa** `rendering/legacy_action_renderers`
  - 当前残留信号：counter, interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_action_renderers.hpp](../../../include/openswd3/rendering/legacy_action_renderers.hpp)；[src/rendering/legacy_action_renderers.cpp](../../../src/rendering/legacy_action_renderers.cpp)。
  - 类型：[PORT-2a8734c66051](invoke-removal-ports.md#port-2a8734c66051), [PORT-08f8d487dd54](invoke-removal-ports.md#port-08f8d487dd54)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/rendering/legacy_action_renderers.hpp](../../../include/openswd3/rendering/legacy_action_renderers.hpp)：70, 72
  - `counter` [src/rendering/legacy_action_renderers.cpp](../../../src/rendering/legacy_action_renderers.cpp)：111, 117

- [ ] **CHAIN-ddf4b1615395** `rendering/legacy_blitter`
  - 当前残留信号：arguments, operation。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[src/rendering/legacy_blitter.cpp](../../../src/rendering/legacy_blitter.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [src/rendering/legacy_blitter.cpp](../../../src/rendering/legacy_blitter.cpp)：1921, 1922, 1933, 1935, 1936, 1937, 1939
  - `operation` [src/rendering/legacy_blitter.cpp](../../../src/rendering/legacy_blitter.cpp)：749, 863, 980, 1017, 1024, 1032, 1679, 1688, 1697, 1707, 1718, 1729, 1740, 1750, 1761, 1770, 1780, 1789, 1798, 1807, 1817, 1827, 1836, 1845, 1855, 1865, 1875, 1886, 1895

- [ ] **CHAIN-d843511e8764** `rendering/legacy_countdown`
  - 当前残留信号：counter, interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_countdown.hpp](../../../include/openswd3/rendering/legacy_countdown.hpp)；[src/rendering/legacy_countdown.cpp](../../../src/rendering/legacy_countdown.cpp)。
  - 类型：[PORT-89846717adb9](invoke-removal-ports.md#port-89846717adb9), [PORT-ecf120dbfa3e](invoke-removal-ports.md#port-ecf120dbfa3e)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/rendering/legacy_countdown.hpp](../../../include/openswd3/rendering/legacy_countdown.hpp)：84
  - `counter` [src/rendering/legacy_countdown.cpp](../../../src/rendering/legacy_countdown.cpp)：101

- [ ] **CHAIN-5c132f0b4a10** `rendering/legacy_drawing_helpers`
  - 当前残留信号：counter。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_drawing_helpers.hpp](../../../include/openswd3/rendering/legacy_drawing_helpers.hpp)；[src/rendering/legacy_drawing_helpers.cpp](../../../src/rendering/legacy_drawing_helpers.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/rendering/legacy_drawing_helpers.hpp](../../../include/openswd3/rendering/legacy_drawing_helpers.hpp)：76
  - `counter` [src/rendering/legacy_drawing_helpers.cpp](../../../src/rendering/legacy_drawing_helpers.cpp)：178

- [ ] **CHAIN-b5e7d3072c55** `rendering/legacy_formatted_text`
  - 当前残留信号：counter。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_formatted_text.hpp](../../../include/openswd3/rendering/legacy_formatted_text.hpp)；[src/rendering/legacy_formatted_text.cpp](../../../src/rendering/legacy_formatted_text.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/rendering/legacy_formatted_text.hpp](../../../include/openswd3/rendering/legacy_formatted_text.hpp)：49
  - `counter` [src/rendering/legacy_formatted_text.cpp](../../../src/rendering/legacy_formatted_text.cpp)：63

- [ ] **CHAIN-05d2880615fa** `rendering/legacy_tiled_frame`
  - 当前残留信号：counter。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_tiled_frame.hpp](../../../include/openswd3/rendering/legacy_tiled_frame.hpp)；[src/rendering/legacy_tiled_frame.cpp](../../../src/rendering/legacy_tiled_frame.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/rendering/legacy_tiled_frame.hpp](../../../include/openswd3/rendering/legacy_tiled_frame.hpp)：47
  - `counter` [src/rendering/legacy_tiled_frame.cpp](../../../src/rendering/legacy_tiled_frame.cpp)：133

- [ ] **CHAIN-691dc3cab651** `resource_io/data_directory`
  - 当前残留信号：arguments。
  - 历史状态：`excluded`；本快照：待复核。
  - 文件：[include/openswd3/resource_io/data_directory.hpp](../../../include/openswd3/resource_io/data_directory.hpp)；[src/resource_io/data_directory.cpp](../../../src/resource_io/data_directory.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/resource_io/data_directory.hpp](../../../include/openswd3/resource_io/data_directory.hpp)：44
  - `arguments` [src/resource_io/data_directory.cpp](../../../src/resource_io/data_directory.cpp)：69, 72, 80, 82, 90, 100, 301, 306

- [ ] **CHAIN-a4bd9d63fd44** `special_modes/legacy_standard_mode`
  - 当前残留信号：arguments, counter, interface, invoke, operation, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)；[src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)。
  - 类型：[PORT-6150ecc45d27](invoke-removal-ports.md#port-6150ecc45d27), [PORT-b67baacff7a6](invoke-removal-ports.md#port-b67baacff7a6), [PORT-5a66171a78a0](invoke-removal-ports.md#port-5a66171a78a0), [PORT-0f6cfa14a197](invoke-removal-ports.md#port-0f6cfa14a197), [PORT-9d2976e0ec3e](invoke-removal-ports.md#port-9d2976e0ec3e), [PORT-2883e01a95f2](invoke-removal-ports.md#port-2883e01a95f2), [PORT-226b70af16c1](invoke-removal-ports.md#port-226b70af16c1), [PORT-60173815f0c7](invoke-removal-ports.md#port-60173815f0c7), [PORT-15885508395d](invoke-removal-ports.md#port-15885508395d), [PORT-082801c27f01](invoke-removal-ports.md#port-082801c27f01), [PORT-b2d3ee9c6ec2](invoke-removal-ports.md#port-b2d3ee9c6ec2), [PORT-890017339787](invoke-removal-ports.md#port-890017339787), [PORT-5ac1fc8dbe44](invoke-removal-ports.md#port-5ac1fc8dbe44), [PORT-30a39f55d9df](invoke-removal-ports.md#port-30a39f55d9df), [PORT-4f2130341b89](invoke-removal-ports.md#port-4f2130341b89), [PORT-98f0f9166d53](invoke-removal-ports.md#port-98f0f9166d53), [PORT-bbd2451bdf89](invoke-removal-ports.md#port-bbd2451bdf89), [PORT-4e264659f423](invoke-removal-ports.md#port-4e264659f423), [PORT-1a79a30ffeff](invoke-removal-ports.md#port-1a79a30ffeff), [PORT-11f51aa89fa6](invoke-removal-ports.md#port-11f51aa89fa6), [PORT-401ec68b803d](invoke-removal-ports.md#port-401ec68b803d), [PORT-cb2a6f512755](invoke-removal-ports.md#port-cb2a6f512755), [PORT-b43455f5e226](invoke-removal-ports.md#port-b43455f5e226), [PORT-e8dd396ef2c9](invoke-removal-ports.md#port-e8dd396ef2c9), [PORT-301205ee587e](invoke-removal-ports.md#port-301205ee587e), [PORT-9ef02929206c](invoke-removal-ports.md#port-9ef02929206c), [PORT-c234f0393e38](invoke-removal-ports.md#port-c234f0393e38), [PORT-b83afa9c93eb](invoke-removal-ports.md#port-b83afa9c93eb), [PORT-0a31ecd7834c](invoke-removal-ports.md#port-0a31ecd7834c), [PORT-b32e1b2cdc82](invoke-removal-ports.md#port-b32e1b2cdc82), [PORT-a98dd2e88ca0](invoke-removal-ports.md#port-a98dd2e88ca0), [PORT-08f7de4b73f0](invoke-removal-ports.md#port-08f7de4b73f0), [PORT-190f4c266df9](invoke-removal-ports.md#port-190f4c266df9), [PORT-7b1f377f89e8](invoke-removal-ports.md#port-7b1f377f89e8), [PORT-a0611b019956](invoke-removal-ports.md#port-a0611b019956), [PORT-8df545dcd7c2](invoke-removal-ports.md#port-8df545dcd7c2), [PORT-27aeb06d8cfd](invoke-removal-ports.md#port-27aeb06d8cfd), [PORT-860e4fa62995](invoke-removal-ports.md#port-860e4fa62995), [PORT-27fbf26e4f06](invoke-removal-ports.md#port-27fbf26e4f06), [PORT-77debf8b3c2c](invoke-removal-ports.md#port-77debf8b3c2c), [PORT-69b0e4310f94](invoke-removal-ports.md#port-69b0e4310f94), [PORT-764b39aa7ad7](invoke-removal-ports.md#port-764b39aa7ad7), [PORT-1f08ab418d75](invoke-removal-ports.md#port-1f08ab418d75), [PORT-c8fab518a22f](invoke-removal-ports.md#port-c8fab518a22f), [PORT-d600df7b54bd](invoke-removal-ports.md#port-d600df7b54bd), [PORT-5f33d6080f39](invoke-removal-ports.md#port-5f33d6080f39), [PORT-4c7f47a3eb96](invoke-removal-ports.md#port-4c7f47a3eb96), [PORT-604d25c6e388](invoke-removal-ports.md#port-604d25c6e388), [PORT-f16180caea76](invoke-removal-ports.md#port-f16180caea76), [PORT-9e3fe2840361](invoke-removal-ports.md#port-9e3fe2840361), [PORT-98b07ad2c5e5](invoke-removal-ports.md#port-98b07ad2c5e5), [PORT-4ac389c112f2](invoke-removal-ports.md#port-4ac389c112f2), [PORT-1b5618d52c4d](invoke-removal-ports.md#port-1b5618d52c4d), [PORT-9d47b870c013](invoke-removal-ports.md#port-9d47b870c013), [PORT-ecf7b561c363](invoke-removal-ports.md#port-ecf7b561c363), [PORT-c642760a2098](invoke-removal-ports.md#port-c642760a2098), [PORT-650ab29bc2af](invoke-removal-ports.md#port-650ab29bc2af), [PORT-f448de2d7d21](invoke-removal-ports.md#port-f448de2d7d21), [PORT-a9f046e81eb1](invoke-removal-ports.md#port-a9f046e81eb1), [PORT-4bc9d20f3e26](invoke-removal-ports.md#port-4bc9d20f3e26), [PORT-071862d78d6d](invoke-removal-ports.md#port-071862d78d6d), [PORT-4fae7fa4785d](invoke-removal-ports.md#port-4fae7fa4785d), [PORT-be3fbc993329](invoke-removal-ports.md#port-be3fbc993329), [PORT-3ade156458ce](invoke-removal-ports.md#port-3ade156458ce), [PORT-c81606c60245](invoke-removal-ports.md#port-c81606c60245), [PORT-78ad16d29f4e](invoke-removal-ports.md#port-78ad16d29f4e), [PORT-abf788bb8c88](invoke-removal-ports.md#port-abf788bb8c88), [PORT-74a54ac015d2](invoke-removal-ports.md#port-74a54ac015d2), [PORT-cad6ba627883](invoke-removal-ports.md#port-cad6ba627883), [PORT-0e6fd985c0f9](invoke-removal-ports.md#port-0e6fd985c0f9), [PORT-ec7da6e39a5e](invoke-removal-ports.md#port-ec7da6e39a5e), [PORT-a335a80f59c9](invoke-removal-ports.md#port-a335a80f59c9), [PORT-388be6ad964d](invoke-removal-ports.md#port-388be6ad964d), [PORT-79248d20d342](invoke-removal-ports.md#port-79248d20d342), [PORT-4bc67aa34e87](invoke-removal-ports.md#port-4bc67aa34e87), [PORT-faeb0576fd0f](invoke-removal-ports.md#port-faeb0576fd0f), [PORT-26b9671db8fa](invoke-removal-ports.md#port-26b9671db8fa), [PORT-ad2f39b67ec2](invoke-removal-ports.md#port-ad2f39b67ec2), [PORT-d76783483169](invoke-removal-ports.md#port-d76783483169), [PORT-f7be2b98551a](invoke-removal-ports.md#port-f7be2b98551a), [PORT-84be156866b8](invoke-removal-ports.md#port-84be156866b8), [PORT-358eb3ea4fad](invoke-removal-ports.md#port-358eb3ea4fad), [PORT-a6aad0a268b8](invoke-removal-ports.md#port-a6aad0a268b8), [PORT-657571ee675d](invoke-removal-ports.md#port-657571ee675d), [PORT-3c29ec0d4f6a](invoke-removal-ports.md#port-3c29ec0d4f6a), [PORT-5d8eb418d0ae](invoke-removal-ports.md#port-5d8eb418d0ae), [PORT-e100d690c49a](invoke-removal-ports.md#port-e100d690c49a), [PORT-2e3ec804b45e](invoke-removal-ports.md#port-2e3ec804b45e), [PORT-eca06f1d07b0](invoke-removal-ports.md#port-eca06f1d07b0), [PORT-e91405a267f7](invoke-removal-ports.md#port-e91405a267f7), [PORT-d643a4747dda](invoke-removal-ports.md#port-d643a4747dda)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `arguments` [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：228, 519, 791, 6228
  - `counter` [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：332, 602, 659, 672, 731, 734, 885, 894, 904, 972, 999, 1032, 1053, 1054, 1080, 1112, 1619, 1647, 1729, 1767, 1805, 1848, 1890, 1920, 1947, 1970, 2040, 2072, 2082, 2108, 2424, 2547, 2548, 2667, 2839, 2942, 2982, 3023, 3063, 3099, 3140, 3205, 3270, 3326, 3423, 3583, 4123, 4251, 4324, 4352, 4424, 4442, 4467, 4485, 4507, 4549, 4578, 4627, 4656, 4958, 5022, 5075, 5100, 5201, 5239, 5467, 5556, 5768, 5784, 5802, 5908, 5925, 5942, 5966, 5992, 6034, 6070, 6076, 6118, 6178, 6282, 6283, 6308, 6319, 6549, 6731, 7511, 7537, 7611
  - `invoke` [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：1140, 1392, 2025, 2057, 2093, 4746, 5564, 5720, 6327
  - `operation` [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：2360, 3358, 4680, 4919, 5648, 6259
  - `arguments` [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：1781, 1785, 3025, 3031, 3093, 3099, 3109, 3112, 4275, 4279, 16056, 16062, 16409, 16416
  - `counter` [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：370, 381, 417, 423, 433, 535, 587, 596, 630, 664, 671, 786, 924, 1060, 1118, 1184, 1304, 1438, 1445, 1455, 1471, 1487, 1495, 1504, 1513, 1788, 1816, 1817, 1826, 1976, 2178, 2357, 2360, 2369, 2498, 2501, 2888, 2891, 2908, 2929, 2939, 2967, 2977, 2998, 3006, 3035, 3103, 3278, 3348, 3461, 3472, 3494, 3517, 3533, 3543, 3557, 3584, 3592, 3602, 3608, 3666, 3691, 3704, 3726, 3731, 3765, 3873, 3888, 3906, 3971, 3986, 4004, 4069, 4079, 4221, 4225, 4234, 4248, 4249, 4285, 4324, 4358, 4363, 4369, 4381, 4398, 4403, 4431, 4454, 4466, 4476, 4484, 4499, 4507, 4528, 4537, 4604, 4616, 4645, 4755, 4764, 5238, 5240, 5380, 5396, 5643, 5647, 5650, 5655, 5902, 5920, 5951, 6102, 6108, 6125, 6135, 6143, 6150, 6170, 6231, 6237, 6254, 6264, 6272, 6279, 6299, 6359, 6362, 6395, 6405, 6413, 6420, 6440, 6499, 6502, 6520, 6530, 6541, 6561, 6607, 6628, 6693, 6710, 6726, 6739, 6787, 6828, 6848, 6864, 6876, 6895, 6919, 6934, 6966, 6985, 7005, 7022, 7075, 7091, 7136, 7181, 7192, 7207, 7242, 7246, 7263, 7308, 7398, 7420, 7441, 7463, 7484, 7499, 7519, 7545, 7584, 7603, 7621, 7639, 7656, 7668, 7685, 7707, 7766, 7939, 7962, 7987, 8010, 8060, 8197, 8289, 8364, 8478, 8536, 8551, 8798, 8812, 8822, 8837, 10347, 10375, 10386, 10475, 10477, 10479, 10503, 10513, 10520, 10523, 10540, 10558, 10561, 10574, 10577, 10580, 10592, 10607, 10635, 10652, 10662, 10671, 10789, 10802, 10814, 10820, 10909, 10919, 10932, 10942, 10953, 10956, 10998, 11008, 11016, 11041, 11081, 11091, 11099, 11130, 11741, 11744, 11772, 11811, 11830, 11847, 11893, 11928, 11942, 11980, 11986, 12004, 12014, 12039, 12050, 12078, 12113, 12135, 12148, 12164, 12174, 12182, 12198, 12230, 12233, 12250, 12258, 12263, 12273, 12283, 12292, 12297, 12331, 12335, 12346, 12356, 12365, 12389, 12445, 12449, 12478, 12488, 12496, 12527, 12562, 12567, 12576, 12586, 12595, 12624, 12661, 12666, 12679, 12689, 12698, 12730, 13228, 13239, 14405, 14857, 14876, 14884, 14893, 14898, 15351, 15359, 15364, 15369, 15372, 15383, 15395, 15437, 15442, 15461, 15605, 15637, 15649, 15671, 15695, 15731, 15741, 15747, 15749, 15757, 15762, 15770, 16016, 16018, 16021, 16027, 16035, 16066, 16081, 16087, 16185, 16219, 16220, 16225, 16422, 16486, 16498, 16500, 16503, 16681, 16711, 16749, 16798, 16845, 16860, 16867, 16916, 16997, 17071, 17080, 17087, 17103, 17110, 17141, 17158, 17171, 17181, 17191, 17200, 17216, 17227, 17231, 17245, 17256, 17260, 17278, 17290, 17296, 17300, 17318, 17337, 17384, 17400, 17416, 17471, 17490, 17505, 17508, 17518, 17532, 17569, 17588, 17604, 17607, 17617, 17629, 17633, 17664, 17672, 17677, 17682, 17685, 17696, 17705, 17717, 17751, 17759, 17764, 17769, 17772, 17783, 17795, 17826, 17834, 17839, 17844, 17847, 17858, 17867, 17879, 18364, 18374, 18446, 18456, 18688, 18701, 18710, 18727, 18749, 18797, 18819, 18851, 18865, 18897, 18911, 18918, 18931, 18941, 18953, 19062, 19072, 19084, 19158, 19165, 19188, 19208, 19214, 19235, 19247, 19300, 19312, 19329, 19338, 19348, 19364, 19375, 19389, 19393, 19411, 19417, 19427, 19449, 19462, 20477, 20482, 20489, 20504, 20507, 20516, 20541, 20965, 20968, 20988, 21000, 21012, 21037, 22690, 22699, 22713, 22723, 22733, 22744, 22757, 22769, 22811, 22833, 22855, 22962, 22982, 23002, 23034, 23071, 23123, 23135, 23144, 23161, 23193, 23194, 23215, 23216, 23238, 23258, 23318, 23319, 23342, 23343, 23364, 23365, 23406, 23469, 23499, 23519, 23523, 23528, 23548, 23558, 23590, 23624, 23654, 23681, 23701, 23705, 23710, 23730, 23740, 23773, 23801, 23835, 23840, 23843, 23864, 23874, 23898, 23919, 23923, 23928, 23948, 23958, 23989, 24018, 24048, 24073, 24090, 24094, 24099, 24119, 24129, 24159, 24179, 24210, 24220, 24223, 24244, 24253, 24267, 24281, 24289, 24308, 24318, 24342, 24368, 24390, 24403, 24411, 24430, 24440, 24470, 24497, 24509, 24530, 24541, 24550, 24576, 24619, 24637, 24644, 24841, 24861, 24992, 25009, 25023, 25024, 25119, 25138, 25143, 25149, 25182, 25195, 25203, 25208, 25249, 25259, 25270, 25281, 25293, 25312, 25323, 25357, 25367, 25385, 25395, 25401, 25404, 25426, 25444, 25459, 25505, 25518, 25538, 25542, 25548, 25557, 25561, 25578, 25594, 25631, 25644, 25762, 25778, 25786, 25814, 25824, 25846, 25895, 25907, 25911, 25927
  - `invoke` [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：315, 320, 12789, 12963, 12973, 12985, 13055, 13150, 13185, 13216, 17985, 17990, 18003, 18016, 18023, 18029, 18037, 18060, 18074, 18090, 18107, 18116, 18132, 18139, 18144, 18164, 18170, 18175, 18184, 18190, 18195, 20590, 20911, 22255, 22357, 22361, 22366, 22375, 22387, 22392, 22397, 22402, 22407, 22412, 22423, 22433, 22440, 25777, 25823, 25894
  - `operation` [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：4992, 5024, 5044, 5059, 5148, 5152, 7881, 7887, 11154, 16414, 16421, 19485, 19562, 19597, 19615, 19638, 19718, 19802, 19934, 19940, 24592
  - `register` [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：3265, 3266, 3267, 22022, 22023

- [ ] **CHAIN-a1dd3f38c83c** `world_map/legacy_random_encounter`
  - 当前残留信号：counter, interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_random_encounter.hpp](../../../include/openswd3/world_map/legacy_random_encounter.hpp)；[src/world_map/legacy_random_encounter.cpp](../../../src/world_map/legacy_random_encounter.cpp)。
  - 类型：[PORT-a955630ca2d7](invoke-removal-ports.md#port-a955630ca2d7)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/world_map/legacy_random_encounter.hpp](../../../include/openswd3/world_map/legacy_random_encounter.hpp)：125
  - `counter` [src/world_map/legacy_random_encounter.cpp](../../../src/world_map/legacy_random_encounter.cpp)：205, 278

- [ ] **CHAIN-0d8b21a0dd24** `world_map/legacy_world_debug_hotkeys`
  - 当前残留信号：counter, interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_debug_hotkeys.hpp](../../../include/openswd3/world_map/legacy_world_debug_hotkeys.hpp)；[src/world_map/legacy_world_debug_hotkeys.cpp](../../../src/world_map/legacy_world_debug_hotkeys.cpp)。
  - 类型：[PORT-5d0a423dd6fb](invoke-removal-ports.md#port-5d0a423dd6fb)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/world_map/legacy_world_debug_hotkeys.hpp](../../../include/openswd3/world_map/legacy_world_debug_hotkeys.hpp)：79
  - `counter` [src/world_map/legacy_world_debug_hotkeys.cpp](../../../src/world_map/legacy_world_debug_hotkeys.cpp)：68

- [ ] **CHAIN-1732ead91ec1** `world_map/legacy_world_debug_overlay`
  - 当前残留信号：arguments, counter, interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_debug_overlay.hpp](../../../include/openswd3/world_map/legacy_world_debug_overlay.hpp)；[src/world_map/legacy_world_debug_overlay.cpp](../../../src/world_map/legacy_world_debug_overlay.cpp)。
  - 类型：[PORT-0004f3a1aedd](invoke-removal-ports.md#port-0004f3a1aedd)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/world_map/legacy_world_debug_overlay.hpp](../../../include/openswd3/world_map/legacy_world_debug_overlay.hpp)：89
  - `arguments` [src/world_map/legacy_world_debug_overlay.cpp](../../../src/world_map/legacy_world_debug_overlay.cpp)：241, 245
  - `counter` [src/world_map/legacy_world_debug_overlay.cpp](../../../src/world_map/legacy_world_debug_overlay.cpp)：207

- [ ] **CHAIN-daf77d99d354** `world_map/legacy_world_frame_composition`
  - 当前残留信号：counter, interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_frame_composition.hpp](../../../include/openswd3/world_map/legacy_world_frame_composition.hpp)；[src/world_map/legacy_world_frame_composition.cpp](../../../src/world_map/legacy_world_frame_composition.cpp)。
  - 类型：[PORT-911f7d3cb272](invoke-removal-ports.md#port-911f7d3cb272)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/world_map/legacy_world_frame_composition.hpp](../../../include/openswd3/world_map/legacy_world_frame_composition.hpp)：91
  - `counter` [src/world_map/legacy_world_frame_composition.cpp](../../../src/world_map/legacy_world_frame_composition.cpp)：110

- [ ] **CHAIN-623b1461f240** `world_map/legacy_world_item_lifecycle`
  - 当前残留信号：counter, interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_item_lifecycle.hpp](../../../include/openswd3/world_map/legacy_world_item_lifecycle.hpp)；[src/world_map/legacy_world_item_lifecycle.cpp](../../../src/world_map/legacy_world_item_lifecycle.cpp)。
  - 类型：[PORT-3e4b216ba4ea](invoke-removal-ports.md#port-3e4b216ba4ea)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/world_map/legacy_world_item_lifecycle.hpp](../../../include/openswd3/world_map/legacy_world_item_lifecycle.hpp)：118
  - `counter` [src/world_map/legacy_world_item_lifecycle.cpp](../../../src/world_map/legacy_world_item_lifecycle.cpp)：10

- [ ] **CHAIN-9bfde3789c71** `world_map/legacy_world_map_role_paths`
  - 当前残留信号：counter, interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_map_role_paths.hpp](../../../include/openswd3/world_map/legacy_world_map_role_paths.hpp)；[src/world_map/legacy_world_map_role_paths.cpp](../../../src/world_map/legacy_world_map_role_paths.cpp)。
  - 类型：[PORT-c08113b1b719](invoke-removal-ports.md#port-c08113b1b719)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/world_map/legacy_world_map_role_paths.hpp](../../../include/openswd3/world_map/legacy_world_map_role_paths.hpp)：70
  - `counter` [src/world_map/legacy_world_map_role_paths.cpp](../../../src/world_map/legacy_world_map_role_paths.cpp)：271

- [ ] **CHAIN-ca32d6b584f3** `world_map/legacy_world_role_lifecycle`
  - 当前残留信号：counter, interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_role_lifecycle.hpp](../../../include/openswd3/world_map/legacy_world_role_lifecycle.hpp)；[src/world_map/legacy_world_role_lifecycle.cpp](../../../src/world_map/legacy_world_role_lifecycle.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `counter` [include/openswd3/world_map/legacy_world_role_lifecycle.hpp](../../../include/openswd3/world_map/legacy_world_role_lifecycle.hpp)：83, 85, 86
  - `counter` [src/world_map/legacy_world_role_lifecycle.cpp](../../../src/world_map/legacy_world_role_lifecycle.cpp)：180, 224, 242

- [ ] **CHAIN-5c131d235518** `world_map/legacy_world_role_surface_occupancy`
  - 当前残留信号：operation。
  - 历史状态：`excluded`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_role_surface_occupancy.cpp](../../../src/world_map/legacy_world_role_surface_occupancy.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `operation` [src/world_map/legacy_world_role_surface_occupancy.cpp](../../../src/world_map/legacy_world_role_surface_occupancy.cpp)：49, 73

- [ ] **CHAIN-d44414d75dab** `world_map/legacy_world_story_vm`
  - 当前残留信号：interface, operation, register。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_story_vm.hpp](../../../include/openswd3/world_map/legacy_world_story_vm.hpp)；[src/world_map/legacy_world_story_vm.cpp](../../../src/world_map/legacy_world_story_vm.cpp)。
  - 类型：[PORT-80c4115ee102](invoke-removal-ports.md#port-80c4115ee102), [PORT-a00d387a8a8c](invoke-removal-ports.md#port-a00d387a8a8c), [PORT-9193c3470f12](invoke-removal-ports.md#port-9193c3470f12)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
  - `operation` [include/openswd3/world_map/legacy_world_story_vm.hpp](../../../include/openswd3/world_map/legacy_world_story_vm.hpp)：509
  - `operation` [src/world_map/legacy_world_story_vm.cpp](../../../src/world_map/legacy_world_story_vm.cpp)：5045, 5046, 5048, 5050, 7254, 7259
  - `register` [src/world_map/legacy_world_story_vm.cpp](../../../src/world_map/legacy_world_story_vm.cpp)：270, 271, 272

- [ ] **CHAIN-c4efce45b978** `app/auxiliary_text_dialog`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/app/auxiliary_text_dialog.hpp](../../../include/openswd3/app/auxiliary_text_dialog.hpp)；[src/app/auxiliary_text_dialog.cpp](../../../src/app/auxiliary_text_dialog.cpp)。
  - 类型：[PORT-187e0eae1881](invoke-removal-ports.md#port-187e0eae1881)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-1657e95023e0** `app/battle_transition`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/app/battle_transition.hpp](../../../include/openswd3/app/battle_transition.hpp)；[src/app/battle_transition.cpp](../../../src/app/battle_transition.cpp)。
  - 类型：[PORT-b6ef63f2e8e5](invoke-removal-ports.md#port-b6ef63f2e8e5)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-8db48edc261e** `app/command_line`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/app/command_line.hpp](../../../include/openswd3/app/command_line.hpp)；[src/app/command_line.cpp](../../../src/app/command_line.cpp)。
  - 类型：[PORT-a211718f6100](invoke-removal-ports.md#port-a211718f6100), [PORT-579677afd1a8](invoke-removal-ports.md#port-579677afd1a8)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-9cf666759578** `app/external_launch`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/app/external_launch.hpp](../../../include/openswd3/app/external_launch.hpp)；[src/app/external_launch.cpp](../../../src/app/external_launch.cpp)。
  - 类型：[PORT-900aed6aeea4](invoke-removal-ports.md#port-900aed6aeea4)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-ee611b67e5af** `app/frame_runtime`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/app/frame_runtime.hpp](../../../include/openswd3/app/frame_runtime.hpp)；[src/app/frame_runtime.cpp](../../../src/app/frame_runtime.cpp)。
  - 类型：[PORT-45b0eb6aaab0](invoke-removal-ports.md#port-45b0eb6aaab0)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-17a917c916a3** `app/host_window_event`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/app/host_window_event.hpp](../../../include/openswd3/app/host_window_event.hpp)；[src/app/host_window_event.cpp](../../../src/app/host_window_event.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-8360f1767e6c** `app/idle_runtime`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/app/idle_runtime.hpp](../../../include/openswd3/app/idle_runtime.hpp)；[src/app/idle_runtime.cpp](../../../src/app/idle_runtime.cpp)。
  - 类型：[PORT-95e765f3bcaf](invoke-removal-ports.md#port-95e765f3bcaf)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-ab5e20069d62** `app/initialization`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/app/initialization.hpp](../../../include/openswd3/app/initialization.hpp)；[src/app/initialization.cpp](../../../src/app/initialization.cpp)。
  - 类型：[PORT-34f546f6ea95](invoke-removal-ports.md#port-34f546f6ea95)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-7526fad6a960** `app/legacy_new_game_transition`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/app/legacy_new_game_transition.hpp](../../../include/openswd3/app/legacy_new_game_transition.hpp)；[src/app/legacy_new_game_transition.cpp](../../../src/app/legacy_new_game_transition.cpp)。
  - 类型：[PORT-db41a19a4f4f](invoke-removal-ports.md#port-db41a19a4f4f)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-807b758eb669** `app/platform_backend_initialization`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/app/platform_backend_initialization.hpp](../../../include/openswd3/app/platform_backend_initialization.hpp)；[src/app/platform_backend_initialization.cpp](../../../src/app/platform_backend_initialization.cpp)。
  - 类型：[PORT-e6ec14094bc4](invoke-removal-ports.md#port-e6ec14094bc4)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-adad3be76227** `app/process_startup`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/app/process_startup.hpp](../../../include/openswd3/app/process_startup.hpp)；[src/app/process_startup.cpp](../../../src/app/process_startup.cpp)。
  - 类型：[PORT-94b597ca4098](invoke-removal-ports.md#port-94b597ca4098)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-7d264e7dc94b** `app/screenshot`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/app/screenshot.hpp](../../../include/openswd3/app/screenshot.hpp)；[src/app/screenshot.cpp](../../../src/app/screenshot.cpp)。
  - 类型：[PORT-331ae1e7a096](invoke-removal-ports.md#port-331ae1e7a096)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-b8af85a1c22a** `app/startup`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/app/startup.hpp](../../../include/openswd3/app/startup.hpp)；[src/app/startup.cpp](../../../src/app/startup.cpp)。
  - 类型：[PORT-d99e02f3609c](invoke-removal-ports.md#port-d99e02f3609c)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-9cbd6a882843** `app/startup_dialog`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/app/startup_dialog.hpp](../../../include/openswd3/app/startup_dialog.hpp)；[src/app/startup_dialog.cpp](../../../src/app/startup_dialog.cpp)。
  - 类型：[PORT-0271e5d485cf](invoke-removal-ports.md#port-0271e5d485cf)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-0eb65d1f3004** `app/window_events`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/app/window_events.hpp](../../../include/openswd3/app/window_events.hpp)；[src/app/window_events.cpp](../../../src/app/window_events.cpp)。
  - 类型：[PORT-e229428027e4](invoke-removal-ports.md#port-e229428027e4), [PORT-7bbd2a19d876](invoke-removal-ports.md#port-7bbd2a19d876), [PORT-98ae3f027e5d](invoke-removal-ports.md#port-98ae3f027e5d)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-5fc7c93ef05b** `asset_runtime/legacy_action_draw_bridge`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/asset_runtime/legacy_action_draw_bridge.hpp](../../../include/openswd3/asset_runtime/legacy_action_draw_bridge.hpp)；[src/asset_runtime/legacy_action_draw_bridge.cpp](../../../src/asset_runtime/legacy_action_draw_bridge.cpp)。
  - 类型：[PORT-a736bd231a94](invoke-removal-ports.md#port-a736bd231a94), [PORT-7bc64e8c1dc4](invoke-removal-ports.md#port-7bc64e8c1dc4)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-f9deee1b0049** `asset_runtime/legacy_ani_activity`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/asset_runtime/legacy_ani_activity.hpp](../../../include/openswd3/asset_runtime/legacy_ani_activity.hpp)；[src/asset_runtime/legacy_ani_activity.cpp](../../../src/asset_runtime/legacy_ani_activity.cpp)。
  - 类型：[PORT-f054803d8402](invoke-removal-ports.md#port-f054803d8402)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-f762841cfcba** `asset_runtime/legacy_ani_drift_effect`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/asset_runtime/legacy_ani_drift_effect.hpp](../../../include/openswd3/asset_runtime/legacy_ani_drift_effect.hpp)；[src/asset_runtime/legacy_ani_drift_effect.cpp](../../../src/asset_runtime/legacy_ani_drift_effect.cpp)。
  - 类型：[PORT-67ef5828086f](invoke-removal-ports.md#port-67ef5828086f), [PORT-09c4355bd91b](invoke-removal-ports.md#port-09c4355bd91b), [PORT-abb3faee5b8d](invoke-removal-ports.md#port-abb3faee5b8d)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-2b12700ff6a4** `asset_runtime/legacy_ani_follower_effect`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/asset_runtime/legacy_ani_follower_effect.hpp](../../../include/openswd3/asset_runtime/legacy_ani_follower_effect.hpp)；[src/asset_runtime/legacy_ani_follower_effect.cpp](../../../src/asset_runtime/legacy_ani_follower_effect.cpp)。
  - 类型：[PORT-e8fbbb8c7e03](invoke-removal-ports.md#port-e8fbbb8c7e03), [PORT-e8bc0053f21d](invoke-removal-ports.md#port-e8bc0053f21d)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-64c4470c49b7** `asset_runtime/legacy_ani_spark_effect`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/asset_runtime/legacy_ani_spark_effect.hpp](../../../include/openswd3/asset_runtime/legacy_ani_spark_effect.hpp)；[src/asset_runtime/legacy_ani_spark_effect.cpp](../../../src/asset_runtime/legacy_ani_spark_effect.cpp)。
  - 类型：[PORT-ccd99c810ed4](invoke-removal-ports.md#port-ccd99c810ed4)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-0129f3368c24** `asset_runtime/legacy_ani_streak_effect`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/asset_runtime/legacy_ani_streak_effect.hpp](../../../include/openswd3/asset_runtime/legacy_ani_streak_effect.hpp)；[src/asset_runtime/legacy_ani_streak_effect.cpp](../../../src/asset_runtime/legacy_ani_streak_effect.cpp)。
  - 类型：[PORT-73b0991c193b](invoke-removal-ports.md#port-73b0991c193b)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-3db8bc53a7b0** `asset_runtime/legacy_asset_cache_limits`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/asset_runtime/legacy_asset_cache_limits.hpp](../../../include/openswd3/asset_runtime/legacy_asset_cache_limits.hpp)；[src/asset_runtime/legacy_asset_cache_limits.cpp](../../../src/asset_runtime/legacy_asset_cache_limits.cpp)。
  - 类型：[PORT-5ec7908b0939](invoke-removal-ports.md#port-5ec7908b0939)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-24695e768f2e** `audio_video/legacy_audio_coordination`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/audio_video/legacy_audio_coordination.hpp](../../../include/openswd3/audio_video/legacy_audio_coordination.hpp)；[src/audio_video/legacy_audio_coordination.cpp](../../../src/audio_video/legacy_audio_coordination.cpp)。
  - 类型：[PORT-7f9d00e1572c](invoke-removal-ports.md#port-7f9d00e1572c)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-09de7d0e761d** `audio_video/legacy_media_acquisition`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/audio_video/legacy_media_acquisition.hpp](../../../include/openswd3/audio_video/legacy_media_acquisition.hpp)；[src/audio_video/legacy_media_acquisition.cpp](../../../src/audio_video/legacy_media_acquisition.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-3ea8dd1ad454** `audio_video/legacy_video`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/audio_video/legacy_video.hpp](../../../include/openswd3/audio_video/legacy_video.hpp)；[src/audio_video/legacy_video.cpp](../../../src/audio_video/legacy_video.cpp)。
  - 类型：[PORT-cbc1b7b01790](invoke-removal-ports.md#port-cbc1b7b01790)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-a006e2bd831d** `audio_video/legacy_world_music`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/audio_video/legacy_world_music.hpp](../../../include/openswd3/audio_video/legacy_world_music.hpp)；[src/audio_video/legacy_world_music.cpp](../../../src/audio_video/legacy_world_music.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-7443d06cd0fc** `battle/legacy_battle_action_rotation_resources`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_action_rotation_resources.hpp](../../../include/openswd3/battle/legacy_battle_action_rotation_resources.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-3ba8001da183** `battle/legacy_battle_actor_action_kind`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_action_kind.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_kind.hpp)；[src/battle/legacy_battle_actor_action_kind.cpp](../../../src/battle/legacy_battle_actor_action_kind.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-087fd718e640** `battle/legacy_battle_actor_action_mode`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_action_mode.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_mode.hpp)；[src/battle/legacy_battle_actor_action_mode.cpp](../../../src/battle/legacy_battle_actor_action_mode.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-e4f9e427e384** `battle/legacy_battle_actor_availability_block`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_availability_block.hpp](../../../include/openswd3/battle/legacy_battle_actor_availability_block.hpp)；[src/battle/legacy_battle_actor_availability_block.cpp](../../../src/battle/legacy_battle_actor_availability_block.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-2a3b0639c609** `battle/legacy_battle_actor_base_initialization`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_base_initialization.hpp](../../../include/openswd3/battle/legacy_battle_actor_base_initialization.hpp)；[include/openswd3/battle/legacy_battle_actor_lifecycle.hpp](../../../include/openswd3/battle/legacy_battle_actor_lifecycle.hpp)；[src/battle/legacy_battle_actor_base_initialization.cpp](../../../src/battle/legacy_battle_actor_base_initialization.cpp)；[src/battle/legacy_battle_actor_lifecycle.cpp](../../../src/battle/legacy_battle_actor_lifecycle.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-8dfba362685e** `battle/legacy_battle_actor_base_release`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_base_release.hpp](../../../include/openswd3/battle/legacy_battle_actor_base_release.hpp)；[include/openswd3/battle/legacy_battle_actor_lifecycle.hpp](../../../include/openswd3/battle/legacy_battle_actor_lifecycle.hpp)；[src/battle/legacy_battle_actor_base_release.cpp](../../../src/battle/legacy_battle_actor_base_release.cpp)；[src/battle/legacy_battle_actor_lifecycle.cpp](../../../src/battle/legacy_battle_actor_lifecycle.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-359382e0d7e0** `battle/legacy_battle_actor_display_kind`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_display_kind.hpp](../../../include/openswd3/battle/legacy_battle_actor_display_kind.hpp)；[src/battle/legacy_battle_actor_display_kind.cpp](../../../src/battle/legacy_battle_actor_display_kind.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-44345655d97c** `battle/legacy_battle_actor_field_26b8_high_bit_clear`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_field_26b8_high_bit_clear.hpp](../../../include/openswd3/battle/legacy_battle_actor_field_26b8_high_bit_clear.hpp)；[src/battle/legacy_battle_actor_field_26b8_high_bit_clear.cpp](../../../src/battle/legacy_battle_actor_field_26b8_high_bit_clear.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-f897e988d7ac** `battle/legacy_battle_actor_idle_state`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_idle_state.hpp](../../../include/openswd3/battle/legacy_battle_actor_idle_state.hpp)；[src/battle/legacy_battle_actor_idle_state.cpp](../../../src/battle/legacy_battle_actor_idle_state.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-06caabd40026** `battle/legacy_battle_actor_lifecycle`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_lifecycle.hpp](../../../include/openswd3/battle/legacy_battle_actor_lifecycle.hpp)；[src/battle/legacy_battle_actor_lifecycle.cpp](../../../src/battle/legacy_battle_actor_lifecycle.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-5992dcc2aeea** `battle/legacy_battle_actor_message_percent_refresh`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_message_percent_refresh.hpp](../../../include/openswd3/battle/legacy_battle_actor_message_percent_refresh.hpp)；[src/battle/legacy_battle_actor_message_percent_refresh.cpp](../../../src/battle/legacy_battle_actor_message_percent_refresh.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-71328137a7aa** `battle/legacy_battle_actor_start_gate`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_start_gate.hpp](../../../include/openswd3/battle/legacy_battle_actor_start_gate.hpp)；[src/battle/legacy_battle_actor_start_gate.cpp](../../../src/battle/legacy_battle_actor_start_gate.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-c33fdd484a6a** `battle/legacy_battle_actor_start_gate_increment`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_start_gate_increment.hpp](../../../include/openswd3/battle/legacy_battle_actor_start_gate_increment.hpp)；[src/battle/legacy_battle_actor_start_gate_increment.cpp](../../../src/battle/legacy_battle_actor_start_gate_increment.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-f8eaea4fa41a** `battle/legacy_battle_actor_start_gate_latch_query`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_start_gate_latch_query.hpp](../../../include/openswd3/battle/legacy_battle_actor_start_gate_latch_query.hpp)；[src/battle/legacy_battle_actor_start_gate_latch_query.cpp](../../../src/battle/legacy_battle_actor_start_gate_latch_query.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-66a0ae0cf41b** `battle/legacy_battle_actor_startup_reset`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_startup_reset.hpp](../../../include/openswd3/battle/legacy_battle_actor_startup_reset.hpp)；[src/battle/legacy_battle_actor_startup_reset.cpp](../../../src/battle/legacy_battle_actor_startup_reset.cpp)。
  - 类型：[PORT-78f64a54c45e](invoke-removal-ports.md#port-78f64a54c45e), [PORT-97d222967168](invoke-removal-ports.md#port-97d222967168)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-809dd4fc17dd** `battle/legacy_battle_actor_target_selection_count_increment`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_target_selection_count_increment.hpp](../../../include/openswd3/battle/legacy_battle_actor_target_selection_count_increment.hpp)；[src/battle/legacy_battle_actor_target_selection_count_increment.cpp](../../../src/battle/legacy_battle_actor_target_selection_count_increment.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-55a716327eff** `battle/legacy_battle_actor_target_selection_count_query`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_target_selection_count_query.hpp](../../../include/openswd3/battle/legacy_battle_actor_target_selection_count_query.hpp)；[src/battle/legacy_battle_actor_target_selection_count_query.cpp](../../../src/battle/legacy_battle_actor_target_selection_count_query.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-28835bd2bfb5** `battle/legacy_battle_actor_target_selection_latch_query`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_target_selection_latch_query.hpp](../../../include/openswd3/battle/legacy_battle_actor_target_selection_latch_query.hpp)；[src/battle/legacy_battle_actor_target_selection_latch_query.cpp](../../../src/battle/legacy_battle_actor_target_selection_latch_query.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-3147f719b21d** `battle/legacy_battle_actor_target_selection_latch_set`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_target_selection_latch_set.hpp](../../../include/openswd3/battle/legacy_battle_actor_target_selection_latch_set.hpp)；[src/battle/legacy_battle_actor_target_selection_latch_set.cpp](../../../src/battle/legacy_battle_actor_target_selection_latch_set.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-eeab47ee6ec7** `battle/legacy_battle_actor_turn_completion`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_turn_completion.hpp](../../../include/openswd3/battle/legacy_battle_actor_turn_completion.hpp)；[src/battle/legacy_battle_actor_turn_completion.cpp](../../../src/battle/legacy_battle_actor_turn_completion.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-90fe404ab974** `battle/legacy_battle_attack_order_dequeue`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_attack_order_dequeue.hpp](../../../include/openswd3/battle/legacy_battle_attack_order_dequeue.hpp)；[include/openswd3/battle/legacy_battle_frame_selection.hpp](../../../include/openswd3/battle/legacy_battle_frame_selection.hpp)；[src/battle/legacy_battle_attack_order_dequeue.cpp](../../../src/battle/legacy_battle_attack_order_dequeue.cpp)；[src/battle/legacy_battle_frame_selection.cpp](../../../src/battle/legacy_battle_frame_selection.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-1d2829a6c415** `battle/legacy_battle_attack_order_entry`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_attack_order_entry.hpp](../../../include/openswd3/battle/legacy_battle_attack_order_entry.hpp)；[src/battle/legacy_battle_attack_order_entry.cpp](../../../src/battle/legacy_battle_attack_order_entry.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-cf32f5364376** `battle/legacy_battle_attack_order_insert`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_attack_order_insert.hpp](../../../include/openswd3/battle/legacy_battle_attack_order_insert.hpp)；[src/battle/legacy_battle_attack_order_insert.cpp](../../../src/battle/legacy_battle_attack_order_insert.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-bcb81dd1d7f5** `battle/legacy_battle_attack_order_remove`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_attack_order_remove.hpp](../../../include/openswd3/battle/legacy_battle_attack_order_remove.hpp)；[src/battle/legacy_battle_attack_order_remove.cpp](../../../src/battle/legacy_battle_attack_order_remove.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-45a3372aa999** `battle/legacy_battle_debug_state`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_debug_state.hpp](../../../include/openswd3/battle/legacy_battle_debug_state.hpp)。
  - 类型：[PORT-b505b28f9b0d](invoke-removal-ports.md#port-b505b28f9b0d), [PORT-1b5a6c2e0ec3](invoke-removal-ports.md#port-1b5a6c2e0ec3)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-8cdb47b3067e** `battle/legacy_battle_definition_archive`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_definition_archive.hpp](../../../include/openswd3/battle/legacy_battle_definition_archive.hpp)；[src/battle/legacy_battle_definition_archive.cpp](../../../src/battle/legacy_battle_definition_archive.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-04a10e318812** `battle/legacy_battle_display_surface_runtime`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_display_surface_runtime.hpp](../../../include/openswd3/battle/legacy_battle_display_surface_runtime.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-24155353f482** `battle/legacy_battle_file_lifecycle`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_file_lifecycle.hpp](../../../include/openswd3/battle/legacy_battle_file_lifecycle.hpp)；[src/battle/legacy_battle_file_lifecycle.cpp](../../../src/battle/legacy_battle_file_lifecycle.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-b3dea06844c3** `battle/legacy_battle_fixed_count_chain`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_fixed_count_chain.hpp](../../../include/openswd3/battle/legacy_battle_fixed_count_chain.hpp)；[src/battle/legacy_battle_fixed_count_chain.cpp](../../../src/battle/legacy_battle_fixed_count_chain.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-c4b18246fd2b** `battle/legacy_battle_fixed_object_reset`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_fixed_object_reset.hpp](../../../include/openswd3/battle/legacy_battle_fixed_object_reset.hpp)；[src/battle/legacy_battle_fixed_object_reset.cpp](../../../src/battle/legacy_battle_fixed_object_reset.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-9d4adfc75ccd** `battle/legacy_battle_frame_completion`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_frame_completion.hpp](../../../include/openswd3/battle/legacy_battle_frame_completion.hpp)；[src/battle/legacy_battle_frame_completion.cpp](../../../src/battle/legacy_battle_frame_completion.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-36be8d436c29** `battle/legacy_battle_frame_effect_control`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_frame_effect_control.hpp](../../../include/openswd3/battle/legacy_battle_frame_effect_control.hpp)。
  - 类型：[PORT-5e1aa8e7b00a](invoke-removal-ports.md#port-5e1aa8e7b00a)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-d07375a49360** `battle/legacy_battle_frame_input_resolution_state`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_frame_input_resolution_state.hpp](../../../include/openswd3/battle/legacy_battle_frame_input_resolution_state.hpp)。
  - 类型：[PORT-75c4b984e77a](invoke-removal-ports.md#port-75c4b984e77a)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-4a7c890477b1** `battle/legacy_battle_frame_music_prefix`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_frame_music_prefix.hpp](../../../include/openswd3/battle/legacy_battle_frame_music_prefix.hpp)；[src/battle/legacy_battle_frame_music_prefix.cpp](../../../src/battle/legacy_battle_frame_music_prefix.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-11b870391807** `battle/legacy_battle_frame_refresh`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_frame_refresh.hpp](../../../include/openswd3/battle/legacy_battle_frame_refresh.hpp)；[src/battle/legacy_battle_frame_refresh.cpp](../../../src/battle/legacy_battle_frame_refresh.cpp)。
  - 类型：[PORT-6ea02dfb62aa](invoke-removal-ports.md#port-6ea02dfb62aa)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-969cf7aa084b** `battle/legacy_battle_frame_selection`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_frame_selection.hpp](../../../include/openswd3/battle/legacy_battle_frame_selection.hpp)；[src/battle/legacy_battle_frame_selection.cpp](../../../src/battle/legacy_battle_frame_selection.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-4b884467a629** `battle/legacy_battle_group_a_resource_cleanup`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_resource_cleanup.hpp](../../../include/openswd3/battle/legacy_battle_group_a_resource_cleanup.hpp)；[src/battle/legacy_battle_group_a_resource_cleanup.cpp](../../../src/battle/legacy_battle_group_a_resource_cleanup.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-9058bf93f453** `battle/legacy_battle_group_a_reward_profile_state`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_reward_profile_state.hpp](../../../include/openswd3/battle/legacy_battle_group_a_reward_profile_state.hpp)。
  - 类型：[PORT-164486c15a77](invoke-removal-ports.md#port-164486c15a77)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-ac2732988f31** `battle/legacy_battle_group_a_startup_reset`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_startup_reset.hpp](../../../include/openswd3/battle/legacy_battle_group_a_startup_reset.hpp)；[src/battle/legacy_battle_group_a_startup_reset.cpp](../../../src/battle/legacy_battle_group_a_startup_reset.cpp)。
  - 类型：[PORT-ba1fcfb22dfa](invoke-removal-ports.md#port-ba1fcfb22dfa)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-d6e12fed867f** `battle/legacy_battle_group_a_storage`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_storage.hpp](../../../include/openswd3/battle/legacy_battle_group_a_storage.hpp)；[src/battle/legacy_battle_group_a_storage.cpp](../../../src/battle/legacy_battle_group_a_storage.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-a77b7620211f** `battle/legacy_battle_group_b_resource_cleanup`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_resource_cleanup.hpp](../../../include/openswd3/battle/legacy_battle_group_b_resource_cleanup.hpp)；[src/battle/legacy_battle_group_b_resource_cleanup.cpp](../../../src/battle/legacy_battle_group_b_resource_cleanup.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-026ed1750c8c** `battle/legacy_battle_group_b_startup_reset`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_startup_reset.hpp](../../../include/openswd3/battle/legacy_battle_group_b_startup_reset.hpp)；[src/battle/legacy_battle_group_b_startup_reset.cpp](../../../src/battle/legacy_battle_group_b_startup_reset.cpp)。
  - 类型：[PORT-6c8d111752ec](invoke-removal-ports.md#port-6c8d111752ec)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-4f3251e309c9** `battle/legacy_battle_group_b_storage`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_storage.hpp](../../../include/openswd3/battle/legacy_battle_group_b_storage.hpp)；[src/battle/legacy_battle_group_b_storage.cpp](../../../src/battle/legacy_battle_group_b_storage.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-4fa5e6a2383f** `battle/legacy_battle_growth_completion_caption`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_growth_completion_caption.hpp](../../../include/openswd3/battle/legacy_battle_growth_completion_caption.hpp)；[src/battle/legacy_battle_growth_completion_caption.cpp](../../../src/battle/legacy_battle_growth_completion_caption.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-5a07e249d289** `battle/legacy_battle_mon_file_runtime`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_mon_file_runtime.hpp](../../../include/openswd3/battle/legacy_battle_mon_file_runtime.hpp)；[src/battle/legacy_battle_mon_file_runtime.cpp](../../../src/battle/legacy_battle_mon_file_runtime.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-41d9b3022995** `battle/legacy_battle_mon_stream_runtime`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_mon_stream_runtime.hpp](../../../include/openswd3/battle/legacy_battle_mon_stream_runtime.hpp)；[src/battle/legacy_battle_mon_stream_runtime.cpp](../../../src/battle/legacy_battle_mon_stream_runtime.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-0e6ac6f49506** `battle/legacy_battle_mon_text_runtime`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_mon_text_runtime.hpp](../../../include/openswd3/battle/legacy_battle_mon_text_runtime.hpp)；[src/battle/legacy_battle_mon_text_runtime.cpp](../../../src/battle/legacy_battle_mon_text_runtime.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-4b43e6b94ab7** `battle/legacy_battle_outcome_state`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_outcome_state.hpp](../../../include/openswd3/battle/legacy_battle_outcome_state.hpp)。
  - 类型：[PORT-d2d6d7766342](invoke-removal-ports.md#port-d2d6d7766342)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-cc50a503ec21** `battle/legacy_battle_player_item_quantity`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_player_item_quantity.hpp](../../../include/openswd3/battle/legacy_battle_player_item_quantity.hpp)；[src/battle/legacy_battle_player_item_quantity.cpp](../../../src/battle/legacy_battle_player_item_quantity.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-13ef6fa5de7a** `battle/legacy_battle_render_geometry`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_render_geometry.hpp](../../../include/openswd3/battle/legacy_battle_render_geometry.hpp)；[src/battle/legacy_battle_render_geometry.cpp](../../../src/battle/legacy_battle_render_geometry.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-f3d9a96baa55** `battle/legacy_battle_runtime_shutdown`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_runtime_shutdown.hpp](../../../include/openswd3/battle/legacy_battle_runtime_shutdown.hpp)；[src/battle/legacy_battle_runtime_shutdown.cpp](../../../src/battle/legacy_battle_runtime_shutdown.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-e91fed87c6c7** `battle/legacy_battle_screen_flash`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`verified`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_screen_flash.hpp](../../../include/openswd3/battle/legacy_battle_screen_flash.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-a2d76b267fa2** `battle/legacy_battle_shared_phase`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_shared_phase.hpp](../../../include/openswd3/battle/legacy_battle_shared_phase.hpp)。
  - 类型：[PORT-d0741dd9f3ec](invoke-removal-ports.md#port-d0741dd9f3ec)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-7c24287fd730** `battle/legacy_battle_single_effect_frame`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_single_effect_frame.hpp](../../../include/openswd3/battle/legacy_battle_single_effect_frame.hpp)；[src/battle/legacy_battle_single_effect_frame.cpp](../../../src/battle/legacy_battle_single_effect_frame.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-f472b6e459a1** `include/openswd3/app/display_refresh.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/app/display_refresh.hpp](../../../include/openswd3/app/display_refresh.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-786ff07130cf** `include/openswd3/app/frame_dispatch.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/app/frame_dispatch.hpp](../../../include/openswd3/app/frame_dispatch.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-1c08dc5f2980** `include/openswd3/asset_runtime/legacy_act_archive.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/asset_runtime/legacy_act_archive.hpp](../../../include/openswd3/asset_runtime/legacy_act_archive.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-ce8fdd0e25e6** `include/openswd3/asset_runtime/legacy_act_runtime.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/asset_runtime/legacy_act_runtime.hpp](../../../include/openswd3/asset_runtime/legacy_act_runtime.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-9036882c10d3** `include/openswd3/asset_runtime/legacy_ani_archive.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/asset_runtime/legacy_ani_archive.hpp](../../../include/openswd3/asset_runtime/legacy_ani_archive.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-91d701eceabd** `include/openswd3/asset_runtime/legacy_ani_row_copy_effect.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/asset_runtime/legacy_ani_row_copy_effect.hpp](../../../include/openswd3/asset_runtime/legacy_ani_row_copy_effect.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-061bf21cd37f** `include/openswd3/asset_runtime/legacy_guest_address_reservation.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/asset_runtime/legacy_guest_address_reservation.hpp](../../../include/openswd3/asset_runtime/legacy_guest_address_reservation.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-aeb4a7a66054** `include/openswd3/asset_runtime/legacy_tsw_archive.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/asset_runtime/legacy_tsw_archive.hpp](../../../include/openswd3/asset_runtime/legacy_tsw_archive.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-abdf7b348626** `include/openswd3/asset_runtime/legacy_tsw_runtime.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/asset_runtime/legacy_tsw_runtime.hpp](../../../include/openswd3/asset_runtime/legacy_tsw_runtime.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-34328959bbf4** `include/openswd3/audio_video/legacy_audio_output.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/audio_video/legacy_audio_output.hpp](../../../include/openswd3/audio_video/legacy_audio_output.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-6ed38f8a02b4** `include/openswd3/audio_video/legacy_audio_parameters.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/audio_video/legacy_audio_parameters.hpp](../../../include/openswd3/audio_video/legacy_audio_parameters.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-ffa24337c12a** `include/openswd3/audio_video/legacy_sample_commands.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/audio_video/legacy_sample_commands.hpp](../../../include/openswd3/audio_video/legacy_sample_commands.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-d6566842cdb1** `include/openswd3/audio_video/legacy_sample_manager.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/audio_video/legacy_sample_manager.hpp](../../../include/openswd3/audio_video/legacy_sample_manager.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-eab189588def** `include/openswd3/audio_video/legacy_sequence_manager.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/audio_video/legacy_sequence_manager.hpp](../../../include/openswd3/audio_video/legacy_sequence_manager.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-84f4dfc92253** `include/openswd3/audio_video/legacy_snd_archive.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/audio_video/legacy_snd_archive.hpp](../../../include/openswd3/audio_video/legacy_snd_archive.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-ff0ed3330b82** `include/openswd3/audio_video/legacy_stream_commands.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/audio_video/legacy_stream_commands.hpp](../../../include/openswd3/audio_video/legacy_stream_commands.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-7cc97e2a9fac** `include/openswd3/audio_video/legacy_stream_manager.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/audio_video/legacy_stream_manager.hpp](../../../include/openswd3/audio_video/legacy_stream_manager.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-135995198788** `include/openswd3/battle/legacy_battle_actor_field_26c0.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_field_26c0.hpp](../../../include/openswd3/battle/legacy_battle_actor_field_26c0.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-f6ce9e01c04b** `include/openswd3/battle/legacy_battle_actor_frame_raw_block.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_actor_frame_raw_block.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_raw_block.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-08984e863508** `include/openswd3/battle/legacy_battle_assets.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_assets.hpp](../../../include/openswd3/battle/legacy_battle_assets.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-71c69e358381** `include/openswd3/battle/legacy_battle_auxiliary_record.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_auxiliary_record.hpp](../../../include/openswd3/battle/legacy_battle_auxiliary_record.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-610d84ea0fcd** `include/openswd3/battle/legacy_battle_color_fade.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_color_fade.hpp](../../../include/openswd3/battle/legacy_battle_color_fade.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-0a028220ae20** `include/openswd3/battle/legacy_battle_directional_scan.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_directional_scan.hpp](../../../include/openswd3/battle/legacy_battle_directional_scan.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-b5be6a1613fc** `include/openswd3/battle/legacy_battle_exit_cleanups.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_exit_cleanups.hpp](../../../include/openswd3/battle/legacy_battle_exit_cleanups.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-033fb46a8d93** `include/openswd3/battle/legacy_battle_group_a_action_execution_state.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_action_execution_state.hpp](../../../include/openswd3/battle/legacy_battle_group_a_action_execution_state.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-95249969eec2** `include/openswd3/battle/legacy_battle_group_a_final_processing_state.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_a_final_processing_state.hpp](../../../include/openswd3/battle/legacy_battle_group_a_final_processing_state.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-b140bce2c5e9** `include/openswd3/battle/legacy_battle_group_b_order.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_group_b_order.hpp](../../../include/openswd3/battle/legacy_battle_group_b_order.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-b9777719352a** `include/openswd3/battle/legacy_battle_image_rotation.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_image_rotation.hpp](../../../include/openswd3/battle/legacy_battle_image_rotation.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-0e7adb97036d** `include/openswd3/battle/legacy_battle_mon_text.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_mon_text.hpp](../../../include/openswd3/battle/legacy_battle_mon_text.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-42b4513a4abd** `include/openswd3/battle/legacy_battle_music_path.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_music_path.hpp](../../../include/openswd3/battle/legacy_battle_music_path.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-360429056701** `include/openswd3/battle/legacy_battle_particle_frame.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_particle_frame.hpp](../../../include/openswd3/battle/legacy_battle_particle_frame.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-3183a0941ff9** `include/openswd3/battle/legacy_battle_particle_spawn.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_particle_spawn.hpp](../../../include/openswd3/battle/legacy_battle_particle_spawn.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-be21f646f049** `include/openswd3/battle/legacy_battle_save_fame.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_save_fame.hpp](../../../include/openswd3/battle/legacy_battle_save_fame.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-23043502b1b0** `include/openswd3/battle/legacy_battle_save_party_extension.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_save_party_extension.hpp](../../../include/openswd3/battle/legacy_battle_save_party_extension.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-48ec300729aa** `include/openswd3/battle/legacy_battle_script_curve.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_script_curve.hpp](../../../include/openswd3/battle/legacy_battle_script_curve.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-8645faa143f1** `include/openswd3/battle/legacy_battle_setup.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_setup.hpp](../../../include/openswd3/battle/legacy_battle_setup.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-7e971a6d0de9** `include/openswd3/battle/legacy_battle_timing.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/battle/legacy_battle_timing.hpp](../../../include/openswd3/battle/legacy_battle_timing.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-c39b7534624c** `include/openswd3/compat/legacy_decimal.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/compat/legacy_decimal.hpp](../../../include/openswd3/compat/legacy_decimal.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-eaf0dc6b61cb** `include/openswd3/compat/legacy_stubs.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/compat/legacy_stubs.hpp](../../../include/openswd3/compat/legacy_stubs.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-42f85108d353** `include/openswd3/compat/types.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/compat/types.hpp](../../../include/openswd3/compat/types.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-c5b1db9e8ac3** `include/openswd3/diagnostics/log.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/diagnostics/log.hpp](../../../include/openswd3/diagnostics/log.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-773250dd4569** `include/openswd3/input_time_rng/legacy_crt_rng.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/input_time_rng/legacy_crt_rng.hpp](../../../include/openswd3/input_time_rng/legacy_crt_rng.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-548b3ea39c0e** `include/openswd3/input_time_rng/legacy_frame_clock.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/input_time_rng/legacy_frame_clock.hpp](../../../include/openswd3/input_time_rng/legacy_frame_clock.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-d8a6ec61bcfa** `include/openswd3/input_time_rng/legacy_input.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/input_time_rng/legacy_input.hpp](../../../include/openswd3/input_time_rng/legacy_input.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-ed1a64314e65** `include/openswd3/input_time_rng/legacy_secondary_rng.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/input_time_rng/legacy_secondary_rng.hpp](../../../include/openswd3/input_time_rng/legacy_secondary_rng.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-971cb30349a7** `include/openswd3/media_ffmpeg/legacy_ffmpeg_backends.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/media_ffmpeg/legacy_ffmpeg_backends.hpp](../../../include/openswd3/media_ffmpeg/legacy_ffmpeg_backends.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-95eefeec4942** `include/openswd3/rendering/legacy_blitter.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_blitter.hpp](../../../include/openswd3/rendering/legacy_blitter.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-a3be53f91a02** `include/openswd3/rendering/legacy_fixed_tile_writer.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_fixed_tile_writer.hpp](../../../include/openswd3/rendering/legacy_fixed_tile_writer.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-9960c8c12c31** `include/openswd3/rendering/legacy_frame_color.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_frame_color.hpp](../../../include/openswd3/rendering/legacy_frame_color.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-554cdc267bae** `include/openswd3/rendering/legacy_framebuffer.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_framebuffer.hpp](../../../include/openswd3/rendering/legacy_framebuffer.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-8ac31b50eb92** `include/openswd3/rendering/legacy_glyph_atlas.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_glyph_atlas.hpp](../../../include/openswd3/rendering/legacy_glyph_atlas.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-bb2e9cf6579d** `include/openswd3/rendering/legacy_glyph_cache.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_glyph_cache.hpp](../../../include/openswd3/rendering/legacy_glyph_cache.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-2f567fa74bbe** `include/openswd3/rendering/legacy_glyph_writer.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_glyph_writer.hpp](../../../include/openswd3/rendering/legacy_glyph_writer.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-39173a913eb8** `include/openswd3/rendering/legacy_image_command_stream.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_image_command_stream.hpp](../../../include/openswd3/rendering/legacy_image_command_stream.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-96671df4a1fb** `include/openswd3/rendering/legacy_packed_row.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_packed_row.hpp](../../../include/openswd3/rendering/legacy_packed_row.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-c5902707e147** `include/openswd3/rendering/legacy_pixel_conversion.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_pixel_conversion.hpp](../../../include/openswd3/rendering/legacy_pixel_conversion.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-adbc3617973a** `include/openswd3/rendering/legacy_rectangle_effect.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_rectangle_effect.hpp](../../../include/openswd3/rendering/legacy_rectangle_effect.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-daf7ec509435** `include/openswd3/rendering/legacy_scaled_rle_writer.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_scaled_rle_writer.hpp](../../../include/openswd3/rendering/legacy_scaled_rle_writer.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-3abea59ab033** `include/openswd3/rendering/legacy_text_renderer.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_text_renderer.hpp](../../../include/openswd3/rendering/legacy_text_renderer.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-1af47af72e6f** `include/openswd3/rendering/legacy_text_renderer_runtime.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_text_renderer_runtime.hpp](../../../include/openswd3/rendering/legacy_text_renderer_runtime.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-4db8385aab2e** `include/openswd3/resource_io/legacy_cm_cache.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/resource_io/legacy_cm_cache.hpp](../../../include/openswd3/resource_io/legacy_cm_cache.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-b307ea1ed6d8** `include/openswd3/resource_io/legacy_dbcs_text_buffer.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/resource_io/legacy_dbcs_text_buffer.hpp](../../../include/openswd3/resource_io/legacy_dbcs_text_buffer.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-b27814f9ae7b** `include/openswd3/resource_io/legacy_environment.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/resource_io/legacy_environment.hpp](../../../include/openswd3/resource_io/legacy_environment.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-0c98255e11b6** `include/openswd3/resource_io/legacy_file.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/resource_io/legacy_file.hpp](../../../include/openswd3/resource_io/legacy_file.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-e4afbffd2ebd** `include/openswd3/resource_io/legacy_lmf_archive.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/resource_io/legacy_lmf_archive.hpp](../../../include/openswd3/resource_io/legacy_lmf_archive.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-bc85e376c033** `include/openswd3/resource_io/legacy_lzo1x.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/resource_io/legacy_lzo1x.hpp](../../../include/openswd3/resource_io/legacy_lzo1x.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-e88c8f32946f** `include/openswd3/resource_io/legacy_memory_manager.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/resource_io/legacy_memory_manager.hpp](../../../include/openswd3/resource_io/legacy_memory_manager.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-077279c3a3aa** `include/openswd3/resource_io/legacy_resource_databases.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/resource_io/legacy_resource_databases.hpp](../../../include/openswd3/resource_io/legacy_resource_databases.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-13fb685ee960** `include/openswd3/resource_io/legacy_save_container.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/resource_io/legacy_save_container.hpp](../../../include/openswd3/resource_io/legacy_save_container.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-1eaa128bf147** `include/openswd3/resource_io/legacy_save_slots.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/resource_io/legacy_save_slots.hpp](../../../include/openswd3/resource_io/legacy_save_slots.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-76fdb291a451** `include/openswd3/resource_io/window_configuration.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/resource_io/window_configuration.hpp](../../../include/openswd3/resource_io/window_configuration.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-219971e56ad1** `include/openswd3/special_modes/legacy_save_preview.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/special_modes/legacy_save_preview.hpp](../../../include/openswd3/special_modes/legacy_save_preview.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-ca64ccc0ffb9** `include/openswd3/story_scene/legacy_dialog_geometry.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/story_scene/legacy_dialog_geometry.hpp](../../../include/openswd3/story_scene/legacy_dialog_geometry.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-00887ddc2979** `include/openswd3/world_map/legacy_cm_cache_generator.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_cm_cache_generator.hpp](../../../include/openswd3/world_map/legacy_cm_cache_generator.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-ffcb1d906713** `include/openswd3/world_map/legacy_cm_cache_loader.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_cm_cache_loader.hpp](../../../include/openswd3/world_map/legacy_cm_cache_loader.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-ba4411a2e4a7** `include/openswd3/world_map/legacy_cm_cache_runtime.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_cm_cache_runtime.hpp](../../../include/openswd3/world_map/legacy_cm_cache_runtime.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-880002d32556** `include/openswd3/world_map/legacy_maps_world_database.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_maps_world_database.hpp](../../../include/openswd3/world_map/legacy_maps_world_database.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-dd92ef70420f** `include/openswd3/world_map/legacy_role_spatial_query.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_role_spatial_query.hpp](../../../include/openswd3/world_map/legacy_role_spatial_query.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-921b9cace168** `include/openswd3/world_map/legacy_world_background.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_background.hpp](../../../include/openswd3/world_map/legacy_world_background.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-69b38f3c7e73** `include/openswd3/world_map/legacy_world_builtin_colors.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_builtin_colors.hpp](../../../include/openswd3/world_map/legacy_world_builtin_colors.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-166b49f1c56d** `include/openswd3/world_map/legacy_world_camera_pan.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_camera_pan.hpp](../../../include/openswd3/world_map/legacy_world_camera_pan.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-acc89b791df0** `include/openswd3/world_map/legacy_world_direction_adjustment.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_direction_adjustment.hpp](../../../include/openswd3/world_map/legacy_world_direction_adjustment.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-0d73365e33f6** `include/openswd3/world_map/legacy_world_direction_input.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_direction_input.hpp](../../../include/openswd3/world_map/legacy_world_direction_input.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-236b57a25247** `include/openswd3/world_map/legacy_world_facing.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_facing.hpp](../../../include/openswd3/world_map/legacy_world_facing.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-d853fe9846d6** `include/openswd3/world_map/legacy_world_facing_role_query.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_facing_role_query.hpp](../../../include/openswd3/world_map/legacy_world_facing_role_query.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-4fb0323080a7** `include/openswd3/world_map/legacy_world_frame_tail.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_frame_tail.hpp](../../../include/openswd3/world_map/legacy_world_frame_tail.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-fb05808ba5de** `include/openswd3/world_map/legacy_world_interpolation.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_interpolation.hpp](../../../include/openswd3/world_map/legacy_world_interpolation.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-5c3654894fc7** `include/openswd3/world_map/legacy_world_map_business.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_map_business.hpp](../../../include/openswd3/world_map/legacy_world_map_business.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-3889981a9989** `include/openswd3/world_map/legacy_world_map_session.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_map_session.hpp](../../../include/openswd3/world_map/legacy_world_map_session.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-f4f6a8dd31f8** `include/openswd3/world_map/legacy_world_pathfinding.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_pathfinding.hpp](../../../include/openswd3/world_map/legacy_world_pathfinding.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-711311aacf5d** `include/openswd3/world_map/legacy_world_player_control.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_player_control.hpp](../../../include/openswd3/world_map/legacy_world_player_control.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-0df00273756c** `include/openswd3/world_map/legacy_world_player_motion.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_player_motion.hpp](../../../include/openswd3/world_map/legacy_world_player_motion.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-a98b92205621** `include/openswd3/world_map/legacy_world_render_session.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_render_session.hpp](../../../include/openswd3/world_map/legacy_world_render_session.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-2e7ee1ba1cb3** `include/openswd3/world_map/legacy_world_role_lookup.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_role_lookup.hpp](../../../include/openswd3/world_map/legacy_world_role_lookup.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-dd48f2d0a655** `include/openswd3/world_map/legacy_world_role_map_update.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_role_map_update.hpp](../../../include/openswd3/world_map/legacy_world_role_map_update.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-3562bd8467ea** `include/openswd3/world_map/legacy_world_role_post_materialization.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_role_post_materialization.hpp](../../../include/openswd3/world_map/legacy_world_role_post_materialization.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-17bf4ccfac37** `include/openswd3/world_map/legacy_world_role_preload.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_role_preload.hpp](../../../include/openswd3/world_map/legacy_world_role_preload.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-961a98f05a73** `include/openswd3/world_map/legacy_world_role_record.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_role_record.hpp](../../../include/openswd3/world_map/legacy_world_role_record.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-f4757fa64c3f** `include/openswd3/world_map/legacy_world_role_surface_occupancy.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_role_surface_occupancy.hpp](../../../include/openswd3/world_map/legacy_world_role_surface_occupancy.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-4142f61e4073** `include/openswd3/world_map/legacy_world_role_transfer.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_role_transfer.hpp](../../../include/openswd3/world_map/legacy_world_role_transfer.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-afdbe85d8879** `include/openswd3/world_map/legacy_world_runtime_session.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_runtime_session.hpp](../../../include/openswd3/world_map/legacy_world_runtime_session.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-78e2e4b757aa** `include/openswd3/world_map/legacy_world_selection_scroll.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_selection_scroll.hpp](../../../include/openswd3/world_map/legacy_world_selection_scroll.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-0ba9cbae2bb8** `include/openswd3/world_map/legacy_world_special_frame_loader.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_special_frame_loader.hpp](../../../include/openswd3/world_map/legacy_world_special_frame_loader.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-cc3ee8870ae8** `include/openswd3/world_map/legacy_world_story_paths.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_story_paths.hpp](../../../include/openswd3/world_map/legacy_world_story_paths.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-f52e3d5ca512** `include/openswd3/world_map/legacy_world_transient_reset.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_transient_reset.hpp](../../../include/openswd3/world_map/legacy_world_transient_reset.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-fc252db8afc4** `input_time_rng/legacy_mouse_frame_state`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/input_time_rng/legacy_mouse_frame_state.hpp](../../../include/openswd3/input_time_rng/legacy_mouse_frame_state.hpp)。
  - 类型：[PORT-7cb2fa0d8bbe](invoke-removal-ports.md#port-7cb2fa0d8bbe)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-1f87b5a55e66** `input_time_rng/legacy_text_input`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/input_time_rng/legacy_text_input.hpp](../../../include/openswd3/input_time_rng/legacy_text_input.hpp)；[src/input_time_rng/legacy_text_input.cpp](../../../src/input_time_rng/legacy_text_input.cpp)。
  - 类型：[PORT-f4bce2b0680e](invoke-removal-ports.md#port-f4bce2b0680e)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-63dc3d4adb95** `platform/external_launch_sdl3`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[src/platform/sdl3/external_launch_sdl3.cpp](../../../src/platform/sdl3/external_launch_sdl3.cpp)；[src/platform/sdl3/external_launch_sdl3.hpp](../../../src/platform/sdl3/external_launch_sdl3.hpp)。
  - 类型：[PORT-5f00d9c08b8b](invoke-removal-ports.md#port-5f00d9c08b8b)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-3c4fc927a928** `platform/single_instance`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[src/platform/sdl3/single_instance.hpp](../../../src/platform/sdl3/single_instance.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-98162d4c3c9c** `platform/startup_dialog_sdl3`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[src/platform/sdl3/startup_dialog_sdl3.cpp](../../../src/platform/sdl3/startup_dialog_sdl3.cpp)；[src/platform/sdl3/startup_dialog_sdl3.hpp](../../../src/platform/sdl3/startup_dialog_sdl3.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-69f879679e50** `platform/world_role_runtime_adapter`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[src/platform/sdl3/world_role_runtime_adapter.cpp](../../../src/platform/sdl3/world_role_runtime_adapter.cpp)；[src/platform/sdl3/world_role_runtime_adapter.hpp](../../../src/platform/sdl3/world_role_runtime_adapter.hpp)。
  - 类型：[PORT-20ac79adcfa2](invoke-removal-ports.md#port-20ac79adcfa2)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-80e424de3d9e** `rendering/legacy_bmp_writer`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_bmp_writer.hpp](../../../include/openswd3/rendering/legacy_bmp_writer.hpp)；[src/rendering/legacy_bmp_writer.cpp](../../../src/rendering/legacy_bmp_writer.cpp)。
  - 类型：[PORT-20458ee417fa](invoke-removal-ports.md#port-20458ee417fa)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-e093eb77d2d7** `rendering/legacy_effect_panel`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_effect_panel.hpp](../../../include/openswd3/rendering/legacy_effect_panel.hpp)；[src/rendering/legacy_effect_panel.cpp](../../../src/rendering/legacy_effect_panel.cpp)。
  - 类型：[PORT-6f7b7ea2473d](invoke-removal-ports.md#port-6f7b7ea2473d)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-fef825a327ae** `rendering/legacy_pause_overlay`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_pause_overlay.hpp](../../../include/openswd3/rendering/legacy_pause_overlay.hpp)；[src/rendering/legacy_pause_overlay.cpp](../../../src/rendering/legacy_pause_overlay.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-9d066b9bcff2** `rendering/legacy_presentation`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_presentation.hpp](../../../include/openswd3/rendering/legacy_presentation.hpp)；[src/rendering/legacy_presentation.cpp](../../../src/rendering/legacy_presentation.cpp)。
  - 类型：[PORT-7819f49903f3](invoke-removal-ports.md#port-7819f49903f3)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-17203c61746e** `rendering/legacy_timed_messages`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/rendering/legacy_timed_messages.hpp](../../../include/openswd3/rendering/legacy_timed_messages.hpp)；[src/rendering/legacy_timed_messages.cpp](../../../src/rendering/legacy_timed_messages.cpp)。
  - 类型：[PORT-3f093075afdf](invoke-removal-ports.md#port-3f093075afdf), [PORT-d645d7d74025](invoke-removal-ports.md#port-d645d7d74025), [PORT-8c97736660d8](invoke-removal-ports.md#port-8c97736660d8)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-2fddad5835b6** `special_modes/legacy_initial_menu`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/special_modes/legacy_initial_menu.hpp](../../../include/openswd3/special_modes/legacy_initial_menu.hpp)；[src/special_modes/legacy_initial_menu.cpp](../../../src/special_modes/legacy_initial_menu.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-5c5661d704d0** `src/app/display_refresh.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/app/display_refresh.cpp](../../../src/app/display_refresh.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-d52efcddf065** `src/app/frame_dispatch.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/app/frame_dispatch.cpp](../../../src/app/frame_dispatch.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-9895c0949acc** `src/asset_runtime/legacy_act_archive.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/asset_runtime/legacy_act_archive.cpp](../../../src/asset_runtime/legacy_act_archive.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-ba46f5a622b0** `src/asset_runtime/legacy_act_runtime.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/asset_runtime/legacy_act_runtime.cpp](../../../src/asset_runtime/legacy_act_runtime.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-fb37d0afa977** `src/asset_runtime/legacy_ani_archive.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/asset_runtime/legacy_ani_archive.cpp](../../../src/asset_runtime/legacy_ani_archive.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-7820338fd40a** `src/asset_runtime/legacy_ani_row_copy_effect.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/asset_runtime/legacy_ani_row_copy_effect.cpp](../../../src/asset_runtime/legacy_ani_row_copy_effect.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-7a344a07e276** `src/asset_runtime/legacy_guest_address_reservation.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/asset_runtime/legacy_guest_address_reservation.cpp](../../../src/asset_runtime/legacy_guest_address_reservation.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-bdb8c3f30f23** `src/asset_runtime/legacy_tsw_archive.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/asset_runtime/legacy_tsw_archive.cpp](../../../src/asset_runtime/legacy_tsw_archive.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-d44256600c1f** `src/asset_runtime/legacy_tsw_runtime.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/asset_runtime/legacy_tsw_runtime.cpp](../../../src/asset_runtime/legacy_tsw_runtime.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-5b5280952e5c** `src/audio_video/legacy_audio_output.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/audio_video/legacy_audio_output.cpp](../../../src/audio_video/legacy_audio_output.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-f2b94ed65e2b** `src/audio_video/legacy_audio_parameters.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/audio_video/legacy_audio_parameters.cpp](../../../src/audio_video/legacy_audio_parameters.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-3e415150bc35** `src/audio_video/legacy_sample_commands.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/audio_video/legacy_sample_commands.cpp](../../../src/audio_video/legacy_sample_commands.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-72fadcaeae72** `src/audio_video/legacy_sample_manager.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/audio_video/legacy_sample_manager.cpp](../../../src/audio_video/legacy_sample_manager.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-5dac04e03a5e** `src/audio_video/legacy_sequence_manager.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/audio_video/legacy_sequence_manager.cpp](../../../src/audio_video/legacy_sequence_manager.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-f8f4c061cc96** `src/audio_video/legacy_snd_archive.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/audio_video/legacy_snd_archive.cpp](../../../src/audio_video/legacy_snd_archive.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-0448811aebb8** `src/audio_video/legacy_stream_commands.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/audio_video/legacy_stream_commands.cpp](../../../src/audio_video/legacy_stream_commands.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-e29e00487a08** `src/audio_video/legacy_stream_manager.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/audio_video/legacy_stream_manager.cpp](../../../src/audio_video/legacy_stream_manager.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-30f06c75ef13** `src/battle/legacy_battle_action_rotation_resources.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/battle/legacy_battle_action_rotation_resources.cpp](../../../src/battle/legacy_battle_action_rotation_resources.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-a1be1e196150** `src/battle/legacy_battle_actor_frame_flag_helpers.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/battle/legacy_battle_actor_frame_flag_helpers.hpp](../../../src/battle/legacy_battle_actor_frame_flag_helpers.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-e17f197fb062** `src/battle/legacy_battle_assets.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/battle/legacy_battle_assets.cpp](../../../src/battle/legacy_battle_assets.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-63cb37630fe1** `src/battle/legacy_battle_color_fade.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/battle/legacy_battle_color_fade.cpp](../../../src/battle/legacy_battle_color_fade.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-75c9b664534b** `src/battle/legacy_battle_directional_scan.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/battle/legacy_battle_directional_scan.cpp](../../../src/battle/legacy_battle_directional_scan.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-52ac7821404e** `src/battle/legacy_battle_display_surface_runtime.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/battle/legacy_battle_display_surface_runtime.cpp](../../../src/battle/legacy_battle_display_surface_runtime.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-482c7046c4d5** `src/battle/legacy_battle_image_rotation.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/battle/legacy_battle_image_rotation.cpp](../../../src/battle/legacy_battle_image_rotation.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-c12148c1cce8** `src/battle/legacy_battle_particle_frame.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/battle/legacy_battle_particle_frame.cpp](../../../src/battle/legacy_battle_particle_frame.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-4abf02250c10** `src/battle/legacy_battle_particle_spawn.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/battle/legacy_battle_particle_spawn.cpp](../../../src/battle/legacy_battle_particle_spawn.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-a523544a06de** `src/battle/legacy_battle_save_fame.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/battle/legacy_battle_save_fame.cpp](../../../src/battle/legacy_battle_save_fame.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-14146cbd0185** `src/battle/legacy_battle_save_party_extension.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/battle/legacy_battle_save_party_extension.cpp](../../../src/battle/legacy_battle_save_party_extension.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-b46709013ac5** `src/battle/legacy_battle_script_curve.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/battle/legacy_battle_script_curve.cpp](../../../src/battle/legacy_battle_script_curve.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-94cc9a8b2a4b** `src/battle/legacy_battle_setup.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/battle/legacy_battle_setup.cpp](../../../src/battle/legacy_battle_setup.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-cd55edc1f371** `src/battle/legacy_battle_timing.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/battle/legacy_battle_timing.cpp](../../../src/battle/legacy_battle_timing.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-9383da343c44** `src/compat/legacy_decimal.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/compat/legacy_decimal.cpp](../../../src/compat/legacy_decimal.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-a2471ba36ccd** `src/diagnostics/log.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/diagnostics/log.cpp](../../../src/diagnostics/log.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-6de624e384f8** `src/input_time_rng/legacy_crt_rng.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/input_time_rng/legacy_crt_rng.cpp](../../../src/input_time_rng/legacy_crt_rng.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-70ec053a5920** `src/input_time_rng/legacy_frame_clock.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/input_time_rng/legacy_frame_clock.cpp](../../../src/input_time_rng/legacy_frame_clock.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-94e08fcf7b03** `src/input_time_rng/legacy_input.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/input_time_rng/legacy_input.cpp](../../../src/input_time_rng/legacy_input.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-a58d67b43d6e** `src/input_time_rng/legacy_secondary_rng.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/input_time_rng/legacy_secondary_rng.cpp](../../../src/input_time_rng/legacy_secondary_rng.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-d6d81a870207** `src/platform/sdl3/event_translation.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/platform/sdl3/event_translation.cpp](../../../src/platform/sdl3/event_translation.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-d688c1892d0b** `src/platform/sdl3/event_translation.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/platform/sdl3/event_translation.hpp](../../../src/platform/sdl3/event_translation.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-95b11930e7ac** `src/platform/sdl3/external_uri.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/platform/sdl3/external_uri.cpp](../../../src/platform/sdl3/external_uri.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-d3de0a898d79** `src/platform/sdl3/external_uri.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/platform/sdl3/external_uri.hpp](../../../src/platform/sdl3/external_uri.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-1e7dedcae802** `src/platform/sdl3/keyboard_snapshot_sdl3.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/platform/sdl3/keyboard_snapshot_sdl3.cpp](../../../src/platform/sdl3/keyboard_snapshot_sdl3.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-d24bb190f3a9** `src/platform/sdl3/keyboard_snapshot_sdl3.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/platform/sdl3/keyboard_snapshot_sdl3.hpp](../../../src/platform/sdl3/keyboard_snapshot_sdl3.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-5594e13550d5** `src/platform/sdl3/legacy_ffmpeg_backends.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/platform/sdl3/legacy_ffmpeg_backends.cpp](../../../src/platform/sdl3/legacy_ffmpeg_backends.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-fa1a5d40575f** `src/platform/sdl3/mouse_sdl3.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/platform/sdl3/mouse_sdl3.cpp](../../../src/platform/sdl3/mouse_sdl3.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-fade2aea428f** `src/platform/sdl3/mouse_sdl3.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/platform/sdl3/mouse_sdl3.hpp](../../../src/platform/sdl3/mouse_sdl3.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-35ed2bf46339** `src/platform/sdl3/single_instance.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/platform/sdl3/single_instance.cpp](../../../src/platform/sdl3/single_instance.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-ba755a60d08b** `src/platform/sdl3/text_encoding_sdl3.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/platform/sdl3/text_encoding_sdl3.cpp](../../../src/platform/sdl3/text_encoding_sdl3.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-ca867733513d** `src/platform/sdl3/text_encoding_sdl3.hpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/platform/sdl3/text_encoding_sdl3.hpp](../../../src/platform/sdl3/text_encoding_sdl3.hpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-c5ca11dbb7f9** `src/rendering/legacy_fixed_tile_writer.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/rendering/legacy_fixed_tile_writer.cpp](../../../src/rendering/legacy_fixed_tile_writer.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-b139498c86f4** `src/rendering/legacy_frame_color.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/rendering/legacy_frame_color.cpp](../../../src/rendering/legacy_frame_color.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-29884e88261a** `src/rendering/legacy_framebuffer.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/rendering/legacy_framebuffer.cpp](../../../src/rendering/legacy_framebuffer.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-76ef51ade64e** `src/rendering/legacy_glyph_atlas.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/rendering/legacy_glyph_atlas.cpp](../../../src/rendering/legacy_glyph_atlas.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-b10d84cc43e6** `src/rendering/legacy_glyph_cache.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/rendering/legacy_glyph_cache.cpp](../../../src/rendering/legacy_glyph_cache.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-e0b3e7a06b5a** `src/rendering/legacy_glyph_writer.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/rendering/legacy_glyph_writer.cpp](../../../src/rendering/legacy_glyph_writer.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-652ed40b1cb2** `src/rendering/legacy_image_command_stream.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/rendering/legacy_image_command_stream.cpp](../../../src/rendering/legacy_image_command_stream.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-26fa487a2ee4** `src/rendering/legacy_packed_row.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/rendering/legacy_packed_row.cpp](../../../src/rendering/legacy_packed_row.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-61e6899583ac** `src/rendering/legacy_pixel_conversion.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/rendering/legacy_pixel_conversion.cpp](../../../src/rendering/legacy_pixel_conversion.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-34e59c041a10** `src/rendering/legacy_rectangle_effect.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/rendering/legacy_rectangle_effect.cpp](../../../src/rendering/legacy_rectangle_effect.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-28bd17b88140** `src/rendering/legacy_scaled_rle_writer.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/rendering/legacy_scaled_rle_writer.cpp](../../../src/rendering/legacy_scaled_rle_writer.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-218a59a666ab** `src/rendering/legacy_text_renderer.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/rendering/legacy_text_renderer.cpp](../../../src/rendering/legacy_text_renderer.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-26653b01ab65** `src/rendering/legacy_text_renderer_runtime.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/rendering/legacy_text_renderer_runtime.cpp](../../../src/rendering/legacy_text_renderer_runtime.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-189eb9838aaa** `src/resource_io/legacy_cm_cache.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/resource_io/legacy_cm_cache.cpp](../../../src/resource_io/legacy_cm_cache.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-761c1222a592** `src/resource_io/legacy_dbcs_text_buffer.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/resource_io/legacy_dbcs_text_buffer.cpp](../../../src/resource_io/legacy_dbcs_text_buffer.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-83704b3568fe** `src/resource_io/legacy_environment.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/resource_io/legacy_environment.cpp](../../../src/resource_io/legacy_environment.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-68d28da0e03c** `src/resource_io/legacy_file.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/resource_io/legacy_file.cpp](../../../src/resource_io/legacy_file.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-7b5b12825801** `src/resource_io/legacy_lmf_archive.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/resource_io/legacy_lmf_archive.cpp](../../../src/resource_io/legacy_lmf_archive.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-12443b10cb29** `src/resource_io/legacy_lzo1x.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/resource_io/legacy_lzo1x.cpp](../../../src/resource_io/legacy_lzo1x.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-7f49784a8f8c** `src/resource_io/legacy_lzo1x_compressor.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/resource_io/legacy_lzo1x_compressor.cpp](../../../src/resource_io/legacy_lzo1x_compressor.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-d09b5ad9449d** `src/resource_io/legacy_memory_manager.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/resource_io/legacy_memory_manager.cpp](../../../src/resource_io/legacy_memory_manager.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-f66ff47e787a** `src/resource_io/legacy_resource_databases.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/resource_io/legacy_resource_databases.cpp](../../../src/resource_io/legacy_resource_databases.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-01fafa4f5568** `src/resource_io/legacy_save_container.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/resource_io/legacy_save_container.cpp](../../../src/resource_io/legacy_save_container.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-2f8005c384fa** `src/resource_io/legacy_save_slots.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/resource_io/legacy_save_slots.cpp](../../../src/resource_io/legacy_save_slots.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-34b4e09defd5** `src/resource_io/window_configuration.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/resource_io/window_configuration.cpp](../../../src/resource_io/window_configuration.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-9c38ba33274e** `src/special_modes/legacy_save_preview.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/special_modes/legacy_save_preview.cpp](../../../src/special_modes/legacy_save_preview.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-543947d018b9** `src/story_scene/legacy_dialog_geometry.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/story_scene/legacy_dialog_geometry.cpp](../../../src/story_scene/legacy_dialog_geometry.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-23bba239d9d3** `src/world_map/legacy_cm_cache_generator.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_cm_cache_generator.cpp](../../../src/world_map/legacy_cm_cache_generator.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-ecdfba3b9790** `src/world_map/legacy_cm_cache_loader.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_cm_cache_loader.cpp](../../../src/world_map/legacy_cm_cache_loader.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-892e067c67d6** `src/world_map/legacy_cm_cache_runtime.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_cm_cache_runtime.cpp](../../../src/world_map/legacy_cm_cache_runtime.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-bbee39931e89** `src/world_map/legacy_maps_world_database.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_maps_world_database.cpp](../../../src/world_map/legacy_maps_world_database.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-f70a18623559** `src/world_map/legacy_role_spatial_query.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_role_spatial_query.cpp](../../../src/world_map/legacy_role_spatial_query.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-364a2d9b958e** `src/world_map/legacy_world_background.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_background.cpp](../../../src/world_map/legacy_world_background.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-21c5c99569b8** `src/world_map/legacy_world_builtin_colors.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_builtin_colors.cpp](../../../src/world_map/legacy_world_builtin_colors.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-48a5a56d74ff** `src/world_map/legacy_world_camera_pan.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_camera_pan.cpp](../../../src/world_map/legacy_world_camera_pan.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-d09677725461** `src/world_map/legacy_world_direction_adjustment.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_direction_adjustment.cpp](../../../src/world_map/legacy_world_direction_adjustment.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-58dc4d413777** `src/world_map/legacy_world_direction_input.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_direction_input.cpp](../../../src/world_map/legacy_world_direction_input.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-bce05682be8f** `src/world_map/legacy_world_facing.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_facing.cpp](../../../src/world_map/legacy_world_facing.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-505ebef80ddf** `src/world_map/legacy_world_facing_role_query.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_facing_role_query.cpp](../../../src/world_map/legacy_world_facing_role_query.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-3c133801bcdc** `src/world_map/legacy_world_frame_tail.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_frame_tail.cpp](../../../src/world_map/legacy_world_frame_tail.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-6771f51330ec** `src/world_map/legacy_world_interpolation.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_interpolation.cpp](../../../src/world_map/legacy_world_interpolation.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-0b685c3675ba** `src/world_map/legacy_world_map_business.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_map_business.cpp](../../../src/world_map/legacy_world_map_business.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-ebb756df97ea** `src/world_map/legacy_world_map_session.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_map_session.cpp](../../../src/world_map/legacy_world_map_session.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-c809b74a7b97** `src/world_map/legacy_world_pathfinding.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_pathfinding.cpp](../../../src/world_map/legacy_world_pathfinding.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-bdf69e196033** `src/world_map/legacy_world_player_control.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_player_control.cpp](../../../src/world_map/legacy_world_player_control.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-08d5bd5e73a9** `src/world_map/legacy_world_player_motion.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_player_motion.cpp](../../../src/world_map/legacy_world_player_motion.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-70f992576304** `src/world_map/legacy_world_render_session.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_render_session.cpp](../../../src/world_map/legacy_world_render_session.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-f10f829d0ada** `src/world_map/legacy_world_role_lookup.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_role_lookup.cpp](../../../src/world_map/legacy_world_role_lookup.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-2d91feee8713** `src/world_map/legacy_world_role_map_update.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_role_map_update.cpp](../../../src/world_map/legacy_world_role_map_update.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-b209162eb62f** `src/world_map/legacy_world_role_post_materialization.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_role_post_materialization.cpp](../../../src/world_map/legacy_world_role_post_materialization.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-a81c0cd91e38** `src/world_map/legacy_world_role_preload.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_role_preload.cpp](../../../src/world_map/legacy_world_role_preload.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-a88621a5e7fc** `src/world_map/legacy_world_role_transfer.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_role_transfer.cpp](../../../src/world_map/legacy_world_role_transfer.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-354e671fcdc6** `src/world_map/legacy_world_runtime_session.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_runtime_session.cpp](../../../src/world_map/legacy_world_runtime_session.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-e1d085361eae** `src/world_map/legacy_world_save_map_overrides.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_save_map_overrides.cpp](../../../src/world_map/legacy_world_save_map_overrides.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-5548629e126e** `src/world_map/legacy_world_save_restore.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_save_restore.cpp](../../../src/world_map/legacy_world_save_restore.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-3bbf3f3acabd** `src/world_map/legacy_world_selection_scroll.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_selection_scroll.cpp](../../../src/world_map/legacy_world_selection_scroll.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-62b7dddb379e** `src/world_map/legacy_world_special_frame_loader.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_special_frame_loader.cpp](../../../src/world_map/legacy_world_special_frame_loader.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-7bbd49c329a0** `src/world_map/legacy_world_story_paths.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_story_paths.cpp](../../../src/world_map/legacy_world_story_paths.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **FILE-894539f89f17** `src/world_map/legacy_world_transient_reset.cpp`
  - 当前残留信号：无关键词命中，仍待语义复核。
  - 历史状态：`not_in_previous_inventory`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_transient_reset.cpp](../../../src/world_map/legacy_world_transient_reset.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-e093caf567da** `story_scene/legacy_dialog_control`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/story_scene/legacy_dialog_control.hpp](../../../include/openswd3/story_scene/legacy_dialog_control.hpp)；[src/story_scene/legacy_dialog_control.cpp](../../../src/story_scene/legacy_dialog_control.cpp)。
  - 类型：[PORT-c406ab9cca7d](invoke-removal-ports.md#port-c406ab9cca7d)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-f08c319a47b6** `story_scene/legacy_dialog_runtime`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/story_scene/legacy_dialog_runtime.hpp](../../../include/openswd3/story_scene/legacy_dialog_runtime.hpp)；[src/story_scene/legacy_dialog_runtime.cpp](../../../src/story_scene/legacy_dialog_runtime.cpp)。
  - 类型：[PORT-a9c84532fd9a](invoke-removal-ports.md#port-a9c84532fd9a)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-206c58b2a009** `story_scene/legacy_dialog_text`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/story_scene/legacy_dialog_text.hpp](../../../include/openswd3/story_scene/legacy_dialog_text.hpp)；[src/story_scene/legacy_dialog_text.cpp](../../../src/story_scene/legacy_dialog_text.cpp)。
  - 类型：[PORT-5560115ba4f6](invoke-removal-ports.md#port-5560115ba4f6)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-2efc12f64f13** `world_map/legacy_movement_collision`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_movement_collision.hpp](../../../include/openswd3/world_map/legacy_movement_collision.hpp)；[src/world_map/legacy_movement_collision.cpp](../../../src/world_map/legacy_movement_collision.cpp)。
  - 类型：[PORT-980920a4709e](invoke-removal-ports.md#port-980920a4709e), [PORT-12d0496618d5](invoke-removal-ports.md#port-12d0496618d5)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-e916d4c1ea31** `world_map/legacy_moving_actions`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_moving_actions.hpp](../../../include/openswd3/world_map/legacy_moving_actions.hpp)；[src/world_map/legacy_moving_actions.cpp](../../../src/world_map/legacy_moving_actions.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-4a5c1e8d4374** `world_map/legacy_picture_actions`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_picture_actions.hpp](../../../include/openswd3/world_map/legacy_picture_actions.hpp)；[src/world_map/legacy_picture_actions.cpp](../../../src/world_map/legacy_picture_actions.cpp)。
  - 类型：[PORT-d5fee01d31a7](invoke-removal-ports.md#port-d5fee01d31a7)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-09792c89ca57** `world_map/legacy_role_head_actions`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_role_head_actions.hpp](../../../include/openswd3/world_map/legacy_role_head_actions.hpp)；[src/world_map/legacy_role_head_actions.cpp](../../../src/world_map/legacy_role_head_actions.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-030818649640** `world_map/legacy_world_collision_talk`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_collision_talk.hpp](../../../include/openswd3/world_map/legacy_world_collision_talk.hpp)；[src/world_map/legacy_world_collision_talk.cpp](../../../src/world_map/legacy_world_collision_talk.cpp)。
  - 类型：[PORT-54a06032340a](invoke-removal-ports.md#port-54a06032340a)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-11fe98cbcf7c** `world_map/legacy_world_cursor`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_cursor.hpp](../../../include/openswd3/world_map/legacy_world_cursor.hpp)；[src/world_map/legacy_world_cursor.cpp](../../../src/world_map/legacy_world_cursor.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-de7f3edae674** `world_map/legacy_world_dialog_runtime`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_dialog_runtime.hpp](../../../include/openswd3/world_map/legacy_world_dialog_runtime.hpp)；[src/world_map/legacy_world_dialog_runtime.cpp](../../../src/world_map/legacy_world_dialog_runtime.cpp)。
  - 类型：[PORT-9a9f9f7e18af](invoke-removal-ports.md#port-9a9f9f7e18af), [PORT-c94c56f211eb](invoke-removal-ports.md#port-c94c56f211eb)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-29c2cc7180e4** `world_map/legacy_world_facing_talk`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_facing_talk.hpp](../../../include/openswd3/world_map/legacy_world_facing_talk.hpp)；[src/world_map/legacy_world_facing_talk.cpp](../../../src/world_map/legacy_world_facing_talk.cpp)。
  - 类型：[PORT-66dcb15417c5](invoke-removal-ports.md#port-66dcb15417c5)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-86de8c4041f6** `world_map/legacy_world_flagged_roles`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_flagged_roles.hpp](../../../include/openswd3/world_map/legacy_world_flagged_roles.hpp)；[src/world_map/legacy_world_flagged_roles.cpp](../../../src/world_map/legacy_world_flagged_roles.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-5e6c5c136354** `world_map/legacy_world_frame_coordinator`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_frame_coordinator.hpp](../../../include/openswd3/world_map/legacy_world_frame_coordinator.hpp)；[src/world_map/legacy_world_frame_coordinator.cpp](../../../src/world_map/legacy_world_frame_coordinator.cpp)。
  - 类型：[PORT-e5caa4d15024](invoke-removal-ports.md#port-e5caa4d15024)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-b9d09741b314** `world_map/legacy_world_frame_runtime`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_frame_runtime.hpp](../../../include/openswd3/world_map/legacy_world_frame_runtime.hpp)；[src/world_map/legacy_world_frame_runtime.cpp](../../../src/world_map/legacy_world_frame_runtime.cpp)。
  - 类型：[PORT-bc68e5f72522](invoke-removal-ports.md#port-bc68e5f72522), [PORT-d5738efc831a](invoke-removal-ports.md#port-d5738efc831a)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-991001d7cac7** `world_map/legacy_world_head_sign_actions`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_head_sign_actions.hpp](../../../include/openswd3/world_map/legacy_world_head_sign_actions.hpp)；[src/world_map/legacy_world_head_sign_actions.cpp](../../../src/world_map/legacy_world_head_sign_actions.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-e692608030d3** `world_map/legacy_world_indexed_objects`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_indexed_objects.hpp](../../../include/openswd3/world_map/legacy_world_indexed_objects.hpp)；[src/world_map/legacy_world_indexed_objects.cpp](../../../src/world_map/legacy_world_indexed_objects.cpp)。
  - 类型：[PORT-45f210907f93](invoke-removal-ports.md#port-45f210907f93), [PORT-21921a6efc43](invoke-removal-ports.md#port-21921a6efc43)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-1be98ce62407** `world_map/legacy_world_interaction`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_interaction.hpp](../../../include/openswd3/world_map/legacy_world_interaction.hpp)；[src/world_map/legacy_world_interaction.cpp](../../../src/world_map/legacy_world_interaction.cpp)。
  - 类型：[PORT-c524857f12ae](invoke-removal-ports.md#port-c524857f12ae)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-d45ec52209c6** `world_map/legacy_world_load_progress`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_load_progress.hpp](../../../include/openswd3/world_map/legacy_world_load_progress.hpp)；[src/world_map/legacy_world_load_progress.cpp](../../../src/world_map/legacy_world_load_progress.cpp)。
  - 类型：[PORT-6aa707fd82e4](invoke-removal-ports.md#port-6aa707fd82e4)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-586f36231047** `world_map/legacy_world_party_role_actions`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_party_role_actions.hpp](../../../include/openswd3/world_map/legacy_world_party_role_actions.hpp)；[src/world_map/legacy_world_party_role_actions.cpp](../../../src/world_map/legacy_world_party_role_actions.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-b8e007e48275** `world_map/legacy_world_path_requests`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_path_requests.hpp](../../../include/openswd3/world_map/legacy_world_path_requests.hpp)；[src/world_map/legacy_world_path_requests.cpp](../../../src/world_map/legacy_world_path_requests.cpp)。
  - 类型：[PORT-f5d91ea8758f](invoke-removal-ports.md#port-f5d91ea8758f)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-fdb7255cad6a** `world_map/legacy_world_path_script`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_path_script.hpp](../../../include/openswd3/world_map/legacy_world_path_script.hpp)；[src/world_map/legacy_world_path_script.cpp](../../../src/world_map/legacy_world_path_script.cpp)。
  - 类型：[PORT-1743d43fcf00](invoke-removal-ports.md#port-1743d43fcf00)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-e3dffa9bac9a** `world_map/legacy_world_player_post_frame`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_player_post_frame.hpp](../../../include/openswd3/world_map/legacy_world_player_post_frame.hpp)；[src/world_map/legacy_world_player_post_frame.cpp](../../../src/world_map/legacy_world_player_post_frame.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-11a8a8a5cfad** `world_map/legacy_world_roles`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_roles.hpp](../../../include/openswd3/world_map/legacy_world_roles.hpp)；[src/world_map/legacy_world_roles.cpp](../../../src/world_map/legacy_world_roles.cpp)。
  - 类型：[PORT-a913b1475959](invoke-removal-ports.md#port-a913b1475959), [PORT-e8a9ed211ce0](invoke-removal-ports.md#port-e8a9ed211ce0), [PORT-7247e75a9812](invoke-removal-ports.md#port-7247e75a9812)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-d6fa06743efc** `world_map/legacy_world_save_item_lists`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[src/world_map/legacy_world_save_item_lists.cpp](../../../src/world_map/legacy_world_save_item_lists.cpp)。
  - 类型：无具名Port/Dispatch/Adapter定义，检查函数与数据协议。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-8cfb6326db77** `world_map/legacy_world_save_restore`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_save_restore.hpp](../../../include/openswd3/world_map/legacy_world_save_restore.hpp)。
  - 类型：[PORT-aa1159223764](invoke-removal-ports.md#port-aa1159223764)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。

- [ ] **CHAIN-6ba33e14da0c** `world_map/legacy_world_spatial_audio`
  - 当前残留信号：interface。
  - 历史状态：`pending`；本快照：待复核。
  - 文件：[include/openswd3/world_map/legacy_world_spatial_audio.hpp](../../../include/openswd3/world_map/legacy_world_spatial_audio.hpp)；[src/world_map/legacy_world_spatial_audio.cpp](../../../src/world_map/legacy_world_spatial_audio.cpp)。
  - 类型：[PORT-d2da76fbb056](invoke-removal-ports.md#port-d2da76fbb056)。
  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。
