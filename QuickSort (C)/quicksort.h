#pragma once
#include <stdio.h>

void quicksort(int left, int right,int a[])   //从left开始，到right，进行排序
{
	int i, j, t, temp;
	if (left > right)
		return;

	temp = a[left];                   //确定参照数
	i = left;
	j = right;

	while (i != j)
	{
		while (a[j] >= temp&&i < j)  //右边大于等于参照数的不管
			j--;
		while (a[i] <= temp&&i < j)  //左边小于等于参照数的不管
			i++;

		if (i != j)
		{
			t = a[i];                //交换
			a[i] = a[j];
			a[j] = t;
		}
	}

	a[left] = a[i];                  //参照数和中间的数交换位置
	a[i] = temp;

	quicksort(left, i - 1, a);         //递归，继续左边的排序
	quicksort(i + 1, right, a);        //递归，继续右边的排序
}