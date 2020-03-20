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


#ifndef BASE_EVENT_H
#define BASE_EVENT_H
#include <stdlib.h>
#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _EVENT_TYPES_
#define _EVENT_TYPES_

#define EVENT_MOD_NAME            TRACE_M_OS
#define EVENT_ERR(s...)       base_trace_print(MEM_MOD_NAME, BASE_TRACE_LEVEL_MASK_ERR, s)
#define EVENT_WRN(s...)      base_trace_print(MEM_MOD_NAME, BASE_TRACE_LEVEL_MASK_WRN, s)
#define EVENT_INF(s...)       base_trace_print(MEM_MOD_NAME, BASE_TRACE_LEVEL_MASK_INF, s)

#undef   OSIX_WAIT
#undef   OSIX_NO_WAIT
#define  OSIX_WAIT               0
#define  OSIX_NO_WAIT            2

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* BASE_EVENT_H */
