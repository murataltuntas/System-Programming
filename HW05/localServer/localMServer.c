/*                          *
 *       Murat ALTUNTAS     *
 *         111044043        *
 *      wheel of function   *
 *         localMServer     */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <math.h>

#define BUFSIZE 4096 /* Buffer Size */
#define FUNC_FILE "function.dat" /* Fonksiyon Dosyasi */

/* Signal Handle function */
void sigHandle(int sig);
/* olusturulan thread e gonderilern void pointer fonksiyon */
void* changeValue(void* data);
/* Average calculate function */
double average();
/* Mathematical String Parsing Function*/
double parsing(double doubleValue, char func[]);
/* 4 islem fonksiyonu */
double islem(char operator, double lValue, double rValue);

/* Shared memory de paylasilan structer */
struct myWheel{
		int intSize; /* carkin boyutu */
		int intRand;/* Random sayi uretmek icin gerekli degisken */
		double* wheel; /* cark */
};

FILE* filePointer;/* function.dat okumak icin */
FILE* fpServer; /* server logfile icin olusturulan file pointer */
pthread_t  thread_id[BUFSIZE]; /* Thread ID */
char LOGSERVER[64]; /* server logfile name */
char strFunction[BUFSIZE]; /* Fonksiyon stringi */
int intNumOfThread; /* Thread sayisi */
int intThreadCount = 0; /* olusan her thread i sayan counter */
int intFdLogFl[BUFSIZE]; /* threadlerin acilan logfilelarinin file decriptlerinin  tutuldugu array */
double* doubleCark; /* wheel (cark) */
sem_t semlock; /* semaphore */

int main(int argc, char *argv[])
{
	/* calistirilirken 4 arguman olmasi gerekir */
    if ( argc !=  4)
    {
        fprintf(stderr, "Usage: %s <buffer size> <#ofThreads> <update time>\n", argv[0]);
        exit(-1);
    }
	
	int i,j; /* Donguler icin */
	int argSize = atoi(argv[1]); /* 1. arguman buffer size ( carktaki parca sayisi ) */
	intNumOfThread = atoi(argv[2]); /* 2. arguman thread sayisi */
	int sleepTime = atoi(argv[3]); /* 3. arguman ortalamanin alinacagi zaman araligi. */
	long zamanFark , difTime; /* zaman farki hesabi icin */
	int err; /* error return value */
	int shmid; /* shared memory olusturmak icin */
	key_t key; /* shared memory icin gerekli key */
	time_t curtime; /* Serverin calismaya basladigi zamani verir */
	struct timeval basla , bitis ; /* Zaman hesaplari icin */
	struct timeval start , finish ; /* Zaman hesaplari icin */
	struct myWheel* shm; /* wheel structer */

	signal( SIGINT, sigHandle); /* Signal */

	key = 6985; /* Keye rastgele bir sayi atanir. */

	/* shared memory getter */
	if ((shmid = shmget(key, sizeof(struct myWheel), (0666 | IPC_CREAT))) < 0) {
		perror("shmget");
		return 1;
	}

	/* shared memory at. */
	if ((shm = (struct myWheel*)shmat(shmid, NULL, 0)) == (void *) -1) {
		perror("shmat");
		return 1;
	}

	/* Malloc ile buffer size kadar yer alinir */
    doubleCark = (double*)malloc(argSize * sizeof(double));
    
	/* Shared memorydeki struct doldurulur */
	shm->intSize = argSize;
	srand(time(NULL)); /*herhangi bir sayi cikmasi icin gerekli kod */
	shm->intRand=rand()%(atoi(argv[1]));
    shm->wheel = doubleCark;
    
	for (i = 0; i < shm->intSize; ++i)
	{
		shm->wheel[i] = (double)((i+1)/(double)(shm->intSize + 1));
	}

	/* Fonksiyonun alinacagi dosya acilir. */
	/* Dosya acilmazsa hata verir ve program sonlanir */
    if ( (filePointer = fopen(FUNC_FILE,"r")) == NULL)
    {
        perror("Error opening file function.dat");
        exit(1);
    }

    /* Dosyadan fonksiyon stringi okunur */
    fscanf(filePointer,"%s",strFunction);
    /* dosya kapanir */
    fclose(filePointer);

    /* Server log adi olusturulur */
    sprintf(LOGSERVER,"SERVER_%ld.log",(long)getpid());
    /* Ortalama degerlerin yazilacagi server logfile acilir */
    fpServer = fopen(LOGSERVER,"a");
    /* Dosya acilmazsa hata verir ve program sonlanir */
    if (fpServer == NULL)
    {
        perror("Error opening file");
        exit(1);
    }

    /* server log file'ina baglanti zamani yazilir */
    fprintf(fpServer,"**Birth Time: %s",ctime (&curtime));

	/* Semaphore initialization*/
	if (sem_init(&semlock, 0, 1) == -1) {
		perror("Failed to initialize semaphore");
		return 1;
	}

	/* Threadler olusturulur */
	for(i=0; i<intNumOfThread; i++)
	{
		if (err = pthread_create(thread_id + i, NULL, changeValue, &semlock))
		{
			fprintf(stderr, "Failed to create thread: %s\n", strerror(err));
			return 1;
		}
		pthread_detach(thread_id[i]);
	}
		
	fprintf(fpServer,"%d Pieces of Thread Created.\n",intNumOfThread);
	fprintf(fpServer," Average      Time\n");

	gettimeofday(&basla,NULL);	/* Zaman baslangici */
	gettimeofday(&start,NULL);	/* Zaman baslangici */

	while(1)
	{
			gettimeofday(&bitis,NULL); /* zaman bitisi */
			/* zaman farki */
			zamanFark = (bitis.tv_sec-basla.tv_sec)*1000 + (bitis.tv_usec - basla.tv_usec)/1000;

			if (zamanFark >= sleepTime)
			{
				gettimeofday(&finish,NULL); /* zaman bitisi */
				/* zaman farki */
				difTime = (finish.tv_sec - start.tv_sec)*1000 + (finish.tv_usec - start.tv_usec)/1000;
				/* her update time da bir ortalama alinip logfile yazilir*/
				fprintf(fpServer,"%f        %ld\n",average(),difTime);
				srand(time(NULL)); /*herhangi bir sayi cikmasi icin gerekli kod */
				shm->intRand=rand()%(shm->intSize); /* cark random cevrilir */
				gettimeofday(&basla,NULL) ;	/* Zaman baslangici */
			}

		/* Semaphore unlock */
		if (sem_post(&semlock) == -1)
		fprintf(stderr, "Thread failed to unlock semaphore\n");
		usleep(100);
	}

	/* Shared Memory detached */
	if(shmdt(shm) != 0)
	fprintf(stderr, "Could not close memory segment.\n");

return 0;
}

