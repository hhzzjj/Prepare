#include <iostream>
#include <assert.h>
using namespace std;

char *mystrcpy(char *s1, const cahr *s2)
{
  assert(s1!=NULL);
  assert(s2!=NULL);
  char *p=s1;
  while((*s1++=*s2++)!='\0');
  return p;
}
  
//返回内容为指向目标内存的地址指针，这样可以在需要字符指针的函数中使用，如strlen(strcpy(str1,str2)
