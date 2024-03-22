#ifndef ELF_H 
#define ELF_H 
#include "types.h"

typedef uint32_t Elf32_Addr ;
typedef uint16_t Elf32_Half ; 
typedef uint32_t Elf32_Off ;
typedef uint32_t Elf32_Word ; 

#define EI_NIDENT (16)

#pragma pack(1) 

typedef struct
{
    unsigned char e_ident[EI_NIDENT];     /* Magic number and other info */
    Elf32_Half    e_type;                 /* Object file type */
    Elf32_Half    e_machine;              /* Architecture */
    Elf32_Word    e_version;              /* Object file version */
    Elf32_Addr    e_entry;                /* Entry point virtual address */
    Elf32_Off     e_phoff;                /* Program header table file offset */
    Elf32_Off     e_shoff;                /* Section header table file offset */
    Elf32_Word    e_flags;                /* Processor-specific flags */
    Elf32_Half    e_ehsize;               /* ELF header size in bytes */
    Elf32_Half    e_phentsize;            /* Program header table entry size */
    Elf32_Half    e_phnum;                /* Program header table entry count */
    Elf32_Half    e_shentsize;            /* Section header table entry size */
    Elf32_Half    e_shnum;                /* Section header table entry count */
    Elf32_Half    e_shstrndx;             /* Section header string table index */
} Elf32_Ehdr ;


typedef struct
{
    Elf32_Word  p_type;          /* Segment type */
    Elf32_Off   p_offset;        /* Segment file offset */
    Elf32_Addr  p_vaddr;         /* Segment virtual address */
    Elf32_Addr  p_paddr;         /* Segment physical address */
    Elf32_Word  p_filesz;        /* Segment size in file */
    Elf32_Word  p_memsz;         /* Segment size in memory */
    Elf32_Word  p_flags;         /* Segment flags */
    Elf32_Word  p_align;         /* Segment alignment */
}Elf32_Phdr ; 

// p_type的取值
#define	PT_NULL		0		 /* Program header table entry unused */
#define PT_LOAD		1		 /* Loadable program segment */
#define PT_DYNAMIC	2		 /* Dynamic linking information */
#define PT_INTERP	3		 /* Program interpreter */
#define PT_NOTE		4		 /* Auxiliary information */
#define PT_SHLIB	5		 /* Reserved */
#define PT_PHDR		6		 /* Entry for header table itself */


#define ET_EXEC         2   // 可执行文件
#define ET_386          3   // 80386处理器

#pragma pack() // 恢复默认值

#endif 