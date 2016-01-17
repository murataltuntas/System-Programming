/*        Murat   ALTUNTAS          *
 *          111044043               *
 *   System Programming Midterm     *
 ************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <ctype.h>
#include <math.h>

//#define FIFO_FILE1 "MYFIFO1" /*fifo for integrate*/
//#define FIFO_FILE2 "MYFIFO2" /*fifo for derivate*/
#define LOGFILE "logfile.txt" /* logFile */
#define BUFSIZE 1024
#define MAXOPSTACK 64
#define MAXNUMSTACK 64

/* Unary minus */
double eval_uminus(double a1, double a2);
/* Exponent */
double eval_exp(double a1, double a2);
/* Multiplication */
double eval_mul(double a1, double a2);
/* Division */
double eval_div(double a1, double a2);
/* Mod */
double eval_mod(double a1, double a2);
/* Addition */
double eval_add(double a1, double a2);
/* Subtract */
double eval_sub(double a1, double a2);

enum {ASSOC_NONE=0, ASSOC_LEFT, ASSOC_RIGHT};

struct op_s {
	char op;
	int prec;
	double assoc;
	int unary;
	double (*eval)(double a1, double a2);
} ops[]={
	{'_', 10, ASSOC_RIGHT, 1, eval_uminus},
	{'^', 9, ASSOC_RIGHT, 0, eval_exp},
	{'*', 8, ASSOC_LEFT, 0, eval_mul},
	{'/', 8, ASSOC_LEFT, 0, eval_div},
	{'%', 8, ASSOC_LEFT, 0, eval_mod},
	{'+', 5, ASSOC_LEFT, 0, eval_add},
	{'-', 5, ASSOC_LEFT, 0, eval_sub},
	{'(', 0, ASSOC_NONE, 0, NULL},
	{')', 0, ASSOC_NONE, 0, NULL}
};

struct op_s *getop(char ch)
{
	int i;
	for(i=0; i<sizeof ops/sizeof ops[0]; ++i) {
		if(ops[i].op==ch) return ops+i;
	}
	return NULL;
}

int fifo, fifo1, fifo2;
char FIFO_FILE[BUFSIZE]; /* server'in clientlardan veri almasi icin, olusturulacak fifo icin tanimlanan degisken */
char FIFO_FILE1[BUFSIZE]; /* serverdan clientlara veri gondermek icin, olusturulacak fifo icin tanimlanan degisken */
char FIFO_FILE2[BUFSIZE]; /* serverdan clientlara veri gondermek icin, olusturulacak fifo icin tanimlanan degisken */
char *ClientPid;
int client_control=0;/* Client in bagli olup olmadigi controlu */
int clientArr[BUFSIZE];/* client pid lerin tutuldugu array */
int sayac=0; /* baglanan Client sayisi icin */
struct op_s *opstack[MAXOPSTACK]; /* operator stack */
int nopstack=0;
double numstack[MAXNUMSTACK]; /* numaric stack */
int nnumstack=0;
/* push operator */
void push_opstack(struct op_s *op);
/* pop operator */
struct op_s *pop_opstack()
{
	if(!nopstack) {
		fprintf(stderr, "ERROR: Operator stack empty\n");
		exit(EXIT_FAILURE);
	}
	return opstack[--nopstack];
}
/* push  element */
void push_numstack(double num);
/* pop element */
double pop_numstack();

void shunt_op(struct op_s *op);
/* islemler */
double operation(char *strng, int timeT);
/* Siganl function handle */
void handle( int sig);

