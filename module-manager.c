/*
 * Tencent is pleased to support the open source community by making IoT Hub available.
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT

 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "qcloud_iot_export.h"
#include "lite-utils.h"
#include "handle_module.h"
#include "module_ops.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <stdbool.h>
#include <pthread.h>
#define MAX_SIZE_OF_TOPIC_CONTENT 100
#ifdef AUTH_MODE_CERT
    static char sg_cert_file[PATH_MAX + 1];      // full path of device cert file
    static char sg_key_file[PATH_MAX + 1];       // full path of device key file
#endif


#define CELL_ID_UPDATE_TIME 40
#define RSSI_UPDATE_TIME 20
#define IDLE_INTERVAL 1000
#define YIELD_TIMEOUT 200

static void fresh_rssi();
static void fresh_cellid();

typedef struct freshTimer{
		char szName[20];
		int nInterval;
		time_t nLast;
		void (*pfunc)();
}TIMER;

TIMER sTimer[] = {
	{"TIME1",RSSI_UPDATE_TIME,0,fresh_rssi},
	{"TIME2",CELL_ID_UPDATE_TIME,0,fresh_cellid}
};

#define TIMER_COUNT sizeof(sTimer)/sizeof(sTimer[0])

static DeviceInfo sg_devInfo;

#define MAX_LENGTH_OF_UPDATE_JSON_BUFFER 200
#define MAX_STR_LEN 512
#define MAX_RECV_LEN (512 + 1)

static MQTTEventType sg_subscribe_event_result = MQTT_EVENT_UNDEF;
static bool sg_control_msg_arrived = false;
static char sg_data_report_buffer[2048];
size_t sg_data_report_buffersize = sizeof(sg_data_report_buffer) / sizeof(sg_data_report_buffer[0]);

#define TOTAL_PLATINFO_COUNT 1
#define TOTAL_SELFINFO_COUNT 5
#define TOTAL_PROPERTY_COUNT 3
#define TOTAL_COUNT TOTAL_PLATINFO_COUNT + TOTAL_SELFINFO_COUNT+ TOTAL_PROPERTY_COUNT
#define MAX_STR_NAME_LEN 64
#define RSSI_INDEX 7
#define CELLID_INDEX 8

static sDataPoint    sg_DataTemplate[TOTAL_COUNT];

static char Property_Name_Array[][MAX_STR_NAME_LEN] = {
#define MODULE_OP(OBJ, CMD1, CMD_TPYE1, CMD2, CMD_TYPE2, NAME, CLIENT, DELIM_OPS, POST_PROCESS)		\
        #NAME,
        MODULE_OPS
#undef MODULE_OP
""};

static char sg_property_data[TOTAL_COUNT][MAX_STR_LEN];

bool log_handler(const char * message){
	openlog("5G-SDK", LOG_CONS | LOG_PID, 0);
	syslog(LOG_USER | LOG_INFO, "%s", message);
	closelog();
	return true;
}



static void _init_all_data(void)
{
	int i;
	for(i = 0; i < TOTAL_COUNT; i++){
		get_current_module()->module_ops->module_ops_iter[i](sg_property_data[i]);
		sg_DataTemplate[i].data_property.key = Property_Name_Array[i];
		sg_DataTemplate[i].data_property.data = sg_property_data[i];
		sg_DataTemplate[i].data_property.type = TYPE_TEMPLATE_STRING;
	}
};


// MQTT event callback
static void event_handler(void *pclient, void *handle_context, MQTTEventMsg *msg)
{
	uintptr_t packet_id = (uintptr_t)msg->msg;
        switch(msg->event_type) {
                case MQTT_EVENT_UNDEF:
			Log_i("undefined event occur.");
			break;
		case MQTT_EVENT_DISCONNECT:
		        Log_i("MQTT disconnect.");
		        break;
		case MQTT_EVENT_RECONNECT:
		        Log_i("MQTT reconnect.");
		        break;
		case MQTT_EVENT_SUBCRIBE_SUCCESS: 
		        sg_subscribe_event_result = msg->event_type;
		        Log_i("subscribe success, packet-id=%u", (unsigned int)packet_id);
		        break;
		case MQTT_EVENT_SUBCRIBE_TIMEOUT: 
		        sg_subscribe_event_result = msg->event_type;
		        Log_i("subscribe wait ack timeout, packet-id=%u", (unsigned int)packet_id);
		        break;
		case MQTT_EVENT_SUBCRIBE_NACK:
		        sg_subscribe_event_result = msg->event_type;
		        Log_i("subscribe nack, packet-id=%u", (unsigned int)packet_id);
		        break;
		case MQTT_EVENT_PUBLISH_SUCCESS:
		        Log_i("publish success, packet-id=%u", (unsigned int)packet_id);
		        break;
		case MQTT_EVENT_PUBLISH_TIMEOUT:
		        Log_i("publish timeout, packet-id=%u", (unsigned int)packet_id);
		        break;
		case MQTT_EVENT_PUBLISH_NACK:
		        Log_i("publish nack, packet-id=%u", (unsigned int)packet_id);
		        break;
		default:
		        Log_i("Should NOT arrive here.");
		        break;
		}
}

//Setup MQTT construct parameters
static int _setup_connect_init_params(TemplateInitParams* initParams)
{
	int ret;
        ret = HAL_GetDevInfo((void *)&sg_devInfo);
        if(QCLOUD_RET_SUCCESS != ret){
		return ret;
	}
        initParams->device_name = sg_devInfo.device_name;
        initParams->product_id = sg_devInfo.product_id;
#ifdef AUTH_MODE_CERT
        /* TLS with certs*/
        char certs_dir[PATH_MAX + 1] = "certs";
        char current_path[PATH_MAX + 1]; 
        char *cwd = getcwd(current_path, sizeof(current_path));
        if (cwd == NULL)
        {
	         Log_e("getcwd return NULL");
	         return QCLOUD_ERR_FAILURE;
	}
	sprintf(sg_cert_file, "%s/%s/%s", current_path, certs_dir, sg_devInfo.dev_cert_file_name);
	sprintf(sg_key_file, "%s/%s/%s", current_path, certs_dir, sg_devInfo.dev_key_file_name);
        initParams->cert_file = sg_cert_file;
        initParams->key_file = sg_key_file;
