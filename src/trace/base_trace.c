/************************************************************************
*                                                                     
*                                                           
*                                                                  
*  Project Code:                                  
*  Create Date:                                      
*  Author:                                  
*  Modify Date:                                          
*  Module Name:                                  
*  Others:                                                            
*                                                                      
*                         
*                                             
*                                                                      
************************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <types.h>
#include <trace.h>

#define BASE_TRACE_STR_LEN_MAX			256	/* trace string max lenght		*/
#define BASE_TRACE_MODULE_NAME_LEN_MAX	50	/* trace module name max lenght	*/

/* trace level all and trace module all */
#define BASE_TRACE_LEVEL_ALL	0xFFFFFFFF
#define BASE_TRACE_MODULE_ALL	0xFFFFFFFF

/* trace module and level */
static UINT32 gbasetracemodule	= BASE_TRACE_MODULE_MASK_OS;
static UINT32 gbasetracelevel 	= ( BASE_TRACE_LEVEL_MASK_ERR);
#define BASE_TRACE_MODLUE	gbasetracemodule
#define BASE_TRACE_LEVEL	gbasetracelevel

static UINT32 gbasetaceocallog = 1;

static UINT32 gbasetaceshowtime = 1;

/* trace global enable/disable */
static UINT32 gbasetaceenable = 1;
static CHAR gbasetracebuffer[BASE_TRACE_MODULE_NAME_LEN_MAX] = {0};

static sem_t *gSysLogSemId = 0;

#define   SEEK_START   0        /* offset from start of file    */
#define   SEEK_CURR    1        /* offset from current position */
#define   SEEK_END     2        /* offset from end of file      */

#define SYS_LOG_MAX_LENGTH 1000000

void base_trace_getLength (const CHAR *format, va_list VarArgList, UINT32 *pi4ArgCount, UINT32 *pi4VarArgLength)
{
    const CHAR		*formatString;
    const CHAR		*pi1TmpFmtString;
    const CHAR		*pi1TmpArgString;
    const UINT32		u4MaxIntLength = 19;
    UINT32	u4ArgCount = 0;
    CHAR*	pAddr;
    UINT32	u4VarArgLength 	= 0;

    formatString = pi1Fmt;
    while (*formatString)
    {
        if (*formatString == '%')
        {
            formatString++;
            if (*formatString != '%')
            {
                pAddr = (CHAR*)va_arg (VarArgList, fwk_addr_t);
                u4ArgCount++;
                pi1TmpFmtString = formatString;
                if(*pi1TmpFmtString == '-')
                {
                    ++pi1TmpFmtString;
                }
                while((*pi1TmpFmtString >= '0') &&(*pi1TmpFmtString <= '9'))
                {
                    ++pi1TmpFmtString;
                }
                if((*pi1TmpFmtString == 'd') || (*pi1TmpFmtString == 'x') || (*pi1TmpFmtString == 'X') ||(*pi1TmpFmtString == 'u'))
                {
                    u4VarArgLength += u4MaxIntLength;
                }
                else if(*pi1TmpFmtString == 'c')
                {
                    ++u4VarArgLength;
                }
                else if(*pi1TmpFmtString == 's')
                {
                    if(pAddr != 0)
                    {
                        pi1TmpArgString = pAddr;
                        while(*pi1TmpArgString != '\0')
                        {
                            ++u4VarArgLength;
                            ++pi1TmpArgString;
                        }
                    }
                }
            }
        }
        formatString++;
    }
    *pi4ArgCount = u4ArgCount;
    *pi4VarArgLength = u4VarArgLength;
    return;
}

