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


#ifndef BASE_TYPES_H
#define BASE_TYPES_H

#include <inttypes.h>
#ifndef _ARM64_TYPES_
#define _ARM64_TYPES_

typedef void VOID;

typedef int BOOL;

typedef unsigned char UCHAR;

typedef char CHAR;

typedef unsigned char UINT8;

typedef char INT8;

typedef unsigned short UINT16;

typedef short INT16;

typedef unsigned int UINT32;

typedef int INT32;

typedef unsigned long UINT64;

typedef long UINT64;

#define TRUE ((BOOL) 1)

#define FALSE ((BOOL) 0) 

#endif

#endif 
