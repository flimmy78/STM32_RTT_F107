//
//  THFTPAPI.c
//  MyFTP
//
//  Created by TanHao on 13-6-6.
//  Copyright (c) 2013�� http://www.tanhao.me. All rights reserved.
//
#include "thftpapi.h"
 
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dfs_posix.h> 
#include <rtthread.h>
#include "file.h"
#include "datetime.h"
#include "debug.h"
#include "usr_wifi232.h"
 
//��������,���ؽ��
int ftp_sendcmd_re( int sock, char *cmd, void *re_buf, ssize_t *len)
{
    char        *buf;
    ssize_t     r_len;
    fd_set rd;
	struct timeval timeout;
	
	buf = malloc(512);
    if (SendToSocketB(cmd, strlen(cmd)) == -1 )
	{
		free(buf);
		return -1;
	}	

	r_len = RecvSocketData(SOCKET_B,buf,5);
	if ( r_len == 1 ) 
	{
		free(buf);
		return -1;
	}
	buf[r_len] = 0;
	 
	if (len != NULL) *len = r_len-9;
	if (re_buf != NULL) sprintf(re_buf, "%s", &buf[9]);
	free(buf);
	return 0;

}
 
//��������,���ر��
int ftp_sendcmd( int sock, char *cmd )
{
    char     *buf;
    int      result;
    ssize_t  len;

	buf = malloc(512);
    result = ftp_sendcmd_re(sock, cmd, buf, &len);
    if (result == 0)
    {
        sscanf( buf, "%d", &result );
    }
    free(buf); 
    return result;
}
 
//��¼ftp������
int login_server( int sock, char *user, char *pwd )
{
    char    *buf;
    int     result;

	buf = malloc(128); 
    sprintf( buf, "USER %s\r\n", user );
    result = ftp_sendcmd( sock, buf );
    if ( result == 230 )
	{
		free(buf);
		return 0;
	}
    else if ( result == 331 ) {
        sprintf( buf, "PASS %s\r\n", pwd );
        if ( ftp_sendcmd( sock, buf ) != 230 ) 
		{
			free(buf);
			return -1;
		}
		free(buf);
        return 0;
    }
    else
    {
    	free(buf);
    	return -1;
    }
        
}
 
int create_datasock( int ctrl_sock )
{
    int     lsn_sock;
    int     port;
    int     len;
    struct sockaddr_in sin;
    char    cmd[128];
     
    lsn_sock = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( lsn_sock == -1 ) return -1;
    memset( (char *)&sin, 0, sizeof(sin) );
    sin.sin_family = AF_INET;
    if( bind(lsn_sock, (struct sockaddr *)&sin, sizeof(sin)) == -1 ) {
        closesocket( lsn_sock );
        return -1;
    }
     
    if( listen(lsn_sock, 2) == -1 ) {
        closesocket( lsn_sock );
        return -1;
    }
     
    len = sizeof( struct sockaddr );
    if ( getsockname( lsn_sock, (struct sockaddr *)&sin, (socklen_t *)&len ) == -1 )
    {
        closesocket( lsn_sock );
        return -1;
    }
    port = sin.sin_port;
     
    if( getsockname( ctrl_sock, (struct sockaddr *)&sin, (socklen_t *)&len ) == -1 )
    {
        closesocket( lsn_sock );
        return -1;
    }
     
    sprintf( cmd, "PORT %d,%d,%d,%d,%d,%d\r\n",
            sin.sin_addr.s_addr&0x000000FF,
            (sin.sin_addr.s_addr&0x0000FF00)>>8,
            (sin.sin_addr.s_addr&0x00FF0000)>>16,
            (sin.sin_addr.s_addr&0xFF000000)>>24,
            port>>8, port&0xff );
     
    if ( ftp_sendcmd( ctrl_sock, cmd ) != 200 ) {
        closesocket( lsn_sock );
        return -1;
    }
    return lsn_sock;
}
 
//���ӵ�PASV�ӿ�
int ftp_pasv_connect( int c_sock )
{
    int     r_sock;
    int     send_re;
    ssize_t len;
    int     addr[6];
    char    buf[512];
    char    re_buf[512];
     
    //����PASV����ģʽ
    memset(buf,0x00, sizeof(buf));
    sprintf( buf, "PASV\r\n");
    send_re = ftp_sendcmd_re( c_sock, buf, re_buf, &len);
    if (send_re == 0) {
        sscanf(re_buf, "%*[^(](%d,%d,%d,%d,%d,%d)",&addr[0],&addr[1],&addr[2],&addr[3],&addr[4],&addr[5]);
    }
     
    //����PASV�˿�
    memset(buf,0x00, sizeof(buf));
    sprintf( buf, "%d.%d.%d.%d",addr[0],addr[1],addr[2],addr[3]);
	printf("%s %d\n",buf,(addr[4]*256+addr[5]));
    r_sock = socket_connect(buf,addr[4]*256+addr[5]);
     
    return r_sock;
}
 
