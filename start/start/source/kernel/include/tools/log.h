#ifndef LOG_H
#define LOG_H

void log_init(void) ; // 日志输出要使用的硬件初始化

void log_printf(const char* fmt , ... ); // ... 表示函数参数包 


#endif 
