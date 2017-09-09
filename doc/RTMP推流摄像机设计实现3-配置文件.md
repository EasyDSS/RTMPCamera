#RTMP推流摄像头设计实现3 - 配置文件

##概要
用户对推流通道的配置信息需要长期保存，以便程序重启时，仍能得到用户的配置，固需要一个配置文件来存储这些信息。
配置文件常规格式有多种，INI、XML、JSON等，由于推流配置项并不复杂，这里选用最简单的INI格式来存储。
网上搜索后并没找到合意INI库，本着自以为简单的心理，准备自己去实现，等实现时才觉得其实并不简单，接口要易用，支持中文等。
但已经开始，就把它搞完吧。

##接口定义
- 句柄的申请与释放
    eini_handle eini_alloc(void);
    void eini_free(eini_handle h);
- 配置文件的导入与保存
    int eini_load_file(eini_handle h, const char* file);
    int eini_save_file(eini_handle h, const char* file);
- INI段与参数的基本操作
    int eini_count(eini_handle h, int secidx);
    int eini_query(eini_handle h, int secidx, const char* name, int keyidx);
    int eini_new(eini_handle h, int secidx, int parmidx, const char* name, const char* value);
    int eini_get(eini_handle h, int secidx, int parmidx, char* namebuf, char* valuebuf);
    int eini_set(eini_handle h, int secidx, int parmidx, const char* name, const char* value);
    int eini_delete(eini_handle h, int secidx, int parmidx);
- 快捷操作，方便用户直接调用
    int eini_ext_new(eini_handle h, const char* sec_key, const char* value);
    int eini_ext_get(eini_handle h, const char* sec_key, char* valuebuf);
    int eini_ext_scanf(eini_handle h, const char* sec_key, const char* fmt, ...);
    int eini_ext_set(eini_handle h, const char* sec_key, const char* value);
    int eini_ext_printf(eini_handle h, const char* sec_key, const char* fmt, ...);
    int eini_ext_delete(eini_handle h, const char* sec_key);

##实现方案
为了方便调用，想了很多接口定义，最终形成上述，感觉还是复杂了点，先这样了。
INI文件中多个段及段内参数对构成，每个参数对由key=value组成。
为了接口简易及易实现，对段名及key作了约束，
段名与key由字母或数字或下划线组成，且必须字母开头（变量名定义）
参数值只要不含有控制字符就行，（支持中文）
剩下就是写代码实现了。


## 获取更多信息 ##
邮件：[support@easydarwin.org](mailto:support@easydarwin.org) 
WEB：[www.EasyDarwin.org](http://www.easydarwin.org)
Copyright &copy; EasyDarwin.org 2012-2017
![EasyDarwin](http://www.easydarwin.org/skin/easydarwin/images/wx_qrcode.jpg)
