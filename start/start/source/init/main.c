
#include "applib/lib_syscall.h"
#include <stdio.h>
#include "main.h" 
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/file.h>
#include "fs/file.h"
#include "dev/tty.h"

int main(int argc , char** argv ) {
   // int a = 3 / 0 ; 
   *(char*)0 = 0x12 ; 
   return 0 ; 
}
