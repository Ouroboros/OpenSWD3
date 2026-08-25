# 建立FFDC缺省道具记录并写数量1和固定名称 `0x0044D5D0`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0044D5D0..0x0044D611`，32行，无外部FUNCTION CHUNK。直接caller分布于全局/世界初始化、炼妖、护驾、选择记录和后续玩家道具路径；callee仅一次176字节分配。

函数严格执行：

1. 申请176字节记录。
2. 清零44个dword。
3. 再显式写`combined_value=0`和`next=null`。
4. 写ID低16为`0xFFDC`。
5. 写数量A为1；数量B及其余字段保持清零。
6. 把原版固定缺省名称复制到记录内名称区，返回记录。

typed实现以`LegacyStandardModeQuantityPorts`复用缺省名称初始化。分配null只在原memset点typed-stop，不调用名称初始化，也不伪造记录。

## 2. 验证

UT预先把目标记录填入非零ID、A/B、combined、flags和旧名称，验证成功后只保留FFDC、A=1、固定名称和null next，其余测试字段清零；另覆盖分配停止时返回null且不初始化名称。

workpack双生成稳定为`187/227`，SHA256为`e3e8bafa0dee2c889beaf5f1a2b3d6b057f71655932bcb3574bdd2a749d1f019`；下一项为`0x0044D620`。
