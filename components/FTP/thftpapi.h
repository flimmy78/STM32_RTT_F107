#ifndef __THFTPAPI_H__
#define __THFTPAPI_H__
//
//  THFTPAPI.h
//  MyFTP
//
//获取连接服务器IP
void getFTPConf(char *FTPIP,int *port,char* user,char *password);
 
//连接服务器  OK
int ftp_connect( char *host, int port, char *user, char *pwd );
//断开服务器  OK
int ftp_quit( int c_sock);
 
//设置表示类型 OK
int ftp_type( int c_sock, char mode );
 
 
//下载文件
int ftp_retrfile( int c_sock, char *s, char *d ,unsigned long long *stor_size, int *stop);
//上传文件
int ftp_storfile( int c_sock, char *s, char *d ,unsigned long long *stor_size, int *stop);
 


//下载文件
int ftpgetfile(char *host, int port, char *user, char *pwd,char *remotefile,char *localfile);
	
//上传文件
int ftpputfile(char *host, int port, char *user, char *pwd,char *remotefile,char *localfile);

int putfile(char *remoteFile, char *localFile);
int getfile(char *remoteFile, char *localFile);

#endif /*__THFTPAPI_H__*/
