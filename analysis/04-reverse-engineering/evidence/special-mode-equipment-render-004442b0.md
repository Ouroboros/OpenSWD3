# 装备物品模式主渲染 `0x004442B0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x004442B0..0x00444D9E`，1235行，无FUNCTION CHUNK。code caller `0x00444FC0`仍独立待审；3B480另绑定为渲染callback。B9A0、B9C0与AE40已关闭并直接复用；其余平台绘制、颜色、动作装载与资源解析callee通过typed render request和最窄端口隔离。

## 数据所有权

保留两条不同链：`record_head`是总链，用于入口自动转场和末尾动画索引；`visible_record_head`是当前窗口链，只用于列表逐行绘制。E40已有字段继续作为共享快照：`first_render_zero`对应列表绘制偏移，signed `second_render_zero`对应面板速度，`final_zero`对应四组滚动条衰减nibble，`published_action_count`对应页签/面板结果；不复制第二套别名。`frame_source_word`仅表达原低16位平台资源句柄。

## 固定面板与普通列表

入口按原顺序构造三种颜色。mode1仅在transition端口非零时用B9A0索引总链；节点存在且text index不等于FFDC时写速度128、mode5。随后准备表面、绘制两组frame/tiled-frame和三项页签；未选页签先按`(1,-4,-4,-4)`暗化，选中页签恢复原色并左上偏移1。

总链为空时绘制empty文本。否则从窗口链迭代`visible_record_count`项：保存当前选中节点；type低四位1/0分别切换颜色并取消选中偏移，其他type保留选中偏移；mode大于1再次暗化。名称按`%-14s`，cost的bit15先格式化`&7FFF`，bit14随后以`&3FFF`覆盖。每项先发布物品tile，再按action variant表`8,1,2,3,4,-1,5,6,7,-1,9,10`决定动作绘制；mode1选中项最后绘制selection。

## 滚动条与分支面板

总记录数大于24时，依次衰减`final_zero`低、次低nibble并形成overlay flags 1/2；ratio严格经double除法再转float，直接调用AE40，回写四个动态Y边界。

mode2绘制party面板，跳过FFFF marker；选中party使用高亮色、左上偏移与selection，Y只随有效party累加。普通列表保存的选中节点非空且非FFDC时绘制底部遮罩、shared-text详情和全屏恢复矩形。

mode15绘制split panel，从`special_window_offset`开始，最多处理`hover_record_count`且不越过`special_record_count`；文本来自BE90 typed记录，hover项绘制selection。总数大于可见数时依次衰减bits8..11和12..15两个nibble，以double ratio直接调用AE40并回写第二组四个动态Y边界。

mode17/18绘制三行对话记录。原指针为空时的52-byte临时分配、默认字符串、三次16-byte绘制和释放由三条fallback typed request等价表达；非空记录不足三项时在split panel之后typed-stop。

## 面板速度与停止点

返回值先取viewport。仅当速度非零或viewport等于360时，signed速度算术右移1并从viewport扣除：正速度下限360，非正速度上限480，命中边界时清速度。状态写入后用B9C0索引总链；缺失只在原动画记录解引用点typed-stop。成功时绘制移动frame、tiled-frame及记录动画，并返回最后一个平台绘制结果。

UT覆盖普通选中项、bit15/bit14覆盖cost、variant映射、两项低byte滚动衰减、页签暗化、自动mode5转场、动画状态前缀、动作加载和variant表停止、窗口链停止、mode15筛选/选择/高byte滚动、mode17/18不足记录及fallback三行。定向测试通过。

workpack双生成稳定为`109/227`，SHA256均为`10ff286e2018b858e2d4fd2ef253d8713ce29a737b8763e68544f558e9fe87a4`；下一单元`0x00444DB0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
