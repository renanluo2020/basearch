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


#ifndef BASE_TRACE_H_
#define BASE_TRACE_H_
#include <types.h>

typedef enum {
    TRACE_M_CLEAR           = 0,
    TRACE_M_OS                 = 1,
    TRACE_M_CLI                = 2,
    TRACE_M_TIMER           = 3,
    TRACE_M_ALL              /*ALL is max*/
} trace_module_t;

typedef enum {
    TRACE_L_CLEAR             = 0, /*clear*/    
    TRACE_L_ERR                 = 1, /*always print*/
    TRACE_L_WRN                = 2,
    TRACE_L_INF                 = 3,
    TRACE_L_ALL              /*ALL is max*/
} trace_level_t;

/* bitmask for trace module are 32 bits: bit 1 to bit 32 */
#define BASE_TRACE_MODULE_MASK_CLEAR   (0x00000000)		
#define BASE_TRACE_MODULE_MASK_OS	      (0x00000001)		
#define BASE_TRACE_MODULE_MASK_CLI	      (0x00000002)		
#define BASE_TRACE_MODULE_MASK_TIMER   (0x00000004)		
#define BASE_TRACE_MODULE_MASK_ALL	      (0x0000000f)		

/* bitmask for trace level are 32 bits: bit 1 to bit 32 */
#define BASE_TRACE_LEVEL_MASK_CLEAR      (0x00000000)		
#define BASE_TRACE_LEVEL_MASK_ERR	      (0x00000001)		
#define BASE_TRACE_LEVEL_MASK_WRN	      (0x00000002)		
#define BASE_TRACE_LEVEL_MASK_INF	      (0x00000004)		
#define BASE_TRACE_LEVEL_MASK_ALL	      (0x0000000f)		

#define BSCE_TRACE_MAXLEN_NAME	128

typedef struct _Bace_Trace_Module_Name_Mapping
{
	trace_module_t	mod_id;
	CHAR	              name[BSCE_TRACE_MAXLEN_NAME];
	UINT32	              bitvalue;   
} tbasetracemodulenamemapping;

typedef struct _Bace_Trace_Level_Name_Mapping
{
	trace_level_t         level_id;
	CHAR	              name[BSCE_TRACE_MAXLEN_NAME];
	UINT32	              bitvalue;   
} tbasetracelevelnamemapping;

tbasetracemodulenamemapping	basetracemodulenamemapping[]=
{
	{TRACE_M_CLEAR,   "clear",	    BASE_TRACE_MODULE_MASK_CLEAR},
	{TRACE_M_OS,        "os",	    BASE_TRACE_MODULE_MASK_OS},
	{TRACE_M_CLI,       "cli",	    BASE_TRACE_MODULE_MASK_CLI},
	{TRACE_M_TIMER,   "timer",	    BASE_TRACE_MODULE_MASK_TIMER},
	{TRACE_M_ALL,        "all",	    BASE_TRACE_MODULE_MASK_ALL},	
};

tbasetracelevelnamemapping	basetracelevelnamemapping[]=
{
    {TRACE_L_CLEAR,       "clear",        BASE_TRACE_LEVEL_MASK_CLEAR},
    {TRACE_L_ERR,           "err",        BASE_TRACE_LEVEL_MASK_ERR},
    {TRACE_L_WRN,          "warn",      BASE_TRACE_LEVEL_MASK_WRN},
    {TRACE_L_INF,            "info",        BASE_TRACE_LEVEL_MASK_INF},
    {TRACE_L_ALL,            "all",        BASE_TRACE_LEVEL_MASK_ALL},    
};

#define BACE_TRACE_LOG_MGT_SEM_NAME	            ((const UINT8 *)"tracelog")
#define BACE_TRACE_LOG_SEM_INITCOUNT              1

#define BACE_TRACE_LOCAL_LOG_NAME "trace.log"
#define BACE_TRACE_LOCAL_LOG_PATH "/opt/config/log"

#ifdef __cplusplus
extern "C" {
#endif

/* trace api. */
extern VOID base_trace_print(const UINT32 module, const UINT32 level, const CHAR *format, ...);
extern VOID base_trace_set(const UINT32 mod, const UINT32 level);
extern VOID base_trace_show(VOID);


#ifdef __cplusplus
} /* extern C */
#endif

#endif /* BASE_TRACE_H_ */
