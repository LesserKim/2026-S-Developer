#include "pch.h"
#include <stdio.h>

void PrintFunc(const char* pszContext)
{
	printf("%s\n", pszContext);
}

void PrintFunc(const wchar_t* pszContext)
{
	wprintf(L"%s\n", pszContext);
}