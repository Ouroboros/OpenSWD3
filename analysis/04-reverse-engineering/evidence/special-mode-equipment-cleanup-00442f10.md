# 装备物品模式清理 `0x00442F10`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00442F10..0x00442F37`，23行，无FUNCTION CHUNK。code callers为443BD0和4441A0；444FC0另绑定为模式清理callback，三者尚待独立关闭。直接callee 444F00尚未关闭，保留装备记录列表清理窄端口；4885A0以workspace token释放端口表达。

## 精确顺序

1. 调444F00等价记录列表清理；callee可改共享状态。
2. 调用返回后重读workspace token。
3. 写mode enabled为0。
4. 以重读token调用4885A0等价释放，保留其EAX。
5. 写global mode `0x36`。
6. 返回释放EAX。

原函数不清workspace token，modern同样保留callee调用后token值，不把已释放owner擅自归零。release无条件执行，即token为0也照原调用。

444F00端口不可用时在原call site停止，不读取token、不清mode、不释放、不写global mode；若callee边界在返回false前已提交typed状态变更，则这些先前副作用保留。

UT令444F00把token从`11111111`改为`AABBCCDD`，验证F10必须重读并释放后者，随后mode0、global mode54及EAX -7。失败UT验证callee已提交token可保留，而mode/global/release均不发生。定向测试通过。

workpack双生成稳定为`97/227`，SHA256均为`e266c74f86647b581c7b4110f4068d2509cb265fce7b5ce2202612c69f6bc573`；下一单元`0x00442F40`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
