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

#include <stdio.h>
#include "handle_module.h"
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>



#define MAXTIMER 2
#define MAX_STR_LEN 512

int fake_cellid_query(char *info){
	srand((unsigned)time(NULL));
	char res[6];
	for(int i=0;i<5;i++){
		int a = rand()%15+0;
		if(a<10){
			res[i] = a+'0'; 
		}else{
			res[i] = a+55;
		}	
	}
    return sprintf(info,"%s",res);
	
}

int fake_rssi_query(char *info){	
    int a;
    srand((unsigned)time(NULL));
    a = rand()%70+10;
    return sprintf(info, "-%ddBm",a);
}

int fake_fw_ver_query(char *info)
{
	return sprintf(info, "1.0");
}
int fake_imsi_query(char *info)
{
	return sprintf(info, "46003000000000");
}   
int fake_msisdn_query(char *info)
{
	return sprintf(info, "13800138000");
}   
int fake_ipaddr_query(char *info)
{
	return sprintf(info, "127.0.0.1");
}

int fake_manufacturer_query(char *info)
{
	return sprintf(info, "Tencent");
}

int fake_model_query(char *info)
{
	return sprintf(info, "fake");
}
int fake_operator_query(char *info)
{
	return sprintf(info, "China Mobile");
}





union module_ops fake_ops = {
	.module_ops_impl.fw_ver_query = fake_fw_ver_query,
	.module_ops_impl.imsi_query=fake_imsi_query,
	.module_ops_impl.ipaddr_query=fake_ipaddr_query,
	.module_ops_impl.msisdn_query=fake_msisdn_query,
	.module_ops_impl.manufacturer_query=fake_manufacturer_query,
	.module_ops_impl.model_query=fake_model_query,
	.module_ops_impl.operator_query=fake_operator_query,
	.module_ops_impl.rssi_query=fake_rssi_query,
	.module_ops_impl.cellid_query=fake_cellid_query,
	};
