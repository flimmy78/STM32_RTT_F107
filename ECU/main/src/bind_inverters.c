/*****************************************************************************/
/*  File      : bind_inverters.c                                             */
/*****************************************************************************/
/*  History:                                                                 */
/*****************************************************************************/
/*  Date       * Author          * Changes                                   */
/*****************************************************************************/
/*  2017-03-05 * Shengfeng Dong  * Creation of the file                      */
/*             *                 *                                           */
/*****************************************************************************/

/*****************************************************************************/
/*  Include Files                                                            */
/*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "variation.h"
#include "zigbee.h"
#include "bind_inverters.h"
#include "channel.h"
#include <rtdef.h>
#include <rtthread.h>
#include "debug.h"
#include "file.h"
#include "rthw.h"

/*****************************************************************************/
/*  Definitions                                                              */
/*****************************************************************************/

#define MIN_GROUP_NUM 12

/*****************************************************************************/
/*  Variable Declarations                                                    */
/*****************************************************************************/
extern ecu_info ecu;
extern inverter_info inverter[MAXINVERTERCOUNT];
extern struct rt_device serial4;	

unsigned char rateOfProgress = 0;		//µ±Ç°½ø¶È
#define ZIGBEE_SERIAL (serial4)

/*****************************************************************************/
/*  Function Implementations                                                 */
/*****************************************************************************/
void saveraduistostruct(char *id,int raduis)
{
	FILE *fp = NULL;
	fp = fopen("/tmp/radui.txt", "a");
	if(fp)
	{
		fprintf(fp, "%s %X\n", id,raduis);
		fclose(fp);
	}
}

int getaddrOldOrNew(char *id)
{
	int  ret,index;
	inverter_info *curinverter = inverter;
	int short_addr = 0;
	char recvMsg[256];
	char inverterid[13];
	char command[21] = {0xAA, 0xAA, 0xAA, 0xAA, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x00, 0xAE, 0x06};

	command[15]=((id[0]-0x30)*16+(id[1]-0x30));
	command[16]=((id[2]-0x30)*16+(id[3]-0x30));
	command[17]=((id[4]-0x30)*16+(id[5]-0x30));
	command[18]=((id[6]-0x30)*16+(id[7]-0x30));
	command[19]=((id[8]-0x30)*16+(id[9]-0x30));
	command[20]=((id[10]-0x30)*16+(id[11]-0x30));

	//åéä¸æ¥åå°éåå¨IDçå½ä»?
	clear_zbmodem();
	ZIGBEE_SERIAL.write(&ZIGBEE_SERIAL, 0,command, 21);
	print2msg(ECU_DBG_MAIN,"Get each inverter's short address", id);
	printhexmsg(ECU_DBG_MAIN,"Sent", command, 21);

	//æ¥æ¶
	ret = zigbeeRecvMsg(recvMsg, 5);
	snprintf(inverterid, sizeof(inverterid), "%02x%02x%02x%02x%02x%02x",
			recvMsg[4], recvMsg[5], recvMsg[6], recvMsg[7], recvMsg[8], recvMsg[9]);
	if ((11 == ret)
		&& (0xFF == recvMsg[2])
		&& (0xFF == recvMsg[3])
		&& (0 == strcmp(id, inverterid))) {
		//è·åç­å°åæå
		short_addr = recvMsg[0]*256 + recvMsg[1];
		curinverter = inverter;
		for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)			//ææéåå¨è½®è®?
		{
			if(!strcmp(curinverter->id,id))
			{
				curinverter->shortaddr = short_addr;
				break;
			}
		}
			
		return 1;
	}
	else if ((11 == ret)
		&& (0 == strcmp(id, inverterid))) {
		saveraduistostruct(inverterid,recvMsg[2]);	//ä¿å­è·¯ç±æ·±åº¦å°ç»æä½
		curinverter = inverter;
		for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)			//ææéåå¨è½®è®?
		{
			if(!strcmp(curinverter->id,inverterid))
			{
				curinverter->zigbee_version = recvMsg[3];
				break;
			}
		}

		//æå­ç»å®æ å¿
			curinverter = inverter;
		for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)			//ææéåå¨è½®è®?
		{
			if(!strcmp(curinverter->id,inverterid))
			{
				curinverter->inverterstatus.bindflag = 1;
				break;
			}
		}
		return 1;
	}

	return 0;
}

