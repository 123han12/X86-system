
#include <stdint.h>
int main (int argc , char* argv[] ) ; 


extern uint8_t __bss_start__[] , __bss_end__[] ; 
void cstart(int argc , char** argv ) {
    // 这里需要对.bss段进行数据的清零
    uint8_t* start = __bss_start__ ; 
    while(start < __bss_end__) {
        *start++ = 0 ; 
    }
    main(argc , argv ) ; 
}