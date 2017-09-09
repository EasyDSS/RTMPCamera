## RTMP推流摄像机设计方案 ##

 - 基于**EasyRTSPClient** / **摄像机SDK**、**EasyAACEncoder**、**EasyRTMP**整合出，以设备编码源或RTSP流源作为音视频输入源，同时集成AAC转码的RTMP推流整套API方案；
 - 基于上述API做一个推流管理器：配置、控制各推流通道及监视推流状态，同时向用户提供简单的web配置接口，用户可以通过浏览器配置通道推流地址及音视频源选择，及监视通道状态；


### RTMP推流摄像机实现方案 ###

 1. 整合的RTMP推流API
 
	基本上就是基于EasyRTMP、EasyAACEncoder及EasyRTSPClient库再封装出一个扩展型RTMP推流API，若音频编码不是aac则调AasyAACEncoder arm版转码，若源是rtsp则调EasyRTSPClient实现，如果license失败就只能屏蔽相应功能。
	
 2. RTMP推流方案HTTP RESTful服务

	管理器根据配置文件配置及管理推流，同时通过http服务接受用户配置并存入配置文件，推流配置包括基本源配置(有设备源和RTSP源)，推流控制包括启停通道，设置RTMP推流地址；

	HTTP服务提供推流状态及现有配置显示，及配置SET服务，浏览器请求后结合html+RESTful用于显示当前配置, 用户点击设置后浏览器通过http_get的url_query_params提交配置参数，http简易服务器自己实现，http解析可以使用http-parser库，配置文件使用ini文件格式，配置文件原形：

		[base]
		EasyRTMP_License=xxx
		EasyAACEncoder_License=xxx
		EasyRTSPClient_License=xxx

		Enable=true
		Source=local/RTSP
		RtspUrl=rtsp://xxxx
		RtmpUrl=rtmp://xxxx
		

	web页面原形
		只提供一个页面用于状态显示及配置

		连接状态：已连接推流摄像机/未连接推流摄像机
		推流状态: 推流中|通道错误|推流停止
		控制: [启动] | [禁用]
		源选择: 设备 | [RTSP]		[应用]
		RTSP源地址: [rtsp://xxx]		[设置]
		RTMP地址: [rtmp://xxx]		[设置]
		

### 接口设计 ###
1. 获取配置
	请求:
	GET /get_config HTTP/1.1
	回复:
	Content-Type: applicantion/json
	{
		"data" : {
			"status" : "Runing|Error|Disabled",
			"enable" : true|false,
			"source" : "device|rtsp",
			"rtsp_url" : "rtsp://xxxx",
			"rtmp_url" : "rtmp://xxxx"
		},
		"code" : 0,
		"msg" : "成功"
	}

2. 设置配置
	请求:
	POST /set_config HTTP/1.1
	Content-Type: applicantion/json
	{
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


## 获取更多信息 ##

邮件：[support@easydarwin.org](mailto:support@easydarwin.org) 

WEB：[www.EasyDarwin.org](http://www.easydarwin.org)

Copyright &copy; EasyDarwin.org 2012-2017

![EasyDarwin](http://www.easydarwin.org/skin/easydarwin/images/wx_qrcode.jpg)