void send11order(char *inverterid,int count)
{
	int i;
	int check=0;
	char sendbuff[23]={0xAA,0xAA,0xAA,0xAA,0x11,0x00,0x00,0xCC,0xCC,0xcc,0x00,0x00,0x00,0x00,0x08,0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xdd,0xdd};
	sendbuff[7]=ecu.panid/256;
	sendbuff[8]=ecu.panid%256;
	sendbuff[9]=ecu.channel;
	for(i=4;i<12;i++)
		check=check+sendbuff[i];
	sendbuff[12] = check/256;
	sendbuff[13] = check%256;
	sendbuff[15]=((inverterid[0]-0x30)*16+(inverterid[1]-0x30));
	sendbuff[16]=((inverterid[2]-0x30)*16+(inverterid[3]-0x30));
	sendbuff[17]=((inverterid[4]-0x30)*16+(inverterid[5]-0x30));
	sendbuff[18]=((inverterid[6]-0x30)*16+(inverterid[7]-0x30));
	sendbuff[19]=((inverterid[8]-0x30)*16+(inverterid[9]-0x30));
	sendbuff[20]=((inverterid[10]-0x30)*16+(inverterid[11]-0x30));
	sendbuff[21]=(2*count)/256;
	sendbuff[22]=(2*count)%256;
	ZIGBEE_SERIAL.write(&ZIGBEE_SERIAL, 0,sendbuff, 21);
	printhexmsg(ECU_DBG_MAIN,"Ready Change Inverter Panid (11order)", sendbuff, 23);
	rt_hw_s_delay(1);
}

void send22order()
{
	int i;
	int check=0;
	char sendbuff[15]={0xAA,0xAA,0xAA,0xAA,0x22,0x00,0x00,0xCC,0xCC,0xcc,0x00,0x00,0x00,0x00,0x00};
	sendbuff[7]=ecu.panid/256;
	sendbuff[8]=ecu.panid%256;
	sendbuff[9]=ecu.channel;
	for(i=4;i<12;i++)
		check=check+sendbuff[i];
	sendbuff[12] = check/256;
	sendbuff[13] = check%256;

	for(i=0;i<3;i++){
		ZIGBEE_SERIAL.write(&ZIGBEE_SERIAL, 0,sendbuff, 15);
		printhexmsg(ECU_DBG_MAIN,"Change Panid Now (22order)", sendbuff, 15);
		rt_hw_s_delay(1);
	}
}

void getshortadd(char *recvbuff)
{
	int index;
	inverter_info *curinverter = inverter;
	char curinverterid[13];
	sprintf(curinverterid,"%02x%02x%02x%02x%02x%02x",recvbuff[4],recvbuff[5],recvbuff[6],recvbuff[7],recvbuff[8],recvbuff[9]);
	curinverter = inverter;
	for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)			//ææéåå¨è½®è®?
	{
		if(!strcmp(curinverter->id,curinverterid))
		{
			curinverter->shortaddr = (recvbuff[0]*256+recvbuff[1]);
		}
	}
}


