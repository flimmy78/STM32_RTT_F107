#include <stdlib.h>
#include <string.h>
#include "remote_control_protocol.h"
#include "debug.h"
#include "rthw.h"
#include "threadlist.h"
int mysystem(const char *command)
{
	int res;

	print2msg(ECU_DBG_CONTROL_CLIENT,"Execute:",(char*) command);
	//��Ҫ�������ڴ�����
	if(!memcmp(command,"reboot",6))
	{
		res = 0;
		reboot();
	}else if(!memcmp(command,"restart UPDATE",14))
	{
		restartThread(TYPE_UPDATE);
		res = 0;
	}else if(!memcmp(command,"restart IDWRITE",15))
	{
		restartThread(TYPE_IDWRITE);
		res = 0;
	}else if(!memcmp(command,"restart MAIN",12))
	{
		restartThread(TYPE_MAIN);
		res = 0;
	}else if(!memcmp(command,"restart CLIENT",14))
	{
		restartThread(TYPE_CLIENT);
		res = 0;
	}else if(!memcmp(command,"restart NTP",11))
	{
		restartThread(TYPE_NTP);
		res = 0;
	}else if(!memcmp(command,"ftpput",6))
	{
		//�ϴ�����
	}
		
	printdecmsg(ECU_DBG_CONTROL_CLIENT,"res",res);
	if(-1 == res){
		printmsg(ECU_DBG_CONTROL_CLIENT,"Failed to execute: system error.");
		return CMD_ERROR;
	}
	else{

		if(0 == res){
			printmsg(ECU_DBG_CONTROL_CLIENT,"Execute successfully.");
		}
		else{
			printdecmsg(ECU_DBG_CONTROL_CLIENT,"Failed to execute: shell failed", res);
			return CMD_ERROR;
		}

	}
	return SUCCESS;
}