int main (int argc, char *argv[])
{
	pid_t childpid;
	int fd;
	int j,i=0;/* donguler icin */
	int deger=0; /* Zaman hesabi icin */
	int milisecond; /* Zaman atamalari ve hesabi icin */
	int nanoToMiliSec; /* zaman hesabi icin */
	int len;/* read kontrolu */
	char buf[BUFSIZE]; /* Okuma islemi icin gerekli string */
	char prt[BUFSIZE]; /* sprintf de kullanilan string ,turev icin */
	char prt2[BUFSIZE]; /* sprintf de kullanilan string ,integral icin */
	char *str[2]; /* Clientten alinan string */
	long zamanFark ; /* zaman farki hesabi icin */
    time_t curtime;
    struct tm *loctime; /* for time */

	/* calistirilirken 3 arguman olmasi gerekir */
    if ( argc != 3 )
    {
        fprintf(stderr, "Usage: %s <function> <Time#>\n", argv[0]);
        exit(-1);
    }

	/* Ctrl + C   sinyali yakalanir */
    signal( SIGINT, handle);

    /* program basladiginda onceden var olan bir logfile var ise silinir. */
	/*remove(LOGFILE);*/

    /* clientin programa baglanacagi fifo olusturulur. */
    sprintf(FIFO_FILE,"%ld",(long)getpid());
	if (fd=mkfifo(FIFO_FILE, 0666) == -1)
	{
	/* create a named pipe */
		if (errno != EEXIST)
		{
			fprintf(stderr, "[%ld]:failed to create named pipe %s: %s\n",(long)getpid(), FIFO_FILE, strerror(errno));
			return 1;
		}
	}

	/* clientin programa baglanacagi fifo acilir */
	fifo = open( FIFO_FILE, O_RDONLY );
	if( fifo < 0 ) {
		perror( "FIFO open" );
	}

	while(1)
	{

      	deger=0;
      	/* client baglandiginda okuma yapar. */
		len = read( fifo, buf, BUFSIZE );
		if( len < 0 ) {
			perror( "child read from FIFO" );
			exit(0);
		}
		else if(len != 0)
		{
			str[0] = strtok (buf,".");/* Turev mi yoksa integral mi oldugunu anlamak icin */
			ClientPid = strtok(NULL, ".");/* client pid */
			str[1] = strtok (NULL, "\0");/* Time */

			clientArr[sayac]=atoi(ClientPid);
			client_control=1;
			sayac++;

			if ((childpid = fork()) == -1)
			{
				perror("Failed to fork");
				return 1;
			}

			if (childpid == 0)  /* The child writes */
			{
				nanoToMiliSec = ((atol(argv[2]))/1000000);/* nanosecond to milisecond */
				milisecond = (atol(str[1]));/* clientten gelen milisecond */

				if(atoi(str[0]) == 2)/* Turev */
				{
					sprintf(FIFO_FILE2,"FIFO_%s",ClientPid);
					/* serverin turev clientine veri gonderdigi fifo olusturulur */
					fifo2 = open( FIFO_FILE2, O_WRONLY );
					if( fifo2 < 0 ) {
						perror( "FIFO open" );
						exit(0);
					}

					/* Get the current time. */
					curtime = time (NULL);
					printf("Derivative Client[%d] Connect...\n",sayac);
					fputs (ctime (&curtime), stdout);
					/* clientin dogdugu zaman ve pid ler cliente gonderilir */
					sprintf(prt,"%ld**Birth Time: %s*Derivative Client[%d] Connect... Child pid:[%ld] Client Pid:[%s]\n                 *Value*                    *time*",(long)getpid(),ctime (&curtime),sayac,(long)getpid(),ClientPid);
					if( write( fifo2, prt, BUFSIZE ) < 0 )
					{
						perror( "child write to FIFO" );
						exit(0);
					}
					
					struct timeval basla , bitis ;
					gettimeofday(&basla,NULL) ;	/* Zaman baslangici */
				    for (j = 0 ; j  < (milisecond/nanoToMiliSec); j++)
				    {
				    	deger += nanoToMiliSec;
				        usleep(nanoToMiliSec*1000);/* microsecond cinsinde */
/*						printf("[%d] Derivative\n", j+1);*/

						/* Turev li degerler client e gonderilir */
						sprintf(prt,"%ld*  %20f %20d   Derivative Client Pid:[%s]",(long)getpid(),operation(argv[1],deger),deger,ClientPid);
						if( write( fifo2, prt, BUFSIZE ) < 0 )
						{
							perror( "child write to FIFO" );
							exit(0);
						}
						gettimeofday(&bitis,NULL); /* zaman bitisi */
						/* zaman farki */
	  					zamanFark = (bitis.tv_sec-basla.tv_sec)*1000 + (bitis.tv_usec - basla.tv_usec)/1000;
	  					if (zamanFark >= (milisecond/nanoToMiliSec))
	  					{
							/* Get the current time. */
							curtime = time (NULL);
							printf("Derivative Client[%d] Quit...\n",sayac);
							fputs (ctime (&curtime), stdout);
							/* clientin ciktigi zaman ve pid ler cliente gonderilir */
							sprintf(prt,"%ld*Derivative Client[%d] Quit...\nChild Killed. Child pid : [%ld]\n**Executed time : '%ld' Die Time: %s",(long)getpid(),sayac,(long)getpid(),zamanFark,ctime (&curtime));
							if( write( fifo2, prt, BUFSIZE ) < 0 )
							{
								perror( "child write to FIFO" );
								exit(0);
							}
							
							/* client e yazmanin bittigini belirten ifade. */
							/* bu ifadeyi goren client logfile a yazmayi durdurur. */
							if( write( fifo2, "bitti", BUFSIZE ) < 0 )
							{
								perror( "child write to FIFO" );
								exit(0);
							}
	  						break;
	  					}
					}
					

				}
				else if(atoi(str[0]) == 1)/* Integral */
				{
					sprintf(FIFO_FILE1,"FIFO_%s",ClientPid);
					/* serverin integral clientine veri gonderdigi fifo olusturulur */
					fifo1 = open( FIFO_FILE1, O_WRONLY );
					if( fifo1 < 0 ) {
						perror( "FIFO open" );
						exit(0);
					}
	
					/* Get the current time. */
					curtime = time (NULL);
					printf("Integrate Client[%d] Connect...\n",sayac);
					fputs (ctime (&curtime), stdout);
					/* clientin dogdugu zaman ve pid ler cliente gonderilir */
					sprintf(prt2,"%ld**Birth Time: %s*Integrate Client[%d] Connect... Child pid:[%ld] Client Pid:[%s]\n       *Value*    *time*",(long)getpid(),ctime (&curtime),sayac,(long)getpid(),ClientPid);
					if( write( fifo1, prt2, BUFSIZE ) < 0 )
					{
						perror( "child write to FIFO" );
						exit(0);
					}
					double integrateResult;
					struct timeval basla , bitis ;
					gettimeofday(&basla,NULL) ;	/* Zaman baslangici */
				    
				    for (j = 0 ; j  < (milisecond/nanoToMiliSec); j++)
				    {
				    	deger += nanoToMiliSec;
				        usleep(nanoToMiliSec*1000);
/*						printf("[%d] Integral\n", j+1);*/
						
						integrateResult = ((((operation(argv[1],deger))+(operation(argv[1],0)))/2) * deger);
						/* integralli li degerler client e gonderilir */
						sprintf(prt2,"%ld*  %20f %20d   Integrate Client Pid:[%s]",(long)getpid(),integrateResult,deger,ClientPid);
						if( write( fifo1, prt2, BUFSIZE ) < 0 )
						{
							perror( "child write to FIFO" );
							exit(0);
						}
						gettimeofday(&bitis,NULL); /* zaman bitisi */
						/* zaman farki */
	  					zamanFark = (bitis.tv_sec-basla.tv_sec)*1000 + (bitis.tv_usec - basla.tv_usec)/1000;
	  					if (zamanFark >= (milisecond/nanoToMiliSec))
	  					{
	  						/* Get the current time. */
							curtime = time (NULL);
							printf("Integrate Client[%d] Quit...\n",sayac);
							fputs (ctime (&curtime), stdout);
							/* clientin ciktigi zaman ve pid ler cliente gonderilir */
							sprintf(prt2,"%ld*Integral Client[%d] Quit...\nChild Killed. Child pid : [%ld]\n**Executed time : '%ld' Die Time: %s",(long)getpid(),sayac,(long)getpid(),zamanFark,ctime (&curtime));
							if( write( fifo1, prt2, BUFSIZE ) < 0 )
							{
								perror( "child write to FIFO" );
								exit(0);
							}

							if( write( fifo1, "bitti", BUFSIZE ) < 0 )
							{
								perror( "child write to FIFO" );
								exit(0);
							}

							break;
	  					}
	
					}
					
				}

				exit(0);
			}
			else/* parent */
			{

//				wait(NULL);
			}
		}
	}

	close(fifo2);
	unlink(FIFO_FILE2);
	close(fifo1);
	unlink(FIFO_FILE1);
	close(fifo);
	unlink(FIFO_FILE);
	return 0;
}