//ç»å®éåå?
void bind_inverters()
{
  int num = 0,i = 0,index = 0;
	inverter_info *curinverter = inverter;
	unsigned short temppanid=ecu.panid;int k;
	char recvbuff[256];
   //0.è®¾ç½®ä¿¡é
   rateOfProgress = 0;
   process_channel();
   rateOfProgress = 40;
   zb_change_ecu_panid(); //å°ECUçPANIDåä¿¡éè®¾ç½®æéç½®æä»¶ä¸­ç

	//1.ç»å®å·²ç»æç­å°åçéåå?å¦ç»å®å¤±è´¥ï¼åéè¦éæ°è·åç­å°å	
	//å¯¹æ¯ä¸ªéåå¨è¿è¡ç»å®?
	curinverter = inverter;
	for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)			//ææéåå¨è½®è®?
	{
		if((curinverter->shortaddr != 0)&&(curinverter->inverterstatus.bindflag == 0))
		{
			if (!zb_off_report_id_and_bind(curinverter->shortaddr)) {
				//ç»å®å¤±è´¥,éç½®ç­å°å
				curinverter->shortaddr = 0;
			}
		}
	}	


	rateOfProgress = 41;
	curinverter = inverter;
	num = 0;
	for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)			//ææéåå¨è½®è®?
	{
		if(curinverter->shortaddr == 0)
			num++;
	}

	if(num>0)
	{
		rateOfProgress = 42;
		//æ¸ç©ºç­å°å
		zb_restore_ecu_panid_0xffff(ecu.channel);
		rateOfProgress = 43;
		for(i=0;i<5;i++)
		{
			
			curinverter = inverter;
			num = 0;
			for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)			//ææéåå¨è½®è®?
			{
				if((curinverter->shortaddr == 0) && (curinverter->inverterstatus.bindflag == 0))
					num++;
			}

			ecu.panid=0xFFFF;
			if(num==0)
				break;
			curinverter = inverter;
			for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)			//ææéåå¨è½®è®?
			{
				if((curinverter->shortaddr == 0) && (curinverter->inverterstatus.bindflag == 0))
				{
					zb_change_inverter_channel_one(curinverter->id,ecu.channel);//ææéåå¨è®¾ç½®æ0xFFFF
					rt_hw_s_delay(3);//zigbeeRecvMsg(recvbuff,5);
				}
			}
			
			curinverter = inverter;
			for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)			//ææéåå¨è½®è®?
			{
				if((curinverter->shortaddr == 0) && (curinverter->inverterstatus.bindflag == 0))
				{
					for(k=0;k<3;k++){
						if(1==getaddrOldOrNew(curinverter->id))
						{
							rateOfProgress += 1;
							if(rateOfProgress >= 64) rateOfProgress = 64;
							break;
						}
						rt_hw_s_delay(2);
					}
				
//					if(-1!=zigbeeRecvMsg(recvbuff,5))
//						getshortadd(recvbuff);
				}
			}
		}
		rateOfProgress=65;
		for(i=0;i<3;i++)			//æ°ç»ç½?
		{
			curinverter = inverter;
			for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)			//ææéåå¨è½®è®?
			{
				if(curinverter->shortaddr == 0)
				{
					num++;
				}
			}
			ecu.panid=temppanid;
			curinverter = inverter;
			for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)			//ææéåå¨è½®è®?
			{
				if(curinverter->shortaddr == 0)
				{
					send11order(curinverter->id,num);
					if(-1!=zigbeeRecvMsg(recvbuff,5))
						getshortadd(recvbuff);
					rateOfProgress += 1;
					if(rateOfProgress >= 89) rateOfProgress = 89;
				}
			}
		}
		//æ§ç»ç½?
		rateOfProgress=90;
		ecu.panid=temppanid;
		curinverter = inverter;
		for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)			//ææéåå¨è½®è®?
		{
			if(curinverter->inverterstatus.bindflag == 0)
			{
				for(i=0;i<3;i++)
					zb_change_inverter_channel_one(curinverter->id,ecu.channel);
			}
		}
		curinverter = inverter;
		for(index=0; (index<MAXINVERTERCOUNT)&&(12==strlen(curinverter->id)); index++, curinverter++)			//ææéåå¨è½®è®?
		{
			curinverter->inverterstatus.bindflag=0;
		}			
		
		ecu.panid=0xFFFF;
		rateOfProgress = 95;
		send22order();
		rt_hw_s_delay(10);
		ecu.panid=temppanid;
		zb_change_ecu_panid();
	}
	rt_hw_s_delay(10);
	rateOfProgress = 100;
	updateID();

}

