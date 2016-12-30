/**
 * @company             
 */

#ifndef  __TRACE_H__
#define  __TRACE_H__

#include <stdio.h>
#include <string.h>
#include <map>
#include <iostream>

#ifdef __TRACE__
#define TRACE CTrace __Trace__(__FUNCTION__)
#else
#define PRINT_TRACE do{}while(0)
#define TRACE do{}while(0)
#endif

#ifndef LOG_DEBUG
#define LOG_DEBUG(fmt, a...) fprintf(stderr, "[%d in %s]"fmt, __LINE__, __FILE__, ##a )
#endif

std::map<std::string, int> FuncCnt;
class CTrace
{
	public:
		CTrace(const char * p)
		{
			strncpy(m_Function, p, 256);
			LOG_DEBUG("------------%s Enter------------\n", m_Function);
			FuncCnt[m_Function]++;
		}
		~CTrace()
		{
			LOG_DEBUG("------------%s Exit-------------\n", m_Function);
		}
	private:
		char m_Function[256];
};

#endif   /* ----- #ifndef __TRACE_H__  ----- */