/* Unary minus */
double eval_uminus(double a1, double a2) 
{
	return -a1;
}
/* Exponent */
double eval_exp(double a1, double a2)
{
	return a2<0 ? 0 : (a2==0?1:a1*eval_exp(a1, a2-1));
}
/* Multiplication */
double eval_mul(double a1, double a2) 
{
	return a1*a2;
}
/* Division */
double eval_div(double a1, double a2) 
{
	if(!a2) {
		fprintf(stderr, "ERROR: Division by zero\n");
		exit(EXIT_FAILURE);
	}
	return a1/a2;
}
/* Mod */
double eval_mod(double a1, double a2) 
{
	if(!a2) {
		fprintf(stderr, "ERROR: Division by zero\n");
		exit(EXIT_FAILURE);
	}
	return fmod(a1,a2);
}
/* Addition */
double eval_add(double a1, double a2) 
{
	return a1+a2;
}
/* Subtract */
double eval_sub(double a1, double a2) 
{
	return a1-a2;
}
/* push operator */
void push_opstack(struct op_s *op)
{
	if(nopstack>MAXOPSTACK-1) {
		fprintf(stderr, "ERROR: Operator stack overflow\n");
		exit(EXIT_FAILURE);
	}
	opstack[nopstack++]=op;
}
/* push  element */
void push_numstack(double num)
{
	if(nnumstack>MAXNUMSTACK-1) {
		fprintf(stderr, "ERROR: Number stack overflow\n");
		perror( "ERROR: Number stack overflow" );
		exit(EXIT_FAILURE);
	}
	numstack[nnumstack++]=num;
}
/* pop element */
double pop_numstack()
{
	if(!nnumstack) {
		fprintf(stderr, "ERROR: Number stack empty\n");
		exit(EXIT_FAILURE);
	}
	return numstack[--nnumstack];
}

