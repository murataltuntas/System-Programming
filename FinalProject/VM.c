/***************************
 *     Murat ALTUNTAS
 *       111044043
 *   Verifier Module (VM)
 ***************************/
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
#include <signal.h>

/* Matrix Print Function */
void matrix_print(double **matrix, int MATRIX_SIZE);
/* Creat Unit Matrix Function */
void birimMatrix(double **matrix, int MATRIX_SIZE);
/* Compare Unit Matrix Function */
void compareUnitMatrix(double** first, double** second, int MATRIX_SIZE);
/* Signal Handle */
void signalHandle( int sig);

double  **matris,       /* gercek matris */
        **invMatris,    /* gercek matrisin tersi */
        **birimMat,     /* carpim sonucu olusan birim matris */
        **birimMatris;  /* olusturulan birim matris */

char LOGFILE[64]; /* logfile name */
FILE* fdLogFile; /* log file pointer */
int intSize; /* Matris boyutu  */
int sockfd2 = 0; /* socket descriptor */
int listenfd = 0;/* socket descriptor */
int connfd = 0; /* accept descriptor */
int succesCount = 0, failCount = 0; /* num of successful and failed matrix counter. */
int intTrace; /* Printf ler icin */

int main(int argc, char *argv[])
{
    int totalMatrix; /* toplam matris sayisi */
    double doubleEleman; /* matrislerin elemanlarini okumak icin */
    int i,j,k; /* for loop */
    struct sockaddr_in serv_addr; /* ITM socket */
    struct sockaddr_in serv_addr2; /* MMM socket */
    struct hostent *hp2; /* for getbyhostname (MMM) */
    struct timeval basla , bitis ; /* Zaman hesaplari icin */
	struct timeval start , finish ; /* Zaman hesaplari icin */
	long zamanFark , difTime; /* zaman farki hesabi icin */

    if(argc != 2)
    {
        printf("\n Usage: %s <ip of server> \n",argv[0]);
        return 1;
    } 

    /* Ctrl + C   sinyali yakalanir */
    signal( SIGINT, signalHandle);
    signal( SIGPIPE, signalHandle);

    /* log file adi olusturulur */
    sprintf(LOGFILE,"logFile_Verifier_%ld.log",(long)getpid());
    /* Matrislerin yazilacagi logfile acilir */
    fdLogFile = fopen(LOGFILE,"a");
    /* Dosya acilmazsa hata verir ve program sonlanir */
    if (fdLogFile == NULL)
    {
        perror("Error opening file \"logFile_Verifier_.log\"");
        exit(1);
    }

	/* VM ve ITM arasinda */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000); 

    if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
    {
        perror("\nError  Bind : ");
        return 1;
    }

    listen(listenfd, 10);
    /*********************************************/

    /* VM ve MMM arasinda */    
    if((sockfd2 = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 
    memset(&serv_addr2, '0', sizeof(serv_addr2)); 
    serv_addr2.sin_family = AF_INET;
    hp2 = gethostbyname(argv[1]);
    bcopy(hp2->h_addr, &(serv_addr2.sin_addr.s_addr),hp2->h_length);
    serv_addr2.sin_port = htons(7777);
    if( connect(sockfd2, (struct sockaddr *)&serv_addr2, sizeof(serv_addr2)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       return 1;
    }
    /*********************************************/
    printf("^-^ CONNECTED ^-^\n");

    /* Baglanti beklenir */
    connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
	
	gettimeofday(&start,NULL);	/* Zaman baslangici */
    
    read(connfd, &intSize, sizeof(int)); /* ITM den matris size alinir. */
    read(connfd, &totalMatrix, sizeof(int)); /* ITM den total matris sayisi alinir. */
    read(connfd, &intTrace, sizeof(int)); /* ITM den printf control alinir. */

    /* Matrislere matris boyutu kadar yer alinir. */
    matris = (double**)malloc(sizeof(double*) * intSize);
    invMatris = (double**)malloc(sizeof(double*) * intSize);
    birimMat = (double**)malloc(sizeof(double*) * intSize);
    birimMatris = (double**)malloc(sizeof(double*) * intSize);

    for(i=0;i<intSize;++i)
    {
        matris[i] = (double*)malloc(sizeof(double) * intSize);
        invMatris[i] = (double*)malloc(sizeof(double) * intSize);
        birimMat[i] = (double*)malloc(sizeof(double) * intSize);
        birimMatris[i] = (double*)malloc(sizeof(double) * intSize);
    }

    /* Birim matris uretilir */
    birimMatrix(birimMatris, intSize);

    printf("\n");
    /* toplam matris sayisi kadar donen dongu */
    for (k = 0; k < totalMatrix; ++k)
    {
    	gettimeofday(&basla,NULL);	/* Zaman baslangici */
        if (intTrace == 1)
            printf("-> %d. Verifier procedures.\n", (k+1));

    	fprintf(fdLogFile,"\n/########################## %d. Matrix ###############################/\n",(k+1));
        /* VM ve ITM arasinda */

        fprintf(fdLogFile,"\nA matrisi\n----------\n");
        /* ITM den original matrix alinir */
        for (i = 0; i < intSize; ++i)
            for (j = 0; j < intSize; ++j)
            { 
                read(connfd, &doubleEleman, sizeof(double));
                matris[i][j] = doubleEleman;
            }
        matrix_print(matris, intSize);

        fprintf(fdLogFile,"\nA' matrisi\n----------\n");
        /* ITM den invers matrix alinir */
        for (i = 0; i < intSize; ++i)
            for (j = 0; j < intSize; ++j)
            {
                read(connfd, &doubleEleman, sizeof(double));
                invMatris[i][j] = doubleEleman;
            }
        matrix_print(invMatris, intSize);
/**********************************/

        /* VM ve MMM arasinda */
        /* original matris MMM e gonderilir */
        for (i = 0; i < intSize; ++i)
            for (j = 0; j < intSize; ++j)
                write(sockfd2, &matris[i][j] , sizeof(double));

        /* inverse matrix MMM e gonderilir */
        for (i = 0; i < intSize; ++i)
            for (j = 0; j < intSize; ++j)
                write(sockfd2, &invMatris[i][j] , sizeof(double));

        fprintf(fdLogFile,"\nCarpim Sonucu Uretilen Birim Matris\n--------------------\n");
        /* MMM den carpim sonucu uretilen birim matris alinir. */
        for (i = 0; i < intSize; ++i)
            for (j = 0; j < intSize; ++j)
            {
                read(sockfd2, &doubleEleman, sizeof(double));
                birimMat[i][j] = doubleEleman;
            }
        matrix_print(birimMat, intSize);
        /* gelen matris ile gercek birim matris karsilastirilir. */
        compareUnitMatrix(birimMatris, birimMat, intSize);
        
		usleep(10);
		gettimeofday(&bitis,NULL); /* zaman bitisi */
		/* zaman farki */
		zamanFark = (bitis.tv_sec-basla.tv_sec)*1000 + (bitis.tv_usec - basla.tv_usec)/1000;
    
        if (intTrace == 1)
    		printf("\tThe time taken to process %d : %ld milisecond\n",(k+1),zamanFark);
	}

    /* Malloc ile alinan yerler geri verilir */
	for (i=0;i<intSize;i++)
	{
	    free(matris[i]);
	    free(invMatris[i]);
	    free(birimMat[i]);
	    free(birimMatris[i]);
	}

	free(matris);
	free(invMatris);
	free(birimMat);
	free(birimMatris);

    printf("\nOperations Have Completed\n");
    gettimeofday(&finish,NULL); /* zaman bitisi */
    /* zaman farki */
    difTime = (finish.tv_sec - start.tv_sec)*1000 + (finish.tv_usec - start.tv_usec)/1000;
    
    if (intTrace == 1)
    {
        printf("\nTotal elapsed time : %ld milisecond\n",difTime);
        printf("-> Number of inverted matrices : %d\n", succesCount);
        printf("-> Number of non-invertible matrices : %d\n", failCount);
    }
    /* Logfile kapatilir */
    fclose(fdLogFile);

    /* Soketler kapatilir. */
    close(sockfd2);
    sleep(1);
	close(connfd);
	close(listenfd);

    return 0;
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

/* Birim matris olusturan fonksiyon */
void birimMatrix(double **matrix, int MATRIX_SIZE)
{
    int i,j;

    for (i = 0; i < MATRIX_SIZE; ++i)
    {
        for (j = 0; j < MATRIX_SIZE; ++j)
        {
            if (i==j)
            {
                matrix[i][j]=1;
            }
            else
            {
                matrix[i][j]=0;
            }
        }
    }
}
/* Compare unit matrix function */
void compareUnitMatrix(double** first, double** second, int MATRIX_SIZE)
{
    int i,j; /* for loop */
    int sart = 0;

    for (i = 0; i < MATRIX_SIZE; ++i)
    {
        for (j = 0; j < MATRIX_SIZE; ++j)
        {
            if ( ( -0.0001 > (first[i][j] - second[i][j])) || ((first[i][j] - second[i][j]) > 0.0001))
            {
                if (sart == 0)
                {
                    if (intTrace == 1)
                        printf("  The matrix wasn't verified. *FAILED*!\n");

                    failCount++;
                }
                sart = 1;
            } 
        }
    }

    if (sart == 0)
    {
        if (intTrace == 1)
            printf("  The matrix was verified. *SUCCESSFUL*!\n");
        
        succesCount++;
    }

}

/* Signal Handle */
void signalHandle( int sig)
{
    int i;
    close(sockfd2);
    
    printf("\n***** SIGNAL CATCH *****\n");
    printf("***** DISCONNECTED *****\n");

    /* Malloc ile alinan yerler geri verilir. */
    for (i=0;i<intSize;i++)
	{
	    free(matris[i]);
	    free(invMatris[i]);
	    free(birimMat[i]);
	    free(birimMatris[i]);
	}

	free(matris);
	free(invMatris);
	free(birimMat);
	free(birimMatris);

    /* logfile kapatilir. */
    if (fdLogFile != 0)
    {
        fclose(fdLogFile);
    }

    /* Socketler kapatilir */
    sleep(1);
    close(connfd);
    close(listenfd);

    exit( sig );
}
/*############### End Of Program #################*/