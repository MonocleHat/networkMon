#include <signal.h>
#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
using namespace std;
char SOCKPATH[] = "/tmp/netmonsock";
static void sigHandler(int);
  bool SIGONOFF = false;
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
   char intfCommand[528];
   char BUF[128];
   //-=-=-=-=-=-=-=-=-=-=-
   //Storing our data collection
    int carr_up_cnt = 0;
    int carr_down_cnt = 0;
    int rx_bytes = 0;
    int rx_dropped = 0;
    int rx_errors = 0;
    int rx_packets = 0;
    int tx_bytes = 0;
    int tx_dropped = 0;
    int tx_errors = 0;
    int tx_packets = 0;
    char opstate[32];
    
    char filePath[256];
  
    //-=-=-=-=-=-=-=-=-=-
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
   cout << getpid() << "::CONNECTED" << endl;
    intfName = argv[1]; //set name up
    sprintf(intfCommand,"%d:READY",getpid());
    cout << intfCommand << endl;
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
    while(running && !SIGONOFF){
        //collect and output interface data
        read(sockfd,BUF,sizeof(BUF));
        if(strncmp("GETDATA",BUF,7)==0){
           ifstream statfile;
           sprintf(filePath,"/sys/class/net/%s/operstate",intfName);
           statfile.open(filePath);
           if(statfile.is_open()){
               statfile >> opstate;
               statfile.close();
           }
           sprintf(filePath,"/sys/class/net/%s/carrier_up_count",intfName);
           statfile.open(filePath);
           if(statfile.is_open()){
               statfile >> carr_up_cnt;
               statfile.close();
           }
           sprintf(filePath,"/sys/class/net/%s/carrier_down_count",intfName);
           statfile.open(filePath);
           if(statfile.is_open()){
               statfile >> carr_down_cnt;
               statfile.close();
           }

           sprintf(filePath,"/sys/class/net/%s/statistics/rx_bytes",intfName);
           statfile.open(filePath);
           if(statfile.is_open()){
               statfile >> rx_bytes;
               statfile.close();
           }
           sprintf(filePath,"/sys/class/net/%s/statistics/rx_dropped",intfName);
           statfile.open(filePath);
           if(statfile.is_open()){
               statfile >> rx_dropped;
               statfile.close();
           }
            sprintf(filePath,"/sys/class/net/%s/statistics/rx_errors",intfName);
           statfile.open(filePath);
           if(statfile.is_open()){
               statfile >> rx_errors;
               statfile.close();
           }
           sprintf(filePath,"/sys/class/net/%s/statistics/rx_packets",intfName);
           statfile.open(filePath);
           if(statfile.is_open()){
               statfile >> rx_packets;
               statfile.close();
           }
           sprintf(filePath,"/sys/class/net/%s/statistics/tx_bytes",intfName);
           statfile.open(filePath);
           if(statfile.is_open()){
               statfile >> tx_bytes;
               statfile.close();
           }
              sprintf(filePath,"/sys/class/net/%s/statistics/tx_dropped",intfName);
           statfile.open(filePath);
           if(statfile.is_open()){
               statfile >> tx_dropped;
               statfile.close();
           }
               sprintf(filePath,"/sys/class/net/%s/statistics/tx_errors",intfName);
           statfile.open(filePath);
           if(statfile.is_open()){
               statfile >> tx_errors;
               statfile.close();
           }
             sprintf(filePath,"/sys/class/net/%s/statistics/tx_packets",intfName);
           statfile.open(filePath);
           if(statfile.is_open()){
               statfile >> tx_packets;
               statfile.close();
           }
           int len = sprintf(intfCommand, "%s::Operating State:%s\n rx_bytes:%d rx_dropped:%d rx_packets:%d rx_errors:%d\n tx_bytes:%d tx_dropped:%d tx_packets:%d tx_errors:%d\ncarrier_UP:%d carrier_DOWN:%d",intfName,opstate,rx_bytes,rx_dropped,rx_packets,rx_errors,tx_bytes,tx_dropped,tx_packets,tx_errors,carr_up_cnt,carr_down_cnt);
        }
    }
    if(SIGONOFF == true){
        close(sockfd);
        cout << getpid() << " :: CLOSING" << endl;
        exit(1);
    }


}

static void sigHandler(int sig){
    switch(sig){
        case SIGINT:
            cout << "SIGINT::CAUGHT BY CHILD" << endl;
            break;
        case SIGUSR1:
            cout << "SIGUSR1::CAUGHT BY CHILD" << endl;
            break;
        case SIGUSR2:
            cout << "SIGUSR2::CAUGHT BY CHILD" << endl;
            SIGONOFF = true;
            break;
            
    }
}
