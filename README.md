# 腾讯云物联开发平台5G SDK
腾讯云物联开发平台 (IoT Explorer) 5G SDK是腾讯5G物联开发套件[^1]的设备端组件，通过与IoT Explorer配合，为宽带物联应用提供5G模组远程运维，接入腾讯边缘接入和网络加速平台(TSEC)实现VPN组网，空口加速等功能，降低用户使用5G网络及边缘计算的门槛。

## 5G物联开发套件概述

![devkit](https://main.qcloudimg.com/raw/b7d7a992856201c47a0135a4a0ff67df/devkit.png)

5G物联开发套件由5G SDK，腾讯边缘接入和网络加速平台TSEC [^2]，IoT Explorer 三部分组成。套件涉及四个层次，分别为终端，网络层，云端和行业应用。其中,

- **终端：**可以是各种行业设备，例如机器人，AGV，叉车，无人车等。这些设备内置5G模组，通过配套SDK直接向设备的host cpu呈现一个虚拟以太网口，可用于网络通信。根据用户需求，可以在TSEC的配合下建立企业的虚拟私有网络； 
- **网络层：**包括5G基站、UPF及TSEC网关。TSEC网关位于靠近用户的边缘机房，为用户的数据优选最佳转发路径，同时为用户的特殊组网需求提供本地路由； 
- **云端：** 主要分为通用能力和场景化能力。通用能力方面，模组利用SDK接入IoT Explorer，支持海量设备的运维和监控能力；场景化能力方面，利用TSEC控制器，对外提供QoS加速，切片以及边缘计算的API，供业务调用，以实现对设备的QoS，切片管理；
- **行业应用：**套件本身不包含上层行业应用，但对外提供两套RESTful API接口，IoT Explorer相关的数据模板和运维接口，TSEC的场景化API接口。行业应用利用这些API可以很方便地操作终端设备，与终端设备建立通信连接。

关于5G物联开发套件的更多细节，请参考《腾讯5G物联开发套件与实践案例》[^1].


## 5G SDK架构

![architecture](https://main.qcloudimg.com/raw/49489e2ba2c9135cbebec7cc04d14ebd/5G-SDK-architecture.png)

5G SDK集成了IoT Explorer C-SDK[^3] ，通过MQTT协议与IoT Explorer保持双向连接，实现5G模组状态上报、用户指令下发等基础通信功能。

除此之外，SDK还包括：

- 应用层

  提供模组管理、故障管理、软件安装和Web服务功能。

  - 模组管理通过模组的通用AT指令或专用信令接口（例如高通的QMI）读取模组/网络状态、配置网络参数和发起拨号操作，实现终端入网和运行状态监控。
  - 故障管理利用网络故障诊断工具，系统日志以及其它诊断手段在进程运行异常时对网络、模组以及上位机进行故障定位并对可排除的故障进行系统恢复。当检测到无法排除的故障时，记录当前故障的特征，待设备通过其它途径恢复正常后，上报该次故障。
  - 软件安装监听并执行用户通过IoT Explorer下发的软件安装指令。利用此功能的典型应用是VPN客户端软件的安装。
  - Web服务为本地应用提供一组RESTFul API访问及配置模组，调用QoS加速功能。

- 网络层

  提供隧道配置和QoS加速功能。

  - 隧道配置负责创建并管理从终端到TSEC网关的IPSEC或其它类型的IP隧道（比如IPinIP），实现VPN组网。包括信令面配置和数据面配置两部分：信令面配置通过接收IoT Explorer下发的相关指令，启动VPN客户端与TSEC网关协商私网IP地址和数据加密方法，并周期地确认协商结果的有效性，当发现协商结果失效或对端"掉线"时，重新发起协商或删除该隧道端口；数据面配置创建虚拟隧道接口，并将协商好的IP地址配置到该接口为用户应用程序提供数据封装和加解密等服务。
  - QoS加速为本地应用提供4G/5G空口带宽及时延保障。应用程序调用QoS加速相关的RESTFul API，向TSEC发起QoS加速请求，后者通过调用运营商能力开放平台的接口为该应用创建专用数据通道实现空口QoS保障。QoS加速屏蔽了运营商的差异性，向应用程序提供一个统一的接口。

- 驱动层

  提供驱动配置功能。根据模组厂商的具体实现，为模组的USB接口加载相应的驱动，比如option.ko和cdc_ether.ko来创建虚拟网卡和虚拟串口，实现数据收发和AT指令的下发。
  

**注1**：为方便用户体验IoT Explorer的5G模组管理功能，SDK还提供了模组仿真器供无5G模组的环境下使用。

**注2**：上述组件将根据5G模组的对接情况，TSEC和IoT Explorer开发的进度分阶段发布。

## SDK目录结构

| 名称                               | 说明                                                         |
| ---------------------------------- | ------------------------------------------------------------ |
| qcloud-iot-explorer-sdk-embedded-c | IoT Explorer C-SDK代码库                                     |
| mm-lib                             | 模组管理库，包含已完成对接的5G模组的管理函数集，以插件形式提供 |
| module-manager.c                   | 模组管理的顶层函数，调用mm-lib和C-SDK上报模组和网络的状态，同时接收IoT Explorer下发的指令创建、配置或者销毁IP隧道 |
| device_info.json                   | 设备信息文件，取代C-SDK下的同名文件为5G SDK提供模组鉴权信息  |
| Makefile                           | 使用Makefile直接编译                                         |
## 编译流程

- 运行git submodule, 更新子模块C-SDK并切换到v3.1.0.
    git submodule update --init
- 运行make，编译可执行文件module-manager
    ./make
## 支持的操作系统

- Ubuntu 18.04

## 支持的5G模组
- Tencent : fake (模组仿真器)


[^1]:[腾讯5G物联开发套件与实践案例]( https://cloud.tencent.com/developer/article/1564370)
[^2]: [腾讯边缘接入和网络加速平台](https://tech.qq.com/a/20190626/007290.htm)
[^3]: [IoT Explorer C-SDK](https://github.com/tencentyun/qcloud-iot-explorer-sdk-embedded-c)
