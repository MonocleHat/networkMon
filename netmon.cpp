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
static void sigHandler(int);
pid_t *arrpid;
int infCount = 0;
bool running = true;

//begin program
int main(){
	signal(SIGINT, sigHandler);
	char BUF[BUFLEN];
	string intfname;
	string *intfarr; //holds list of interface names (intfname objects);
	//we've omitted the signals here (no structs)
	char choice;//user enters 'Y'/'y'/'N'/'n'
	bool parentProc = true; //for parent process to utilize

	//sock construction variables
	struct sockaddr_un addr;
	int sockfd; //main socket
	int cliSock; //client sockets
	cout << "-=-=-=-=-=-= Spinning Up =-=-=-=-=-=-" << endl;
	cout << "NetMon Program (0.5.0)" << endl;
	cout << "Before running we need information!" << endl;
	cout << "Specify number of interfaces to monitor: ";
	cin >> infCount;
	cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" << endl;
	cout << "Enter each interface name" << endl;
	intfarr = new string[infCount];
	arrpid = new pid_t[infCount];
	for(int i = 0; i < infCount; i++){
		cout << "Intf [" << i << "]: ";
		cin >> intfname;
		intfarr[i] = intfname;
	}
	cout << "Review the following information" << endl;
	for (int i = 0; i < infCount; i++){
		cout << "intf [" << i << "]: " << intfarr[i] << endl;
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
        int ret,len;
        while(running){
            if((cliSock = accept(sockfd,NULL,NULL))==-1){
                cout << "NETMONERROR" << strerror(errno) << endl;
                unlink(SOCKPATH);
                close(sockfd);
                exit(-1);
            }
            cout << "connected" << endl;
            ret = read(cliSock,BUF,BUFLEN);
            if (ret>0){
                if (strcmp(BUF,"READY")==0){
                   len = sprintf(BUF,"GETDATA")+1;
                   ret = write(cliSock,BUF,len);
                    if(ret < 0){
                        cout <<"NETMON::ERR WRITING GETDATA::" << strerror(errno) << endl;
                        close(sockfd);
                        exit(-1);
                    }
                }
                if (strcmp(BUF,"LINKDOWN")==0){
                    cout << "Setting Link Up" << endl;
                    len = sprintf(BUF,"SETLINK")+1;
                    ret = write(cliSock,BUF,len);
                    if(ret < 0 ){
                        cout << "NETMON::ERR WRITING SETLINK" << strerror(errno) << endl;
                        close(sockfd);
                        exit(-1);
                    }
                }

            }
            
        }
	}

}

static void sigHandler(int sig){
	switch(sig){
		case SIGINT:
			cout << "MAIN::SIGINT CAUGHT" << endl;
			for(int i = 0; i < infCount; i++){
				kill(arrpid[i], SIGINT);
			}
			running = false;
			break;
	}
}