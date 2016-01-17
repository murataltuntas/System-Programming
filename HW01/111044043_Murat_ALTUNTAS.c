/*
 *				 111044043_Murat_ALTUNTAS.c 					*
 *																*
 *		   				grep programi							*
 *																*
 *	usage:$  ./111044043_Murat_ALTUNTAS - filename "string"		*
 *																*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int boyutBelirle(char *file);

int main(int argc, char **argv)
{
	/* 1. 2. yada 3. arguman eksik ise hata mesaji verir ve program sonlanir. */
	/* calistirilabilir dosya ile file name arasinda  -  olmalidir. */
	if ((argv[1] == NULL) || (argv[2] == NULL) || (argv[3] == NULL) || ('-' != argv[1][0]))
	{
		printf("Yanlis Kullanim\n");
		printf("Dogru yazim sekli\n");
		printf(" ./111044043_Murat_ALTUNTAS - filename \"string\" \n");
	    return(-1);
	}

	int boyut; /* max satir boyutu */
	char *str; /* Line okumak icin gerekli degisken. */

	boyut = boyutBelirle(argv[2]);
	
	/* Hafizadan yer alinir. */
	str=(char *)malloc(((boyut+1)*sizeof(char)));
	
	FILE * inp;
	char *c; /* Okunan satirin atanacagi degisken */
	char *external; /* aranacak kelime icin kullanilan degisken */
	int i=0,j=0; /* Donguler icin gerekli degiskenler. */
	int count1=1,count2=0,count3=0, sayac=0; /* Gerekli sayaclar */
	int uzunluk; /* aranacak kelimenin uzunlugu icin */

	/* Aranacak kelime */
	external = argv[3];

	/* aranacak kelimenin uzunlugu */
	uzunluk = strlen(argv[3]);

	/* Dosya acilir */
	inp=fopen(argv[2],"r");

	/* Dosya acilmazsa yada yok ise hata verir ve program sonlanir */
	if (inp == NULL)
	{
		perror("Error opening file");
	    return(-1);
	}

	/* Dosya sonuna kadar donen dongu */
	do{
		/* Satir okunur */
		c = fgets(str,(boyut + 1),inp);

		while(str[j]!='\0') /* okunan satirin son karakterine kadar (NULL) donen dongu. */
			{
				/* aranan Argumanin karakterleri ile aradigimiz satirdaki
					karakterler uyusuyormu diye kontrol edilir. */
				if(external[i]==str[j])
					{
						i++;
					}
				else
					{
						i=0;
					}

				if(external[i]=='\0') /* Aranan Argumanin son karakterine (NULL) ulasildiysa , arguman 
										bulunmustur demektir ve bulundugu satir numarasi ekrana basilir. */
				{
					printf("Line Number: %d\n",count1);
					i=0;
					j = j - uzunluk + 1;
					count3++;
					count2++;
				}
				j++;
			}

			/* Stringlerin Bulundugu Satir Sayisini hesaplar */
			if(count2 >= 1)
				sayac++;

			i=0;
			j=0;
			count1 ++; 
			count2 = 0;

	}while(c != NULL);

	printf("Toplam Bulunan String Sayisi: %d \n", count3);
	printf("Stringlerin Bulundugu Satir Sayisi: %d\n", sayac);

	/* Dosya kapatilir */
	fclose(inp);

	/* Hafizadan Alinan Yer Hafizaya Tekrar Geri Verilir */
	free(str);

	return 0; 
}

/* En uzun satirin kac karakterden olustugunu belirler */
int boyutBelirle(char *file)
{
	FILE * fp;
	
	/* Dosya acilir */
	fp=fopen(file,"r");
	
	/* Dosya acilmazsa yada yok ise hata verir ve program sonlanir */
	if (fp == NULL)
	{
		perror("Error opening file");
	    exit(1);
	}

	int max=0; /* Maximum satir uzunlugu */
	int say=1; /* gerekli sayac */
	int eoF; /* (End Of File) while dongusu icin */
	char c;

	eoF = fscanf(fp,"%c",&c);

	/* dosya sonuna kadar okuma yapar */
	while(eoF != EOF)
	{
		eoF = fscanf(fp,"%c",&c);
		say++;

		/* Satir sonuna geldiginde satirdaki karakter sayisini degiskene atar ve sayaci sifirlar */
		if ((c == '\n')||(c == '\0'))
		{
			if (max < say)
			{
				max = say;
			}
			say = 0;
		}
	}
	
	/* Dosya kapatilir */
	fclose(fp);

	return max;
}
/*######## End Of The Program #######*/