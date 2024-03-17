#include "tools/bitmap.h"  
#include "tools/klib.h" 


uint32_t bitmap_byte_count(int bit_count)
{
    return (bit_count + 7) / 8 ;  // 向上取整  
}

// bitmap 用来管理一个内存位图，bits 表示开始地址，count表示位的个数，init_bit表示初始值。
// 对内存位图进行一个初始化
void bitmap_init(bitmap_t* bitmap , uint8_t* bits , int count , int init_bit )
{
    bitmap->bitcount = count ; 
    bitmap->bits = bits ; 
    
    int bytes = bitmap_byte_count(bitmap->bitcount) ;  // 整个位图所需要的字节数，向上取整

    // 通过获取的bytes对从bits开始的位图内存进行初始化, 将其初始化为init_bit(0 或者 1)
    kernel_memset(bitmap->bits , init_bit ? 0xFF : 0x00 , bytes ) ; 
}

int bitmap_get_bit(bitmap_t* bitmap , int index ) 
{
    return bitmap->bits[index / 8 ] & (1 << (index % 8) ) ; 
}

// 从第index个bit位开始，将连续count个位的值置为bit 其中bit 的值为0或者1 
void bitmap_set_bit(bitmap_t* bitmap , int index , int count , int bit )
{
    for(int i = 0 ; (i < count ) && (index < bitmap->bitcount ); i++ , index++ )
    {
        if(bit) 
        {
            bitmap->bits[index / 8] |= (1 << (index % 8) ) ; 
        } else {
            bitmap->bits[index / 8] &= ~(1 << (index % 8) ) ; 
        }
    }
}

int bitmap_is_set(bitmap_t* bitmap , int index ) 
{
    return bitmap_get_bit(bitmap , index ) ? 1 : 0 ; 
}



// 找到count个连续的位的值为bit的位置
int bitmap_alloc_nbits(bitmap_t* bitmap , int bit , int count ) 
{   
    int search_index = 0 ; 
    int ok_index = -1 ; 
    while(search_index < bitmap->bitcount )
    {
        if(bitmap_get_bit(bitmap , search_index) != bit ) {
            search_index ++ ; 
            continue ; 
        }

        ok_index = search_index ; 
        int i = 1 ; 
        for( ; i < count && (search_index < bitmap->bitcount ) ; i ++ )
        {
            if(bitmap_get_bit(bitmap , search_index++) != bit )
            {
                ok_index = -1 ; 
                break ; 
            }
        }
        if(i >= count ) 
        {
            bitmap_set_bit(bitmap , ok_index , count , bit ? 0 : 1 ) ; 
            return ok_index ; 
        }  
    }

    return -1 ; 
}

