#include <stdarg.h>
#include <cstdio>
#include "Macros.h"
#include "flog.h"

int Flog_SeverityToIndex(Flog_Severity severity)
{
	for(int i = 0; i < 8; i++){
		if(((int)severity >> i) & 1){
			return i;
		}
	}

	return 8;
}

const char* Flog_SeverityToString(Flog_Severity severity)
{
	static const char *severityString[] = { "D1", "D2", "D3", "VV", "II", "WW", "EE", "FF", "??" };
	return severityString[Flog_SeverityToIndex(severity)];
}


static const Flog_Severity logFileSeverity = Flog_SDebug3;

void Flog_Log(const char* file, uint32_t lineNumber, Flog_Severity severity, const char* format, ...)
{
	// never log anything to file if this isn't a debug build
	if(!(IS_DEBUG_BUILD))
		return;

	va_list fmtargs;
	char buffer[4096];

	buffer[sizeof(buffer) - 1] = '\0';

	va_start(fmtargs, format);
	vsnprintf(buffer, sizeof(buffer) - 1, format, fmtargs);
	va_end(fmtargs);
	
	printf("[%s] %s:%d %s\r\n", Flog_SeverityToString(severity), file, lineNumber, buffer);
	
	if(severity >= logFileSeverity){ 
		FILE* logfile = fopen("frameserver.log", "a");
		if(logfile){
			fprintf(logfile, "[%s] %s:%d %s\r\n", Flog_SeverityToString(severity), file, lineNumber, buffer);
			fclose(logfile);
		}
	}

	fflush(stdout);
}