#ifdef REMOTE_LOG
INT32 base_trace_remote_log(CHAR *pu1Msg )
{
	struct sockaddr_in sLogServer;
	INT4 i4err=0;

        /* kw.2440, the actual time buffer size should be 21,
         * But I changed this to 64 to satisify Klocwork.
         */
	UINT1 u1TimeBuf[64];

	/* gu4SysLogServerIp = inet_addr("172.30.15.171");	 */
	sLogServer.sin_family = AF_INET;
	sLogServer.sin_addr.s_addr = htonl(gu4SysLogServerIp);
	sLogServer.sin_port = SYSLOG_PORT;

	memset(&gu1LogBuffer,0,sizeof(gu1LogBuffer));
	memset(&u1TimeBuf,0,sizeof(u1TimeBuf));

	UtlGetTimeStr((CHR1 *)&u1TimeBuf);

	sprintf(gu1LogBuffer,"<%d> %s %s %s",191,SYSLOG_MODNAME,u1TimeBuf,pu1Msg);
	i4err = sendto ( gi4SyslogFd, gu1LogBuffer, strlen(gu1LogBuffer), 0,
						  (struct sockaddr *)&sLogServer, sizeof(struct sockaddr_in));

	if ( i4err <= 0 )
	{
		printf("SysLogSend: sendto failed \n");
		return BASE_E_FAIL;
	}
	return BASE_E_NONE;
}
#endif


INT3 base_trace_local_log_retire(UINT1* u1logpath, UINT1* u1logname)
{
    UINT1 au1tmpsrt[128] = {0};

    sprintf(au1tmpsrt, "%s/%s.1", u1logpath, u1logname);
    unlink(au1tmpsrt);

    sprintf(au1tmpsrt, "mv %s/%s %s/%s.1", u1logpath, u1logname, u1logpath, u1logname);
    vsystem(au1tmpsrt);

    return BASE_E_NONE;
}

INT32 base_trace_local_log(CHAR* logstr, CHAR* logpath, CHAR* logname)
{
    INT32    i4retval = 0;
    CHAR   filename[256] = {0};
    FILE *logfp = NULL;

    sprintf(filename, "%s/%s", logpath, logname);

    logfp = fopen(filename,"at+");/*file open*/
    if (logfp == NULL)
    {
        fprintf(stderr, "fopen() failed:%s!\n", strerror(errno));
        return BASE_E_FAIL;
    }

    printf("Open the file fd = %d\n", fileno(logfp));

    fseek(logfp, 0, SEEK_END);

    if (ftell(logfp) > SYS_LOG_MAX_LENGTH)
    {
        fclose(logfp);
        base_trace_local_log_retire(logpath, logname);/*backup file*/
        logfp = fopen(filename,"at+");
    }

    if (! logfp)
    {
        return BASE_E_FAIL;
    }

    if (! logstr)
    {
        return BASE_E_FAIL;
    }
    
    ret = flock(fileno(logfp), LOCK_EX);/*file lock*/
    if (ret != 0)
    {
        fprintf(stderr, "flock() failed:%s!\n", strerror(errno));
        return BASE_E_FAIL;
    }

    i4retval = fprintf((FILE *)logfp, "%s", logstr); /*write file*/

    if(! i4retval)
    {
        perror("fprintf");
    }
    
    flock(fileno(logfp), LOCK_UN);/*file unlock*/

    fclose(logfp);/*file close*/
   
    return 0;    
}

