#include "loader.h"
#include "common/elf.h"

// 这个模块的代码实现的是从硬盘指定扇区开始，读取指定个数个扇区放到内存的指定位置的功能，不需太过纠结这个原理。
static void read_disk(int sector , int sector_count , uint8_t* buf ) 
{
    outb(0x1F6, (uint8_t) (0xE0));

	outb(0x1F2, (uint8_t) (sector_count >> 8));
    outb(0x1F3, (uint8_t) (sector >> 24));		// LBA参数的24~31位
    outb(0x1F4, (uint8_t) (0));					// LBA参数的32~39位
    outb(0x1F5, (uint8_t) (0));					// LBA参数的40~47位

    outb(0x1F2, (uint8_t) (sector_count));
	outb(0x1F3, (uint8_t) (sector));			// LBA参数的0~7位
	outb(0x1F4, (uint8_t) (sector >> 8));		// LBA参数的8~15位
	outb(0x1F5, (uint8_t) (sector >> 16));		// LBA参数的16~23位

	outb(0x1F7, (uint8_t) 0x24) ; 

    uint16_t * data_buf = (uint16_t*) buf ; 
    while(sector_count -- ) 
    {
        while((inb(0x1F7) & 0x88 ) != 0x8) {}  // 检测当前扇区的数据是否准备好了。
        for(int i = 0 ; i < SECTOR_SIZE / 2 ; i ++ )
        {
            *data_buf++ = inw(0x1F0) ; 
        }
    }


}


// 从file_buffer 处读取到elf 文件，并将其解析。
static uint32_t reload_elf_file(uint8_t* file_buffer)
{
    Elf32_Ehdr * elf_hdr = (Elf32_Ehdr*) file_buffer ; 

    // 检查是否为有效的elf文件
    if(
        (elf_hdr->e_ident[0] != 0x7F) || 
        (elf_hdr->e_ident[1] != 0x45) || 
        (elf_hdr->e_ident[2] != 0x4C) || 
        (elf_hdr->e_ident[3] != 0x46) )  
    {
        // 走到这里说明不是有效的elf文件
        return 0 ;
    }
    for(int i = 0 ; i < elf_hdr->e_phnum ; i ++ )
    {
        Elf32_Phdr * phdr = (Elf32_Phdr* )(file_buffer + elf_hdr->e_phoff) + i ; 
        if(phdr->p_type != PT_LOAD ) continue ;  //  如果这个段能被加载 就略过

        uint8_t * src = file_buffer + phdr->p_offset ; 
        uint8_t * dest = (uint8_t* )phdr->p_paddr ;
        for(int j = 0 ; j < phdr->p_filesz ; ++j  )
        {
            *dest++ = *src++ ; 
        }

        dest = (uint8_t * )phdr->p_paddr + phdr->p_filesz ; 
        for(int j = 0 ; j < phdr->p_memsz - phdr->p_filesz ; j ++ ) *dest++ = 0x00 ;   // 对 p_memsz 比 p_filesz 多余出来的部分清零。

    }

    return elf_hdr->e_entry ; 
}


static void die(uint32_t code )
{
    for(;;) {}
} 

#define CR4_PSE         (1 << 4 ) 
#define CR0_PG           (1 << 31 )

#define PDE_P            (1 << 0 )
#define PDE_W            (1 << 1 )
#define PDE_PS          (1 << 7 )
void enable_page_mode(void)
{
    static uint32_t page_dir[1024] __attribute__((aligned(4096)) ) = {
        [0] = PDE_P | PDE_W | PDE_PS  | 0x0 
    } ;

    uint32_t cr4 = read_cr4() ; 
    write_cr4(cr4 | CR4_PSE ) ;

    write_cr3((uint32_t)page_dir) ; 

    write_cr0(read_cr0() | CR0_PG) ; 


}

// 这里没有开启分页机制，直接就是物理地址
void load_kernel(void)
{
    // 将 SYS_KERNEL_LOAD_ADDR 转换为函数指针，并且调用这个函数指针 就能跳入到内核进行执行了。
    read_disk(100 , 500 , (uint8_t *)SYS_KERNEL_LOAD_ADDR ) ;  

    uint32_t kernel_entry = reload_elf_file((uint8_t*)SYS_KERNEL_LOAD_ADDR) ;
    if(kernel_entry == 0 )
    {
        // 表示未将kernel 可执行文件装载好。
        die(-1) ; 
    }
    enable_page_mode() ; 
    
    ((void (*)(boot_info_t* ) ) kernel_entry )(&boot_info) ;    



    for( ;; ) {} 
}