/*************************************************************
 *                    Murat ALTUNTAS                         *
 *                      111044043                            *
 *     Bu program bir klasore girip o klasor icindeki        *
 *  tüm dosya ve klasorler icinde aradigimiz kelimeyi arar.  *
 ***************** *******************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>

#define OUTPUT_FILE_NAME "output_result_111044043.txt"

FILE *output;
int bulBeni(char *arguman, char *sstring);
int boyutBelirle(char *file); /* Aranan dosyanin en uzun satirinin kac karakterden olustugunu dondurur. */
int grep(char *fileName, char *stringg); /* arama fonksiyonu */
void print(int lineNumber); /* ekrana basma fonksiyonu */

int main (int argc, char * argv[])
{
    int line;

    output = fopen(OUTPUT_FILE_NAME,"w"); /* butun printlerin icine yazilacagi dosya acilir */

    /* calistirilirken 6 arguman olmasi gerekir */
    if ( argc != 6 )
    {
        fprintf(stderr, "Usage: %s directory_name -g string -l <Line#>\n", argv[0]);
        printf("OR\n");
        fprintf(stderr, "Usage: %s directory_name -l <Line#> -g string\n", argv[0]);
        fclose(output);
        exit(-1);
    }

    /* 1. siralama sekli */
    if((!strcmp(argv[2],"-g"))&&(!strcmp(argv[4],"-l")))
    {
        bulBeni(argv[1], argv[3]);
        line = atoi(argv[5]);

        if(line < 0)
        {
            line *= (-1);
        }

    } /* 2. siralama sekli */
    else if((!strcmp(argv[4],"-g"))&&(!strcmp(argv[2],"-l")))
    {
        bulBeni(argv[1], argv[5]);
        line = atoi(argv[3]);

        if(line < 0)
        {
            line *= (-1);
        }

    }
    else
    {
        fprintf(stderr, "Usage: %s directory_name -g string -l <Line#>\n", argv[0]);
        printf("OR\n");
        fprintf(stderr, "Usage: %s directory_name -l <Line#> -g string\n", argv[0]);
        fclose(output);
        exit(-1);
    }

    fclose(output); /* dosya kapatilir */

	if(line == 0)
	{
	   printf("Line 0 yada karakter olamaz\n");
	   return 0;
	}
	print(line); /* ekrana basilir. */

    return 0;
}  /* end main */

