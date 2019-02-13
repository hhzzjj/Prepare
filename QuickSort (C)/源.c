#include <stdio.h>
#include "quicksort.h"


void main()
{
	int a[20], n;
	scanf("%d", &n);
	for (int i = 1; i <= n; i++)    //注意，这里数组从a[1]开始存储的
	{
		scanf("%d", &a[i]);
	}

	quicksort(1, n,a);

	for (int i = 1; i <= n; i++)
		printf("%d", a[i]);

	getchar(); getchar();
}


