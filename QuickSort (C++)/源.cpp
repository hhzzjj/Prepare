#include <iostream>
#include "quicksort.h"
using namespace std;


void main()
{
	int a[20], n;
	cin>>n;
	for (int i = 1; i <= n; i++)    //注意，这里数组从a[1]开始存储的
	{
		cin>>a[i];
	}

	quicksort(1, n, a);

	for (int i = 1; i <= n; i++)
		cout<<a[i];


}
