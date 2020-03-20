
#ifndef BASE_ERROR_CODE_H
#define BASE_ERROR_CODE_H



typedef enum {
    BASE_E_NONE                  = 0,
    BASE_E_FAIL                   = -1,
    BASE_E_PARAM                = -2,
    BASE_E_EMPTY                = -3,
    BASE_E_FULL                   = -4,
    BASE_E_NOT_FOUND       = -5,
    BASE_E_EXISTS               = -6,
    BASE_E_TIMEOUT            = -7,
    BASE_E_BUSY                  = -8,
    
    BASE_E_MAX                   = -9         
} base_error_t;

#endif 

