/*                          *
 *       Murat ALTUNTAS     *
 *         111044043        *
 *      wheel of function   *
 *           MServer        */

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
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define BUFSIZE 4096 /* Buffer Size */ 

/* Signal Handle */
void sigHandle(int sig);
/* Average calculate function */
double average();

/* Shared memory de paylasilan structer */
struct myWheel{
		int intSize; /* carkin boyutu */
		int intRand;/* Random sayi uretmek icin gerekli degisken */
		double wheel[BUFSIZE]; /* cark */
		int intClientID; /* Client ID */
		int clientCount; /* Client Counter */
		int intClient_control; /* Clientin baglandigini gosteren control degiskeni */
};

char LOGSERVER[64]; /* server logfile name */
FILE* fpServer; /* server logfile icin olusturulan file pointer */
int intClientArr[BUFSIZE]; /* Client array */
int intClientCount = 0; /* Client counter */
int intSignalControl = 0; /* Signal control */

int main(int argc, char **argv)
{
	if ( argc !=  3)
    {
        fprintf(stderr, "Usage: %s <buffer size> <update time>\n", argv[0]);
        exit(-1);
    }

    /* Signal */
    signal( SIGINT, sigHandle);

	int i; /* donguler icin */
	int argSize = atoi(argv[1]); /* 1. arguman buffer size ( carktaki parca sayisi ) */
	int sleepTime = atoi(argv[2]); /* 2. arguman ortalamanin alinacagi zaman araligi. */
	int shmid; /* shared memory olusturmak icin */
	long zamanFark, difTime; /* zaman farki hesabi icin */
	key_t key; /* shared memory icin gerekli key */
	time_t curtime; /* Serverin calismaya basladigi zamani verir */
	struct timeval basla , bitis ; /* Zaman hesaplari icin */
	struct timeval start , finish ; /* Zaman hesaplari icin */
	struct myWheel* shm; /* wheel structer */
 	
    /*
     * Shared memory segment at 6012
     */
	key = 6012;
 
    /*
     * Create the segment and set permissions.
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
	shm->intSize = argSize;
	srand(time(NULL)); /*herhangi bir sayi cikmasi icin gerekli kod */
	shm->intRand=rand()%(argSize);
	shm->clientCount = 0;
	shm->intClient_control = 0;
	for (i = 0; i < shm->intSize; ++i)
	{
		shm->wheel[i] = (double)((i+1)/(double)(shm->intSize + 1));
	}

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
    fprintf(fpServer,"**Birth Time: %s Average        Time\n",ctime (&curtime));

    gettimeofday(&start,NULL);	/* Zaman baslangici */
	gettimeofday(&basla,NULL);	/* Zaman baslangici */

	while(1)
	{
		/* Clientin baglanma durumunu kontrol eder */
		/* Baglanti durumunda client ID yi array e ekler */
		if(shm->intClient_control == 1)
		{
			printf("CLIENT %d CONNECTED\n", shm->intClientID);
			intClientArr[intClientCount] = shm->intClientID;
			++intClientCount;
			intSignalControl = 1;
			shm->intClient_control = 0;
		}

		gettimeofday(&bitis,NULL); /* zaman bitisi */
		/* zaman farki */
		zamanFark = (bitis.tv_sec-basla.tv_sec)*1000 + (bitis.tv_usec - basla.tv_usec)/1000;

		if (zamanFark >= sleepTime)
		{
			gettimeofday(&finish,NULL); /* zaman bitisi */
			/* zaman farki */
			difTime = (finish.tv_sec - start.tv_sec)*1000 + (finish.tv_usec - start.tv_usec)/1000;
			fprintf(fpServer,"%f        %ld\n",average(),difTime);
			srand(time(NULL)); /*herhangi bir sayi cikmasi icin gerekli kod */
			shm->intRand=rand()%(shm->intSize);
			gettimeofday(&basla,NULL) ;	/* Zaman baslangici */
		}
	}

	/* Shared Memory detached */
	if(shmdt(shm) != 0)
		fprintf(stderr, "Could not close memory segment.\n");
 
	return 0;
}

/* Average calculate function */
double average()
{
	int j; /* Donguler icin */
	struct myWheel *shm; /* wheel structer */
	key_t key;
	int shmid; /* shared memory olusturmak icin */
	double doubleSum=0; /* Toplam icin */
	double doubleAverage=0; /* Ortalama icin */
	key = 6012;

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

/*	printf("Average = %f\n", doubleAverage); */

	/* ortalama alindiktan sonra shared memory kilidi acilir */
	shmctl(shmid, SHM_UNLOCK ,NULL);
	return doubleAverage;
}

/* Signal Handle */
void sigHandle(int sig)
{
	int i; /* donguler icin */

	printf("*** Catch Ctrl ^C ***\n");

	/* Bagli clientler oldurulur */
	if (intSignalControl == 1)
	{
		for (i = 0; i < intClientCount; ++i)
		{
			kill(intClientArr[i],SIGINT);
		}
	}

	/* server logfile kapatilir */
	if (fpServer != 0)
	{
		fclose(fpServer);
	}

	sleep(1);
	exit(sig);
}
/******** END OF THE PROGRAM ********/