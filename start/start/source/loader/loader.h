#ifndef LOADER_H 
#define  LOADER_H

#include "common/boot_info.h"
#include "common/types.h"
#include "common/cpu_instr.h"


typedef struct SMAP_entry{
    uint32_t BaseL; // base address uint64_t
    uint32_t BaseH;
    uint32_t LengthL; // length uint64_t
    uint32_t LengthH;
    uint32_t Type; // entry Type
    uint32_t ACPI; // extended
} __attribute__((packed)) SMAP_entry_t ;


#endif 