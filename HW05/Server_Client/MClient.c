/*                          *
 *       Murat ALTUNTAS     *
 *         111044043        *
 *      wheel of function   *
 *          MClient         */

#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <math.h>
 
#define BUFSIZE 4096 /* Buffer Size */
#define FUNC_FILE "function.dat"

/* olusturulan thread e gonderilern void pointer fonksiyon */
void* changeValue(void* data);
/* Signal Handle function */
void sigHandle(int sig);
/* Mathematical String Parsing Function*/
double parsing(double doubleValue, char func[]);
/* 4 islem fonksiyonu */
double islem(char operator, double lValue, double rValue);

struct myWheel{
		int intSize; /* carkin boyutu */
        int intRand;/* Random sayi uretmek icin gerekli degisken */
        double wheel[BUFSIZE]; /* cark */
        int intClientID; /* Client ID */
        int clientCount; /* Client Counter */
        int intClient_control; /* Clientin baglandigini gosteren control degiskeni */
};

int intNumOfThread; /* thread sayisi */
sem_t semlock; /* semaphore */
pthread_t  thread_id[BUFSIZE]; /* Thread ID */
char LOGCLIENT[64]; /* client logfile name */
FILE* fpClient; /* client logfile icin olusturulan file pointer */
FILE* filePointer; /* function.dat okumak icin */
char strFunction[BUFSIZE]; /* Fonksiyon stringi */


int main(int argc, char *argv[])
{
	
    if ( argc !=  2)
    {
        fprintf(stderr, "Usage: %s <#ofThreads>\n", argv[0]);
        exit(-1);
    }

    signal( SIGINT, sigHandle); /* Signal */
    printf("CONNECTED %d\n",getpid());

    int i; /* Donguler icin */
	int shmid; /* shared memory olusturmak icin */
	key_t key; /* shared memory icin gerekli key */
	int err; /* return value */
	intNumOfThread = atoi(argv[1]); /* 1. arguman thread sayisi */
	struct timeval start , finish ; /* Zaman hesaplari icin */
	struct myWheel* shm; /* wheel structer */
	
	/*
	* We need to get the segment named
	* "6012", created by the server.
	*/
	key = 6012;
 
	/*
	* Locate the segment.
	*/
 	if ((shmid = shmget(key, sizeof(struct myWheel), (0666 | IPC_CREAT))) < 0) {
		perror("shmget");
		return 1;
	}

	/*
	* Now we attach the segment to our data space.
	*/
	if ((shm = (struct myWheel*)shmat(shmid, NULL, 0)) == (void *) -1) {
		perror("shmat");
		return 1;
	}

    /* Shared memorydeki struct doldurulur */
	shm->intClientID = getpid();
	shm->clientCount++;
	shm->intClient_control = 1;

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
 
	sprintf(LOGCLIENT,"CLIENT_%ld.log",(long)getpid());
    /* Ortalama degerlerin yazilacagi server logfile acilir */
    fpClient = fopen(LOGCLIENT,"a");
    /* Dosya acilmazsa hata verir ve program sonlanir */
    if (fpClient == NULL)
    {
        perror("Error opening file");
        exit(1);
    }

    fprintf(fpClient,"   Thread ID          x       F(x)\n");

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

	while(1)
	{
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
        
        for (i = 0; i < 2; ++i)
        {
            printf("%s\n", strFunc[i]);
        }

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

        doubleResult = cos(islem(charOperator, atof(strSayi[0]), atof(strSayi[1])));

    }

    return (doubleResult);
}

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

/* olusturulan thread e gonderilern void pointer fonksiyon */
void* changeValue(void* data)
{
	sem_t *semlockp;
	semlockp = (sem_t *)data;
	key_t key;
	int shmid;
	struct myWheel *shm; /* wheel structer */
	struct timeval start , finish ; /* Zaman hesaplari icin */
	long difTime; /* zaman farki */
	double doubleResult; /* Sonuc */

	key = 6012;

	if ((shmid = shmget(key, sizeof(struct myWheel), (0666 | IPC_CREAT))) < 0) {
		perror("shmget");
	}

	if ((shm = (struct myWheel*)shmat(shmid, NULL, 0)) == (void *) -1) {
		perror("shmat");
	}

    gettimeofday(&start,NULL);	/* Zaman baslangici */

	while(1)
	{
	
		sem_wait(semlockp);
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
/*		printf("Size= %d , thread id= %lu \n", shm->intRand,pthread_self());  */
		fprintf(fpClient,"%lu    %f    %f\n",pthread_self(), doubleResult,shm->wheel[shm->intRand]);
	    
        /* index 1 arttirilir */
		shm->intRand++;
	}

    /* Shared Memory detached */
	if(shmdt(shm) != 0)
		fprintf(stderr, "Could not close memory segment.\n");
}

void sigHandle(int sig)
{
	int i;
	printf("*** Catch Ctrl ^C ***\n");

    /* Threadler cancel edilir */
	for (i = 0; i < intNumOfThread; ++i)
	{
		pthread_cancel(thread_id[i]);
	}

    /* semaphore destroy edilir */
	if (sem_destroy(&semlock) == -1)
		fprintf(stderr, "Thread failed to destroy semaphore\n");
	
    /* Client logfile kapatilir. */
	if (fpClient != 0)
	{
		fclose(fpClient);
	}

	sleep(1);
	exit(sig);
}
/******** END OF THE PROGRAM ********/