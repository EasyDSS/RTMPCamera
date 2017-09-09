#RTMP推流摄像头设计实现2 - HTTP服务

##概要
为方便用户及平台控制推流通道，本设计提供了HTTP配置接口，由于HTTP服务可以封装成相对独立的模块，固决定封装一个简单的HTTP服务，用于提供简单的配置服务。

##接口定义
- http消息定义
    typedef struct
    {
        char method[8];		//request only
        char uri[128];		//request only
        unsigned int status_code;	//responsse only
        unsigned int version;		//0-HTTP/1.0 1-HTTP/1.1
        ...
    } ehttp_msg;

- 事件回调接口
    typedef struct
    {
        int (*on_client_comming) (const char* ipaddr, unsigned int port, void* pdata, void** ppdata_new);
        int (*on_client_request) (const ehttp_msg* reqmsg, ehttp_msg* resmsg, void* pdata);
        void (*on_client_close) (void* pdata);
    } ehttpd_callback;

- 创建http服务
    ehttpd_handle ehttpd_create(unsigned int port, const ehttpd_callback* cb, void* pdata);
- 事务处理
    int ehttpd_do_event(ehttpd_handle handle);
- 销毁服务
    void ehttpd_release(ehttpd_handle handle);

##实现方案
本模块旨在实现一个简单的http服务模型，用于实现简单的http配置服务。
由于仅用于配置，这里只简单实现，越简单越好，不支持多线程。
http事件处理全在ehttpd_do_event里，每调一次此函数，内部会去查询socket是否有新的请求到来，
如果有新请求数据，就进行http解析，解析完成后回调on_client_request通知外部去处理请求，
外部处理后，将结果填入resmsg里，返回后内部会把消息格式化后返回到http客户端，
整个流程非常清晰。


## 获取更多信息 ##
邮件：[support@easydarwin.org](mailto:support@easydarwin.org) 
WEB：[www.EasyDarwin.org](http://www.easydarwin.org)
Copyright &copy; EasyDarwin.org 2012-2017
![EasyDarwin](http://www.easydarwin.org/skin/easydarwin/images/wx_qrcode.jpg)
