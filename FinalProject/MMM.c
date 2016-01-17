/****************************************
 *           Murat ALTUNTAS
 *             111044043
 *   Matrix Multiplication Module (MMM)
 ****************************************/

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

#define BUFSIZE 4096 /* Buffer Size */

/* Multiplication Thread */
void* multiplyThread(void* data);
/* Exit Function */
void exitHandleFunc();
/* Matrix Print Function */
void matrix_print(double **matrix, int MATRIX_SIZE);
/* Matrix Multiplication Function */
void multiplyMatrix(double** first, double** second, double** multiply, int size);
/* Signal Handle */
void signalHandle( int sig);

int listenfd = 0, listenfd2 = 0; /* socket descriptor */
int connfd = 0, connfd2 = 0; /* accept descriptor */
double **firstMat, /* inverse U Matrix */
       **secondMat, /* inverse L Matrix */
       **multiplyMat, /* Multiply matrix */
       **matris, /* original matrix */
       **invMatris, /* inverse matrix */
       **birimMat; /* unit matrix */
int intSize; /* Matris boyutu  */
int numberOfThreads; /* thread sayisi */
int intThreadCounter = 0; /* Thread Counter */
int totalMatrix;/* Matris sayisi */
char LOGFILE[64]; /* Log file name */
FILE* fdLogFile; /* Log file pointer */
sem_t semlock; /* semaphore */
pthread_t  thread_id[BUFSIZE]; /* Thread ID */
struct timeval basla , bitis ; /* Zaman hesaplari icin */
struct timeval start , finish ; /* Zaman hesaplari icin */
long zamanFark , difTime; /* zaman farki hesabi icin */
int intTrace; /* Printf ler icin */

int main(int argc, char *argv[])
{
    struct sockaddr_in serv_addr;
    struct sockaddr_in serv_addr2;
    pid_t  pid;
    int err;
    
    /* Ctrl + C   sinyali yakalanir */
    signal( SIGINT, signalHandle);
    signal( SIGPIPE, signalHandle);
    
    /* mod1 ve mod2 arasinda */
    listenfd2 = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr2, '0', sizeof(serv_addr2));
    serv_addr2.sin_family = AF_INET;
    serv_addr2.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr2.sin_port = htons(8888);

    if(bind(listenfd2, (struct sockaddr*)&serv_addr2, sizeof(serv_addr2)) == -1)
    {
        perror(" Error  Bind : ");
        return 1;
    }
    listen(listenfd2, 10);
    /**********************************************/

    /* mod2 ve mod3 arasinda */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(7777); 

    if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
    {
        perror(" Bind Error: ");
        return 1;
    }
    listen(listenfd, 10);
    /***********************************/

    int counterSize=0;
    
    int i, j, k;
    
    /* log file adi olusturulur */
    sprintf(LOGFILE,"logFile_Multiplication_%ld.log",(long)getpid());
    /* Matrislerin yazilacagi logfile acilir */
    fdLogFile = fopen(LOGFILE,"a");
    /* Dosya acilmazsa hata verir ve program sonlanir */
    if (fdLogFile == NULL)
    {
        perror("Error opening file \"logFile_Multiplication_.log\"");
        exit(1);
    }

    printf("^-^ CONNECTED ^-^\n");
    printf("< Wait VM and RMP - LUDM - ITM >\n");

    connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); /* MMM - VM*/
    printf("Verifier Module is Connect\n");

    connfd2 = accept(listenfd2, (struct sockaddr*)NULL, NULL); /*MMM - ITM*/
    printf("RMP_LUDM_ITM is Connect\n");

	gettimeofday(&start,NULL);	/* Zaman baslangici */
    
    read(connfd2, &intSize, sizeof(int)); /* ITM den matris size alinir. */
    read(connfd2, &totalMatrix, sizeof(int)); /* ITM den total matris sayisi alinir. */
    read(connfd2, &numberOfThreads, sizeof(int)); /* ITM den uretilecek thread sayisi alinir. */
    read(connfd2, &intTrace, sizeof(int)); /* ITM den printf control alinir. */

    /* Matrislere matris boyutu kadar yer alinir. */
    firstMat = (double**)malloc(sizeof(double*) * intSize);
    secondMat = (double**)malloc(sizeof(double*) * intSize);
    multiplyMat = (double**)malloc(sizeof(double*) * intSize);
    matris = (double**)malloc(sizeof(double*) * intSize);
    invMatris = (double**)malloc(sizeof(double*) * intSize);
    birimMat = (double**)malloc(sizeof(double*) * intSize);

    for(i=0;i<intSize;++i)
    {
        firstMat[i] = (double*)malloc(sizeof(double) * intSize);
        secondMat[i] = (double*)malloc(sizeof(double) * intSize);
        multiplyMat[i] = (double*)malloc(sizeof(double) * intSize);
        matris[i] = (double*)malloc(sizeof(double) * intSize);
        invMatris[i] = (double*)malloc(sizeof(double) * intSize);
        birimMat[i] = (double*)malloc(sizeof(double) * intSize);
    }

    printf("\n");

    /* Semaphore initialization*/
	if (sem_init(&semlock, 0, 1) == -1) {
		perror("Failed to initialize semaphore");
		return 1;
	}

	/* Threadler olusturulur */
	for(i=0; i<numberOfThreads; i++)
	{
		if (err = pthread_create(thread_id + i, NULL, multiplyThread, NULL))
		{
			fprintf(stderr, "Failed to create thread: %s\n", strerror(err));
			exitHandleFunc();
			return 1;
		}
		pthread_detach(thread_id[i]);
	}

	while(1);
    return 0;
}

