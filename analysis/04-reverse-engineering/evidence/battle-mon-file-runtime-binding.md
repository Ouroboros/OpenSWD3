# MON文件在SDL中的打开、定位与短读

状态：具名接口迁移的定向测试、ASan及SDL链接验证通过。

## 范围与独立性

SDL文件端口复用`LegacyFile`，通过`open_file(path, data_directory)`、
`seek_file(handle, distance, origin)`和
`read_file(handle, destination, requested_bytes)`直接访问文件。
profile与definition解析器使用对应的具名端口方法，共享同一文件会话。
通用操作枚举、请求包及寄存器回复包已删除。

可观察结果：按原打开参数访问MON文件；定位返回实际位置；读取只覆盖
实际读到的字节，短读保留目标后缀；无效句柄报告失败。错误路径不再
返回无条件成功，也不在指定文件打开失败后改读另一份文件。

当前接口迁移同时覆盖解析器、存储端口及其调用方。完整SDL初始化与
实际续玩仍未完成。下文历史验证不能替代当前迁移的验证。

## LST与API合同

- `00476AAF..00476AC6`及`00476E26..00476E3D`：只读、共享读、
  OPEN_ALWAYS（4）、普通属性（80h），安全属性和模板句柄为空。
- 两个caller在打开失败后返回零，不执行后续目录读取。
- `00476AF6..00476B6C`：profile依次定位目录probe、索引和内容。
- `00476E6E..00476EE0`：definition第二次定位使用FILE_CURRENT；
  距离为`low16(id)*4-4`，必须按有符号32位解释。
- `00476B1D/00476B54/00476B9B`和
  `00476E94/00476EC8/00476F13`使用ReadFile及实际字节数输出。
  读取不足不清除未覆盖的调用者存储。

文件端口固定使用上述同步打开参数。定位距离使用有符号32位值，定位
方式使用具名枚举；超出span的读取明确拒绝。接口不再接收高位距离指针、
打开标志组合或非文件操作请求。

## 持有者与平台差异

`LegacyBattleMonFileRuntime`持有每次成功打开的`LegacyFile`。
句柄是登记槽的身份，不截断宿主指针；打开失败不销毁先前成功打开的对象。
现有解析器的惰性打开状态决定是否再次调用open，端口不另设一份状态。
宿主会话结束时由RAII关闭文件，不增加游戏逻辑中的显式关闭调用。

Windows使用既有原生文件实现，其他宿主复用已隔离的POSIX文件实现。
原字面量`mon.dat`在区分大小写的宿主上可解析同目录已有`MON.DAT`；
此处理在打开前完成，不允许自定义路径失败后转读默认文件。
打开和定位返回实际句柄、位置；读取返回成功状态及实际字节数。
ECX/EDX等系统API易失寄存器不进入接口。

## 具名接口迁移验证

本批将MON文件、解析流、说明存储、加载器及调用方改为具名调用，
删除通用操作编号、参数包和寄存器回复。脚本修改敌方动作时实际释放
已加载的说明文字，随后清零定义中的说明标识。

暂存源码在独立副本中验证，与保留在工作区的帧刷新改动分开。
最终77个暂存文件与验证副本逐字节一致，随后仅更新本节验证记录。
六项core测试各1/1通过：战斗setup、actor_frame_316、MON定义、
MON说明释放、LEVEL profile及初始菜单。setup包含真实文件访问、
profile解析和脚本调用方；定义及LEVEL测试读取现有游戏数据。
MON定义和说明释放的ASan各1/1通过，工作区SDL可执行目标链接通过。

最终日志为`build/tmp/runtime/mon-final-staged-{setup,actor,definition,
release,level,menu}.log`、`mon-final-{definition,release}-asan.log`
和`mon-final-sdl.log`，均位于同一runtime目录。
工作区setup仍有六处未暂存帧刷新改动导致的断言失败；上述暂存版本
不含这些改动，其setup通过。没有运行游戏，不将这些结果等同于
完整敌方初始化、实际续玩或原版动态差分。

## 已发布文件行为修复的历史验证

暂存树导出到`build/tmp/runtime/mon-file-review-snapshot/`进行构建，
不包含其他未提交的MON存储或敌方重置代码。

文件测试直接使用临时真实文件，覆盖：

- 默认文件名解析、正常读与请求长度限制；
- 有符号相对定位、绝对位置返回、负绝对位置失败；
- 短读后缀、EOF零字节成功、EOF后重新定位；
- 打开失败后原文件及其游标保留；
- 无效句柄的read/seek失败；
- OPEN_ALWAYS创建新文件及不同成功句柄；
- 目标范围不足的明确拒绝。

首轮隔离core及ASan各1/1通过；SDL首次配置因隔离目录缺少既有
FFmpeg静态依赖包而失败。已将仓库的同版本依赖包链接到隔离目录，
没有修改依赖实现或业务源码。两处测试空行修正已同步到暂存树和快照，
最终`proc_d9c3`验证通过：core和ASan各1/1，SDL可执行目标链接成功。
日志分别为`build/tmp/runtime/mon-file-isolated-final-core.log`、
`build/tmp/runtime/mon-file-isolated-final-asan.log`及
`build/tmp/runtime/mon-file-isolated-final-sdl.log`。SDL本轮仅构建，未运行UT。
隔离构建含既有源码警告，不宣称零warning。新增文件格式检查和暂存差异
检查通过；文件端口及SDL三个调用分支已逐项对照上述LST与底层文件合同。
没有启动游戏程序，不将构建或UT记为实际续玩或原版动态差分。