#else
        initParams->device_secret = sg_devInfo.device_secret;
#endif
        initParams->command_timeout = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
        initParams->keep_alive_interval_ms = QCLOUD_IOT_MQTT_KEEP_ALIVE_INTERNAL;
        initParams->auto_connect_enable = 1;     
	initParams->event_handle.h_fp = event_handler;
	return QCLOUD_RET_SUCCESS;
}

static void OnControlMsgCallback(void *pClient, const char *pJsonValueBuffer, uint32_t valueLength, DeviceProperty *pProperty) 
{
	int i = 0;
        for (i = 0; i < TOTAL_PROPERTY_COUNT; i++) {
		/* handle self defined string/json here. Other properties are dealed in _handle_delta()*/
		if (strcmp(sg_DataTemplate[i].data_property.key, pProperty->key) == 0) {
			sg_DataTemplate[i].state = eCHANGED;
			Log_i("Property=%s changed", pProperty->key);                       
			sg_control_msg_arrived = true;
			return;
		}
	}
	Log_e("Property=%s changed no match", pProperty->key);
}

static void OnReportReplyCallback(void *pClient, Method method, ReplyAck replyAck, const char *pJsonDocument, void *pUserdata) {
	Log_i("recv report_reply(ack=%d): %s", replyAck,pJsonDocument);
}

//register data template properties
static int _register_data_template_property(void *pTemplate_client)
{
	int i,rc;
	for (i = 0; i < TOTAL_PROPERTY_COUNT; i++) {
		rc = IOT_Template_Register_Property(pTemplate_client, &sg_DataTemplate[i+TOTAL_COUNT-TOTAL_PROPERTY_COUNT].data_property, OnControlMsgCallback);
		if (rc != QCLOUD_RET_SUCCESS) {
			rc = IOT_Template_Destroy(pTemplate_client);
			Log_e("register device data template property failed, err: %d", rc);
			return rc;
		} else {
			Log_i("data template property=%s registered.", sg_DataTemplate[i].data_property.key);
		}
	}
        return QCLOUD_RET_SUCCESS;
}


static void deal_down_data_logic()
{
	printf("deal down\n");
	return;
}

