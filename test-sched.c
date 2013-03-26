/* Include Flags*/ 
#define _GNU_SOURCE
/*Needed Includes */
#include <stdlib.h> 
#include <stdio.h> 
#include <string.h> 
#include <math.h> 
#include <sched.h>
#include <fcntl.h>

/*Handle the signal's children*/
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

/*Needed for forking*/
#include <unistd.h>

/*Needed for CPU intensive large numbers*/
#include <gmp.h>

static void sigchld_handle (int sig)
{
	/*This quites warnings from wall/wextra about the lack of a use of sig*/
	(void) sig;
	/* Wait for all dead children.
	 * I'm using a non-blocking call to be sure this signal handler will not
	 * block if a child was cleaned up in another part of the program. 
	 * (But it really shouldn't be)*/
	while (waitpid(-1, NULL, WNOHANG) > 0) {
	}
} 
void IO_process(int inputFile, int writeFile); 
void CPU_process(); 
void HYBRID_process(int writeFile);


int main(int argc, char* argv[]){

	signal(SIGCHLD,sigchld_handle);
	enum load{
		HEAVY=200, 
		MEDIUM=60,
		LIGHT=10,
	};	
	enum type{
		IO, 
		CPU, 
		HYBRID
	};
	int policy, current_load, load_type;
	int i=0;
	pid_t pid;
	char filename[strlen("ofile")+4];
	int inputFile; 
	int writeFile;
	struct sched_param param;

	/*This section processes the arguments supplied  */
	
	/*Sets the default policy if not supplied*/ 
	if(argc<2){
		policy = SCHED_OTHER; 
	} 
	
	/*Sets the default load if not supplied*/ 
	if(argc<3){
		current_load=MEDIUM; 
	}
	/*Sets the default load type if not supplied*/ 
	if(argc<4){ 
		load_type=HYBRID;

	}
	
	/* Set Policy if supplied  */ 
    	if(argc > 1){
		if(!strcmp(argv[1], "SCHED_OTHER")){
	        	policy = SCHED_OTHER;
		}
		else if(!strcmp(argv[1], "SCHED_FIFO")){
			policy = SCHED_FIFO;
		}	
		else if(!strcmp(argv[1], "SCHED_RR")){
		    policy = SCHED_RR;
		}
		else{
		    fprintf(stderr, "Unhandeled scheduling policy\n");
		    exit(EXIT_FAILURE);
		}
	    }

	/* Set load if supplied  */ 
    	if(argc > 2){
		if(!strcmp(argv[2], "LIGHT")){
	        	current_load=LIGHT;
		}
		else if(!strcmp(argv[2], "MEDIUM")){
			current_load=MEDIUM;
		}	
		else if(!strcmp(argv[2], "HEAVY")){
			current_load=HEAVY;
		}
		else{
		    fprintf(stderr, "Unhandeled Load\n");
		    exit(EXIT_FAILURE);
		}
    	}
	/* Set Load Type if supplied  */ 
    	if(argc > 3){
		if(!strcmp(argv[3], "IO")){
	        	if((inputFile=open("junkdata", O_RDONLY|O_SYNC))<0){
				perror("Error opening the datafile"); 
				exit(EXIT_FAILURE);
			}	
			load_type=IO;
		}
		else if(!strcmp(argv[3], "HYBRID")){
			load_type=HYBRID;
		}	
		else if(!strcmp(argv[3], "CPU")){
		   	load_type=CPU;
		}
		else{
		    fprintf(stderr, "Unhandled load type\n");
		    exit(EXIT_FAILURE);
		}
    }

    	/* Set process to max prioty for given scheduler */
 	param.sched_priority = sched_get_priority_max(policy);
  
	if(sched_setscheduler(0, policy, &param)){
		perror("Error setting scheduler policy");
		exit(EXIT_FAILURE);
	}
		
	while((i<current_load)){
		pid=fork();
		/*If we are the child*/
		if(pid==0){
			/* This switch statement calls the particular child 
			 * function based on the load type selected as 
			 * a parameter */ 
			switch(load_type) {
				case IO:
					sprintf(filename,"ofile%d",i);	
					if((writeFile=open(filename,
					O_WRONLY| O_CREAT| O_TRUNC | O_SYNC
					|S_IRUSR| S_IWUSR| S_IRGRP| 
					S_IWGRP| S_IROTH))<0){
					perror("Output file failed to open");
						exit(EXIT_FAILURE);	
					}
					IO_process(inputFile, writeFile);
					if(remove(filename)){
					perror("Couldn't delete output file");
					}
					break; 
				case CPU: 
					CPU_process();
					break; 
				case HYBRID: 
				   	sprintf(filename,"ofile%d",i);
					if((writeFile=open(filename,
					O_WRONLY| O_CREAT| O_TRUNC | O_SYNC|
					 S_IRUSR| S_IWUSR| S_IRGRP|
                                        S_IWGRP| S_IROTH))<0){
                                        perror("Output file failed to open");
                                                exit(EXIT_FAILURE);
                                        }
                                        HYBRID_process(writeFile);
                                        if(remove(filename)){
                                        perror("Couldn't delete output file");
                                        }
                                        break;
				default: 
				/*We should never ever ever get here,
				*but if we do then we need to handle it*/ 
				fprintf(stderr, 
					"Load_Type not selected corrected");
				exit(EXIT_FAILURE);
			}			
			exit(EXIT_SUCCESS);
			
			}	
		/*If forking fails*/ 
		else if(pid<0) {
			perror("Error forking off a child process"); 
			exit(EXIT_FAILURE); 	
		}
		/*We are the parent*/ 
		else {
			i++; 
		}
	} 

	if(pid!=0) {
	       /* This should in theory just cause the parent to wait
		* until all the children are dead*/ 	
		while (waitpid(-1, NULL, 0) > 0) {
		}
	}
	if(load_type==IO && close(inputFile)){
		fprintf(stderr, "Error closing the input file\n");
	}	

	return EXIT_SUCCESS;

}
void IO_process(int inputFile, int writeFile){
	int N;
	/*Reads 8kB from the file at any given point in time*/ 
	int B=8000;
	/*adding room for the zero at the end */
	char buffer[B+1];
	int bytesRead; 
	int buffersize=B*sizeof(char);
	/*Read the 8kb N times where N is 2000
	 *I.E Read a total of 16MB of data*/
	for(N=0; N<2000; N++){
	/*Read data from the file in chunks of N bytes */
	while((bytesRead=read(inputFile,buffer,buffersize))<buffersize){
		if(bytesRead<0) {
			perror("Error reading inputFile");
			exit(EXIT_FAILURE);
		}
		/*while we can't pull all the data rewind the
		 *file back to the begining and try again*/  	
		if(lseek(inputFile,0,SEEK_SET)){
			perror("Error resetting to beginning of file");
			exit(EXIT_FAILURE);
		} 
	} 
	/*Write the data to the outfile*/
	if(write(writeFile,buffer,bytesRead)<0){
		perror("Error writing output file"); 
		exit(EXIT_FAILURE);	
		}
	}
	if(close(writeFile)){
		fprintf(stderr, "Error closing the output File");
	}
}

