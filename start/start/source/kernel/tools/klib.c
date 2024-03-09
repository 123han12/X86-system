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
    while (*str++)
        count++;
    return count;
}
void kernel_memcpy(void *dest, void *src, uint32_t size)
{
    if (!dest || !src || !size)
        return;
    uint8_t *d = (uint8_t *)dest;
    uint8_t *s = (uint8_t *)src;

    while ((size--) && *s)
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
    static const char *map = "0123456789ABCDEF" ; 
    if(num == 0 ) 
    {
        *buf++ = '0' ; 
        *buf = '\0' ; 
        return ; 
    }
 

    // 仅支持部分进制
    if ((base != 2) && (base != 8) && (base != 10) && (base != 16)) {
        *buf = '\0' ; 
        return ;
    }

    if (num < 0) *buf++ = '-';
    num = num >= 0 ? num : -num ; 
    char *start = buf ; 
    while (num)
    {
        int mod = num % base ; 
        *start++ = map[mod];
        num /= base ; 
    }
    char *left = buf , *right = start - 1;
    while (left <  right)
    {
        char c = *left;
        *left = *right;
        *right = c;
        left++, right--;
    }
    *start = '\0' ; 
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