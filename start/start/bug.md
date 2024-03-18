```
-exec x /20xb 0x80000000
0x80000000 <first_task_entry>:	0x66	0x8c	0xd0	0x8e	0xd8	0x8e	0xc0	0x8e
0x80000008 <first_task_entry+8>:	0xe0	0x8e	0xe8	0xe9	0x00	0x00	0x00	0x00
0x80000010 <first_task_main>:	0x55	0x89	0xe5	0x83
```
在执行kernel_memcpy()的时候，拷贝first_task 进程的内存到虚拟地址0x80000000 的时候，在编写kernel_memcpy()的时候，功能函数为
```
void kernel_memcpy(void *dest, void *src, uint32_t size)
{
    if (!dest || !src || !size)
        return;
    uint8_t *d = (uint8_t *)dest;
    uint8_t *s = (uint8_t *)src;

    while ( (size-- ) && *s ) 
    {
        *d++ = *s++;
    }
}
```

**注意上述代码中的判断条件的问题，** ， 上述内存二进制视图是我们实际复制到0x80000000 的数据，其中存在一些字节的值为`0x00` 这就造成了==代码移动出现了问题== , 在此强调kernel_memcpy()函数的作用:**将源地址指定size个字节复制到目的地址src去，无论源地址是否结束**


