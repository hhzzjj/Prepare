#include <stdio.h>
#include "quicksort.h"


void main()
{
	int a[20], n;
	scanf("%d", &n);
	for (int i = 1; i <= n; i++)    //ע�⣬���������a[1]��ʼ�洢��
	{
		scanf("%d", &a[i]);
	}

	quicksort(1, n,a);

	for (int i = 1; i <= n; i++)
		printf("%d", a[i]);

	getchar(); getchar();

}


