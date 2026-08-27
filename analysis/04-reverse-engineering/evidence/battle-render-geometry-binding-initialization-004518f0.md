# 战斗渲染几何绑定初始化包装器 `0x004518F0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`callee_reclaimed`。

## 1. 完整LST范围

权威LST函数为`0x004518F0..0x004518FF`，共14行，无外部FUNCTION CHUNK。

有效指令顺序：

1. 压入固定几何owner `0x0053B0B8`；
2. 将固定绑定对象`0x004FF5B8`写入ECX；
3. 调用`0x0045F0F0`；
4. 直接返回。

函数无条件分支、局部栈帧、caller侧参数回收或返回后处理。

## 2. 调用ABI与返回

`0x0045F0F0`以ECX接收绑定对象，以唯一栈参数接收几何owner，并以`retn 4`回收参数。因此wrapper不得在调用后再次调整ESP。

wrapper没有修改callee EAX；完整32位callee返回就是wrapper返回。

深层callee现已由战斗工作包`audit_order=106`关闭。包装器删除旧对象初始化端口并直接组合精确`0x31F4` typed绑定对象；callee以`retn 4`回收参数的ABI仍由本包装器语义保持。

## 3. typed映射

`initialize_legacy_battle_render_geometry_binding`发布：

- `binding_object_token=0x004FF5B8`；
- `render_geometry_owner_token=0x0053B0B8`；
- `initialization_calls=1`；
- 深层初始化写入几何owner和30条固定索引记录；
- 完整EAX固定返回绑定对象token。

旧地址只作为32位token传递，不转换为主机指针。绑定对象只保留一份typed存储，头部和保留区不被本包装器或深层初始化清零。

相邻静态thunk `0x004518E0`已回收临时opaque entry，现直接传递同一绑定对象并调用本typed helper；固定参数仍只由本helper准备一次。

## 4. 双向追溯

- `0x004518F0..0x004518F4`：固定几何owner映射到深层thiscall唯一栈参数；
- `0x004518F5..0x004518F9`：固定绑定对象映射到深层thiscall ECX；
- `0x004518FA..0x004518FE`：单次callee调用映射到`initialization_calls=1`和30条记录写入；
- `0x004518FF`：无后处理返回映射为绑定对象token EAX。

C++到LST反向追溯没有额外参数、夹值、状态写或返回变换。

## 5. 验证与动态差分

定向测试覆盖：

- 两个固定token精确传入；
- wrapper直接调用一次已关闭深层初始化；
- 深层完整写入30条记录并固定返回绑定对象token；
- 静态thunk直连本helper且复用同一对象；
- thunk不重复准备或改写固定参数；
- battle聚合目标零warning构建、普通定向与独立ASan定向均`1/1`通过。

当前没有原版绑定对象完整内存、几何owner和后续资源读取联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。本固定参数wrapper已由完整LST与已关闭callee闭环。
