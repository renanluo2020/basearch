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
#include <base_types.h>
#include <base_trace.h>

#define BASE_TRACE_STR_LEN_MAX			256	/* trace string max lenght		*/
#define BASE_TRACE_MODULE_NAME_LEN_MAX	50	/* trace module name max lenght	*/

/* trace level all and trace module all */
#define BASE_TRACE_LEVEL_ALL	0xFFFFFFFF
#define BASE_TRACE_MODULE_ALL	0xFFFFFFFF

/* trace module and level */
static UINT32 gu4basetraceModule	= BASE_TRACE_MODULE_FWK;
static UINT32 gu4basetraceLevel 	= (BASE_TRACE_LEVEL_DBG | BASE_TRACE_LEVEL_ERR);
#define BASE_TRACE_MODLUE	gu4basetraceModule
#define FWK_BAISCTRACE_LEVEL	gu4basetraceLevel

/* trace global enable/disable */
static UINT32 gu4basetaceEnable = 1;
static CHAR gai1basetraceBuffer[BASE_TRACE_MODULE_NAME_LEN_MAX] = {0};


void base_trace_getLength (const CHAR *pi1Fmt, va_list VarArgList, UINT32 *pi4ArgCount, UINT32 *pi4VarArgLength)
{
    const CHAR		*pi1FmtString;
    const CHAR		*pi1TmpFmtString;
    const CHAR		*pi1TmpArgString;
    const UINT32		u4MaxIntLength = 19;
    UINT32	u4ArgCount = 0;
    CHAR*	pAddr;
    UINT32	u4VarArgLength 	= 0;

    pi1FmtString = pi1Fmt;
    while (*pi1FmtString)
    {
        if (*pi1FmtString == '%')
        {
            pi1FmtString++;
            if (*pi1FmtString != '%')
            {
                pAddr = (CHAR*)va_arg (VarArgList, fwk_addr_t);
                u4ArgCount++;
                pi1TmpFmtString = pi1FmtString;
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
        pi1FmtString++;
    }
    *pi4ArgCount = u4ArgCount;
    *pi4VarArgLength = u4VarArgLength;
    return;
}

void base_trace_print(const UINT32 module, const UINT32 level, const CHAR *format, ...)
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
	
	if(gu4basetaceEnable != 1)
	{
		printf("Trace is disabled.\r\n");
		return;
	}
	
	if (u4Level == BASE_TRACE_LEVEL_ERR)
	{
		sprintf(gai1basetraceBuffer,"Module[0x%x]Level[0x%x]",u4Mod,u4Level);
		u4Print = 1;
	}
	else if( ((u4Mod) & gu4basetraceModule) && ((u4Level) & gu4basetraceLevel))
	{
		sprintf(gai1basetraceBuffer,"Module[0x%x]Level[0x%x]",u4Mod,u4Level);
		u4Print = 1;
	} 

	if(u4Print == 1)
	{
		va_start (VarArgList, pi1Fmt);
		va_start (VarArgListA, pi1Fmt);

		base_trace_getLength(pi1Fmt, VarArgList, &u4ArgCount, &u4VarArgLength);

		u4NameLength = strlen(gai1basetraceBuffer);

		if(u4NameLength >= BASE_TRACE_MODULE_NAME_LEN_MAX)
		{
			printf ("Length of trace name is error[%d].\n", u4NameLength);
			return;
		}
		if (strcmp (gai1basetraceBuffer, ""))
		{
			u4Pos += sprintf (ai1MsgBuf + u4Pos, "%s: ", gai1basetraceBuffer);
		}
		u4NameLength = u4Pos;

		u4FmtLength = strlen(pi1Fmt);
		u4VarArgLength += u4NameLength;
		u4VarArgLength += u4FmtLength;

		if(u4VarArgLength >= BASE_TRACE_STR_LEN_MAX)
		{
			printf("Length of arguments is error[%d].\n",u4VarArgLength);
			return;
		}
		vsprintf (ai1MsgBuf + u4Pos, pi1Fmt, VarArgListA);
		printf("%s", ai1MsgBuf);

		va_end (VarArgList);
	}
}

void base_trace_setModule(const UINT32 u4Mod, const BOOL Enable)
{
	if(Enable == TRUE)
	{
		gu4basetraceModule |= u4Mod;
	}
	else if (Enable == FALSE)
	{
		gu4basetraceModule &= (~u4Mod);
	}
	else
	{
		printf("param(u4Mod:0x%x,Enable:%d) are error.\r\n",u4Mod, Enable);
	}
}

void base_trace_setLevel(const UINT32 u4Level, const BOOL Enable)
{
	if(Enable == TRUE)
	{
		gu4basetraceLevel |= u4Level;
	}
	else if (Enable == FALSE)
	{
		gu4basetraceLevel &= (~u4Level);
	}
	else
	{
		printf("param(u4Level:0x%x,Enable:%d) are error.\r\n",u4Level, Enable);
	}
}

void base_trace_show(void)
{
	printf("Enabled Basic Trace Module are:0x%x\r\n",gu4basetraceModule);
	printf("Enabled Basic Trace Level are:0x%x\r\n",gu4basetraceLevel);
}

