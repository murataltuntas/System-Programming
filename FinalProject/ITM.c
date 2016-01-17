/*****************************************************************************
 *                  Murat ALTUNTAS
 *                    111044043
 *   Random Matrix Producer - LU Decomposition Module - Inverse Taking Module
 *****************************************************************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <ctype.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>

#define MAX 100
#define MIN -100

/* Creat Matrix Thread */
void* creatMatrixThread(void* data);
/* LU Decompositon Modul Thread */
void* LU(void* data);
/* inverse taking module thread */
void* inverse(void* data);
/* Matrix create function */
void matrix_create(double **matrix, int min_val, int max_val, int MATRIX_SIZE);
/* matrix print function */
void matrix_print(double **matrix, int MATRIX_SIZE);
/* LU decomposition function */
void LU_decomposition(double **l, double **u, double **matrix, int MATRIX_SIZE);
/* inverse matrix function */
void inverseMatrix(double **matrix, int n);
/* Signal Handle */
void signalHandle( int sig);

int matrisSize; /* olusturulacak Matrislerin boyutu */
int sockfd = 0, sockfd2 = 0; /* socket descriptor */
double **matris, /* Asil matris */
       **lMatris, /* L matrisi */
       **uMatris, /* U matrisi */
       **invMatris; /* asil matrisin tersi */
int totalMatrix; /* toplam olusturulacak matris sayisi */
char LOGFILE[64]; /* logfile name */
FILE* fdLogFile; /* log file pointer */
int intTrace; /* Printf ler icin */