/* Matrix Multiplication Function */
void multiplyMatrix(double** first, double** second, double** multiply, int size)
{
    int c, d, k;
    double sum = 0; /* Toplam */

    for ( c = 0 ; c < size ; c++ )
    {
        for ( d = 0 ; d < size ; d++ )
        {
            for ( k = 0 ; k < size ; k++ )
            {
                sum = sum + first[c][k]*second[k][d];
            }

            multiply[c][d] = sum;
            sum = 0;
        }
    }
}

/* print matrix */
void matrix_print(double **matrix, int MATRIX_SIZE)
{
    int row, column;

    for(row = 0; row < MATRIX_SIZE; row++) {
        for(column = 0; column < MATRIX_SIZE; column++) {
            fprintf(fdLogFile,"%f\t", matrix[row][column]);
        }
        fprintf(fdLogFile,"\n");
    }
}
/* Multiplication Thread */
void* multiplyThread(void* data)
{
	int i,j;
	double doubleEleman; /* Matrisin elemanlari */

	while(1)
	{	
		while (sem_wait(&semlock) == -1) /* Entry section */
		if(errno != EINTR) {
			fprintf(stderr, "Thread failed to lock semaphore\n");
			return NULL;
		}
			/* yapilan islem sayisi toplam uretilen matris sayisina ulasinca biter. */
		if ( intThreadCounter < totalMatrix)
		{
				
			gettimeofday(&basla,NULL);	/* Zaman baslangici */

			if (intTrace == 1)
	   			printf("-> %d. Multiplication operation was performed \n", (intThreadCounter + 1));

	    	fprintf(fdLogFile,"\n/########################## %d. Matrix ############################/\n",(intThreadCounter + 1));
	        /* MMM ve ITM arasinda */
	        fprintf(fdLogFile,"U' matrisi\n----------\n");
	        /* U' matrisi ITM den alinir */
	    	for (i = 0; i < intSize; ++i)
	            for (j = 0; j < intSize; ++j)
	            {
	        		read(connfd2, &doubleEleman, sizeof(double));
	        		firstMat[i][j] = doubleEleman;
	        	}
	        matrix_print(firstMat, intSize);

	        fprintf(fdLogFile,"L' matrisi\n----------\n");
	        /* L' matrisi ITM den alinir */
	    	for (i = 0; i < intSize; ++i)
	            for (j = 0; j < intSize; ++j)
	            {
	                read(connfd2, &doubleEleman, sizeof(double));
	                secondMat[i][j] = doubleEleman;
	            }
	        matrix_print(secondMat, intSize);

	        /* Matrisler carpilir */
	        multiplyMatrix(firstMat, secondMat, multiplyMat, intSize);

	        /* carpilan matris ITM e gonderilir. */
	       	for (i = 0; i < intSize; ++i)
	            for (j = 0; j < intSize; ++j)
	                write(connfd2, &multiplyMat[i][j] , sizeof(double));
		/***************************************/
	           /* MMM ve VM arasinda */
	        fprintf(fdLogFile,"A matrisi\n----------\n");
	        /* original matrix VM den alinir */
		    for (i = 0; i < intSize; ++i)
		        for (j = 0; j < intSize; ++j)
		        {
		            read(connfd, &doubleEleman, sizeof(double));
		            matris[i][j] = doubleEleman;
		        }
		    matrix_print(matris, intSize);

		    fprintf(fdLogFile,"A' matrisi\n----------\n");
		    /* inverse matrix VM den alinir */
		    for (i = 0; i < intSize; ++i)
		        for (j = 0; j < intSize; ++j)
		        {
		            read(connfd, &doubleEleman, sizeof(double));
		            invMatris[i][j] = doubleEleman;
		        }
		    matrix_print(invMatris, intSize);

		    /* VM den alinan matrisler carpilir */
		    multiplyMatrix(matris, invMatris, birimMat, intSize);

		    /* carpilan matris VM e gonderilir. */
		    for (i = 0; i < intSize; ++i)
		        for (j = 0; j < intSize; ++j)
		            write(connfd, &birimMat[i][j] , sizeof(double));

		/**********************************************/
			usleep(10);
			gettimeofday(&bitis,NULL); /* zaman bitisi */
			/* zaman farki */
			zamanFark = (bitis.tv_sec-basla.tv_sec)*1000 + (bitis.tv_usec - basla.tv_usec)/1000;

			if (intTrace == 1)
				printf("\tThe time taken to process %d : %ld milisecond\n",(intThreadCounter +1),zamanFark);

		}else{
				exitHandleFunc();
			}

		/* Yapilan thread islemi sayisi, her islemde 1 arttirilir. */
		intThreadCounter++;

		/* Semaphore unlock */
		if (sem_post(&semlock) == -1)
		fprintf(stderr, "Thread failed to unlock semaphore\n");
		usleep(100);
	}
}

