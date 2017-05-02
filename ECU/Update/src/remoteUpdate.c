#include "remoteUpdate.h"
#include <rtthread.h>
#include "thftpapi.h"
#include "flash_if.h"
#include "dfs_posix.h"
#include "rthw.h"
#include "myfile.h"
#include "version.h"
#include "threadlist.h"
#include "debug.h"
#include "file.h"

#define UPDATE_PATH_SUFFIX "ecu-r-m3.bin"
#define UPDATE_PATH "/FTP/ecu.bin"


extern rt_mutex_t record_data_lock; 

int updateECUByVersion(void)
{
	int ret = 0;
	char IPFTPadd[50] = {'\0'};
	char remote_path[100] = {'\0'};
	int port=0;
	char user[20]={'\0'};
	char password[20]={'\0'};
	getFTPConf(IPFTPadd,&port,user,password);

	print2msg(ECU_DBG_UPDATE,"FTPIP",IPFTPadd);
	printdecmsg(ECU_DBG_UPDATE,"port",port);
	print2msg(ECU_DBG_UPDATE,"user",user);
	print2msg(ECU_DBG_UPDATE,"password",password);
	
	//获取服务器IP地址
	sprintf(remote_path,"/ECU_R_M3/V%s.%s/%s",MAJORVERSION,MINORVERSION,UPDATE_PATH_SUFFIX);
	print2msg(ECU_DBG_UPDATE,"VER Path",remote_path);
	rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
	ret=ftpgetfile(IPFTPadd, port, user, password,remote_path,UPDATE_PATH);
	if(!ret)
	{
		//获取到文件，进行更新
		FLASH_Unlock();
		FLASH_If_Erase_APP2();
		FLASH_IF_FILE_COPY_TO_APP2(UPDATE_PATH);
		unlink(UPDATE_PATH);
		reboot();
	}else
	{
		unlink(UPDATE_PATH);
	}
	rt_mutex_release(record_data_lock);	
	return ret;
}

int updateECUByID(void)
{
	int ret = 0;
	char IPFTPadd[50] = {'\0'};
	char remote_path[100] = {'\0'};
	char ecuID[13] = {'\0'};
	int port = 0;
	char user[20]={'\0'};
	char password[20]={'\0'};
	
	getFTPConf(IPFTPadd,&port,user,password);

	print2msg(ECU_DBG_UPDATE,"FTPIP",IPFTPadd);
	printdecmsg(ECU_DBG_UPDATE,"port",port);
	print2msg(ECU_DBG_UPDATE,"user",user);
	print2msg(ECU_DBG_UPDATE,"password",password);
	//获取ECU的ID
	get_ecuid(ecuID);
	
	//获取服务器IP地址
	sprintf(remote_path,"/ECU_R_M3/%s/%s",ecuID,UPDATE_PATH_SUFFIX);
	print2msg(ECU_DBG_UPDATE,"ID Path",remote_path);
	rt_mutex_take(record_data_lock, RT_WAITING_FOREVER);
	ret=ftpgetfile(IPFTPadd, port, user, password,remote_path,UPDATE_PATH);
	if(!ret)
	{
		//获取到文件，进行更新
		FLASH_Unlock();
		FLASH_If_Erase_APP2();
		FLASH_IF_FILE_COPY_TO_APP2(UPDATE_PATH);
		unlink(UPDATE_PATH);
		reboot();
	}else
	{
		unlink(UPDATE_PATH);
	}
	rt_mutex_release(record_data_lock);	
	return ret;
}

void remote_update_thread_entry(void* parameter)
{
	int i = 0;
	rt_thread_delay(RT_TICK_PER_SECOND * START_TIME_UPDATE);
	while(1)
	{
		for(i = 0;i<3;i++)
		{
			if(-1 != updateECUByVersion())
				break;
		}
		for(i = 0;i<3;i++)
		{
			if(-1 != updateECUByID())
				break;
		}
		
		//rt_thread_delay(RT_TICK_PER_SECOND*10);		
		rt_thread_delay(RT_TICK_PER_SECOND*86400);		
	}	

}
