# 战斗选定资源帧定点绘制 `0x004504E0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与调用图

权威LST完整范围为`0x004504E0..0x00450522`，从`proc`到`endp`共37行，没有外部`FUNCTION CHUNK`。

cdecl四参数依次为资源号、帧索引、目标X、目标Y。callee只有已关闭帧查询`0x004315D0`和软件blitter`0x004170E0`各一次。

四个caller位于`0x0045A30A`、`0x0045A5A8`、`0x0045A7D6`和`0x00465753`。最后一个列表内容caller已直接组合typed实现并复用选择帧内唯一资源帧state；前三个caller留到各自工作包。函数返回值均不是本函数额外定义的业务结果。

## 2. 精确顺序

函数：

1. 以入口资源号和帧索引查询帧；
2. 发布帧记录到`0x004FD78C`；
3. 解引用记录`+0`并发布源到`0x004CD730`；
4. 读取记录`+4`物理尾参数；
5. 以u16零扩展记录宽`+0x0C`和高`+0x0E`；
6. 以`x,y,width,height,flags=0,record+4`调用软件blitter。

没有尺寸门、帧索引归一化、坐标修正或第二次绘制。

provider失败时空记录已发布，源在原`[eax]`读取点前保持入口snapshot。成功时typed state发布完整帧与源。

## 3. palette/辅助与公共后缀

记录`+4`在raw indexed routine中是palette，在RLE routine中可作为辅助数据。typed实现同时以source palette和同一palette字节span表达，不固定清空、不复制成无关缓冲。

正常`completed`、`clipped_out`或`opacity_disabled`返回后，公共后缀清目标高度、水平位移、纵向phase、opacity、RGB偏移和跳行状态，保留放大位。其他状态在原callee故障点typed-stop且不清入口状态。

## 4. 双向追溯

- `0x004504E0..0x004504EF`对应provider查询与帧记录发布；
- `0x004504F4..0x004504F6`对应源解引用与发布；
- `0x004504FC..0x00450519`对应tail、u16宽高、入口坐标和flags0；
- `0x0045051A..0x00450522`对应typed blitter、栈恢复与返回。

反向上，每个state写、request字段和callee调用均有唯一LST来源；无新增裁剪、尺寸替换、palette清空或失败跳过。完整追溯没有未解释基本块、参数、共享写或出口。

## 5. 验证与动态差分

定向测试覆盖资源/帧参数、4×3 indexed8帧的palette实际像素、记录宽高、目标坐标、正常公共后缀和帧缺失前缀。battle聚合目标零warning构建及定向测试通过。

当前没有原版帧记录、共享blitter状态和framebuffer联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整37行LST、两个callee直接组合和固定状态验证已经闭环。
