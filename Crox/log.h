#pragma once
#include <tchar.h>

#define CXLOG_SEVERITY_INFO  0
#define CXLOG_SEVERITY_DEBUG 1
#define CXLOG_SEVERITY_TRACE 2
#define CXLOG_SEVERITY_WARN  3
#define CXLOG_SEVERITY_ERROR 4
#define CXLOG_SEVERITY_FATAL 5

/*
* @brief sets the output file to where the logs are written. (In the Debug Build, if NULL is passed, then the output stream will go onto the MSVC Debug Output. 
* @return - last output log file. 
*/
FILE* log_setLog(_In_opt_ FILE* newFile);

int log_logEntry(_In_ int severity, _Printf_format_string_ TCHAR* fmt, ...);

#define cxINFO(...)		log_logEntry(CXLOG_SEVERITY_INFO,  __VA_ARGS__)
#define cxDEBUG(...)	log_logEntry(CXLOG_SEVERITY_DEBUG, __VA_ARGS__)
#define cxTRACE(...)	log_logEntry(CXLOG_SEVERITY_TRACE, __VA_ARGS__)
#define cxWARN(...)		log_logEntry(CXLOG_SEVERITY_WARN,  __VA_ARGS__)
#define cxERROR(...)	log_logEntry(CXLOG_SEVERITY_ERROR, __VA_ARGS__)
#define cxFATAL(...)	log_logEntry(CXLOG_SEVERITY_FATAL, __VA_ARGS__)