int main(int argc, char *argv[])
{
    if(argc != 7)
    {
        printf("\n Usage: %s <# Matris Size> <# total matrices> <# number of threads> <Trace> <ip of MMM> <ip of VM> \n",argv[0]);
        return 1;
    } 

    srand(time(NULL));
    int i, j, k; /* for loop */
   	struct timeval basla , bitis ; /* Zaman hesaplari icin */
	struct timeval start , finish ; /* Zaman hesaplari icin */
	long zamanFark , difTime; /* zaman farki hesabi icin */
	struct sockaddr_in serv_addr; /* MMM socket */
    struct sockaddr_in serv_addr2; /* VM socket */
    struct hostent *hp; /* for getbyhostname (VM) */
    struct hostent *hp2; /* for getbyhostname (MMM) */
    pthread_t thread[3];   /* thread variable */
    double doubleEleman; /* matrislerin elemanlarini okumak icin */
    int numberOfThreads; /* thread sayisi */

	gettimeofday(&start,NULL);	/* Zaman baslangici */

    matrisSize = atoi(argv[1]); /* olusturulacak Matrislerin boyutu */
    totalMatrix = atoi(argv[2]); /* toplam olusturulacak matris sayisi */
    numberOfThreads = atoi(argv[3]); /* olusturulacak thread sayisi */
    intTrace = atoi(argv[4]); /* printf control */

    /* Matrislere matris boyutu kadar yer alinir. */
    matris = (double**)malloc(sizeof(double*) * matrisSize);
    lMatris = (double**)malloc(sizeof(double*) * matrisSize);
    uMatris = (double**)malloc(sizeof(double*) * matrisSize);
    invMatris = (double**)malloc(sizeof(double*) * matrisSize);
    
    for(i=0;i<matrisSize;++i)
    {
        matris[i] = (double*)malloc(sizeof(double) * matrisSize);
        lMatris[i] = (double*)malloc(sizeof(double) * matrisSize);
        uMatris[i] = (double*)malloc(sizeof(double) * matrisSize);
        invMatris[i] = (double*)malloc(sizeof(double) * matrisSize);
    }

    /* log file adi olusturulur */
    sprintf(LOGFILE,"logFile_RMP_LU_ITM_%ld.log",(long)getpid());
    /* Matrislerin yazilacagi logfile acilir */
    fdLogFile = fopen(LOGFILE,"a");
    /* Dosya acilmazsa hata verir ve program sonlanir */
    if (fdLogFile == NULL)
    {
        perror("Error opening file \"logFile_RMP_LU_ITM_.log\"");
        exit(1);
    }

    /* Ctrl + C  ve SIGPIPE sinyali yakalanir */
    signal( SIGINT, signalHandle);
    signal( SIGPIPE, signalHandle);

	/* ITM ve VM arasinda */
    if((sockfd2 = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 
    memset(&serv_addr2, '0', sizeof(serv_addr2)); 
    serv_addr2.sin_family = AF_INET;
    hp = gethostbyname(argv[6]);
    bcopy(hp->h_addr, &(serv_addr2.sin_addr.s_addr),hp->h_length);
    serv_addr2.sin_port = htons(5000); 
    /***********************************/

    /* ITM ve MMM arasinda */
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 
    memset(&serv_addr, '0', sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET;
    hp2 = gethostbyname(argv[5]);
    bcopy(hp2->h_addr, &(serv_addr.sin_addr.s_addr),hp2->h_length);
    serv_addr.sin_port = htons(8888); 
    /***********************************/

    /* MMM e baglandigi connect */
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       return 1;
    }

    /* VM e baglandigi connect */
    if( connect(sockfd2, (struct sockaddr *)&serv_addr2, sizeof(serv_addr2)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       return 1;
    }

    printf("^-^ CONNECTED ^-^\n");

	write(sockfd, &matrisSize , sizeof(int)); /* MMM e matris size gonderilir. */
	write(sockfd, &totalMatrix , sizeof(int));/* MMM e total matris sayisi gonderilir */
    write(sockfd, &numberOfThreads , sizeof(int)); /* MMM e thread sayisi gonderilir */
    write(sockfd, &intTrace , sizeof(int));
	write(sockfd2, &matrisSize , sizeof(int));/* VM e matris size gonderilir. */
	write(sockfd2, &totalMatrix , sizeof(int));/* VM e total matris sayisi gonderilir */
    write(sockfd2, &intTrace , sizeof(int));

	printf("\n");
	/* toplam matris sayisi kadar donen dongu */
    for (k = 0; k < totalMatrix; ++k)
    {
    	gettimeofday(&basla,NULL);	/* Zaman baslangici */

        if (intTrace == 1)
    	   printf("-> %d. Matrix was processed. \n", (k+1));

    	fprintf(fdLogFile,"\n/########################## %d. Matrix ###############################/\n",(k+1));	
				    
        /* Matris olusturan thread */
    	pthread_create(&thread[0], 0, (void*)&creatMatrixThread, NULL); 
	    pthread_join(thread[0],NULL);

	    fprintf(fdLogFile,"A Matrisi\n----------\n");
	    matrix_print(matris, matrisSize);

        /* L ve U bulan thread */
	    pthread_create(&thread[1], 0, (void*)&LU, NULL); 
	    pthread_join(thread[1],NULL);

	    fprintf(fdLogFile,"\nL Matrisi\n----------\n");
	    matrix_print(lMatris, matrisSize);
	    fprintf(fdLogFile,"\nU Matrisi\n----------\n");
	    matrix_print(uMatris, matrisSize);

        /* Invers bulan thread */
	  	pthread_create(&thread[2], 0, (void*)&inverse, NULL);
	    pthread_join(thread[2],NULL);

	    fprintf(fdLogFile,"\nL' Matrisi\n----------\n");
	    matrix_print(lMatris, matrisSize);
	    fprintf(fdLogFile,"\nU' Matrisi\n----------\n");
	    matrix_print(uMatris, matrisSize);

		/*****************************/

            /* U nun tersi MMM ye gonderilir */
		    for (i = 0; i < matrisSize; ++i)
		        for (j = 0; j < matrisSize; ++j)
		            write(sockfd, &uMatris[i][j] , sizeof(double));

            /* L nin tersi MMM ye gonderilir */
		    for (i = 0; i < matrisSize; ++i)
		        for (j = 0; j < matrisSize; ++j)
		            write(sockfd, &lMatris[i][j] , sizeof(double));

            /* MMM den asil matrisin tersi alinir. */
		    for (i = 0; i < matrisSize; ++i)
		        for (j = 0; j < matrisSize; ++j)
		        {        
		            read(sockfd, &doubleEleman, sizeof(double));
		            invMatris[i][j] = doubleEleman;
		        }
		        fprintf(fdLogFile,"\nA' Matrisi\n----------\n");
		        matrix_print(invMatris, matrisSize);

		/*****************************/
            /* VM e asil matrsi gonderilir. */
		    for (i = 0; i < matrisSize; ++i)
		        for (j = 0; j < matrisSize; ++j)
		            write(sockfd2, &matris[i][j] , sizeof(double));
            /* VM e asil matrsin tersi gonderilir. */
		    for (i = 0; i < matrisSize; ++i)
		        for (j = 0; j < matrisSize; ++j)
		            write(sockfd2, &invMatris[i][j] , sizeof(double));
				/*****************************/
		usleep(100);
		gettimeofday(&bitis,NULL); /* zaman bitisi */
		/* zaman farki */
		zamanFark = (bitis.tv_sec-basla.tv_sec)*1000 + (bitis.tv_usec - basla.tv_usec)/1000;

        if (intTrace == 1)
		  printf("\tThe time taken to process %d : %ld milisecond\n",(k+1),zamanFark);
	}

    /* Alinan yerler geri verilir. */
	for (i=0;i<matrisSize;i++)
    {
        free(matris[i]);
        free(lMatris[i]);
        free(uMatris[i]);
        free(invMatris[i]);
    }
  
    free(lMatris);
    free(uMatris);
    free(invMatris);
    free(matris);

    /* baglantilar kapatilir */
    close(sockfd);
    close(sockfd2);

    printf("\nOperations Have Completed\n");
    gettimeofday(&finish,NULL); /* zaman bitisi */
	/* zaman farki */
	difTime = (finish.tv_sec - start.tv_sec)*1000 + (finish.tv_usec - start.tv_usec)/1000;

    if (intTrace == 1)
	   printf("\nTotal elapsed time : %ld milisecond\n",difTime);
    
    /* Log file kapatilir. */
	fclose(fdLogFile);
    sleep(1);
    return 0;
}

/* create matrix function */
void matrix_create(double **matrix, int min_val, int max_val, int MATRIX_SIZE)
{
    int row, column;
    double fRand;

    for(row = 0; row < MATRIX_SIZE; row++) {
        for(column = 0; column < MATRIX_SIZE; column++) {
        	fRand = (double)rand() / RAND_MAX; /* 0 ile 1 arasinda bir sayi uretilir. */
            /* belirlenen maximum ve minimum 2 sayi arasinda double sayilar uretilir ve matrise doldurulur */
            matrix[row][column] = (min_val + fRand * (max_val - min_val)); 
        }
    }
}

/* print matrix function */
void matrix_print(double **matrix, int MATRIX_SIZE)
{
    int row, column;
    /* Matrisler dosyaya yazilir. */
    for(row = 0; row < MATRIX_SIZE; row++) {
        for(column = 0; column < MATRIX_SIZE; column++) {
            fprintf(fdLogFile,"%f\t", matrix[row][column]);
        }
        fprintf(fdLogFile,"\n");
    }
}

/* L U Decomposition function */
void LU_decomposition(double **l, double **u, double **a, int MATRIX_SIZE)
{
    int i,j,s,k;
    for(k=0;k<MATRIX_SIZE;k++) {
        l[k][k]=1;
     
        for(j=k;j<MATRIX_SIZE;j++) {
            long double sum=0;
            for(s=0;s<=k-1;s++) {
                sum+= l[k][s]*u[s][j];
            }
            u[k][j]=a[k][j]-sum;
        }
     
        for(i=k+1;i<MATRIX_SIZE;i++) {
            long double sum=0;
            for(s=0;s<=k-1;s++) {
                sum+=l[i][s]*u[s][k];
            }
            l[i][k]=(a[i][k]-sum)/u[k][k];
        }
    }
}

/* inverse matrix function */
void inverseMatrix(double **matrisI, int n)
{
    double matrix[40][80], ratio,a;
    int i, j, k;

    for(i = 0; i < n; i++){
        for(j = 0; j < n; j++){
            matrix[i][j] = matrisI[i][j];
        }
    }
    
    for(i = 0; i < n; i++){
        for(j = n; j < 2*n; j++){
            if(i==(j-n))
                matrix[i][j] = 1.0;
            else
                matrix[i][j] = 0.0;
        }
    }
    
    for(i = 0; i < n; i++){
        for(j = 0; j < n; j++){
            if(i!=j){
                ratio = matrix[j][i]/matrix[i][i];
                for(k = 0; k < 2*n; k++){
                    matrix[j][k] -= ratio * matrix[i][k];
                }
            }
        }
    }

    for(i = 0; i < n; i++){
        a = matrix[i][i];
        for(j = 0; j < 2*n; j++){
            matrix[i][j] /= a;
        }
    }

    for(i = 0; i < n; i++){
        for(j = n; j < 2*n; j++){
            matrisI[i][j-n] = matrix[i][j];
        }
    }
}
/* Creat Matrix Thread */
void* creatMatrixThread(void* data)
{
		matrix_create(matris, MIN, MAX, matrisSize);
}
/* LU Decompositon Modul Thread */
void* LU(void* data)
{
		LU_decomposition(lMatris, uMatris, matris, matrisSize);
}
/* inverse taking module thread */
void* inverse(void* data)
{
		inverseMatrix(lMatris, matrisSize);
	    inverseMatrix(uMatris, matrisSize);
}

/* Signal Handle */
void signalHandle( int sig)
{
    /* soketler kapatilir. */
    close(sockfd);
    close(sockfd2);

    printf("\n***** SIGNAL CATCH *****\n");
    printf("***** DISCONNECTED *****\n");

    /* malloc ile alinan yerler geri verilir */
    int i;
    for (i=0;i<matrisSize;i++)
    {
        free(matris[i]);
        free(lMatris[i]);
        free(uMatris[i]);
        free(invMatris[i]);
    }
  
    free(lMatris);
    free(uMatris);
    free(invMatris);
    free(matris);

    /* logfile kapatilir. */
    if (fdLogFile != 0)
    {
        fclose(fdLogFile);
    }

    sleep(1);

    exit( sig );
}
/*############### End Of Program #################*/