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
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <semaphore.h>
#include <types.h>
#include <trace.h>
#include <mem.h>



static char gbasemempool[ BASE_MEM_TOTAL_MEM_SIZE ];

static UINT32 gbasememfreebytesremaining = 0U;

static UINT32 gbasememminfreebytesremaining = 0U;

static UINT32 gbasememallocatedbit = 0;

static base_mem_link_t gbasememstart, *gbasememend = NULL;

static const UINT32 gbasememstructsize	= ( sizeof( base_mem_link_t ) + ( ( UINT32 ) ( BASE_MEM_BYTE_ALIGNMENT - 1 ) ) ) & ~( ( UINT32 ) BASE_MEM_BYTE_ALIGNMENT_MASK );

static sem_t gfwkmemmgmtSemId;

UINT32 base_mem_freesize_get( void )
{
	return gbasememfreebytesremaining;
}

UINT32 base_mem_minusedsize_get( void )
{
	return gbasememminfreebytesremaining;
}

static void base_mem_insert( base_mem_link_t *pMemToInsert )
{
	base_mem_link_t *pxIterator;
	char *puc;

	/* Iterate through the list until a block is found that has a higher address
	than the block being inserted. */
	for( pxIterator = &gbasememstart; pxIterator->next < pMemToInsert; pxIterator = pxIterator->next )
	{
		/* Nothing to do here, just iterate to the right position. */
	}

	/* Do the block being inserted, and the block it is being inserted after
	make a contiguous block of memory? */
	puc = ( char * ) pxIterator;
	if( ( puc + pxIterator->size ) == ( char * ) pMemToInsert )
	{
		pxIterator->size += pMemToInsert->size;
		pMemToInsert = pxIterator;
	}


	/* Do the block being inserted, and the block it is being inserted before
	make a contiguous block of memory? */
	puc = ( char * ) pMemToInsert;
	if( ( puc + pMemToInsert->size ) == ( char * ) pxIterator->next )
	{
		if( pxIterator->next != gbasememend )
		{
			/* Form one big block from the two blocks. */
			pMemToInsert->size += pxIterator->next->size;
			pMemToInsert->next = pxIterator->next->next;
		}
		else
		{
			pMemToInsert->next = gbasememend;
		}
	}
	else
	{
		pMemToInsert->next = pxIterator->next;
	}

	/* If the block being inserted plugged a gab, so was merged with the block
	before and the block after, then it's next pointer will have
	already been set, and should not be set here as that would make it point
	to itself. */
	if( pxIterator != pMemToInsert )
	{
		pxIterator->next = pMemToInsert;
	}
}


static void base_mem_init( void )
{
	int32_t i4res;
	fwk_addr_t ui4Address;
	char *pi1AlignedMem;
	UINT32 ui4MemTotalSize = BASE_MEM_TOTAL_MEM_SIZE;
	base_mem_link_t *pFirstFreeMem;
	
	MEM_INF("F:%s memory init start.\n",__func__);

	i4res = sem_init(&gfwkmemmgmtSemId, 0, 1);
	if (i4res != 0)
	{	  
		MEM_ERR("Semaphore gfwkmemmgmtSemId initialization failed.\n");
		return;
	}

	ui4Address = (fwk_addr_t) gbasemempool;
	if( ( ui4Address & BASE_MEM_BYTE_ALIGNMENT_MASK ) != 0 )
	{
		ui4Address += ( BASE_MEM_BYTE_ALIGNMENT - 1 );
		ui4Address &= ~( (fwk_addr_t) BASE_MEM_BYTE_ALIGNMENT_MASK );
		ui4MemTotalSize -= ui4Address - (fwk_addr_t) gbasemempool;
	}
	pi1AlignedMem = ( char * ) ui4Address;
	
	MEM_INF("ui4Address:0x%x,total size:0x%x.\n",ui4Address,ui4MemTotalSize);

	gbasememstart.next = ( void * ) pi1AlignedMem;
	gbasememstart.size = ( UINT32 ) 0;

	ui4Address = ( (fwk_addr_t) pi1AlignedMem ) + ui4MemTotalSize;
	ui4Address -= gbasememstructsize;
	ui4Address &= ~( (fwk_addr_t) BASE_MEM_BYTE_ALIGNMENT_MASK );
	gbasememend = ( void * ) ui4Address;
	gbasememend->size = 0;
	gbasememend->next = NULL;

	MEM_INF("gbasememstart next:0x%x,gbasememend:0x%x.\n",gbasememstart.next,gbasememend);

	pFirstFreeMem = ( void * ) pi1AlignedMem;
	pFirstFreeMem->size = ui4Address - (fwk_addr_t) pFirstFreeMem;
	pFirstFreeMem->next = gbasememend;

	gbasememminfreebytesremaining = pFirstFreeMem->size;
	gbasememfreebytesremaining = pFirstFreeMem->size;

	/* Work out the position of the top bit in a u32_t variable. */
	gbasememallocatedbit = ( ( UINT32 ) 1 ) << ( ( sizeof( UINT32 ) * 8 ) - 1 );

	MEM_INF("gbasememallocatedbit:0x%x,gbasememfreebytesremaining:0x%x.\n",gbasememallocatedbit,gbasememfreebytesremaining);
}



