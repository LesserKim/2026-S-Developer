// 0310cppcore.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"

int main()
{
    HANDLE hFile = CreateFile(TEXT("d:/hello.txt"), GENERIC_READ_, OPEN_EXISTING_, 0);
    if (NULL == hFile)
        return -1;

    char szBuffer[10000 + 1];
    DWORD dwReadSize = 0; 
    ReadFile(hFile, szBuffer, 10000, &dwReadSize);

    szBuffer[dwReadSize] = 0;
    printf("%s\n", szBuffer);

    CloseFile(hFile);
    return 0; 
}

