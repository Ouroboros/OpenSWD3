# SDL入战镜像设置

本批修正SDL建场对共享镜像值的读取及阵形绑定的覆盖。
完整初始化、零敌方返回、帧与实际续玩仍未验收。

## 原版合同

`451F9A..451FD0`读取`53C000`，完整DWORD精确等于1时，
调用角色镜像设置，再以WORD计算`640-x`。其他值不执行镜像。
`4521DD..452219`先重置队员，再比较同一DWORD；精确1时写
WORD横坐标`640-x`及DWORD阵形横坐标`624-anchor`。
这些读取不回写或归一化镜像值。

`46B9EF..46BA21`的脚本case45直接切换同一状态：1变0，其他值变1。
现有C++对应`startup.mirror_mode`，没有另加入口镜像字段。

## SDL接线

此前SDL向`prepare_legacy_battle_setup`固定传false，
后续`bind_legacy_battle_setup_party_owners`又按该快照写回0/1，
导致实际共享镜像设置被忽略并清零。

建场参数改收完整DWORD，内部只有精确1才生成镜像坐标。
SDL传入当前`battle_runtime_.mirror_mode`，阵形绑定不再回写镜像值。
原bool调用者仍对应0/1，不改变独立资产布局测试的合同。

setup中的`mirrored`只描述其坐标是否已镜像，不作为状态真值。
敌方存储按这一表示还原原WORD，再进入已有共同镜像与MON配置。
队伍阵形绑定同样还原原坐标，实际队员reset后仍读取共享镜像值。
不移动已有镜像设置调用，不改变坐标宽度或角色配置顺序。

## 验证与边界

新增固定状态测试覆盖0、1、2、0x10001、FFFFFFFF及重复0/1进入；
只有1镜像。敌方65535镜像后为641，队员527变113、anchor537变87。
阵形绑定恢复原坐标，同时保留原DWORD，包括非1高位值；
旧未镜像setup也不能清除后来写入的镜像值。
已有双方角色初始化、MON配置和镜像测试继续执行。

core setup 1/1（5.83秒）、ASan setup 1/1（8.88秒）及SDL构建通过。
本批构建日志无警告、测试失败或sanitizer错误；日志位于
`build/tmp/runtime/battle-startup-mirror-{core,asan,sdl}.log`。
未启动游戏，未取得本批原版动态差分；B10仍315/422，316 pending_audit。