//��ʾ����
int ftp_type( int c_sock, char mode )
{
    char    buf[128];
    sprintf( buf, "TYPE %c\r\n", mode );
    if ( ftp_sendcmd( c_sock, buf ) != 200 )
        return -1;
    else
        return 0;
}
 

 
//�����ļ�
int ftp_retrfile( int c_sock, char *s, char *d ,unsigned long long *stor_size, int *stop)
{
    int     d_sock = 0;
    ssize_t     len,write_len,sum=0;
    char    *buf;
    int     handle;
    int     result =0;
    fd_set rd;
		struct timeval timeout;	
	  //char time[20];
	 buf = malloc(1461);
    //�򿪱����ļ�
    handle = fileopen( d,  O_WRONLY | O_CREAT | O_TRUNC, 0 );
    if ( handle == -1 ) 
		{
			free(buf);
			printmsg(ECU_DBG_UPDATE,"ftp_retrfile FILE open failed");
			return -1;
		}
    //���ô���ģʽ
    ftp_type(c_sock, 'I');
     
    //���ӵ�PASV�ӿ�
    d_sock = ftp_pasv_connect(c_sock);
    if (d_sock == -1)
    {
			printmsg(ECU_DBG_UPDATE,"ftp_pasv_connect failed");
      fileclose(handle);
			free(buf);
       return -1;
    }
     
    //����STOR����
    memset(buf,0x00, sizeof(buf));
    sprintf( buf, "RETR %s\r\n", s );
    result = ftp_sendcmd( c_sock, buf );
    if (result >= 300 || result == 0)
    {
			printmsg(ECU_DBG_UPDATE,"RETR response error");
      fileclose(handle);
			free(buf);
      return result;
    }
     
    //��ʼ��PASV��ȡ����
    memset(buf,0x00, sizeof(buf));
		FD_ZERO(&rd);
		FD_SET(d_sock, &rd);
		timeout.tv_sec = 10;
		timeout.tv_usec = 0;

		while (1) {
			len = select(d_sock+1, &rd, NULL, NULL, &timeout);
			if(len <= 0){
				printmsg(ECU_DBG_UPDATE,"ftp_retrfile select out");
				closesocket( d_sock );
				fileclose( handle );
				free(buf);
				return -1;
			}else
			{
				//getcurrenttime(time);
				//printf("%03d :time:%s  :len1:%7d ",++i,time,len);
				if((len = recv( d_sock, buf, 1460, MSG_DONTWAIT )) > 0 )
				{
					sum += len;
					//printf("len2:%7d sum:%7d\n",len,sum);

					write_len = fileWrite( handle, buf, len );
					if (write_len != len || (stop != NULL && *stop))
					{
							closesocket( d_sock );
							fileclose( handle );
							free(buf);
							return -1;
					}
					 
					if (stor_size != NULL)
					{
							*stor_size += write_len;
					}
					memset(buf,0x00, sizeof(buf));
					if(len < 1){	

						printdecmsg(ECU_DBG_UPDATE,"transfer",len);
						break;
					}
				}
				else
				{
					break;
				}
			}				
		}
		closesocket( d_sock );
		fileclose( handle );
		
		
    //����������շ���ֵ
    memset(buf,0x00, sizeof(buf));
    len = recv( c_sock, buf, 512, 0 );
    buf[len] = 0;
    sscanf( buf, "%d", &result );
    if ( result >= 300 ) {
		free(buf);
        return result;
    }
	free(buf);
    return 0;
}


//�ϴ��ļ�
int ftp_storfile( int c_sock, char *s, char *d ,unsigned long long *stor_size, int *stop)
{
    int     d_sock;
    ssize_t     len,send_len;
    char    *buf;
    int     handle;
    int send_re;
    int result = 0;

		buf = malloc(512);
    //�򿪱����ļ�
    handle = fileopen( s,  O_RDONLY,0);
    if ( handle == -1 ) 
		{
			free(buf);
			return -1;
		}
     
    //���ô���ģʽ
    ftp_type(c_sock, 'I');
     
    //���ӵ�PASV�ӿ�
    d_sock = ftp_pasv_connect(c_sock);
    if (d_sock == -1)
    {
      fileclose(handle);
			free(buf);
      return -1;
    }

    //����STOR����
    memset(buf,0x00, sizeof(buf));
    sprintf( buf, "STOR %s\r\n", d );
    send_re = ftp_sendcmd( c_sock, buf );
    if (send_re >= 300 || send_re == 0)
    {
      fileclose(handle);
			free(buf);
      return send_re;
    }
     
    //��ʼ��PASVͨ��д����
    memset(buf,0x00, sizeof(buf));
    while ( (len = fileRead( handle, buf, 512)) > 0)
    {
        send_len = send(d_sock, buf, len, 0);
			
        if (send_len != len ||
            (stop != NULL && *stop))
        {
					printdecmsg(ECU_DBG_UPDATE,"send_len ",send_len);
					printdecmsg(ECU_DBG_UPDATE,"len:%d ",len);
          closesocket( d_sock );
          fileclose( handle );
					free(buf);
          return -1;
        }
         
        if (stor_size != NULL)
        {
            *stor_size += send_len;
        }
    }
    closesocket( d_sock );
    fileclose( handle );
     
    //����������շ���ֵ
    memset(buf,0x00, sizeof(buf));
    len = recv( c_sock, buf, 512, 0 );
    buf[len] = 0;
    sscanf( buf, "%d", &result );
    if ( result >= 300 ) {
			free(buf);
      return result;
    }
		free(buf);
    return 0;
}

 
//���ӷ�����
int ftp_connect( char *host, int port, char *user, char *pwd )
{
    int     c_sock;
    c_sock = connect_server( host, port );
    if ( c_sock == -1 ) return -1;
    if ( login_server( c_sock, user, pwd ) == -1 ) {
        closesocket( c_sock );
        return -1;
    }
    return c_sock;
}
 