void CPU_process(){
	unsigned long i;
	/*Calculates 75000 factorial*/ 
	unsigned long N=7.5e4; 
	/*This sets up the storage variable to store the factorial*/ 
	mpz_t factorial;
	mpz_init(factorial); 
	mpz_set_str(factorial, "1", 10);
	/*Lets be inefficent and mutiply them all one by one*/ 
	for(i=2; i<=N; i++) {
		mpz_mul_ui(factorial,factorial,i);
	}
	/*We are done figuring out 35000 factorial time to cleanup and 
	 *close the program*/ 
	mpz_clear(factorial);
}
void HYBRID_process(int writeFile){
	unsigned long i;
        /*Calculates 5000 factorial*/
        unsigned long N=3e3;
        /*This sets up the storage variable to store the factorial*/
      	int bytesize;  
	char *buffer;
	mpz_t factorial;
        mpz_init(factorial);
        mpz_set_str(factorial, "1", 10);
        /*Lets be inefficent and mutiply them all one by one*/
        for(i=2; i<=N; i++) {
                mpz_mul_ui(factorial,factorial,i);
       		bytesize=gmp_asprintf(&buffer,"Current factoraial of is %Zd",
					factorial); 
	if(write(writeFile,buffer,bytesize)<0){
                perror("Error writing output file");
                exit(EXIT_FAILURE);
                }
        }

        /*We are done figuring out 35000 factorial time to cleanup and 
         *close the program*/
        mpz_clear(factorial);
	free(buffer);
	if(close(writeFile)){
                fprintf(stderr, "Error closing the output File");
        }
	
}
