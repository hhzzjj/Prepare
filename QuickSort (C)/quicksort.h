#pragma once
#include <stdio.h>

void quicksort(int left, int right,int a[])   //��left��ʼ����right����������
{
	int i, j, t, temp;
	if (left > right)
		return;

	temp = a[left];                   //ȷ��������
	i = left;
	j = right;

	while (i != j)
	{
		while (a[j] >= temp&&i < j)  //�ұߴ��ڵ��ڲ������Ĳ���
			j--;
		while (a[i] <= temp&&i < j)  //���С�ڵ��ڲ������Ĳ���
			i++;

		if (i != j)
		{
			t = a[i];                //����
			a[i] = a[j];
			a[j] = t;
		}
	}

	a[left] = a[i];                  //���������м��������λ��
	a[i] = temp;

	quicksort(left, i - 1, a);         //�ݹ飬������ߵ�����
	quicksort(i + 1, right, a);        //�ݹ飬�����ұߵ�����
}