void base_trace_stdio_print(const UINT32 module, const UINT32 level, const CHAR *format, ...)
{
	va_list 	VarArgList;
	va_list 	VarArgListA;
	UINT32	u4ArgCount;
	UINT32	u4VarArgLength = 0;
	UINT32	u4NameLength = 0;
	UINT32	u4Pos = 0;
	UINT32	u4FmtLength = 0;
	CHAR		ai1MsgBuf[BASE_TRACE_STR_LEN_MAX];
	UINT32 	u4Print = 0;
	
	if(gbasetaceenable != 1)
	{
		printf("Trace is disabled.\r\n");
		return;
	}
	
	if (level == BASE_TRACE_LEVEL_MASK_ERR)
	{
		sprintf(gbasetracebuffer,"Module[0x%x]Level[0x%x]",mod,level);
		u4Print = 1;
	}
	else if( ((mod) & gbasetracemodule) && ((level) & gbasetracelevel))
	{
		sprintf(gbasetracebuffer,"Module[0x%x]Level[0x%x]",mod,level);
		u4Print = 1;
	} 

	if(u4Print == 1)
	{
		va_start (VarArgList, format);
		va_start (VarArgListA, format);

		base_trace_getLength(format, VarArgList, &u4ArgCount, &u4VarArgLength);

		u4NameLength = strlen(gbasetracebuffer);

		if(u4NameLength >= BASE_TRACE_MODULE_NAME_LEN_MAX)
		{
			printf ("Length of trace name is error[%d].\n", u4NameLength);
			return;
		}
		if (strcmp (gbasetracebuffer, ""))
		{
			u4Pos += sprintf (ai1MsgBuf + u4Pos, "%s: ", gbasetracebuffer);
		}
		u4NameLength = u4Pos;

		u4FmtLength = strlen(format);
		u4VarArgLength += u4NameLength;
		u4VarArgLength += u4FmtLength;

		if(u4VarArgLength >= BASE_TRACE_STR_LEN_MAX)
		{
			printf("Length of arguments is error[%d].\n",u4VarArgLength);
			return;
		}
		vsprintf (ai1MsgBuf + u4Pos, format, VarArgListA);
		printf("%s", ai1MsgBuf);

		va_end (VarArgList);
	}
}

void base_trace_setModule(const UINT32 mod, const BOOL Enable)
{
	if(Enable == TRUE)
	{
		gbasetracemodule |= mod;
	}
	else if (Enable == FALSE)
	{
		gbasetracemodule &= (~mod);
	}
	else
	{
		printf("param(mod:0x%x,Enable:%d) are error.\r\n",u4Mod, Enable);
	}
}

void base_trace_setLevel(const UINT32 level, const BOOL Enable)
{
	if(Enable == TRUE)
	{
		gbasetracelevel |= level;
	}
	else if (Enable == FALSE)
	{
		gbasetracelevel &= (~level);
	}
	else
	{
		printf("param(level:0x%x,Enable:%d) are error.\r\n",u4Level, Enable);
	}
}

void base_trace_set(const UINT32 mod, const UINT32 level)
{
    if( mod > TRACE_M_ALL  || level > TRACE_L_ALL)
    { 
        printf("Invalid parameter: mod=%d, level=%d. \n", mod, level);
        return BASE_E_PARAM;
    }    
    
	if(TRACE_M_ALL == mod)
	{
		gbasetracemodule |= BASE_TRACE_MODULE_MASK_ALL;
	}
    	else if(TRACE_M_CLEAR == mod)
	{
		gbasetracemodule &= BASE_TRACE_MODULE_MASK_CLEAR;
	}

	else 
	{
		gbasetracemodule |= mod;
	}
    
	if(TRACE_L_ALL == level)
	{
		gbasetracelevel |= BASE_TRACE_LEVEL_MASK_ALL;
	}
    	else if(TRACE_L_CLEAR == level)
	{
		gbasetracelevel &= BASE_TRACE_LEVEL_MASK_CLEAR;
	}

	else 
	{
		gbasetracelevel |= level;
	}

}