static int deal_up_data_logic(DeviceProperty *pReportDataList[], int *pCount)
{
	int i, j;
	for (i = 0, j = 0; i < TOTAL_PROPERTY_COUNT; i++) {
		if(eCHANGED == sg_DataTemplate[i+TOTAL_COUNT-TOTAL_PROPERTY_COUNT].state) {
		pReportDataList[j++] = &(sg_DataTemplate[i+TOTAL_COUNT-TOTAL_PROPERTY_COUNT].data_property);
		sg_DataTemplate[i+TOTAL_COUNT-TOTAL_PROPERTY_COUNT].state = eNOCHANGE;
		}
	}
	*pCount = j;
        return (*pCount > 0)?QCLOUD_RET_SUCCESS:QCLOUD_ERR_FAILURE;
}

static int  _get_sys_info(void *handle, char *pJsonDoc, size_t sizeOfBuffer)
{
	DeviceProperty plat_info[TOTAL_PLATINFO_COUNT+1];
	DeviceProperty self_info[TOTAL_SELFINFO_COUNT+1];
	int i;
	for (i = 0; i < TOTAL_PLATINFO_COUNT; i++)
		plat_info[i] = sg_DataTemplate[i].data_property;
	plat_info[TOTAL_PLATINFO_COUNT].key = NULL;
	for (i =0; i < TOTAL_SELFINFO_COUNT; i++)
		self_info[i] = sg_DataTemplate[i+TOTAL_PLATINFO_COUNT].data_property;
	self_info[TOTAL_SELFINFO_COUNT].key = NULL;
	return IOT_Template_JSON_ConstructSysInfo(handle, pJsonDoc, sizeOfBuffer, plat_info, self_info);
}


static void report_up_props(DeviceProperty *pReportDataList[])
{
	int i;
	for(i = 0; i < TOTAL_PROPERTY_COUNT; i++){
		pReportDataList[i] = &(sg_DataTemplate[i+TOTAL_COUNT-TOTAL_PROPERTY_COUNT].data_property);
	}
	return;
}

static void fresh_rssi()
{
	char *str;
	str = (char *)malloc(sizeof(char) * MAX_STR_LEN);
	//RSSI_Query(str);	
	get_current_module()->module_ops->module_ops_impl.rssi_query(str);
	if(strcmp(str, sg_property_data[RSSI_INDEX])){
		printf("rssi changed!\n");
		strcpy(sg_property_data[RSSI_INDEX], str);
		sg_DataTemplate[RSSI_INDEX].state = eCHANGED;
	}
	return;
}
//
 static void fresh_cellid()
{
	char *str;
	str = (char *)malloc(sizeof(char) * MAX_STR_LEN);
	get_current_module()->module_ops->module_ops_impl.cellid_query(str);
	if(strcmp(str, sg_property_data[CELLID_INDEX])){
		printf("cellid changed!\n");
		strcpy(sg_property_data[CELLID_INDEX], str);
		sg_DataTemplate[CELLID_INDEX].state = eCHANGED;
	}
	return;
}


