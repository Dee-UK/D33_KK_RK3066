#include "fft_fixed_point.h"
#if 0
#include <stdlib.h>
#include <string.h>
#else

#include <linux/string.h>
#include <linux/resume-trace.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/cpu.h>
#include <linux/slab.h>
#include <linux/freezer.h>
#include <linux/vmalloc.h>
#define free(ptr) kfree((ptr))
//typedef unsigned char uchar;

#define malloc(size) kmalloc((size), GFP_ATOMIC)

#define printf(fmt, arg...) \
		printk(KERN_EMERG fmt, ##arg)

#endif


/***********************************************���ұ�*************************************************************/

//ȫ�ֱ���,2���������ݱ�
int Pow2_table[20] = {1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288};

//���ϱ���ж��㻯���sin�������ұ������ϱ���ÿ�������� 2^8 = 256
int sin_table_INT_128[]={
	 0,  13,  25,  38,  50,  62,  74,  86,
	98, 109, 121, 132, 142, 152, 162, 172,
	181, 190, 198, 206, 213, 220, 226, 231,
	237, 241, 245, 248, 251, 253, 255, 256,
	256, 256, 255, 253, 251, 248, 245, 241,
	237, 231, 226, 220, 213, 206, 198, 190,
	181, 172, 162, 152, 142, 132, 121, 109,
	 98,  86,  74,  62,  50,  38,  25,  13
	
};
//FFT�е���Ϊ128��cos�������㻯��Ĳ��ұ�
int cos_table_INT_128[]={
	256,  256,  255,  253,  251,  248,  245,  241,
	237,  231,  226,  220,  213,  206,  198,  190,
	181,  172,  162,  152,  142,  132,  121,  109,
	 98,   86,   74,   62,   50,   38,   25,   13,
	  0,  -13,  -25,  -38,  -50,  -62,  -74,  -86,
	 -98, -109, -121, -132, -142, -152, -162, -172,
	-181, -190, -198, -206, -213, -220, -226, -231,
	-237, -241, -245, -248, -251, -253, -255, -256
};

//FFT������������������������ ע��:ֻ�����ڵ���Ϊ128
int reverse_matrix[]={
	0,  64,  32,  96,  16,  80,  48, 112,   8,  72,  40, 104,  24,  88,  56, 120,
	4,  68,  36, 100,  20,  84,  52, 116,  12,  76,  44, 108,  28,  92,  60, 124,
	2,  66,  34,  98,  18,  82,  50, 114,  10,  74,  42, 106,  26,  90,  58, 122,
	6,  70,  38, 102,  22,  86,  54, 118,  14,  78,  46, 110,  30,  94,  62, 126,
	1,  65,  33,  97,  17,  81,  49, 113,   9,  73,  41, 105,  25,  89,  57, 121,
	5,  69,  37, 101,  21,  85,  53, 117,  13,  77,  45, 109,  29,  93,  61, 125,
	3,  67,  35,  99,  19,  83,  51, 115,  11,  75,  43, 107,  27,  91,  59, 123,
	7,  71,  39, 103,  23,  87,  55, 119,  15,  79,  47, 111,  31,  95,  63, 127
};

/***********************************************���ұ�*************************************************************/



/***************************************************************************************
����Ϊʵ�ֶ�ά�����FFT�任����غ���,��AΪM��N�У���M,N�����ǣ����ݴ���
Date:
    2012-05-07
****************************************************************************************/

/*************************************************************************
Function:
       reverse
Discription:
       ���е���������
	   ���� x(n) �������ذ��桢ż���飬������ͼ����˵����в�����˳���,
	   �������Ǳ�֤���������Ȼ˳��ʱ������˵ı�ַ����
Input Arguments:
       len: ���еĳ���
	   M:   ���ȶ�Ӧ��2��ָ�� ��len = 8,M = 3
Output Arguments:
       b: ����˱�ַ���������ĵ�λ��
Author:
       Wu Lijuan
Note:
       N./A.
Date:
      $ID:  $ 
**************************************************************************/
void reverse(int len, int M,int *b)
{
	//ע��b����������ʼ��Ϊ0
    int i,j;

	//a�ĳ�ʼ��,a���ڴ�ű�ַ�����еĶ�������
	char *a = (char *)malloc(sizeof(char)*M); // a���ڴ��Mλ��������
	memset(a,0,sizeof(char)*M);

    for(i=1; i<len; i++)     // i = 0 ʱ��˳��һ��
    {
        j = 0;
        while(a[j] != 0)
        {
            a[j] = 0;
            j++;
        }

        a[j] = 1;
        for(j=0; j<M; j++)
        {
            b[i] = b[i]+a[j] * Pow2_table[M-1-j];    //pow(2,M-1-j),��������aת��Ϊ10����b
        }
    }
	free(a);
}
/*************************************************************************
Function:
       nexttopow2
Discription:
       y = nexttopow2(x);
	   ��2^yΪ���ڵ���x����С��2�����������ݵ�����,
	   ��x = 12����y = 4(2^4 = 16)
Input Arguments:
       x ������������
Output Arguments:
       y: 2^yΪ���ڵ���x����С��2�����������ݵ�����
Author:
       Wu Lijuan
Note:
       N./A.
Date:
      2013.02.23
**************************************************************************/
int nexttopow2(int x)
{
	int y;
	int i = 0;
	while(x > Pow2_table[i])
	{
		i++;
	}
	y = i;
	return y;
}
/*************************************************************************
Function:
       fft_fixed_point
Discription:
       1D FFT�任 ���ö��㻯���㣬����ٶ�
Input Arguments:
       A: ����FFT�任������,ͬʱҲ��������
       fft_nLen:�����г���
	   fft_M:     ���ȶ�Ӧ��ָ�� eg, 8 = 2^3
Output Arguments:
      
Author:
       Wu Lijuan
Note:
       ����ԭ��ο����ļ�����PPT:���ٸ���Ҷ�任���������㣩P31
Date:
      2013.06.25 
**************************************************************************/
void fft_fixed_point(int fft_nLen, int fft_M, RK_complex_INT * A)
{
	int i;
	int L,dist,p,t;
	int temp1,temp2;
	RK_complex_INT *pr1,*pr2;
	//double temp = 2*PI/fft_nLen;
	
    int WN_Re,WN_Im;       //WN��ʵ�����鲿
	int X1_Re,X1_Im;       //��ʱ������ʵ�����鲿
	int X2_Re,X2_Im;     

	for(L = 1; L <= fft_M; L++)         //���M�ε��εĵ�������
	{
		dist = Pow2_table[L-1];             //dist = pow(2,L-1); �����������ڵ��ľ���
		temp1 = Pow2_table[fft_M-L];        //temp1 = pow(2,fft_M-L);
		temp2 = Pow2_table[L];              //temp2 = pow(2,L); 
		for(t=0; t<dist; t++)                 //ѭ���������WN�ı仯
		{
			p = t * temp1;                    //p = t*pow(2,fft_M-L);  

			//WN_Re = int(cos(temp * p) * Pow2_table[Q_INPUT]);          //W��ȷ�� (double)cos(2*PI*p/fft_nLen); 
  	//		WN_Im = int(-1*sin(temp * p) * Pow2_table[Q_INPUT]);     //(double)(-1*sin(2*PI*p/fft_nLen));
			WN_Re = cos_table_INT_128[p];          //Q15
			WN_Im = -sin_table_INT_128[p];
			
			//ѭ�������ͬ����WN�ĵ�������
			for(i = t; i < fft_nLen; i = i + temp2)           //i=i+pow(2,L)   Note: i=i+pow(2,L) 
			{
				//X(k) = X1(k) + WN * X2(k); ǰ�벿��X(k)���㹫ʽ
				//X(k) = X1(k) - WN * X2(k); ��벿��X(k)���㹫ʽ

				pr1 = A+i;
				pr2 = pr1+dist;

				X1_Re = pr1->real;        //X1_Re = A[i].real; 
				X1_Im = pr1->image;       //X1_Im = A[i].image;
				X2_Re = pr2->real;        //X2_Re = A[i+dist].real;
				X2_Im = pr2->image;       //X2_Im = A[i+dist].image;


				//����WN * X2(k),�Ǹ�����
				int WN_X2_Re = ((long long)WN_Re * X2_Re - (long long)WN_Im * X2_Im) >> Q_INPUT;
				int WN_X2_Im = ((long long)WN_Im * X2_Re + (long long)WN_Re * X2_Im) >> Q_INPUT;
				
				//����X(k) = X1(k) + WN * X2(k);
				pr1->real = X1_Re + WN_X2_Re;     //A[i].real = X1_Re + WN_X2_Re;
				pr1->image = X1_Im + WN_X2_Im;    //A[i].image = X1_Im + WN_X2_Im; 

				//����X(k) = X1(k) - WN * X2(k);
				pr2->real = X1_Re - WN_X2_Re;     //A[i+dist].real = X1_Re - WN_X2_Re;
				pr2->image = X1_Im - WN_X2_Im;    //A[i+dist].image = X1_Im - WN_X2_Im;
			}
		}
	}
}

/*************************************************************************
Function:
       fft_2D_fixed_point
Discription:
       2ά FFT�任 
Input Arguments:
       mLen,nLen ����ĸ߿�
	   M,N;     ���ȶ�Ӧ��2�Ķ��� M = log2(mlen), N = log2(nlen);
       A_In:����Ҫ������任�ľ���,ͬʱҲ��������
	   flag:    0������FFT�任����1   ����FFT�任
Output Arguments:
       
Author:
       Wu Lijuan
Note:
       N./A.
Date:
      2013.06.25
**************************************************************************/
void fft_2D_fixed_point(int mLen,int nLen,int M,int N,RK_complex_INT *A_In,int flag)
{
	int i,j;
	int len = mLen * nLen;

	//int *b = NULL;
	int *b = reverse_matrix;      //ֱ�Ӳ���������Ч��

	if (flag == DXT_INVERSE)
	{
		RK_complex_INT *p = A_In;

		for(i=0; i<len; i++)
		{
			//��任��ʽ�����任��ʽ����,�������鲿����ʵ������鲿��һ������
			//��ͨ������£����Ƕ�ifft��ģֵ������Re*Re + Im*Im,�ʶԽ��ûʲôӰ��
			//ֻ��Ӧ��֪�����������
			(p->image) *= -1;    //A_In[i].image = -A_In[i].image;
			p++;
		}
	}	

	//A���ڴ��������ÿ�е�����
	RK_complex_INT * A;
	A = (RK_complex_INT *)malloc(sizeof(RK_complex_INT)*nLen);


	//�ȶԾ����ÿһ����FFT�任
	for(i=0; i<mLen; i++)
	{
		RK_complex_INT *pr1 = A;
		RK_complex_INT *pr2 = NULL;
		RK_complex_INT *pr3 = A_In + (i<<N);   //��ͬ�е���ʼ��ַ

		for(j=0; j<nLen; j++)
		{
			//A[j].real = A_In[i*nLen+b[j]].real;        //ȡ������������ÿ������
			//A[j].image = A_In[i*nLen+b[j]].image;

			pr2 = pr3 + b[j];                       //ȡ���϶δ��룬��߷���Ч��
			pr1->real = pr2->real;        
			pr1->image = pr2->image;
			pr1++;
		}


		fft_fixed_point(nLen,N,A);                                 //����IFFT�任���任��Ľ���Դ���A

		if (flag == DXT_FORWARD)
		{
			pr1 = A;             //������λָ��
			pr2 = pr3;

			for(j=0; j<nLen; j++)
			{
				//A_In[i*nLen+j].real = A[j].real;           //���任�������ԭ������
				//A_In[i*nLen+j].image = A[j].image;

				pr2->real = pr1->real;
				pr2->image = pr1->image;
				pr1++;
				pr2++;
			}
		}
		else
		{
			pr1 = A;             //������λָ��
			pr2 = pr3;

			for(j=0; j<nLen; j++)
			{
				//A_In[i*nLen+j].real = A[j].real/nLen;           //���任�������ԭ������
				//A_In[i*nLen+j].image = A[j].image/nLen;

				pr2->real = pr1->real >> N;
				pr2->image = pr1->image >>N;
				pr1++;
				pr2++;
			}	
		}

	}
	free(A);
	//free(b);

	//A���ڴ��������ÿ�е�����
	A = (RK_complex_INT *)malloc(sizeof(RK_complex_INT)*mLen);


	//�ȶԾ����ÿһ����FFT�任
	for(i=0; i<nLen; i++)
	{
		RK_complex_INT *pr1 = A;
		RK_complex_INT *pr2 = NULL;
		RK_complex_INT *pr3 = A_In + i;   //��ͬ�е���ʼ��ַ

		for(j=0; j<mLen; j++)
		{
			//A[j].real = A_In[b[j]*nLen+i].real;        //ȡ������������ÿ������
			//A[j].image = A_In[b[j]*nLen+i].image;

			pr2 = pr3 + (b[j]<<N);
			pr1->real = pr2->real;
			pr1->image = pr2->image;
			pr1++;
		}

		fft_fixed_point(mLen,M,A);                                 //����IFFT�任���任��Ľ���Դ���A

		if (flag == DXT_FORWARD)
		{
			pr1 = A;             //������λָ��
			pr2 = pr3;

			for(j=0; j<mLen; j++)
			{
				//A_In[j*nLen+i].real = A[j].real;           //���任�������ԭ������
				//A_In[j*nLen+i].image = A[j].image;

				pr2->real = pr1->real;
				pr2->image = pr1->image;
				pr1++;
				pr2 += nLen;
			}	
		}
		else
		{
			pr1 = A;             //������λָ��
			pr2 = pr3;

			for(j=0; j<mLen; j++)
			{
				//A_In[j*nLen+i].real = A[j].real/mLen;           //���任�������ԭ������
				//A_In[j*nLen+i].image = A[j].image/mLen;

				pr2->real = pr1->real >> M;
				pr2->image = pr1->image >> M;
				pr1++;
				pr2 += nLen;
			}
		}
	}
	free(A);
}




