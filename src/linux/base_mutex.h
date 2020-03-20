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


#ifndef BASE_MEM_H
#define BASE_MEM_H
#include <stdlib.h>
#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _MEM_TYPES_
#define _MEM_TYPES_

#define MEM_MOD_NAME            TRACE_M_OS
#define MEM_ERR(s...)       base_trace_print(MEM_MOD_NAME, BASE_TRACE_LEVEL_MASK_ERR, s)
#define MEM_WRN(s...)      base_trace_print(MEM_MOD_NAME, BASE_TRACE_LEVEL_MASK_WRN, s)
#define MEM_INF(s...)       base_trace_print(MEM_MOD_NAME, BASE_TRACE_LEVEL_MASK_INF, s)

#define BASE_MEM
#ifdef BASE_MEM

#define MEM_MALLOC(s) base_mem_malloc(s)
#define MEM_FREE(p) base_mem_free(p)
#define MEM_CPY(d,s,n) base_mem_cpy((void *)(d), (const void *)(s), (n))
#define MEM_CMP(s1,s2,n) memcmp((const void *)(s1), (const void *)(s2), (n))
#define MEM_SET(s,c,n) base_mem_set((void *)(s), (INT32)(c), (n))

#else

#define MEM_MALLOC(s) malloc(s)
#define MEM_FREE(p) free(p)
#define MEM_CPY(d,s,n) memcpy((void *)(d), (const void *)(s), (n))
#define MEM_CMP(s1,s2,n) memcmp((const void *)(s1), (const void *)(s2), (n))
#define MEM_SET(s,c,n) memset((void *)(s), (INT32)(c), (n))

#endif

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* BASE_MEM_H */