void shunt_op(struct op_s *op)
{
	struct op_s *pop;
	double n1, n2;
	if(op->op=='(') { /* if '(' */
		push_opstack(op);
		return;
	} else if(op->op==')') {
		while(nopstack>0 && opstack[nopstack-1]->op!='(') {
			pop=pop_opstack();/* pop operator */
			n1=pop_numstack();/* pop numeric */
			if(pop->unary) push_numstack(pop->eval(n1, 0));
			else {
				n2=pop_numstack();
				push_numstack(pop->eval(n2, n1));
			}
		}
		if(!(pop=pop_opstack()) || pop->op!='(') {
			fprintf(stderr, "ERROR: Stack error. No matching \'(\'\n");
			exit(EXIT_FAILURE);
		}
		return;
	}

	if(op->assoc==ASSOC_RIGHT) {
		while(nopstack && op->prec<opstack[nopstack-1]->prec) {
			pop=pop_opstack();
			n1=pop_numstack();
			if(pop->unary) push_numstack(pop->eval(n1, 0));
			else {
				n2=pop_numstack();
				push_numstack(pop->eval(n2, n1));
			}
		}
	} else {
		while(nopstack && op->prec<=opstack[nopstack-1]->prec) {
			pop=pop_opstack();
			n1=pop_numstack();
			if(pop->unary) push_numstack(pop->eval(n1, 0));
			else {
				n2=pop_numstack();
				push_numstack(pop->eval(n2, n1));
			}
		}
	}
	push_opstack(op);
}

