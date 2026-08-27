# 战斗菜单上下文前进 `0x00462510`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00462510..0x0046262D`，从proc到endp共128行、70条实际指令、4个静态call、9个跳转、9个局部/返回标签、1个`retn`，没有外部`FUNCTION CHUNK`。

唯一静态caller位于已关闭逐帧输入记录5路径。四个静态call都是既有样本播放边界，固定样本号46。

## 2. 公共入口与独立message重读

入口把共享message装入EAX，并无条件清final-actor pre-frame gate B。message 1、2、4、30不是互斥switch；每个后续条件都重新读取共享message。因此样本回调若依次改写message，同一次调用可以按顺序执行最多四个分支。实现不能缓存入口message后改成单一分支。

未命中任何分支时，返回入口message、caller ECX/EDX，只保留gate清零。

## 3. message 1：动作种类与九字节权限

EAX从共享action kind读取，EDX把selection word零扩展后加5，ECX为action kind加4。只有signed `ecx <= edx`时才把ECX发布为新action kind；随后用当前EAX索引由`0x00524413`前置byte和后续两个dword组成的九字节物理权限域。

权限byte覆盖CL但保留ECX高24位。权限为0时EAX减4并回写action kind。索引越界只在首次真实byte读取typed-stop；此前gate清零和可能发生的action kind前进保持可见，样本与后续message分支被阻断。

样本调用前EAX装载live signed mix位模式，ECX保留低byte替换结果，EDX保留零扩展selection word加5。

## 4. message 2：三类动作索引

共享action category按u32加1并立即回写；signed结果大于等于3时只把存储值改为0，EAX仍保留加1结果。负signed结果不会现代化夹回0。

随后ECX装载live mix位模式，把list selection写1并清panel scroll A，再播放样本。EDX保持进入该分支时的值。

## 5. message 4：装备分类与缓存恢复

current equipment selection先u32加1并回写；signed结果大于等于4时把EAX和存储都置0。负signed结果保留，并在首次真实四槽equipment grid cache读取typed-stop。

有效索引先读取equipment grid selection到EDX，再读取同索引startup scroll cache到EAX；两次读取完成后才分别发布grid selection与panel scroll B。样本调用前ECX为live mix，EAX/EDX为恢复后的scroll/grid值。

## 6. message 30：五格前进与陈旧EAX

共享grid selection按u32加5并先回写；signed结果大于10时只把存储夹为10，EAX仍保留夹值前结果。负signed结果不夹。EDX装载live mix，ECX保持前一路径或caller值，然后播放样本。

## 7. caller回收与验证

逐帧输入原`commit_right`槽保留相同枚举数值并改为reserved。记录5在对话为空且message为3时先执行已关闭菜单选择前进，随后直接执行本函数，普通返回后才进入正向角色动作轮转。本函数permission或equipment typed-stop会保留前缀并阻断后续轮转。

定向测试覆盖未处理message、message 1允许/拒绝/权限越界、message 2回绕与负signed值、message 4缓存恢复与负索引停点、message 30夹值前EAX，以及样本回调依次把message改为2/4/30后同次执行四个分支。caller测试覆盖普通直连、reserved槽零调用与typed-stop阻断动作轮转。

当前缺少原版九字节权限相邻内存、四项装备缓存、样本后端副作用、记录5输入及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
