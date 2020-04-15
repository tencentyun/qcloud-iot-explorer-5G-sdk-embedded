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

#ifndef _MODULE_OPS_H_
#define _MODULE_OPS_H_

#define DEFAULT_CODE ;


#define MODULE_OPS	\
	/*pPlatInfo*/	\
	MODULE_OP(fw_ver, NULL, NO_RESULT, "AT+GMR", SINGLELINE, fw_ver, client, 0, DEFAULT_CODE)  \
        /*pSelfInfo*/   \
	MODULE_OP(Manufacturer, NULL, NO_RESULT, "AT+GMI", SINGLELINE, manufacturer, client, 0, DEFAULT_CODE)	\
	MODULE_OP(Model, NULL, NO_RESULT, "AT+CGMM", SINGLELINE, model, client, 0, DEFAULT_CODE)    \
	MODULE_OP(Operator, "AT+COPS=3,0", NO_RESULT, "AT+COPS?", SINGLELINE, operator, client, 3, DEFAULT_CODE)	\
	MODULE_OP(IMSI, NULL, NO_RESULT, "AT+CIMI", SINGLELINE, imsi, client, 0, DEFAULT_CODE)	\
	MODULE_OP(MSISDN, NULL, NO_RESULT, "AT+CNUM", SINGLELINE, msisdn, client, 2, DEFAULT_CODE)	\
        /*Properties*/  \
	MODULE_OP(IPAddr, NULL, NO_RESULT, "AT+CGPADDR=1", SINGLELINE, ipaddr, client, 2, DEFAULT_CODE)	\
	MODULE_OP(RSSI, NULL, NO_RESULT, "AT+CSQ", SINGLELINE, rssi, client, -1, DEFAULT_CODE) \
	MODULE_OP(CellID, "AT+CREG=2", NO_RESULT, "AT+CREG?", SINGLELINE, cellid, client, -1, DEFAULT_CODE)

#endif

