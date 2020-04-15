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

#ifndef HANDLE_MODULE
#define HANDLE_MODULE

struct module_ops_impl {
	/*query operations*/
	int (*fw_ver_query)(char *info);
	int (*manufacturer_query)(char *info);
	int (*model_query)(char *info);
	int (*operator_query)(char *info);
	int (*imsi_query)(char *info);
	int (*msisdn_query)(char *info);
	int (*ipaddr_query)(char *info);	
	int (*rssi_query)(char *info);
	int (*cellid_query)(char *info);
	/*dial_up operation*/
	int (*dial_up)();
};
#define OPS_COUNT sizeof(struct module_ops_impl)/sizeof(int (*)(char *))
union module_ops {
	/*op implementation*/
	struct module_ops_impl module_ops_impl;
	/*op iterator*/
	int (*module_ops_iter[OPS_COUNT])(char *);

};

struct module {
	const char *vid;
	const char *pid;
	const char *at_port;
	union module_ops *module_ops;
};

struct module* get_current_module();
void set_fake_module();
#endif
