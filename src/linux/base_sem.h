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


#ifndef BASE_SEM_H
#define BASE_SEM_H
#include <stdlib.h>
#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SEM_TYPES_
#define _SEM_TYPES_

#define SEM_MOD_NAME            TRACE_M_OS
#define SEM_ERR(s...)       base_trace_print(MEM_MOD_NAME, BASE_TRACE_LEVEL_MASK_ERR, s)
#define SEM_WRN(s...)      base_trace_print(MEM_MOD_NAME, BASE_TRACE_LEVEL_MASK_WRN, s)
#define SEM_INF(s...)       base_trace_print(MEM_MOD_NAME, BASE_TRACE_LEVEL_MASK_INF, s)

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* BASE_SEM_H */
