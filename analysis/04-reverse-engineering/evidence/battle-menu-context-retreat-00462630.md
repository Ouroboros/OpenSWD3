# 战斗菜单上下文后退 `0x00462630`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00462630..0x00462738`，从proc到endp共122行、66条实际指令、4个静态call、9个跳转、9个局部/返回标签、1个`retn`，没有外部`FUNCTION CHUNK`。

唯一静态caller位于已关闭逐帧输入记录3路径。四个静态call都是既有样本播放边界，固定样本号46。

## 2. 公共入口与强制常量

入口读取共享message到EAX，并无条件清final-actor pre-frame gate B。message 1处理结束或跳过后，函数重新把live message装入ECX并强制把EAX写2，再判断message 2。之后message 4与30继续独立重读共享message。

因此未处理message也返回EAX 2、ECX live message和caller EDX；message 1样本返回的EAX/ECX会被2与live message覆盖，只保留callee EDX。样本回调若依次改写message，同次调用仍可顺序执行四个分支。

## 3. message 1：动作种类回退

EAX读取action kind，ECX计算`eax-4`。只有signed ECX大于等于1时，才把ECX发布为新action kind并作为权限索引；否则仍使用旧EAX。权限来自同一九byte物理域并覆盖CL低byte。

权限为0时EAX加4并回写action kind。索引越界在首次真实byte读取typed-stop；此前gate清零和可能发生的action kind回退保持可见。样本前EAX为live mix位模式，ECX保留权限低byte覆盖，EDX保持进入分支的值。

## 4. message 2：三类动作索引回退

进入检查前EAX固定为2。action category按u32减1并立即回写；signed结果为负时只把存储改为EAX常量2，随后ECX装载live mix，把list selection写1并播放样本。与前进版本不同，本函数不清panel scroll A。

## 5. message 4：装备分类回退

current equipment selection按u32减1并先回写；signed结果为负时把EAX与存储都改为3。非负但大于四槽范围的值不现代化夹值，在首次真实equipment grid cache读取typed-stop。

有效索引先读取grid cache到EDX，再读取startup scroll cache到EAX；两次完成后才发布grid selection与panel scroll B。样本前ECX为live mix，EAX/EDX为恢复后的scroll/grid值。

## 6. message 30：五格回退与陈旧EAX

共享grid selection按u32减5并先回写；signed结果小于1时只把存储夹为1，EAX仍保留减法结果。EDX装载live mix，ECX保留此前live message、上一callee或caller值，再播放样本。

## 7. caller回收与验证

逐帧输入原左向动作槽保留相同枚举数值并改为reserved。记录3先完成热点低word回绕、可选菜单选择后退和反向角色动作轮转，再直连本函数；本函数typed-stop保留已完成轮转并阻断后续输入记录。

定向测试覆盖未处理message强制返回2、message 1允许/拒绝/权限越界与callee覆盖、message 2回绕2、message 4回绕3与正向大索引停点、message 30下限1与陈旧EAX，以及样本回调依次把message改为2/4/30后同次执行四分支。caller测试覆盖普通直连、reserved槽零调用和typed-stop顺序。

当前缺少原版九byte权限相邻内存、四项装备缓存、样本后端副作用、记录3输入/热点链与EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