/* Mathematical String Parsing Function*/
double parsing(double doubleValue, char func[])
{
    char ek[BUFSIZE];/* kullanilacak array */
    char yeni_1[BUFSIZE];/* gelen fonksiyonun duzenlenmis halini tutar */
    int intStrng; /* stringin uzunlugu icin */
    int intEklenecek; /* x yerine yazilacak sayinin karakter sayisini tutar */
    int i,j=0,k=0,l; /* Donguler icin */
    int intXCount=0; /* Stringdeki 'x' sayisini tutar */
    char* strFunc[3]; /* strtok ile parcalanan stringi tutmak icin */
    char* strSayi[2]; /* parsingde 1. kisimdaki sayilari tutmak icin */
    char* strSayi_2[2]; /* parsingde 2. kisimdaki sayilari tutmak icin. */
    char charOperator; /* 1. operator */
    char charOperator_2; /* 2. operator */
    double doubleResult; /* sonuc */

    memset(yeni_1,'\0',BUFSIZE * sizeof(char));
    memset(ek,'\0',BUFSIZE * sizeof(char));    

    /******** Degisim *********/
    sprintf(ek,"%f",doubleValue);
    intStrng=strlen(func);/* stringin uzunlugu */
    intEklenecek=strlen(ek); /* eklenecek stringin uzunlugu */

    for(i=0; i<intStrng; i++)
    {
        if(func[i] == 'x')
        {
            intXCount++; /* Stringdeki 'x' sayisi */
        }   
    }

    for(i=0; i<intStrng; i++)
    {
        if(func[i] == 'x')
        {
            for (k = 0; k < intEklenecek; ++k)
            {
                yeni_1[j]=ek[k];/* stringdeki 'x' yerine zaman yazilir. */
                j++;
            }
        }
        else
        {
            yeni_1[j]=func[i];
            j++;
        }
        yeni_1[j]='\0';/* en sona \0 yazilir. */
    }

    /*****************/
    /* strtok ile ilk parantezden bolunur */
    strFunc[0] = strtok (yeni_1,"(");
    
    /****** pow ******/
    if(!(strcmp(strFunc[0],"pow")))
    {    
        strFunc[1] = strtok (NULL, ",");
        strFunc[2] = strtok (NULL, ")");
      
        /****** 1. Kisim *****/
        
        for (i = 0; i < strlen(strFunc[1]); ++i)
        {
            if((strFunc[1][i] == '-') || (strFunc[1][i] == '+') || (strFunc[1][i] == '*') || (strFunc[1][i] == '/'))
            {
                charOperator = strFunc[1][i];
                break;
            }
        }
        
        /* Strtok ile operatorden bolunur */
        strSayi[0] = strtok(strFunc[1],"-+/*");
        strSayi[1] = strtok(NULL,"\0");

        /****** 2. Kisim *****/

        for (i = 0; i < strlen(strFunc[2]); ++i)
        {
            if((strFunc[2][i] == '-') || (strFunc[2][i] == '+') || (strFunc[2][i] == '*') || (strFunc[2][i] == '/'))
            {
                charOperator_2 = strFunc[2][i];
                break;
            }
        }
        
        /* Strtok ile operatorden bolunur */
        strSayi_2[0] = strtok(strFunc[2],"-+/*");
        strSayi_2[1] = strtok(NULL,"\0");

        /* Sonuc */
        doubleResult = pow(islem(charOperator, atof(strSayi[0]), atof(strSayi[1])),islem(charOperator_2, atof(strSayi_2[0]), atof(strSayi_2[1])));
    }
    
    /****** sqrt ******/
    if(!(strcmp(strFunc[0],"sqrt")))
    {    
        strFunc[1] = strtok (NULL, ")");

        for (i = 0; i < strlen(strFunc[1]); ++i)
        {
            if((strFunc[1][i] == '-') || (strFunc[1][i] == '+') || (strFunc[1][i] == '*') || (strFunc[1][i] == '/'))
            {
                charOperator = strFunc[1][i];
                break;
            }
        }
        
        strSayi[0] = strtok(strFunc[1],"-+/*");
        strSayi[1] = strtok(NULL,"\0");

        /* Sonuc */
        doubleResult = sqrt(islem(charOperator, atof(strSayi[0]), atof(strSayi[1])));
    }
    
    /****** sinus ******/
    if(!(strcmp(strFunc[0],"sin")))
    {    
        strFunc[1] = strtok (NULL, ")");
    
        for (i = 0; i < strlen(strFunc[1]); ++i)
        {
            if((strFunc[1][i] == '-') || (strFunc[1][i] == '+') || (strFunc[1][i] == '*') || (strFunc[1][i] == '/'))
            {
                charOperator = strFunc[1][i];
                break;
            }
        }

        strSayi[0] = strtok(strFunc[1],"-+/*");
        strSayi[1] = strtok(NULL,"\0");

        /* Sonuc */
        doubleResult = sin(islem(charOperator, atof(strSayi[0]), atof(strSayi[1])));
    }
    
    /****** cosinus ******/
    if(!(strcmp(strFunc[0],"cos")))
    {    
        strFunc[1] = strtok (NULL, ")");

        for (i = 0; i < strlen(strFunc[1]); ++i)
        {
            if((strFunc[1][i] == '-') || (strFunc[1][i] == '+') || (strFunc[1][i] == '*') || (strFunc[1][i] == '/'))
            {
                charOperator = strFunc[1][i];
                break;
            }
        }
        
        strSayi[0] = strtok(strFunc[1],"-+/*");
        strSayi[1] = strtok(NULL,"\0");

        /* Sonuc */
        doubleResult = cos(islem(charOperator, atof(strSayi[0]), atof(strSayi[1])));
    }

    return (doubleResult);
}