int bulBeni(char *arguman, char *sstring)
{
    struct stat stDirInfo;
    struct dirent * stFiles;
    DIR * stDirIn;
    pid_t  pid;
    char szFullName[MAXPATHLEN]; /* dosya isimleri icin */
    char szDirectory[MAXPATHLEN];
    struct stat stFileInfo;

    strncpy( szDirectory, arguman, MAXPATHLEN - 1 );

    /* Error check */
    if (lstat( szDirectory, &stDirInfo) < 0)
    {
        perror (szDirectory);
        return;
    }

    if (!S_ISDIR(stDirInfo.st_mode))
        return;
    if ((stDirIn = opendir( szDirectory)) == NULL) /* klasor acilir */
    {
        perror( szDirectory );
        return;
    }

    /* klasorun icerigi okunur */
    while (( stFiles = readdir(stDirIn)) != NULL)
    {
        sprintf(szFullName, "%s/%s", szDirectory, stFiles -> d_name );

        if (lstat(szFullName, &stFileInfo) < 0)
           perror ( szFullName );

        /* klasor mu diye bakilir, kalsor ise onunda icine girilir. */
        if (S_ISDIR(stFileInfo.st_mode))
        {
            if ((strcmp(stFiles->d_name , "..")) && (strcmp(stFiles->d_name , ".")))
            {
                /* recursive olarak ic icedosyalara girilebilir. */
                bulBeni(szFullName, sstring);
            }
        }
        else /* dosya ise */
        {
            pid = fork(); /* prosesler oluşturulur */

            /* Error check */
            if(pid == -1)
            {
                perror("Error: Cannot fork \n");
                exit(1);
            }

            /* pid 0 ise child dir. */
            if (pid == 0) 
            {
                printf("pid:%ld , %s\n",(long)getpid(),szFullName);
                /* grep fonksiyonu cagirilarak islemler yaptirilir. */
                grep(szFullName, sstring);
                exit(1); /* exit ile prosesler oldurulur. */
            }
            else
            {
                wait(NULL);/* parent child lari bekler. */
            }
        }

    }  /* end while */
    /* Dosyalar kapatilir. */
    while ((closedir(stDirIn) == -1) && (errno == EINTR)) ;

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

    /* dosya sonuna kadar okuma yapar */
    while(fscanf(fp,"%c",&c) != EOF)
    {
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

int grep(char *fileName, char *stringg)
{
    int boyut; /* max satir boyutu */
    char *str; /* Line okumak icin gerekli degisken. */

    boyut = boyutBelirle(fileName);
    if(boyut == 0)/* dosya bos ise cik */
        return 0;
    
    /* Hafizadan yer alinir. */
    str=(char *)malloc(((boyut+1)*sizeof(char)));
    
    FILE * inp;
    char *c; /* Okunan satirin atanacagi degisken */
    char *external; /* aranacak kelime icin kullanilan degisken */
    int i=0,j=0; /* Donguler icin gerekli degiskenler. */
    int count1=1,count2=0,count3=0, sayac=0; /* Gerekli sayaclar */
    int uzunluk; /* aranacak kelimenin uzunlugu icin */

    /* Aranacak kelime */
    external = stringg;

    /* aranacak kelimenin uzunlugu */
    uzunluk = strlen(stringg);

    /* Dosya acilir */
    inp=fopen(fileName,"r");

    /* Dosya acilmazsa yada yok ise hata verir ve program sonlanir */
    if (inp == NULL)
    {
        perror("Error opening file\n");
        return(-1);
    }

    /* Satir okunur */
    c = fgets(str,(boyut + 1),inp);

    /* Dosya sonuna kadar donen dongu */
    while(c != NULL)
    {
        
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
                fprintf(output,"Pid= %ld , File: %s Line Number: %d\n",(long)getpid(),fileName,count1);
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

        /* Satir okunur */
        c = fgets(str,(boyut + 1),inp);
    }

    if(count3 != 0)
    {
        fprintf(output,"Toplam Bulunan String Sayisi: %d \n", count3);
    }
    if(sayac != 0)
    {
        fprintf(output,"Stringlerin Bulundugu Satir Sayisi: %d\n", sayac);
    }

    /* Dosya kapatilir */
    fclose(inp);

    /* Hafizadan Alinan Yer Hafizaya Tekrar Geri Verilir */
    free(str);

return 0;
}

void print(int lineNumber)
{
    int boyut; /* max satir boyutu */
    char *str; /* Line okumak icin gerekli degisken. */
    char chr;
    int i=0; /* dongu icin gerekli */
    int count=1;

    boyut = boyutBelirle(OUTPUT_FILE_NAME);
    
    /* Hafizadan yer alinir. */
    str=(char *)malloc(((boyut*2)*sizeof(char)));

    FILE *input;
    /* okunacak dosya acilir */
    input = fopen(OUTPUT_FILE_NAME,"r");

   /* okunan satir NULL degilse if e girer */
    if((fgets(str,(boyut*2),input))!= NULL)
    {    
        do{ 
            /* i , kullanicinin girdigi sayidan kucuk oldugu surece if e girer ve ekrana basar. */
            if (i<lineNumber)
            {
                printf("%d::%s",count,str);
                count++;    
            }

            /* Kullanicinin istedigi sayi kadar ekrana basildiktan sonra kullaniciya tekrar sorulur. */
            if (i == (lineNumber-1))
            {
                printf("\n--less-- Press Q to quit --less--\n");    
                scanf("%c",&chr);

                /* girilen karakter Q yada q ise  */
                if((chr == 'q')||(chr == 'Q'))
                {
                    free(str); /* malloc ile alinan yerler sisteme geri verilir */
                    fclose(input); /* dosya kapatilir */
                    remove(OUTPUT_FILE_NAME); /* okunan dosya silinir. */
                    return;
                }
                else
                {
                    i=-1;
                }
            }
            i++;

        }while((fgets(str,(boyut*2),input))!= NULL);
        /* okunan satir NULL olana kadar doner */
    }

    free(str); /* malloc ile alinan yerler sisteme geri verilir */
    fclose(input); /* dosya kapatilir */
    remove(OUTPUT_FILE_NAME);/* okunan dosya silinir. */
}
/*############### End Of Program #################*/
