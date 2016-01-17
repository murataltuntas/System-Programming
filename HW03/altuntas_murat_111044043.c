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
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <signal.h>
#include <termios.h> 
 
#define BUFSIZE 1024

int bulBeni(char *arguman, char *sstring, int lineNum);
int boyutBelirle(char *file); /* Aranan dosyanin en uzun satirinin kac karakterden olustugunu dondurur. */
int grep(char *fileName, char *stringg, int fd[2]); /* arama fonksiyonu */
void signalChild( int sig); /* Signal */

static volatile sig_atomic_t doneflag = 0;
/* ARGSUSED */
static void setdoneflag(int signo)
{
    doneflag = 1;
}

int main (int argc, char * argv[])
{
    int line;

    /* termios.h kutuphanesi ile getchar() kullanilarak anlik karakter okuma da kullanilan islevler.*/
    static struct termios oldt, newt;
    tcgetattr( STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON);          
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);

    /* Signal icin gerekli kisim. */
    struct sigaction act;
    act.sa_handler = setdoneflag; /* set up signal handler */
    act.sa_flags = 0;
    if ((sigemptyset(&act.sa_mask) == -1) || (sigaction(SIGINT, &act, NULL) == -1))
    {
        perror("Failed to set SIGINT handler");
        return 1;
    }

    /* calistirilirken 6 arguman olmasi gerekir */
    if ( argc != 6 )
    {
        fprintf(stderr, "Usage: %s directory_name -g string -l <Line#>\n", argv[0]);
        printf("OR\n");
        fprintf(stderr, "Usage: %s directory_name -l <Line#> -g string\n", argv[0]);
        exit(-1);
    }

    /* 1. siralama sekli */
    if((!strcmp(argv[2],"-g"))&&(!strcmp(argv[4],"-l")))
    {
        line = atoi(argv[5]);

        if(line < 0)
        {
            line *= (-1);
        }

        if(line == 0)
        {
           printf("Line 0 yada karakter olamaz\n");
           return 0;
        }

        bulBeni(argv[1], argv[3], line);

    } /* 2. siralama sekli */
    else if((!strcmp(argv[4],"-g"))&&(!strcmp(argv[2],"-l")))
    {
        line = atoi(argv[3]);

        if(line < 0)
        {
            line *= (-1);
        }

        if(line == 0)
        {
           printf("Line 0 yada karakter olamaz\n");
           return 0;
        } 
        bulBeni(argv[1], argv[5], line);
    }
    else
    {
        fprintf(stderr, "Usage: %s directory_name -g string -l <Line#>\n", argv[0]);
        printf("OR\n");
        fprintf(stderr, "Usage: %s directory_name -l <Line#> -g string\n", argv[0]);
        exit(-1);
    }

	tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
    return 0;
}  /* end main */

int bulBeni(char *arguman, char *sstring, int lineNum)
{
    struct stat stDirInfo;
    struct dirent * stFiles;
    DIR * stDirIn;
    pid_t  pid;
    char szFullName[MAXPATHLEN]; /* dosya isimleri icin */
    char szDirectory[MAXPATHLEN];
    struct stat stFileInfo;
	int fd[2]; /* For pipe */
	char bufin[BUFSIZE];
    int countLess=0; /* for  Less count */
    char chr; /* for getchar() */

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
                bulBeni(szFullName, sstring, lineNum);
            }
        }
        else /* dosya ise */
        {
            /* pipe */
        	if (pipe(fd) == -1)
            {
                perror("Failed to create the pipe");
                exit(EXIT_FAILURE);
            }
            
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
                printf("\npid:%ld , %s\n",(long)getpid(),szFullName);
                /* Ctrl + C   sinyali yakalanir */
                signal( SIGINT, signalChild );
                /* grep fonksiyonu cagirilarak islemler yaptirilir. */
                grep(szFullName, sstring, fd);
                exit(1); /* exit ile prosesler oldurulur. */
            }
            else /* parent */
            {
                /* childin pipe a yazdigini parent okur */
	       	    close(fd[1]);
    	        while(read(fd[0],bufin,BUFSIZE) > 0)
                {
                    if(countLess == lineNum )
                    {
                        chr = getchar(); /* q ya basilinca program biter */
                        if((chr == 'q')||( chr== 'Q'))
                            exit(1);
                        countLess = 0;
                    }    
    	  	     	printf("%s",bufin);
                    countLess++;
    	  	    }

                /* Sinyal geldiginde if e girer. */
                if (doneflag)
                {
                    /* Pipe yazilanin geri kalaninida okur ve programi bitirir. */
                    close(fd[1]);
                    while(read(fd[0],bufin,BUFSIZE) > 0)
                    {
                        printf("%s",bufin);
                    }
                    printf("\n***** Parent killed. *****\n");
                    exit( doneflag );
                }

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

int grep(char *fileName, char *stringg, int fd[2])
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
	char buf[BUFSIZE]; /* for write */

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

    			sprintf(buf,"Pid= %ld , File: %s Line Number: %d\n",(long)getpid(),fileName,count1);
                close(fd[0]);
                write(fd[1], buf, BUFSIZE ); /* write ile pipe yazilir */
                  	     	
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
        sprintf(buf,"Toplam Bulunan String Sayisi: %d \n",count3);
        close(fd[0]);
        write(fd[1], buf, BUFSIZE );/* write ile pipe yazilir */
    }
    if(sayac != 0)
    {
        sprintf(buf,"Stringlerin Bulundugu Satir Sayisi: %d\n",sayac);
        close(fd[0]);
        write(fd[1], buf, BUFSIZE );/* write ile pipe yazilir */
    }

    /* Dosya kapatilir */
    fclose(inp);

    /* Hafizadan Alinan Yer Hafizaya Tekrar Geri Verilir */
    free(str);

return (count3);
}

/* Signal Handle */
void signalChild( int sig)
{
        printf("\n***** Child killed. *****\n");
        exit( sig );
}
/*############### End Of Program #################*/
