__asm__(".code16gcc");

#include "loader.h"
void protected_mode_entry (void);

static void show_msg(const char *msg)
{
    char c;
    while ((c = *msg++) != '\0')
    {
        // 如果内联汇编有多条指令，则每一行都要加上双引号，并且该行以 \n\t 结尾 , 请将内联汇编写为标准的格式
        __asm__ __volatile__(
            "movb $0xE , %%ah\n\t"
            "movb %[ch] , %%al\n\t"
            "int $0x10\n\t"
            :
            : [ch] "r"(c)
            :
        );
    }
}

boot_info_t boot_info; // 这个结构体变量用来存储返回的信息。

static void detect_memory(void)
{
    uint32_t contID = 0;
    SMAP_entry_t smap_entry;
    int signature, bytes;

    show_msg("try to detect memory.......\r\n");

    boot_info.ram_region_count = 0;
    for (int i = 0; i < BOOT_RAM_REGION_MAX; i++)
    {
        SMAP_entry_t *entry = &smap_entry;

        __asm__ __volatile__("int  $0x15"
                             : "=a"(signature), "=c"(bytes), "=b"(contID)
                             : "a"(0xE820), "b"(contID), "c"(24), "d"(0x534D4150), "D"(entry)
                            );

        if (signature != 0x534D4150)
        {
            show_msg("failed.\r\n");
            return;
        }

        if (bytes > 20 && (entry->ACPI & 0x0001) == 0)
        {
            continue;
        }

        if (entry->Type == 1)
        {
            boot_info.ram_region_cfg[boot_info.ram_region_count].start = entry->BaseL;
            boot_info.ram_region_cfg[boot_info.ram_region_count].size = entry->LengthL;
            boot_info.ram_region_count++;
        }
        if (contID == 0)
            break;
    }

    show_msg("great we are complete the thing that userful detect memory\r\n");
}

uint16_t gdt_table[][4] = {
    {0 , 0 , 0 , 0 } , 
    {0xFFFF , 0x0000 , 0x9A00 , 0x00CF} , 
    {0xFFFF , 0x0000 , 0x9200 , 0x00CF } , 
} ; 


static void enter_protect_mode() 
{
    // 关中断
    cli() ; 

    // 打开A20 地址线
    uint8_t val = inb(0x92) ; 
    outb(0x92 , val | 0x2 ) ; 

    // 设置 gdt 表
    lgdt((uint32_t)gdt_table , sizeof(gdt_table) ) ; 

    // 设置 cr0 的最低位
    uint32_t  CR0 = read_cr0() ; 
    write_cr0(CR0 | (1 << 0 ) ) ;

    // 实现远跳转
    far_jump(8 , (uint32_t)protected_mode_entry ) ; 
}



void loader_entry(void) // 对操作系统的运行环境初始化，加载操作系统到内存中去
{
    show_msg("......loading.......\n\r");

    // 添加代码，实现对内存容量的检测
    detect_memory();

    // 进入保护模式代码
    enter_protect_mode() ; 

    for(; ; ) {} 

}