//�Ͽ�������
int ftp_quit( int c_sock)
{
    int re = 0;
    re = ftp_sendcmd( c_sock, "QUIT\r\n" );
    closesocket( c_sock );
    return re;
}

//�����ļ�
int ftpgetfile(char *host, int port, char *user, char *pwd,char *remotefile,char *localfile)
{
	unsigned long long stor_size = 0;
	int stop = 0,ret = 0;
	int sockfd = ftp_connect( host, port, user, pwd  );
	if(sockfd != -1)
	{
		printdecmsg(ECU_DBG_UPDATE,"ftp connect successful",sockfd);	
	}
	
	ret = ftp_retrfile(sockfd, remotefile, localfile ,&stor_size, &stop);
	printdecmsg(ECU_DBG_UPDATE,"ret",ret);
	printdecmsg(ECU_DBG_UPDATE,"stor_size",stor_size);
	printdecmsg(ECU_DBG_UPDATE,"stop",stop);
	ftp_quit( sockfd);
	return ret;
}

//�ϴ��ļ�
int ftpputfile(char *host, int port, char *user, char *pwd,char *remotefile,char *localfile)
{
	unsigned long long stor_size = 0;
	int stop = 0,ret = 0;
	int sockfd = ftp_connect( host, port, user, pwd  );

	if(sockfd != -1)
	{
		printdecmsg(ECU_DBG_UPDATE,"ftp connect successful",sockfd);	
	}

  ret = ftp_storfile(sockfd, localfile,remotefile ,&stor_size, &stop);
	printdecmsg(ECU_DBG_UPDATE,"ret",ret);
	printdecmsg(ECU_DBG_UPDATE,"stor_size",stor_size);
	printdecmsg(ECU_DBG_UPDATE,"stop",stop);

	ftp_quit( sockfd);
	return ret;
}

int getfile(char *remoteFile, char *localFile)
{
	char FTPIP[50];
	int port=0;
	char user[20]={'\0'};
	char password[20]={'\0'};
	getFTPConf(FTPIP,&port,user,password);

	printf("FTPIP:%s\nport:%d\nuser:%s\npassword:%s\n ",FTPIP,port,user,password);

	return ftpgetfile(FTPIP,port, user, password,remoteFile,localFile);
}

int putfile(char *remoteFile, char *localFile)
{
	char FTPIP[50];
	int port=0;
	char user[20]={'\0'};
	char password[20]={'\0'};
	getFTPConf(FTPIP,&port,user,password);

	printf("FTPIP:%s\nport:%d\nuser:%s\npassword:%s\n ",FTPIP,port,user,password);

	return ftpputfile(FTPIP,port, user, password,remoteFile,localFile);
}

#ifdef RT_USING_FINSH
#include <finsh.h>

FINSH_FUNCTION_EXPORT(getfile,get file from ftp.)

FINSH_FUNCTION_EXPORT(putfile,put file from ftp.)

void sockettest()
{

	struct hostent *host;
	int sock;
	struct sockaddr_in server_addr;
	rt_thread_delay(RT_TICK_PER_SECOND * 10);
	
	
	/* ͨ��������ڲ���url���host��ַ��������������������������� */
	host = gethostbyname("60.190.131.190");

	while(1)
	{
		/* ����һ��socket��������SOCKET_STREAM��TCP���� */
		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		{
			/* ����socketʧ�� */
			rt_kprintf("Socket error\n");
			return;
		}
		printf("sockfd:%d\n",sock);
		/* ��ʼ��Ԥ���ӵķ���˵�ַ */
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(9219);
		server_addr.sin_addr = *((struct in_addr *) host->h_addr);
		rt_memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));
		/* ���ӵ������ */
		if (connect(sock, (struct sockaddr *) &server_addr,
		sizeof(struct sockaddr)) == -1)
		{
			/* ����ʧ�� */
			rt_kprintf("Connect error\n");
			/*�ͷŽ��ջ��� */
			return;
		}
			
	}

	
}

FINSH_FUNCTION_EXPORT(sockettest, eg:sockettest());


#endif

 