# 战斗目标阶段五槽逐帧演出 `0x00471FC0`

状态：`platform_adapted`。完整LST、typed实现、五处caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

权威LST主体为`0x00471FC0..0x004721DE`，proc至endp共231行、144条实际指令、6个call、5个跳转、5个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。五处真实caller均位于`0x00471270`目标阶段推进，分别使用槽零至四。

函数把`+0x2BC8`识别为五个连续152字节action record，而非无类型尾块。typed实现保留槽指针先行计算、action id与variant写入、special mode、动作更新失败、frame读取、signed word起点、行动者终点、iterations乘既有counter的32位signed循环、line-raster固定次数推进、counter递增、sample参数保留递增counter高半、sample word清零、signed完成比较、完成边界夹值绘制、counter/raster清零及slot action record保留。

物理状态复用target-phase五槽action-record数组、spawn counters和line-raster块，以及group-A action-execution、shared frame-source owner。已关闭动作更新、frame provider和line-raster直接typed调用；音频和软件绘制复用共享窄port。frame与shared typed-stop均位于原始访问点并保留此前slot记录、更新和frame副作用。

五个`0x00471270` caller全部改为typed直连，并按原tick阈值与坐标顺序短路传播typed-stop。测试覆盖槽越界、frame/shared原访问点、零迭代、signed循环、counter高半sample、非完成绘制、完成边界、record保留、五槽variant及旧地址零调用。定向测试与独立AddressSanitizer均通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`213/422 = 204 platform_adapted + 9 assembly_exact + 209 pending_audit`，SHA256为`c75dfbed50b46c8a9b12fc3eee511f8c6323bdcb0dbb402e147f402163ed6ba2`。动态差分因原版五槽记录、动作流、frame、音频、绘制与caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