/* Signal Handle */
void signalHandle( int sig)
{
	int i; /* for loop */

	/* islemlerin durmasi icin gerekli isaret gonderilir. */
	intThreadCounter = totalMatrix;

	/* threadler detach edilir. */
	for (i = 0; i < numberOfThreads; ++i)
	{
		pthread_detach(thread_id[i]);
	}
    printf("\n***** SIGNAL CATCH *****\n");
    printf("***** DISCONNECTED *****\n");

    exit (1);       
}

/* Exit Function */
void exitHandleFunc()
{
	int i;

	/* threadler detach edilir. */
	for (i = 0; i < numberOfThreads; ++i)
	{
		pthread_detach(thread_id[i]);
	}

	/* malloc ile alinan yerler geri verilir. */
    for (i=0;i<intSize;i++)
	{
	    free(firstMat[i]);
	    free(secondMat[i]);
	    free(multiplyMat[i]);
	    free(matris[i]);
	    free(invMatris[i]);
	    free(birimMat[i]);
	}

	free(firstMat);
	free(secondMat);
	free(multiplyMat);
	free(matris);
	free(invMatris);
	free(birimMat);

	printf("\nOperations Have Completed\n");
    gettimeofday(&finish,NULL); /* zaman bitisi */
	/* zaman farki */
	difTime = (finish.tv_sec - start.tv_sec)*1000 + (finish.tv_usec - start.tv_usec)/1000;

	if (intTrace == 1)
		printf("\nTotal elapsed time : %ld milisecond\n",difTime);

	/* logfile kapatilir. */
    if (fdLogFile != 0)
    {
        fclose(fdLogFile);
    }

	sleep(1);

	/* socketler kapatilir. */
	close(connfd);
	close(connfd2);
	close(listenfd2);
	close(listenfd);   
    
    exit(1);
}
/*############### End Of Program #################*/