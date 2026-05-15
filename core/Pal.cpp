/* 注册表相关辅助函数 */

#include <stdio.h>
#include <string.h>
#include <limits.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

const char* Get_CurDir() 
{
    static char dir[PATH_MAX]; // 使用静态缓冲区存储目录路径

#ifdef _WIN32
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    int len = wcstombs(dir, buffer, MAX_PATH); // 将宽字符转换为多字节字符
    if (len != -1) {
        char *pos = strrchr(dir, '\\');
        if (pos) {
            *(pos + 1) = '\0'; // 在最后一个分隔符后添加空终止符
        }
        return dir;
    } else {
        // 处理转换错误
        return "";
    }
#else
    ssize_t len = readlink("/proc/self/exe", dir, sizeof(dir) - 1);
    if (len != -1) {
        dir[len] = '\0';
        char *pos = strrchr(dir, '/');
        if (pos) {
            *(pos + 1) = '\0'; // 在最后一个分隔符后添加空终止符
        }
        return dir;
    } else {
        // 处理 readlink 错误
        return "";
    }
#endif
}
