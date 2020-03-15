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
#include <base_types.h>

/* bitmask for trace module are 32 bits: bit 1 to bit 32 */
#define BASE_TRACE_MODULE_SYS	(1 << 0)    	/* system module		*/

/* bitmask for trace level are 32 bits: bit 1 to bit 32 */
#define BASE_TRACE_LEVEL_DBG 	(1 << 0)		/* level debug			*/
#define BASE_TRACE_LEVEL_ERR	(1 << 1)		/* level errors			*/
#define BASE_TRACE_LEVEL_WRN	(1 << 2)		/* level warning		*/
#define BASE_TRACE_LEVEL_INF	(1 << 3)		/* level informations	*/

#ifdef __cplusplus
extern "C" {
#endif

/* trace api. */
extern VOID base_trace_print(const UINT32 module, const UINT32 level, const CHAR *format, ...);
extern VOID base_trace_module_set(const UINT32 module, const BOOL status);
extern VOID base_trace_level_set(const UINT32 level, const BOOL status);
extern VOID base_trace_show(VOID);


#ifdef __cplusplus
} /* extern C */
#endif

#endif /* BASE_TRACE_H_ */
