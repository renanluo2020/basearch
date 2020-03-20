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

typedef struct BASE_MEM_LINK
{
	struct BASE_MEM_LINK *next;		/*<< The next free memory in the list */
	UINT32 size;						/*<< The size of the free memory */
} base_mem_link_t;

#define BASE_MEM_TOTAL_MEM_SIZE		(( UINT32 )128*1024*1024)	/*total 128 Mbytes memory */

#define BASE_MEM_MIN_MEM_SIZE		(( UINT32 )128)			/*minimum 128 bytes memory */

#define BASE_MEM_MIN_MEM_SIZE_MASK	(( UINT32 )0x007FU)		/*minimum 128 bytes memory mask*/

#define BASE_MEM_BYTE_ALIGNMENT		(( UINT32 )8)				/* 8 bytes alignment */

#define BASE_MEM_BYTE_ALIGNMENT_MASK (( UINT32 )0x0007U )		/* bytes alignment mask */

/*memory malloc: input parameter memory size you want to allocate */
extern VOID *base_mem_malloc( UINT32 size );

/*memory free: input parameter memory address you want to free */
extern VOID base_mem_free( VOID *memaddr );

/*memory set: input parameter destination address, value and size */
extern VOID* base_mem_set(VOID *dest, INT32 value, UINT32 size);

/*memory copy: input parameter destination address, source address and size */
extern VOID *base_mem_cpy(VOID *dest, const VOID *src, UINT32 size);

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* BASE_MEM_H */
