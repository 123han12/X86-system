#include "tools/klib.h"
#include "common/types.h"




void kernel_strcpy(char *dest, const char *src)
{
    if (!src || !dest)
        return;
    while (*src)
    {
        *dest++ = *src++;
    }
    *dest = '\0';
}
void kernel_strncpy(char *dest, const char *src, uint32_t size)
{
    if (!src || !dest || size <= 0)
        return;
    char *d = dest;
    const char *s = src;
    while ((size-- > 0) && (*s))
    {
        *d++ = *s++;
    }
    *d = '\0';
}
uint32_t kernel_strncmp(const char *s1, const char *s2, uint32_t size)
{
    if (!s1 || !s2)
        return -1;

    while (*s1 && *s2 && (*s1 == *s2) && size)
    {
        s1++;
        s2++;
        size--;
    }
    return !((*s1 == '\0') || (*s2 == '\0') || (*s1 == *s2));
}

uint32_t kernel_strlen(const char *str)
{
    if (str == (const char *)0)
    {
        return -1;
    }
    uint32_t count = 0;
    while (*str++ != '\0' ) 
        count++;
    return count;
}
void kernel_memcpy(void *dest, void *src, uint32_t size)
{
    if (!dest || !src || !size)
        return;
    uint8_t *d = (uint8_t *)dest;
    uint8_t *s = (uint8_t *)src;

    while (size--)
    {
        *d++ = *s++;
    }
}
void kernel_memset(void *dest, uint8_t v, uint32_t size)
{
    if (!dest || size <= 0)
        return;

    uint8_t *d = (uint8_t *)dest;
    while (size--)
    {
        *d++ = v;
    }
}
uint32_t kernel_memcmp(void *d1, void *d2, uint32_t size)
{
    if (!d1 || !d2 || !size)
        return -1;
    uint8_t *p_d1 = (uint8_t *)d1;
    uint8_t *p_d2 = (uint8_t *)d2;
    while (size--)
    {
        if (*p_d1++ != *p_d2++)
            return 1;
    }
    return 0;
}

void kernel_sprintf(char * buffer, const char * fmt, ...) {
    va_list args;

    va_start(args, fmt);
    kernel_vsprintf(buffer, fmt, args);
    va_end(args);
}

void kernel_itoa(char *buf, int num, int base) // 不处理16进制的负数的情况。
{
        // 转换字符索引[-15, -14, ...-1, 0, 1, ...., 14, 15]
    static const char * num2ch = {"FEDCBA9876543210123456789ABCDEF"};
    char * p = buf;
    int old_num = num;

    // 仅支持部分进制
    if ((base != 2) && (base != 8) && (base != 10) && (base != 16)) {
        *p = '\0';
        return;
    }

    // 只支持十进制负数
    int signed_num = 0;
    if ((num < 0) && (base == 10)) {
        *p++ = '-';
        signed_num = 1;
    }

    if (signed_num) {
        do {
            char ch = num2ch[num % base + 15];
            *p++ = ch;
            num /= base;
        } while (num);
    } else {
        uint32_t u_num = (uint32_t)num;
        do {
            char ch = num2ch[u_num % base + 15];
            *p++ = ch;
            u_num /= base;
        } while (u_num);
    }
    *p-- = '\0';

    // 将转换结果逆序，生成最终的结果
    char * start = (!signed_num) ? buf : buf + 1;
    while (start < p) {
        char ch = *start;
        *start = *p;
        *p-- = ch;
        start++;
    }
}

void kernel_vsprintf(char *buffer, const char *fmt, va_list args)
{
    enum
    {
        NORMAL,
        READ_FMT
    } state = NORMAL;
    char ch;
    char *curr = buffer;
    while ((ch = *fmt++))
    {
        switch (state)
        {
        case NORMAL:
            if (ch == '%')
            {
                state = READ_FMT;
            }
            else
            {
                *curr++ = ch;
            }
            break;
        case READ_FMT:
            if (ch == 's')
            {
                const char *str = va_arg(args, char *);
                uint32_t len = kernel_strlen(str);
                while (len--)
                {
                    *curr++ = *str++;
                }
            }
            else if (ch == 'c')
            {
                char c = va_arg(args, int );
                *curr++ = c;
            }
            else if (ch == 'd')
            {
                int num = va_arg(args , int) ; 
                kernel_itoa(curr , num , 10 ) ; 
                curr += kernel_strlen(curr) ; 
            }
            else if (ch == 'x')
            {
                int num = va_arg(args , int) ; 
                kernel_itoa(curr , num , 16) ; 
                curr += kernel_strlen(curr) ; 
            }
            state = NORMAL;
            break;
        }
    }
}

int strings_count(char** start ){
    int count = 0 ; 
    if(start ) {
        while(*start++) {
            count ++ ; 
        }
    }
    return count ; 
}



char* get_file_name( char* name ) {
    char * s = name ; 
    while(*s != '\0' ) s ++ ; 
    while( (*s != '/') && (*s != '\\' ) && (s >= name ) ) s-- ; 
    return s + 1 ; 
}