/* 4 islem fonksiyonu */
double islem(char operator, double lValue, double rValue)
{
    switch(operator)
    {
        case '+': /* Toplama islemi */
                return (lValue + rValue);
            break;

        case '-': /* Cikarma islemi */
                return (lValue - rValue);
            break;

        case '*': /* Carpma islemi */
                return (lValue * rValue);
            break;

        case '/': /* Bolme islemi */
                return (lValue / rValue);
            break;

        default:
            perror("Hatali islem:");
            exit (0);
    }
}

/* Average calculate function */
double average()
{
	int j; /* Dongu icin */
	struct myWheel *shm; /* wheel structer */
	key_t key;
	int shmid; /* shared memory olusturmak icin */
	double doubleSum=0; /* Toplam icin */
	double doubleAverage=0; /* Ortalama icin */
	key = 6985;

	if ((shmid = shmget(key, sizeof(struct myWheel), (0666 | IPC_CREAT))) < 0) {
		perror("shmget");
	}


	if ((shm = (struct myWheel*)shmat(shmid, NULL, 0)) == (void *) -1) {
		perror("shmat");
	}

	/* Ortalama alirken shared memory kitlenir */
	shmctl(shmid, SHM_LOCK ,NULL);

	for (j = 0; j < shm->intSize; ++j)
	{
		doubleSum +=  shm->wheel[j];
	}

	doubleAverage = (doubleSum / shm->intSize);

	printf("Average = %f\n", doubleAverage);

	/* ortalama alindiktan sonra shared memory kilidi acilir */
	shmctl(shmid, SHM_UNLOCK ,NULL);
	
	return doubleAverage;
}