int c_sdk_thread()
{
	DeviceProperty *pReportDataList[TOTAL_PROPERTY_COUNT];
	sReplyPara replyPara;
	int ReportCount = TOTAL_PROPERTY_COUNT;
	int rc;

	//set log level
	IOT_Log_Set_Level(eLOG_DEBUG);
	//set log handler
	IOT_Log_Set_MessageHandler(log_handler);

	//init connection
	TemplateInitParams init_params = DEFAULT_TEMPLATE_INIT_PARAMS;
	rc = _setup_connect_init_params(&init_params);
	if (rc != QCLOUD_RET_SUCCESS) {
		Log_e("init params err,rc=%d", rc);
		return rc;
	}
	void *client = IOT_Template_Construct(&init_params);
	if (client != NULL) {
		Log_i("Cloud Device Construct Success");
	} else {
	        Log_e("Cloud Device Construct Failed");
	        return QCLOUD_ERR_FAILURE;
	}

	//init sys info and data template
	_init_all_data();

	//register data template propertys here
	rc = _register_data_template_property(client);
	if (rc == QCLOUD_RET_SUCCESS) {
		Log_i("Register data template propertys Success");
	} else {
	        Log_e("Register data template propertys Failed: %d", rc);
	        return rc;
	}

	rc = _get_sys_info(client, sg_data_report_buffer, sg_data_report_buffersize);
	if(QCLOUD_RET_SUCCESS == rc){
		rc = IOT_Template_Report_SysInfo_Sync(client, sg_data_report_buffer, sg_data_report_buffersize, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
		if (rc != QCLOUD_RET_SUCCESS) {
			Log_e("Report system info fail, err: %d", rc);
			//goto exit;
		}
	}else{
		Log_e("Get system info fail, err: %d", rc);
	}
	//get the property changed during offline
	rc = IOT_Template_GetStatus_sync(client, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
	if (rc != QCLOUD_RET_SUCCESS) {
		Log_e("Get data status fail, err: %d", rc);
		return rc;
	}else{ 
	        Log_d("Get data status success");
	}

	//report data when starting
	printf("report data at first:\n");
	report_up_props(pReportDataList);
	rc = IOT_Template_JSON_ConstructReportArray(client, sg_data_report_buffer, sg_data_report_buffersize, ReportCount, pReportDataList);
	if (rc == QCLOUD_RET_SUCCESS) {
		printf("\nsg_data_report_length=%zu, context=%s\n", sg_data_report_buffersize, sg_data_report_buffer);
		rc = IOT_Template_Report(client, sg_data_report_buffer, sg_data_report_buffersize, OnReportReplyCallback, NULL, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
		if (rc == QCLOUD_RET_SUCCESS) {
			Log_i("data template reporte success");
		} else {
		        Log_e("data template reporte failed, err: %d", rc);
		}
	} else {
	        Log_e("construct reporte data failed, err: %d", rc);
	}
	
	
	//int age_count = 0;
	while (IOT_Template_IsConnected(client) || rc == QCLOUD_ERR_MQTT_ATTEMPTING_RECONNECT || rc == QCLOUD_RET_MQTT_RECONNECTED || QCLOUD_RET_SUCCESS == rc) {
		rc = IOT_Template_Yield(client, YIELD_TIMEOUT);
		if (rc == QCLOUD_ERR_MQTT_ATTEMPTING_RECONNECT) {
			HAL_SleepMs(1000);
			continue;
		}
		else if (rc != QCLOUD_RET_SUCCESS) {
		        Log_e("Exit loop caused of errCode: %d", rc);
		}
	        /* handle control msg from server */
	        if (sg_control_msg_arrived) {
			deal_down_data_logic();
			/* control msg should reply, otherwise server treat device didn't receive and retain the msg which would be get by  get status*/
			memset((char *)&replyPara, 0, sizeof(sReplyPara));
			replyPara.code = eDEAL_SUCCESS;
			replyPara.timeout_ms = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
			replyPara.status_msg[0] = '\0';                 //add extra info to replyPara.status_msg when error occured
			rc = IOT_Template_ControlReply(client, sg_data_report_buffer, sg_data_report_buffersize, &replyPara);
			if (rc == QCLOUD_RET_SUCCESS) {
				Log_d("Contol msg reply success");
				sg_control_msg_arrived = false;
			} else {
			        Log_e("Contol msg reply failed, err: %d", rc);
			}
		}
	        /*report msg to server*/
	        /*report the lastest properties's status*/
	        if(QCLOUD_RET_SUCCESS == deal_up_data_logic(pReportDataList, &ReportCount)){
			rc = IOT_Template_JSON_ConstructReportArray(client, sg_data_report_buffer, sg_data_report_buffersize, ReportCount, pReportDataList);
			 if (rc == QCLOUD_RET_SUCCESS) {
				 printf("\nsg_data_report_length=%zu, context=%s\n", sg_data_report_buffersize, sg_data_report_buffer);
				 rc = IOT_Template_Report(client, sg_data_report_buffer, sg_data_report_buffersize, OnReportReplyCallback, NULL, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
			 	 if (rc == QCLOUD_RET_SUCCESS) {
					Log_i("data template reporte success");
				 } else {
				        Log_e("data template reporte failed, err: %d", rc);
				 }
			} else {
			         Log_e("construct reporte data failed, err: %d", rc);
			}
		}
		
		for( int i = 0; i < TIMER_COUNT; i++){
			if(time((time_t*)NULL) - sTimer[i].nLast >= sTimer[i].nInterval){
				sTimer[i].nLast = time((time_t*)NULL);
				sTimer[i].pfunc();
			}
		}
		
		HAL_SleepMs(IDLE_INTERVAL);
	}

	rc = IOT_Template_Destroy(client);
    return rc;
}

int main()
{
	pthread_t thread_id;
	set_fake_module();
        pthread_create(&thread_id, NULL, (void *)c_sdk_thread, NULL);
	pthread_join(thread_id, NULL);
	return 0;
}
