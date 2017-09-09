#RTMP推流摄像头设计实现 - 总述

##简介
传统安防摄像机大都用于本地录像与局域网预览，如想用于互联网直播分享视频，需要在摄像机内部置入互联网推流程序。
现在互联网直播协议大都采用RTMP或HLS协议，大致过程可简化为: 
摄像机RTMP推流 ==> 流服务器分发(RTMP+HLS+HTTPFLV) ==> APP播放(RTMP或HTTPFLV)或PC浏览器播放(RTMP)或手机浏览器(HLS)
本设计只涉及直播推流摄像机部分，基于传统安防摄像机实现一个完整的通用直播推流摄像机功能。
依托EasyDarwin开源服务器及直播APP可以快速构建出低成本、高性价比的直播方案。

##设计概要
传统安防摄像机通常使用其SDK或标准RTSP向外提供视音频流，而音频通常为G711格式，考虑到这些普遍性特征，核心功能应包括取流、音频转码与推流。
而这些功能EasyDarwin社区都有成熟的SDK供使用，我就不再造轮子了，拿来就用。
推流工具使用EasyRTMP，音频转码使用EasyAACEncoder，为设备流提供简易接口使其接入，如果是RTSP流则内部使用EasyRTSPClient拉流，用户只需要提供rtsp地址即可。
除了上述推流SDK功能外，一个完整的软件产品方案还需要辅助功能，如配置接口、推流控制、通道管理、状态反馈等。
为便于用户配置及接入平台，通道控制及推流状态接口采用HTTP+JSON的方式，因此需要实现一个简易的http服务用于配置推流及查询推流状态。
还需要实现一个配置文件，用于保存用户的配置。


##实现概要
1. 基于EasyRTMP、EasyAACEncoder及EasyRTSPClient整合出集设备流输入、RTSP流输入、音频转码为一体的RTMP推流API，暂叫RTMPPushExt
2. 做一个简易的http服务器模型，出于支撑推流配置及状态查询。
3. 做一个配置文件读写工具，用于保存及读取用户的配置信息，如推流URL、流来源。
4. 整合以上功能模块，实现完整的推流摄像机功能。

## 获取更多信息 ##
邮件：[support@easydarwin.org](mailto:support@easydarwin.org) 
WEB：[www.EasyDarwin.org](http://www.easydarwin.org)
Copyright &copy; EasyDarwin.org 2012-2017
![EasyDarwin](http://www.easydarwin.org/skin/easydarwin/images/wx_qrcode.jpg)
