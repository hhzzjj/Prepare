#include <iostream>
#include "quicksort.h"
using namespace std;


void main()
{
	int a[20], n;
	cin>>n;
	for (int i = 1; i <= n; i++)    //ע�⣬���������a[1]��ʼ�洢��
	{
		cin>>a[i];
	}

	quicksort(1, n, a);

	for (int i = 1; i <= n; i++)
		cout<<a[i];


}