void *base_mem_malloc( UINT32 u4Size )
{
	base_mem_link_t *pMem, *pPreviousMem, *pNewMem;
	void *pvReturn = NULL;

	MEM_INF("F:%s size:0x%x.\n",__func__,u4Size);

	if( gbasememend == NULL )
	{
		base_mem_init();
	}

	sem_wait(&gfwkmemmgmtSemId);	
	{
		
		MEM_INF("size:0x%x,gbasememallocatedbit:0x%x.\n",u4Size,gbasememallocatedbit);
		if( ( u4Size & gbasememallocatedbit ) == 0 )
		{
			if( u4Size > 0 )
			{
				u4Size += gbasememstructsize;
				/* minimum alloc and need to alignment */
				if( ( u4Size & BASE_MEM_MIN_MEM_SIZE_MASK ) != 0x00 )
				{
					u4Size += ( BASE_MEM_MIN_MEM_SIZE - ( u4Size & BASE_MEM_MIN_MEM_SIZE_MASK ) );
					if( ( u4Size & BASE_MEM_MIN_MEM_SIZE_MASK ) != 0 )
					{
						MEM_ERR("F:%s size(0x%x) error.\n",__func__,u4Size);
						return NULL;
					}	
				}
			}

			if( ( u4Size > 0 ) && ( u4Size <= gbasememfreebytesremaining ) )
			{
				pPreviousMem = &gbasememstart;
				pMem = gbasememstart.next;
				while( ( pMem->size < u4Size ) && ( pMem->next != NULL ) )
				{
					pPreviousMem = pMem;
					pMem = pMem->next;
				}

				if( pMem != gbasememend )
				{
					pvReturn = ( void * ) ( ( ( char * ) pPreviousMem->next ) + gbasememstructsize );
					pPreviousMem->next = pMem->next;
					if( ( pMem->size - u4Size ) > BASE_MEM_MIN_MEM_SIZE )
					{
						pNewMem = ( void * ) ( ( ( char * ) pMem ) + u4Size );
						pNewMem->size = pMem->size - u4Size;
						pMem->size = u4Size;

						/* Insert the new memory into the list of free memory. */
						base_mem_insert( pNewMem );
					}

					gbasememfreebytesremaining -= pMem->size;

					if( gbasememfreebytesremaining < gbasememminfreebytesremaining )
					{
						gbasememminfreebytesremaining = gbasememfreebytesremaining;
					}

					pMem->size |= gbasememallocatedbit;
					pMem->next = NULL;
				}

			}

		}
	}
	sem_post(&gfwkmemmgmtSemId);

	return pvReturn;
}
 
void base_mem_free( void *pMemaddr )
{
	char *pi1MemAddr = ( char * ) pMemaddr;
	base_mem_link_t *pMem;

	if( pMemaddr != NULL )
	{
		pi1MemAddr -= gbasememstructsize;
		pMem = ( void * ) pi1MemAddr;

		if( ( pMem->size & gbasememallocatedbit ) == 0 )
		{
			MEM_ERR("F:%s memory has free:0x%x.\n",__func__,pMemaddr);
			return;
		}
		if( pMem->next != NULL )
		{
			MEM_ERR("F:%s next is NULL:0x%x.\n",__func__,pMemaddr);
			return;
		}

		if( ( pMem->size & gbasememallocatedbit ) != 0 )
		{
			if( pMem->next == NULL )
			{
				pMem->size &= ~gbasememallocatedbit;
				sem_wait(&gfwkmemmgmtSemId);
				{
					gbasememfreebytesremaining += pMem->size;
					base_mem_insert( ( ( base_mem_link_t * ) pMem ) );
				}
				sem_post(&gfwkmemmgmtSemId);
			}
			return;
		}
	}
	MEM_ERR("F:%s pMemaddr is NULL.\n",__func__);
	return;
}

void* base_mem_set(void *pDest, int32_t i4Value, UINT32 u4Size)
{
	if(pDest == NULL || u4Size <= 0) 
	{
		MEM_ERR("F:%s param is error.\n",__func__);
		return NULL;
	}
    char* pDestStart = (char*)pDest;
    while(u4Size--)
        *pDestStart++ = i4Value;
    return pDestStart;
}

void *base_mem_cpy(void *pDest, const void *pSrc, UINT32 u4Size)
{  
	if(pDest == NULL || pSrc == NULL || u4Size <= 0) 
	{
		MEM_ERR("F:%s param is error.\n",__func__);
		return NULL;
	}
	char *pSrcTemp = (char *)pSrc;
	char *pDstTemp = (char *)pDest;  
	while(u4Size-->0)
		*pDstTemp++ = *pSrcTemp++;  
	return pDest;
} 

