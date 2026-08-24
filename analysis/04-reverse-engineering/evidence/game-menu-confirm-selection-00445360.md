# 确认游戏内菜单选项并打开对应页面 `0x00445360`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00445360..0x004453E7`，65行，无FUNCTION CHUNK。code caller为已关闭450E0输入函数；3B480另把它绑定为G08动作callback。450E0已直接回收。

函数先快照selection并把lifecycle写2。selection17跳过画面索引与flag查询；其他selection把画面索引写为selection加41，且只有flag49精确等于1时互换56/57。

随后以lifecycle2为secondary、前一步发布的横坐标为primary直接调用已关闭3B480。选择11..17的横坐标依次为30、36、42、48、54、60、66，恰好选择G01..G07；54/60两组会在3B480内部按原规则追加一次flag49查询。重绑后以selection原值读取FC0安装的二级初始化表索引11..17并调用目标，最后以命令187和共享sample owner播放确认音效，返回音效callee结果。

selection越界只在原二级表读取点typed-stop，保留lifecycle、画面及主回调重绑副作用；目标为0或typed初始化端口拒绝时在原间接call点停止，不播放音效。result累计本函数和3B480嵌套flag查询。

UT覆盖selection15的画面交换、横坐标54选择G05、二级目标与sample参数；selection17跳过画面查询并选择G07；selection越界、空目标和初始化停止的精确副作用；450E0直接调用后传播提交停止及嵌套查询。

workpack双生成稳定为`120/227`，SHA256均为`b98c081af90fa966a11ef573f8f336c59e3c87437f32afb4f17a90756a0a675c`；下一单元`0x004453F0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
