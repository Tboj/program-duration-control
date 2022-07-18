#include <stdio.h> 
#include "stdlib.h"
#include <string.h>


char* join(char **src, int length) {
    char *result = (char *)malloc(65525);
    for (int i = 0; i < length; i++) {
        strcat(result, src[i]);
    }
    return result;
}

#define FILE_PATH "D:\\my_log.txt" //信息输出文件
int WriteToLog(char *str)
{
    FILE *pfile;
    fopen_s(&pfile, FILE_PATH, "a+");
    if (pfile == NULL)  
    {
        return -1;
    }

    fprintf_s(pfile, "%s\n", str);
    fclose(pfile);
    return 0;
}

void main() {
    char s[65525] = {"QQQ111"};
    WriteToLog(s);
}