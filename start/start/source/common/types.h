#ifndef TYPES_H
#define  TYPES_H

// 为了避免命名冲突，加上_UINT8_T_DECLARED的配置
#ifndef _UINT8_T_DECLARED
#define _UINT8_T_DECLARED
typedef unsigned char uint8_t ; 
#endif
 
#ifndef _UINT16_T_DECLARED
typedef unsigned short uint16_t ;
#define _UINT16_T_DECLARED 
#endif

#ifndef _UINT32_T_DECLARED
typedef unsigned long uint32_t ; 
#endif
 

#endif