# RTMP推流摄像头设计实现 #

## 简介 ##

基于传统安防摄像机实现一个完整的互联网直播推流摄像机功能。
在传统的安防摄像机IPC的基础上进行扩展，将安防设备扩展成互联网直播推流摄像机。
传统安防摄像机大都用于本地录像与局域网预览，如想用于互联网直播分享视频，需要在摄像机内部置入互联网推流程序。
现在互联网直播协议大都采用RTMP或HLS协议，大致过程可简化为: 
摄像机RTMP推流 ==> 流服务器分发(RTMP+HLS+HTTPFLV) ==> APP播放(RTMP或HTTPFLV)或PC浏览器播放(RTMP)或手机浏览器(HLS)
本方案实现的就是这种直播推流摄像机。

## 实现方案 ##

基于EasyRTMP、EasyAACEncoder及EasyRTSPClient整合出，以设备流源或RTSP流源作为音视频输入源，同时集成AAC转码的RTMP推流API。
基于上述API做一个推流通道管理器，配置及控制各推流通道及监视通道状态，同时向用户提供简单的web配置接口，用户可以通过浏览器配置通道推流地址及音视频源选择，及监视通道状态。

## 源文件简介 ##

RTMPPushExt.cpp - 基于EasySDK整合出，以设备流源或RTSP流源作为音视频输入源，同时集成AAC转码的RTMP推流API。
ehttpd.c - 一个简易的HTTP服务器。
einifile.c - 一个简单的INI解析封装库。
RTMPMgr.cpp - 封装上面的组件，实现推流通道的管理、启停控制、状态查询等。
main.cpp - 一个测试例程。以FILE或HiSDK作为设备源。

## 程序接口定义 ##

- 设备流启停通知回调
    typedef struct
    {
        Easy_I32 (*StartStream)(Easy_U32 channelID, EASY_MEDIA_INFO_T* mediaInfo, void* _user_data);
        void (*StopStream)(Easy_U32 channelID, void* _user_data);
    } RTMPMgr_Callback;

- 创建与销毁通道管理器
    RTMPMgr_Handle RTMPMgr_Start(const char* config, RTMPMgr_Callback cb, void* _user_data);
    void RTMPMgr_Stop(RTMPMgr_Handle handle);

- 设备流推流接口
    Easy_I32 RTMPMgr_PushVideo(RTMPMgr_Handle handle, Easy_U32 channelID, const Easy_U8* frame, Easy_U32 size, Easy_U32 ts_ms);
    Easy_I32 RTMPMgr_PushAudio(RTMPMgr_Handle handle, Easy_U32 channelID, const Easy_U8* frame, Easy_U32 size, Easy_U32 ts_ms);

## HTTP配置接口定义 ##

- 获取推流通道配置及状态
请求:
	GET /get_config HTTP/1.1
回复:
	Content-Type: applicantion/json
	{
		"data" : {
			"status" : "Disabled|ConfigErr|StartFailed||RtmpConnecting|RtmpError|RtspSrcConnecting|RtspSrcError|LocalSrcError|Success",
			"enable" : true|false,
			"source" : "device|rtsp",
			"rtsp_url" : "rtsp://xxxx",
			"rtmp_url" : "rtmp://xxxx"
		},
		"code" : 0,
		"msg" : "成功"
	}

- 配置推流通道
请求:
	POST /set_config HTTP/1.1
	Content-Type: applicantion/json
	{
		"channel" : 0,
		"enable" : true|false,
		"source" : "device|rtsp",
		"rtsp_url" : "rtsp://xxxx",
		"rtmp_url" : "rtmp://xxxx"
	}
不改变的配置项可以不填
回复:
	Content-Type: applicantion/json
	{
		"code" : 0,
		"msg" : "成功"
	}

- 一个简单的通道html状态反馈
浏览器地址:
    http://localhost:8000
回复:
    <html>
	<head><title>RtmpPushStatus</title></head>
	<body>
	<p>RTMP推流状态</p>
    Channel%u<br/>attr:%s status:%s<br/>source:%s<br/>url:%s<br/><br/>
    </body>
    </html>

## 配置文件接口 ##

[Base]
EasyRTMP_License=
EasyAACEncoder_License=
EasyRTSPClient_License=
HttpSrvPort=8000
BaseChannelNum=1

[Channel_0]
Enable=true
IsRtspSrc=false
RtspUrl=rtsp://127.0.0.1:8554/live/chn0
RtmpUrl=rtmp://www.easydss.com:10085/live/0000


## 获取更多信息 ##

邮件：[support@easydarwin.org](mailto:support@easydarwin.org) 

WEB：[www.EasyDarwin.org](http://www.easydarwin.org)

Copyright &copy; EasyDarwin.org 2012-2017

![EasyDarwin](http://www.easydarwin.org/skin/easydarwin/images/wx_qrcode.jpg)

