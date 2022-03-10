#include <signal.h>
#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
using namespace std;
char SOCKPATH[] = "/tmp/netmonsock";
static void sigHandler(int);
int main(int argc, char*argv[]){
    /*
    * TODO:
    * store the interface to open in string
    * standup the socket connection
    * connect to parent
    * send "LISTENER READY" to host
    * host send HOST READY to listener
    */
   //-=-=-=-=-=-=-=-=-=-=-
   //Building Signal handler vars
   struct sigaction sigcontrol;
   sigcontrol.sa_handler = sigHandler;
   sigemptyset(&sigcontrol.sa_mask);
   sigcontrol.sa_flags = 0;
   sigaction(SIGINT, &sigcontrol, 0 );
   //-=-=-=-=-=-=-=-=-=-=-
   //Socket and Interface Info
   string intfName;
   struct sockaddr_un addr;
   int sockfd;
   int hosSock, reSock;
    bool running = false;
   char intfCommand[256];
   char BUF[128];
   cout << "DEBUG: " << intfCommand << endl;

   memset(&addr, 0, sizeof(addr));
   if ((sockfd = socket(AF_UNIX, SOCK_STREAM,0))==-1){
       cout << "PROCESS:: " << getpid() << "::";
       perror("intfMon::SockInit");
       exit(-1);
   }
   addr.sun_family = AF_UNIX;
   strncpy(addr.sun_path, SOCKPATH, sizeof(addr.sun_path)-1);
   cout << getpid() << "::INIT!!!" << endl;
   if (connect(sockfd,(struct sockaddr*) &addr,sizeof(addr))<0){
       cout <<"PROCESS:: " << getpid() << "::" << endl;
       perror("intfMon::CONNECT");
       exit(-1);
   }
   cout << getpid() << "::CONNECT" << endl;
    intfName = argv[1]; //set name up
    sprintf(intfCommand,"%d:READY",getpid());

    write(sockfd, intfCommand,sizeof(intfCommand)); //should send ready command
    reSock = read(sockfd, BUF,sizeof(BUF));
    string recv;
	string token;
	string parsedcommand;
            recv = BUF;
			token = recv.substr(0,recv.find(':'));
			recv.erase(0,recv.find(':'));
			parsedcommand = recv;
        if(strncmp("STANDBY",parsedcommand.c_str(),7)==0){
            running = true;
        }
    while(running){
        //collect and output interface data
        read(sockfd,BUF,sizeof(BUF));
        if(strncmp("GETDATA",BUF,7)==0){
            //grab data
        }
    }


}

static void sigHandler(int sig){
    switch(sig){
        case SIGINT:
            cout << "PLACEHOLDER" << endl;
            break;
        case SIGUSR1:
            cout << "PLACEHOLDER" << endl;
            
    }
