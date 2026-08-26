# 战斗当前角色优先顺序 `0x0045B280`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整LST范围

权威函数为`0x0045B280..0x0045B59C`，从proc到endp完整384行、3个call站点、32个`loc_`标签，无外部FUNCTION CHUNK。

两个call站点调用同一角色配对查询callee；尾部第三个call是已关闭的`0x0045B190`角色顺序重建。唯一静态caller位于已关闭的逐帧画面协调器，现已删除opaque边界并直接组合typed实现。

## 2. 四项入口早退

函数先把caller EAX的AL替换为共享byte门。该byte完整等于1时立即返回，保留EAX高24位与caller ECX/EDX。

随后按固定顺序读取两项完整dword mode：

1. group A mode等于1时返回EAX=1；
2. group B mode等于1时返回此前读取的group A mode；
3. 当前角色完整dword等于`0xFFFFFFFF`时返回该值。

所有早退都发生在角色对象、metric表、mask和顺序表访问之前，不发布ready，不清mask。

## 3. 当前角色与配对角色

当前角色按signed `< 8`分组：

- group B以固定基址、`0x2B28`步长计算对象token；callee入口陈旧EAX为索引乘345；
- group A以`index - 8`、固定基址、`0x2F34`步长计算对象token；callee入口陈旧EAX为相对索引乘3021。

callee返回只取AX并按i16符号扩展。group B路径在callee后重读group B mode，完整值为0才对配对索引加8；group A路径重读group A mode，完整值为1才加8。token和加法均保持低32位回绕，异常索引不会在token计算处提前停止。

## 4. 同组低metric前缀

当前角色metric在配对callee完成后才首次访问。函数固定扫描当前角色所在物理组：group B为0..7，group A为8..17。候选必须：

- metric非零；
- 不是当前角色；
- 不是配对角色；
- metric按signed严格小于当前角色metric。

合格候选按signed metric升序稳定插入角色顺序表，同时把对应mask写1。相等值不前移。

原插入循环保留一个重要陈旧尾副作用：发生中部插入时，移动次数是`count - position + 1`，先把活动尾后一槽的旧值再向后复制一次，随后才移动活动项并写入候选。typed实现不把它修正成普通容器insert；任何越界只在对应顺序表read/store首次发生时停止，此前复制不回滚。

## 5. 剩余角色补齐

函数在同组前缀后读取group B与group A完整u32数量并按低32位求和。前缀数量不等于该固定总数时，从当前输出尾反复补齐。

每轮初始候选从完整18槽metric表索引0开始：

- metric必须非零；
- 当前角色仍有效时，当前组内metric大于等于当前角色metric的槽被过滤；
- mask只有精确值1表示已选。

找到候选后会重复读取其metric。18槽都无候选时，在索引18的下一次真实metric读取处typed-stop，不提前清mask。

随后按固定顺序比较：

- group B从索引0扫描到本轮开始时的group B数量；每槽先mask后metric；只有当前角色属于group A或已局部失效时，较小group B metric才能替换候选；
- group A每轮重新读取数量，以`count + 8`的u32边界从索引8扫描；同样先mask后metric；只有当前角色属于group B或已局部失效时，较小group A metric才能替换候选；
- group A扫描后重新读取group B数量，供下一轮使用和最终EDX保留。

两组比较都不重复初始候选的非零门，因此零metric可以按signed比较击败正候选。等值不替换。

## 6. 配对命中与普通发布

普通候选按“读取旧数量、顺序表store、寄存器内数量加1、mask写1、回存数量”发布。

若选中索引等于配对角色，则严格按原顺序：

1. 顺序表写配对角色；
2. 配对mask写1；
3. 输出游标前进；
4. 当前角色mask写1；
5. 顺序表写当前角色；
6. 选择数量一次增加2；
7. 仅把局部当前角色与局部配对角色改为`0xFFFFFFFF`。

共享当前角色dword不在此路径改写。若固定总数只剩一项，数量会越过终值；原循环只检查精确相等，typed实现不夹值或增加现代上限。

## 7. 尾部清理与哨兵重排

选择数量精确等于固定总数后，函数固定清零18 dword mask，再把共享顺序ready写1。随后扫描角色顺序表全部18槽：

- 未发现值18时返回顺序表尾token，ECX为0，EDX保留最后读取的group B数量；
- 首次发现值18时立即直连已关闭的稳定角色顺序重建一次，并返回其完整EAX/ECX/EDX；不继续扫描。

mask清零和ready发布发生在哨兵调用之前。嵌套重建typed-stop不会回滚此前尾部副作用。

## 8. typed-stop与测试

访问顺序分别保持：当前角色对象callee先于当前metric、同组插入的陈旧尾read先于移动store、剩余初始扫描metric先于mask、两组扫描mask先于metric、配对路径顺序表先于mask再到当前角色mask与顺序表。

因此异常索引或数量只在首次真实metric、mask或角色顺序表访问处typed-stop；此前callee、排序前缀、陈旧尾复制、mask和顺序表store保留，异常路径不执行最终mask清零与ready发布。

定向测试覆盖四项入口早退与陈旧寄存器、两组对象token与callee陈旧EAX、i16配对索引、同组signed稳定插入、陈旧尾额外复制、异组优先补齐、零metric比较、配对双发布、当前角色局部失效、callee先于当前metric越界、group B mask索引18停点、异常不清mask、正常ready、顺序表尾返回、值18哨兵直连重建，以及逐帧caller成功与typed-stop传播。

当前缺少原版入口门、两项mode、当前角色、配对callee、两组动态数量、metric表、mask、顺序表、ready和寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
