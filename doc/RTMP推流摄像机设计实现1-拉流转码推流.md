#RTMP推流摄像头设计实现1 - 拉流转码推流

##概要
把EasyRTMP、EasyAACEncoder、EasyRTSPClient重新封装一下，用于实现以设备流或RTSP流为流源，内部自动音频转码，简易RTMP拉流API。

##接口定义
- 创建与销毁推流上下文
    RTMPPushExt_Handle RTMPPushExt_Create(void);
    void RTMPPushExt_Release(RTMPPushExt_Handle handle);
- 初始化推流源为本地设备流或RTSP流
    Easy_I32 RTMPPushExt_InitLocalSource(RTMPPushExt_Handle handle, const EASY_MEDIA_INFO_T* pstruStreamInfo);
    Easy_I32 RTMPPushExt_InitRTSPSource(RTMPPushExt_Handle handle, const char* rtspUrl, const char* username, const char* passwd);
- 设置事件回调
    typedef Easy_I32 (*RTMPPushExt_CallBack)(RTMPPushExt_Handle handle, EVENT_T _event_type, void* _userPtr);
    Easy_I32 RTMPPushExt_InitCallback(RTMPPushExt_Handle handle, RTMPPushExt_CallBack _callback, void* _userPtr);
- 启动推流
    Easy_I32 RTMPPushExt_Start(RTMPPushExt_Handle handle, const char* rtmpUrl, Easy_U32 m_iBufferKSize);
- 本地设备流源发送流接口
    Easy_U32 RTMPPushExt_SendFrame(RTMPPushExt_Handle handle, const AV_Frame* frame);


##实现方案
一个推流上下文，用户初始化时指定流源，是设备流源，还是RTSP流源
如果是设备流源，用户需要指定视频格式(仅支持H264)，音频格式(G711x、G726、AAC)，启动推流后还需调用SendFrame主动发流。
如果是RTSP源，用户只需提供RTSP地址即可，内部会使用EasyRTSPClient实现取流。
如果音频输入不是AAC，内部会调用EasyAACEncoder对其进行转码为AAC后推送。
内部监测推流状态，如RTMP连接状态，RTSP连接状态等，通过回调的方式告知用户。
依靠EasyRTMP的流环形缓冲、智能丢帧、自动重连等特性，可以实现高可靠性推流。


## 获取更多信息 ##
邮件：[support@easydarwin.org](mailto:support@easydarwin.org) 
WEB：[www.EasyDarwin.org](http://www.easydarwin.org)
Copyright &copy; EasyDarwin.org 2012-2017
![EasyDarwin](http://www.easydarwin.org/skin/easydarwin/images/wx_qrcode.jpg)
