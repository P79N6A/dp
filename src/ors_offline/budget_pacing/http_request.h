#ifndef __HTTP__  
#define __HTTP__  
  
#include <string.h>  
#include <stdlib.h>  
#include <stdio.h>  
#include <unistd.h>  
#include <fcntl.h>  
#include <errno.h>  
#include <netdb.h>  
#include <stdarg.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <sys/types.h>        
#include <sys/socket.h>  
#include <iostream>
#include <string>
  
#define BUFSIZE 41000  
#define URLSIZE 1024  
#define INVALID_SOCKET -1  
#define __DEBUG__
  
class HttpRequest  
{  
	public:
	HttpRequest();  
	~HttpRequest();  
	void DebugOut(const char *fmt, ...);  
								          
	int HttpGet(const char* strUrl, char* strResponse);  
	int HttpPost(const char* strUrl, const char* strData, char* strResponse);  
											  
	private:  
	int HttpRequestExec(const char* strMethod, const char* strUrl, const char* strData, char* strResponse);  
	std::string HttpHeadCreate(const char* strMethod, const char* strUrl, const char* strData);  
	std::string HttpDataTransmit(const char *strHttpHead, const int iSockFd);  
			              
	int   GetPortFromUrl(const char* strUrl);  
	std::string GetIPFromUrl(const char* strUrl);  
	std::string GetParamFromUrl(const char* strUrl);  
	std::string GetHostAddrFromUrl(const char* strUrl);  
													          
	int   SocketFdCheck(const int iSockFd);   
																												          
	static int m_iSocketFd;  
};  
  
#endif
