/**
 */

#ifndef  __HEXDUMP_H__
#define  __HEXDUMP_H__
/**
 * ע��: �����涨��ı���Ҫ�������Ը���һ�㣬��Ҫ������ı�������, �����������󣬶����ִ���һ�������
 */

/** 
 * @brief               ��ʮ�����Ƶ���ʽ��ӡbuffer
 * @param pkg			[IN], Ҫ��ӡ��buf
 * @param iLen			[IN], buffer�ĳ���
 * @param f				[IN], ��ӡ�Ļص�������֧�ֺ��ʽ�ģ�����LOG_DEBUG��
 * @detail				f ����֧��f(fmt, ...)�ĸ�ʽ�������֧�֣�����ͨ���궨��һ�������α����
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

