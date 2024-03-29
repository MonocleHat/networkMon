/*
 * NETMON PROTO
 * PoC for assignment 1
 * demonstrates signal setup, and input from user
 * registers a handler for socket
 * outputs data received and closes system. 
 */
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
using namespace std;
//We need to register a socket path
char SOCKPATH[] = "/tmp/netmonsock";
static void sigHandler(int);
pid_t *arrpid;
int infCount = 0;
bool running = true;
int main(){
	char BUF[1024];
	struct sigaction sigcontrol; //for registering signals
	string intfname;
	string *intfarr;
	
	int choice;
	
	bool parentProc = true;
	//socket construction
	//---------------------
	struct sockaddr_un addr;
	int sockfd;
	int cliSock, reSock;
	//---------------------
	//SIGNAL CONSTRUCTION
	//---------------------
	sigcontrol.sa_handler = sigHandler;
	sigemptyset(&sigcontrol.sa_mask);
	sigcontrol.sa_flags = 0;
	sigaction(SIGINT, &sigcontrol, NULL);
	//setupsignals later

	cout << "-----SPINNING UP------" << endl;
	cout << "NetMon program (0.01)" << endl;
	cout << "Before running we need information!" << endl;
	cout << "Specify number of interfaces to monitor: ";
	cin >> infCount;
	cout << "----------------------" << endl;
	cout << "Enter each Interface Name" << endl;
	intfarr = new string[infCount];
	arrpid = new pid_t[infCount];
	for (int i = 0; i < infCount; i++){
		cout << "Intf " << i << ": ";
		cin >> intfname;
		intfarr[i] = intfname;
	}
	cout << "Review the following interfaces, if they are incorrect, press 0 to exit program, if correct, press 1" << endl;
	for (int i = 0; i < infCount; i++){
		cout << " intf: " << i << " :: " << intfarr[i] << endl;
	}
	cout << "Correct? (1=Y/0=N): ";
	cin >> choice;
	bool flag = false;
	while(!flag){

		switch(choice){
			case 0:
				cout << "TERMINATING" << endl;
				delete [] intfarr;
				exit(-1);
				break;
			case 1:
				flag = true;
				break;
			default:
				cout << "Undefined Input; are your interfaces correct? 1=Y/0=N: " << endl;
				cin >> choice;
				break;
		}
	}
	
	for(int i = 0; (i < infCount) & parentProc; i++){
		arrpid[i] = fork();
		if(arrpid[i] == 0){
			parentProc = false;
			execlp("./intfMon", "./intfMon", intfarr[i].c_str(), NULL);
			cout << "SHOULDNT SEE ME" << endl;
		}
	}
	
	if(parentProc){
//choices confirmed we start creating our sockets
	//initialization
	cout << "Initializing sockets, this may take a moment" << endl;
	memset(&addr, 0, sizeof(addr));
	if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0))<0){
		perror("INIT::SOCK::HOST");
		exit(-1);
	}

	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCKPATH, sizeof(addr.sun_path)-1);
	unlink(SOCKPATH);

	//binding
	if(bind(sockfd,(struct sockaddr*)&addr, sizeof(addr)) == -1){
		perror("BIND::SOCK::HOST");
		exit(-1);
	}

	if (listen(sockfd,infCount)==-1){
		cout <<"SERVER: " << strerror(errno) << endl;
		unlink(SOCKPATH);
		close(sockfd);
		exit(-1);
	}
	int totalcnt = 0;
	while(totalcnt != infCount){
		cout << "Waiting for clients to accept" <<endl;
		if((cliSock = accept(sockfd,NULL,NULL))==-1){
			cout << "SERVER ACC::" << strerror(errno) << endl;
			unlink(SOCKPATH);
			close(sockfd);
			exit(-1);
		}
		cout << "waiting for read" << endl;
		memset(&BUF,0,sizeof(BUF));
		reSock=read(cliSock,BUF,sizeof(BUF));
		cout << "NETMON::MAIN::"<< BUF << endl;
		memset(&BUF,0,sizeof(BUF));
			if(BUF == "READY"){
				sprintf(BUF,"STANDBY");
				write(cliSock,BUF,sizeof(BUF));
				totalcnt++;
			}
		//totalcnt++;
		

	}
	cout << "NETMON::OUT OF SETUP" << endl;
	running = true;
	while(running){
		cout << "SENDING GET DATA COMMAND" << endl;
		sprintf(BUF,"GETDATA");
		write(cliSock,BUF,sizeof(BUF));
		cout << "DEBUG::BEFORE SENDING COMMAND" << endl;
		while((reSock=read(cliSock,BUF,sizeof(BUF)))>0){ //this never pings
			cout << "DEBUG::INWHILE--MAIN::" << BUF << endl;
		}
		sleep(3);
	}
	}
	cout << "IF SEE THIS FIRST, IT SKIPPED" << endl;
	pid_t ret = 0;
	int status = -1;
	while(ret>=0){
		ret = wait(&status);
		cout << "PID: " << ret << " has concluded" << endl;
	}
	unlink(SOCKPATH);
	delete [] arrpid;
	delete [] intfarr;
	close(sockfd);
	close(cliSock);
	
	
}

static void sigHandler(int sig){
	switch(sig){
		case SIGINT:
			cout << "MAIN::SIGINT CAUGHT" << endl;
			for(int i = 0; i < infCount; i++){
				kill(arrpid[i], SIGUSR2);
			}
			running = false;
			break;
	}
}