double operation(char *strng, int timeT)
{
	char *expr;
	char *tstart=NULL;
	struct op_s startop={'X', 0, ASSOC_NONE, 0, NULL};	/* Dummy operator to mark start */
	struct op_s *op=NULL;
	double n1, n2;
	struct op_s *lastop=&startop;

	char ek[1024];
	char* str;
	char* yeni; /* kullanilacak array */
	double result; /* islem sonucu */
	int st,ekk,i,j=0,k=0,l,xCount=0;

	sprintf(ek,"%d",timeT);
	st=strlen(strng);/* stringin uzunlugu */
	ekk=strlen(ek); /* eklenecek stringin uzunlugu */

	for(i=0; i<st; i++)
	{
		if(strng[i] == 't')
		{
			xCount++; /* Stringdeki 't' sayisi */
		}	
	}

	yeni=(char *)malloc(((st + (ekk*xCount))*sizeof(char)));

	for(i=0; i<st; i++)
	{
		if(strng[i] == 't')
		{
			for (k = 0; k < ekk; ++k)
			{
				yeni[j]=ek[k];/* stringdeki 't' yerine zaman yazilir. */
				j++;
			}
		}
		else
		{
			yeni[j]=strng[i];
			j++;
		}
		yeni[j]='\0';/* en sona \0 yazilir. */
	}

	for(expr=yeni; *expr; ++expr) {
		if(!tstart) {

			if((op=getop(*expr))) { /* operator mu */
				if(lastop && (lastop==&startop || lastop->op!=')')) {
					if(op->op=='-') op=getop('_');
					else if(op->op!='(') {
						fprintf(stderr, "ERROR: Illegal use of binary operator (%c)\n", op->op);
						free(yeni);
						kill(getppid(),SIGINT);
						kill(getpid(),SIGINT);
						kill(atol(ClientPid),SIGINT);
						exit(EXIT_FAILURE);
					}
				}
				shunt_op(op);
				lastop=op;
			} else if(isdigit(*expr)) tstart=expr; /* sayi mi */
			else if(!isspace(*expr)) { /* farkli bir karakter mi */
				fprintf(stderr, "ERROR: Syntax error\n");
				free(yeni);
				kill(getppid(),SIGINT);
				kill(getpid(),SIGINT);
				kill(atol(ClientPid),SIGINT);
				exit(EXIT_FAILURE);
			}
		} else {
			if(isspace(*expr)) {
				push_numstack(atoi(tstart));
				tstart=NULL;
				lastop=NULL;
			} else if((op=getop(*expr))) {
				push_numstack(atoi(tstart));
				tstart=NULL;
				shunt_op(op);
				lastop=op;
			} else if(!isdigit(*expr)) {
				fprintf(stderr, "ERROR: Syntax error\n");
				free(yeni);
				kill(getppid(),SIGINT);
				kill(getpid(),SIGINT);
				kill(atol(ClientPid),SIGINT);
				exit(EXIT_FAILURE);
			}
		}
	}
	if(tstart) push_numstack(atoi(tstart));

	while(nopstack) {
		op=pop_opstack();
		n1=pop_numstack();
		if(op->unary) push_numstack(op->eval(n1, 0));
		else {
			n2=pop_numstack();
			push_numstack(op->eval(n2, n1));
		}
	}
	if(nnumstack!=1) {
		fprintf(stderr, "ERROR: Number stack has %d elements after evaluation. Should be 1.\n", nnumstack);
		free(yeni);
		return EXIT_FAILURE;
	}

	free(yeni);

	result = numstack[0]; /* sonuc result a atanir. */
	nopstack=0;
	
	for (l = 0; l < MAXNUMSTACK; ++l)/* stack bosaltilir */
	{
		numstack[l] = 0;
	}
	
	nnumstack=0;
	return (result);
}

/* Signal Handle */
void handle( int sig)
{
	int i=0;
    close(fifo);
    printf("****Ctrl + C (SIGINT) catch****\n");

    /* eger pid fifo file ile ayni ise sinyal parenta gelmistir.Fifo */
    if ((long)getpid() == atol(FIFO_FILE))
    {
    	unlink(FIFO_FILE);
    	unlink(FIFO_FILE1);
    	unlink(FIFO_FILE2);
    	printf("*** Parent Killed ***\n");
    	if (client_control==1)
		{
			for (i = 0; i < sayac; ++i)
			{
				kill(clientArr[i],SIGINT);
			}
		}
    }
    else
    {
    	printf("*** Child Killed ***\n");
    }

	close(fifo2);
	close(fifo1);
	exit(sig);
}