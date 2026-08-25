# 模式1按确认提交当前层级 `0x0044E4A0`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0044E4A0..0x0044E99D`，605行，无外部FUNCTION CHUNK。caller为`0x0044DBC0`和`0x0044F920`。switch以`level-1`为索引，显式处理level1、2、3、4、10；level5..9及其他值无副作用。

## 2. level1初始化或关闭

入口先把level加1并复用D5A0释放旧工作区。

- packed低两位为2：level写回1，直接复用E9D0完整关闭特殊模式运行资源。
- packed低两位为1：复用D520反向深克隆玩家链，以head变量构造F800前驱哨兵；F800把有效记录移入排序工作区，D5A0释放留在源链的零权重/bit6记录。工作区combined数量全部清0，运行标志bit0清除；空工作区把level退回1。
- packed低两位为0：按零结尾ID表逐项以D2D0 operation1/数量99建立临时来源链，再经F800构建工作区并释放余链。保留原顺序：重置窗口前先按旧`window+cursor`解析新工作区文字。

非空工作区共用：window/cursor清0，B980统计完整数量，visible head指向head，F7D0统计最多13项；运行标志bit1未置时播放样本BB并置位；最后D6B0索引当前记录并复用FAF0刷新四角色差值。

## 3. level2进入提交

复用已纠正F770计算总权重。仅signed总权重大于0时level加1、清packed bit2并播放BB；非正值保持原状态且无声音。F770循环停止保留此前状态。

## 4. level3取消或提交

入口无条件level加1。

- packed bit2置位：level写2，清零所有工作区combined数量，最后清packed bit2/3并播放BB。
- packed低两位为1：level写2，容量按32位加F770结果。对signed(`first+second`)大于0且combined非0的记录，以combined低16取负调用D2D0 operation0扣除玩家道具；D680找不到同ID记录时从工作区摘下并释放，找到时只清combined。工作区清空则立即返回，不播放声音、不清packed选项；非空时先播放B9，再按原旧visible分支重建窗口、解析文字，level写1，最后清bit2/3并播放BB。
- 其他低两位：容量按32位减F770结果，运行标志先清bit2。每条非零记录严格扫描11个掩码；任一`(mask & filter_flags)==mask`时置运行bit2。随后以D2D0 operation0把combined加入玩家道具并清零。最终运行bit2未置时level写1，否则保留入口加值后的4；清packed bit2/3并播放BB。

## 5. level4、level10与typed-stop

level4先写level1。packed bit0/bit2任一置位或bit3置位时抑制转换且无声音；三位全清时发布`0xC0000001`外部转换请求并播放B9。level10只写level2。

链、clone、F800、D2D0、D680、11掩码、共享文字、D6B0、FAF0及E9D0均在原读取/调用点typed-stop。特别是掩码表不足时保留level加值、容量扣减、运行bit2清除和当前未清数量；F770/D2D0停止保留此前原始副作用。

## 6. 验证

UT覆盖level1玩家链初始化、旧工作区释放/排序/清数量/窗口重置/BB/FAF0，及低模式2完整关闭；level2正权重进入提交；level3取消两条数量、空模式向玩家道具operation0的second数量转移、短掩码前缀停止、玩家模式扣除后工作区清空早退；level4转换发布与bit3抑制；level10返回2。

workpack双生成稳定为`211/227`，SHA256为`352cef8ec7f4c94f2ecc6e25e3f566f707f5988d17c8e9669848d2dadab9d423`。`0x0044DBC0`继续等待callee闭环；下一单元为`0x0044F920`。
