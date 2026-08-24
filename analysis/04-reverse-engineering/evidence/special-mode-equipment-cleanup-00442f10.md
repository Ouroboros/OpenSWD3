# 装备物品模式清理 `0x00442F10`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00442F10..0x00442F37`，23行，无FUNCTION CHUNK。code callers 443BD0和4441A0已直接回收；444FC0另绑定为模式清理callback并待审。直接callee 444F00已关闭并直接复用；4885A0以workspace token释放端口表达。

## 精确顺序

1. 直接调用444F00记录列表回收；callee弹出head、释放FFDC或把普通记录前插回party池。
2. 调用返回后重读workspace token。
3. 写mode enabled为0。
4. 以重读token调用4885A0等价释放，保留其EAX。
5. 写global mode `0x36`。
6. 返回释放EAX。

原函数不清workspace token，modern同样保留callee调用后token值，不把已释放owner擅自归零。release无条件执行，即token为0也照原调用。

444F00在party索引或source root边界停止时，不读取token、不清mode、不释放workspace、不写global mode；已弹出的detached record与剩余head副作用保留。

UT验证空列表后仍读取并释放原workspace token，随后mode0、global mode54及EAX -7；失败UT验证444F00弹出记录后停止，而workspace/mode/global均不改。定向测试通过。

workpack双生成稳定为`97/227`，SHA256均为`e266c74f86647b581c7b4110f4068d2509cb265fce7b5ce2202612c69f6bc573`；下一单元`0x00442F40`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
