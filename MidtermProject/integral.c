/* integral */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

//#define FIFO_FILE1 "MYFIFO1"
#define LOGFILE "logfile.txt"
#define BUFSIZE 1024

void sigHandle(int sig);

int fifo1;
int fd1; /* mkfifo icin */
char FIFO_FILE[BUFSIZE];/* Fifo name icin tanimlanan degisken */
char FIFO_FILE1[BUFSIZE];/* Fifo name icin tanimlanan degisken */
char *childKill;/* Serverda bagli oldugu childin pid ini ogrenmek icin tanimlandi */
FILE* logfp;/* logfile icin */

int main (int argc, char *argv[])
{
	int fifo;
	int len;/* read kontrolu */
	char buf[BUFSIZE];/* Yazma islemi icin gerekli string */
	char buf2[BUFSIZE];/* Okuma islemi icin gerekli string */
	int integral=1; /* integral oldugunu belirten ifade */

	/* calistirilirken 3 arguman olmasi gerekir */
    if ( argc != 3 )
    {
        fprintf(stderr, "Usage: %s <pid> <Time#>\n", argv[0]);
        exit(-1);
    }
    /* Ctrl + C   sinyali yakalanir */
    signal( SIGINT, sigHandle);
	sprintf(FIFO_FILE,"%s",argv[1]);/* Fifo Name */

	sprintf(buf,"%d.%d.%s",integral,getpid(),argv[2]); /*integral oldugunu belirten ifade -  pid - time  */

    /* Fifo acilir */
	fifo = open( FIFO_FILE, O_WRONLY );
	if( fifo < 0 ) {
		perror( "FIFO open" );
		exit(1);
	}
	/* servera gerekli veriler yazilir */
	if( write( fifo, buf, BUFSIZE ) < 0 )
	{
		perror( "child write to FIFO" );
	}

	/* yazan fifo kapatilir */
	close(fifo);
/*****************************************/
	/* serverin integral clientine veri gonderdigi fifo olusturulur */
	sprintf(FIFO_FILE1,"FIFO_%d",getpid());
	if (fd1=mkfifo(FIFO_FILE1, 0666) == -1)
	{
	/* create a named pipe */
		if (errno != EEXIST)
		{
			fprintf(stderr, "[%ld]:failed to create named pipe %s: %s\n",(long)getpid(), FIFO_FILE1, strerror(errno));
			return 1;
		}
	}

	/* serverdan okuyacak fifo acilir */
	fifo1 = open( FIFO_FILE1, O_RDONLY );
	if( fifo1 < 0 ) {
		perror( "FIFO open" );
	}

	logfp = fopen(LOGFILE,"a");
	/* Dosya acilmazsa hata verir ve program sonlanir */
    if (logfp == NULL)
    {
        perror("Error opening file");
        exit(1);
    }

    printf("CALİSİYOR...\n");
	while(1)
	{
		/* serverden yazilani okur. */
		len = read( fifo1, buf2, BUFSIZE );
		if( len < 0 ) {
			perror( "child read from FIFO" );
			exit(0);
		}
		else if(len != 0)
		{
			if(!strcmp(buf2,"bitti"))
			{
				printf("Bitti!\n");
				close(fifo1);
				unlink(FIFO_FILE1);
				fclose(logfp);
				return 0;
			}
			/* logfile yazar */
			fprintf(logfp,"%s\n", buf2);
			/* Clientin bagli oldugu child in Pid'sini alir. */
			childKill = strtok(buf2,"*");
		}
	}
/***********************************************/
	close(fifo1);
	fclose(logfp);
	return 0;
}

void sigHandle(int sig)
{
	printf("*** Client killed ***\n");
	fprintf(logfp,"*** Client killed *** PID:[%ld]\n", (long)getpid());
	/* client e sinyal gelince server da bagli oldugu childi oldurur. */
	kill(atol(childKill),SIGINT);
	close(fifo1);
	unlink(FIFO_FILE1);
	fclose(logfp);
	exit(sig);
}