/**
 */

#ifndef  __HEXDUMP_H__
#define  __HEXDUMP_H__
/**
 * 注意: 宏里面定义的变量要定义得相对复杂一点，不要和外面的变量混了, 否则会引起错误，而这种错误一般很难找
 */

/** 
 * @brief               以十六进制的形式打印buffer
 * @param pkg			[IN], 要打印的buf
 * @param iLen			[IN], buffer的长度
 * @param f				[IN], 打印的回调函数，支持宏格式的，比如LOG_DEBUG等
 * @detail				f 必须支持f(fmt, ...)的格式，如果不支持，可以通过宏定义一个适配的伪函数
 * @sample
 * 						#define ERR(fmt, ...) fprintf(stderr, "[%d in %s]"fmt, __LINE__, __FILE__, ##__VA_ARGS__)
 * 						HEXDUMP(buf, len, ERR);
 **/
#define HEXDUMP(pkg, iLen, f)\
do{	\
	int i = 0; \
	unsigned char * __hexdump_pkg = (unsigned char *)pkg;\
	char __hexdump_buf_[100];\
	char __hexdump_buf_2[100];\
	char __hexdump_buf_3[100];\
	memset(__hexdump_buf_, 0x00, sizeof(__hexdump_buf_)); \
	memset(__hexdump_buf_2, 0x00, sizeof(__hexdump_buf_2)); \
	int j;\
	for(i = 0; i < iLen; i++) \
	{ \
		j = i %16;\
		if(j == 0) \
		{\
			strncat(__hexdump_buf_, " ", 100);\
			strncat(__hexdump_buf_, __hexdump_buf_2, 100);\
			strncat(__hexdump_buf_, "\n", 100);\
			f("%s", __hexdump_buf_);\
			memset(__hexdump_buf_, 0x00, sizeof(__hexdump_buf_)); \
			memset(__hexdump_buf_2, 0x00, sizeof(__hexdump_buf_2)); \
		}\
		snprintf(__hexdump_buf_ + j*3, 4, "%02X ", (unsigned char )__hexdump_pkg[i]);\
		if(isgraph(__hexdump_pkg[i]))\
			snprintf(__hexdump_buf_2 + j*2, 3, "%c ", (unsigned char )__hexdump_pkg[i]);\
		else\
			snprintf(__hexdump_buf_2 + j*2, 3, ". ");\
	} \
	snprintf(__hexdump_buf_3, 100, "%-48.48s %-32.32s\n", __hexdump_buf_, __hexdump_buf_2);\
	f("%s", __hexdump_buf_3);\
}while(0)

#endif   /* ----- #ifndef __HEXDUMP_H__  ----- */