void base_trace_show(void)
{
    int mod=0, level;
    int i=0;
    printf("ALL Basic Trace Module:");
    mod = BASE_TRACE_MODULE_MASK_ALL;
    i = 0;
    while (mod)
    {
        if ((mod&1) == 1)  
        {
            if(i > TRACE_M_CLEAR && i < TRACE_M_ALL)
                printf("%s ", basetracemodulenamemapping[i].name);
        }
        i++;
        mod = mod >> 1;  
    }
    printf("ALL Basic Trace Level:");    
    level = BASE_TRACE_LEVEL_MASK_ALL;
    i = 0;
    while (level)
    {
        if ((level&1) == 1)  
        {
            if(i > TRACE_L_CLEAR && i < TRACE_L_ALL)
                printf("%s\ ", basetracelevelnamemapping[i].name);
        }
        i++;
        level = level >> 1;  
    }

    
    printf("Enable Basic Trace Module:");
    mod = gbasetracemodule;
    i = 0;
    while (mod)
    {
        if ((mod&1) == 1)  
        {
            if(i > TRACE_M_CLEAR && i < TRACE_M_ALL)
                printf("%s\r\n", basetracemodulenamemapping[i].name);
        }
        i++;
        mod = mod >> 1;  
    }
    printf("Enable Basic Trace Level:");    
    level = gbasetracemodule;
    i = 0;
    while (level)
    {
        if ((level&1) == 1)  
        {
            if(i > TRACE_L_CLEAR && i < TRACE_L_ALL)
                printf("%s\r\n", basetracelevelnamemapping[i].name);
        }
        i++;
        level = level >> 1;  
    }
    
}

void base_trace_print(const UINT32 mod, const UINT32 level, const CHAR *format, ...)
{
    va_list 	VarArgList;
    va_list 	VarArgListA;
    UINT32	u4ArgCount;
    UINT32	u4VarArgLength = 0;
    UINT32	u4NameLength = 0;
    UINT32	u4Pos = 0;
    UINT32	u4FmtLength = 0;
    CHAR		ai1MsgBuf[BASE_TRACE_STR_LEN_MAX];
    UINT32 	u4Print = 0;
    time_t              t;
    struct tm         *tm = 0;
   
    if( mod > (TRACE_M_ALL -1) || level > (TRACE_L_ALL-1))
    { 
        printf("Invalid parameter: mod=%d, level=%d. \n", mod, level);
        return BASE_E_PARAM;
    }    
        
    if(gbasetaceenable != 1)
    {
        printf("Trace is disabled.\r\n");
        return;
    }
	
    if (level == TRACE_L_ERR || ( ((mod) & gbasetracemodule) && ((level) & gbasetracelevel)))
    {
        if (gbasetaceshowtime)
        {
            t = time (NULL);
            tm = localtime (&t);

            sprintf (gbasetracebuffer, "[%d:%d:%d: %d:%d:%d] - [%s] - [%s]",
                        tm->tm_hour, tm->tm_min, tm->tm_sec,
                        tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900,
                        basetracemodulenamemapping[mod].name, basetracelevelnamemapping[level].name
                        );
        }	
        else
        {
            sprintf(gbasetracebuffer,"[%s] - [%s]", basetracemodulenamemapping[mod].name, basetracelevelnamemapping[level].name);
        }
    	u4Print = 1;
    }   

    if(u4Print == 1)
    {
        va_start (VarArgList, format);
        va_start (VarArgListA, format);

        base_trace_getLength(format, VarArgList, &u4ArgCount, &u4VarArgLength);
        va_end (VarArgList);

        u4NameLength = strlen(gbasetracebuffer);

        if(u4NameLength >= BASE_TRACE_MODULE_NAME_LEN_MAX)
        {
        	printf ("Length of trace name is error[%d].\n", u4NameLength);
        	return;
        }
        if (strcmp (gbasetracebuffer, ""))
        {
        	u4Pos += sprintf (ai1MsgBuf + u4Pos, "%s: ", gbasetracebuffer);
        }
        u4NameLength = u4Pos;

        u4FmtLength = strlen(format);
        u4VarArgLength += u4NameLength;
        u4VarArgLength += u4FmtLength;

        if(u4VarArgLength >= BASE_TRACE_STR_LEN_MAX)
        {
        	printf("Length of arguments is error[%d].\n",u4VarArgLength);
        	return;
        }
        vsprintf (ai1MsgBuf + u4Pos, format, VarArgListA);
        printf("%s", ai1MsgBuf);

        if(gbasetaceocallog)
        {
            base_trace_local_log(ai1MsgBuf, BACE_TRACE_LOCAL_LOG_PATH, BACE_TRACE_LOCAL_LOG_NAME);
        }
    }
}

