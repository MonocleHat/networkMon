#include <signal.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>
#include "netmon.h"
#define BUFLEN 256
using namespace std;
/*
 * This is a rewrite
 * We need to address a core set of commands for our program
 * each command will give us access to interacting with intfmon
 * we only need the following
 * signal handler, storage for PID's, running count, interface count, SOCKPATH
 */

char SOCKPATH[] = "/tmp/netmonsock";

pid_t *arrpid; //stores an array of process id's
int infCount = 0; //tracks the interface count given to the program
int numCli = 0; //tracks the clients connected
bool running = true;
pthread_mutex_t lock;
void *recv_func(void*arg);
int sockfd; //socket
int main(){
	signal(SIGINT, sigHandler);

	string intfname;
	string *intfarr; //holds list of interface names (intfname objects);
	char choice;//user enters 'Y'/'y'/'N'/'n'
	bool parentProc = true; //for parent process to utilize

	//sock construction variables
	struct sockaddr_un addr;

	cout << "-=-=-=-=-=-= Spinning Up =-=-=-=-=-=-" << endl;
	cout << "NetMon Program (1.0.0)" << endl;
	cout << "Specify number of interfaces to monitor: ";
	cin >> infCount;
	cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" << endl;
	cout << "Enter each interface name" << endl;
	intfarr = new string[infCount];
	arrpid = new pid_t[infCount];
	for(int i = 0; i < infCount; i++){
		cout << "Intf [" << i << "]: ";
		cin >> intfname; //get interface names from client
		intfarr[i] = intfname;
	}
	cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" << endl;
	cout << "Review the following information" << endl;
	for (int i = 0; i < infCount; i++){
		cout << "intf [" << i << "]: " << intfarr[i] << endl; //list interfaces, wait confirmation
	}
	cout << "Are these inputs correct? (Y/N):";
	cin >> choice;
	bool flag = false;
	while(!flag){
		switch(choice){
			case 'Y':
			case 'y':
				cout << "Yes selected" << endl;
				flag = true;
				break;
			case 'N':
			case 'n':
				cout << "TERMINATING PROGRAM" << endl;
				delete [] intfarr;
				exit(-1);
				break;
			default:
				cout << "Error, reenter selection (Y/N):";
				cin >> choice;
				break;
		}
	}
	//confirmation given, so we spin up a child process for each interface
	pthread_t tid[infCount];//holds our threads
	int cliSock[infCount]; //client sockets
	for(int i = 0;(i < infCount) & parentProc; i++){
		arrpid[i]=fork(); 
		if(arrpid[i]==0){
			parentProc = false;
			execlp("./intfmon","./intfmon",intfarr[i].c_str(),NULL);
			cout << "EXEC SHOULD HAVE SWITCHED!!!!" << endl;
		}
	}
	//if we're in our parent process, we begin setting up our sockets here
	if(parentProc){
		//our sockets are created
		cout << "NETMON::Initializing Sockets" << endl;
		memset(&addr,0,sizeof(addr));
		if((sockfd = socket(AF_UNIX,SOCK_STREAM,0))<0){
			perror("INIT::SOCKHOST");
			exit(-1);
		}
		addr.sun_family = AF_UNIX;
		strncpy(addr.sun_path, SOCKPATH, sizeof(addr.sun_path)-1);
		unlink(SOCKPATH);

		//binding
		if(bind(sockfd,(struct sockaddr*)&addr,sizeof(addr)) == -1){
			perror("BIND::SOCKHOST");
			exit(-1);
		}

		//set our listening sockets
		if (listen(sockfd,infCount)==-1){
			cout << "SERVERERR::LISTEN::" << strerror(errno) << endl;
			unlink(SOCKPATH);
			close(sockfd);
			exit(-1);
		}

        
        running = true;
        int ret;
        while(running){
			//while running, accept clients and create new threads for each client connectng
            if((cliSock[numCli] = accept(sockfd,NULL,NULL))==-1){
                cout << "NETMONERROR" << strerror(errno) << endl;
                unlink(SOCKPATH);
                close(sockfd);
                exit(-1);
            }
            cout << "connected" << endl;
			ret = pthread_create(&tid[numCli],NULL,recv_func,&cliSock[numCli]);
			if(ret!=0){
				cout << "err thread [" << numCli << "] " << strerror(errno) << endl;
			}else{
				++numCli;
			}
            
        }
	}

	//on loop end, close each thread
	for(int i = 0; i<numCli;i++){
		pthread_join(tid[i],NULL);
		cout << "thread ["<<i<<"] closed" << endl;
	}
	pid_t ret = 0;
	int status =-1;
	while(ret>=0){
		ret = wait(&status);
		cout << "PID: " << ret << " quit" << endl; //wait for children to return to main process
	}
	//unlink filepath for later use, delete arrays, and close file descriptors
	unlink(SOCKPATH);
	delete []arrpid;
	delete[]intfarr;
	close(sockfd);
	for(int i = 0; i < numCli; i++){
		close(cliSock[i]);
	}
	pthread_mutex_destroy(&lock); //delete mutex
	cout << "Netmon Terminated" << endl;
	
}
//Thread function that runs on creation of a thread
void* recv_func(void* arg){
	char BUF[BUFLEN];
	int ret,len;
	int cliSock=*(int*)arg;
	while(running){
		ret = read(cliSock,BUF,BUFLEN); //read the sockets to see if clients have written
            if (ret>0){
                if (strcmp(BUF,"READY")==0){
					pthread_mutex_lock(&lock); //we lock the thread to write "GETDATA" to the client
                   len = sprintf(BUF,"GETDATA")+1;
                   ret = write(cliSock,BUF,len);
                    if(ret < 0){
                        cout <<"NETMON::ERR WRITING GETDATA::" << strerror(errno) << endl;
                        close(sockfd);
                        exit(-1);
                    }
					pthread_mutex_unlock(&lock);
                }
                if (strcmp(BUF,"LINKDOWN")==0){
					pthread_mutex_lock(&lock); //lock the thread in the event a link goes down, write SETLINK to the client to stand it up
                    cout << "Setting Link Up" << endl;
                    len = sprintf(BUF,"SETLINK")+1;
                    ret = write(cliSock,BUF,len);
                    if(ret < 0 ){
                        cout << "NETMON::ERR WRITING SETLINK" << strerror(errno) << endl;
                        close(sockfd);
                        exit(-1);
                    }
					pthread_mutex_unlock(&lock);
                }
				

            }
			sleep(1);
	}
	len = sprintf(BUF,"SHUTDOWN")+1;
	ret = write(cliSock,BUF,len); //send shutdown command
	pthread_exit(NULL); //return to main//close thread
	 
}
//signal handler function
void sigHandler(int sig){
	switch(sig){
		case SIGINT:
			cout << "MAIN::SIGINT CAUGHT" << endl;
			for(int i=0;i<numCli;i++){
			}
			for(int i = 0; i < infCount; i++){
				kill(arrpid[i], SIGUSR1);
			}
			running = false;
			break;
	}
}