/* olusturulan thread e gonderilern void pointer fonksiyon */
void* changeValue(void* data)
{
	int intCount; /* Thread Counter */
	intCount = intThreadCount;
	intThreadCount++;
	sem_t *semlockp;
	semlockp = (sem_t *)data;
	char LOGTHREAD[128]; /* thread logfile name */
	char str[BUFSIZE]; /* sprintf icin */
	double doubleResult; /* Sonuc */
	key_t key;
	int shmid;
	struct myWheel *shm; /* wheel structer */
	struct timeval start , finish ; /* Zaman hesaplari icin */
	long difTime; /* zaman farki */

	memset(str,'\0',BUFSIZE * sizeof(char));

	key = 6985;

	if ((shmid = shmget(key, sizeof(struct myWheel), (0666 | IPC_CREAT))) < 0) {
		perror("shmget");
	}

	if ((shm = (struct myWheel*)shmat(shmid, NULL, 0)) == (void *) -1) {
		perror("shmat");
	}

	sprintf(LOGTHREAD,"THREAD_%lu.log",pthread_self());

    /* Ortalama degerlerin yazilacagi server logfile acilir */
	if((intFdLogFl[intCount] = open( LOGTHREAD, O_WRONLY | O_APPEND | O_CREAT , S_IRUSR | S_IWUSR)) < 0)
	{
		perror( "Server Failed to create a log file" );	
		exit (0);
	}

	/* write ile logfile a yazilir */
    sprintf(str,"  F(x)         x       Time\n");
    if( write( intFdLogFl[intCount], str ,  strlen(str) ) < 0 )
	{
		perror( "write to LOGFILE" );
		pthread_exit(NULL);
	}

    gettimeofday(&start,NULL);	/* Zaman baslangici */

	while(1)
	{
		while (sem_wait(semlockp) == -1)				/* Entry section */
		if(errno != EINTR) {
			fprintf(stderr, "Thread failed to lock semaphore\n");
			return NULL;
		}
	/****************** start of critical section *******************/
		gettimeofday(&finish,NULL); /* zaman bitisi */
		/* zaman farki */
		difTime = (finish.tv_sec - start.tv_sec)*1000 + (finish.tv_usec - start.tv_usec)/1000;
		
		if (shm->intRand >= shm->intSize)
		{
			shm->intRand = 0;
		}

		doubleResult = shm->wheel[shm->intRand];
		shm->wheel[shm->intRand] = parsing(shm->wheel[shm->intRand],strFunction);
	    sprintf(str,"%f    %f    %ld\n",shm->wheel[shm->intRand],doubleResult,difTime);

	    if( write( intFdLogFl[intCount], str , strlen(str) ) < 0 )
		{
			perror( "write to LOGFILE" );
			return;
		}

		/* index 1 arttirilir */
		shm->intRand++;
	}

	/* Shared Memory detached */
	if(shmdt(shm) != 0)
		fprintf(stderr, "Could not close memory segment.\n");
 
}

/* Signal Handle */
void sigHandle(int sig)
{
	int i; /* donguler icin */

	printf("*** Catch Ctrl ^C ***\n");

	/* Threadler cancel edilir */
	for (i = 0; i < intNumOfThread; ++i)
	{
		pthread_cancel(thread_id[i]);
	}

	/* semaphore destroy edilir */
    if (sem_destroy(&semlock) == -1)
        fprintf(stderr, "Thread failed to destroy semaphore\n");

    /* Server logfile kapatilir. */
	if (fpServer != 0)
	{
		fclose(fpServer);
	}

	/* Thread logfilelari kapatilir */
    for (i = 0; i < intThreadCount; ++i)
    {
        close(intFdLogFl[i]);
    }

    /* Malloc ile alinan yer geri verilir */
    free(doubleCark);
	
	sleep(1);
	exit(0);
}
/******** END OF THE PROGRAM ********/