#include <iostream>
#include <fstream>
#include <signal.h>
#include <string.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#define BUFLEN 256
using namespace std;
void monitorStatistics(string );
void shutdownHandler(int);
char SOCKPATH[] = "/tmp/netmonsock";
bool is_running;
bool is_up;

int main(int argc, char**argv){
    string intfName = argv[1];
    signal(SIGINT,shutdownHandler);
    //socket setup
    struct sockaddr_un addr;
    int sockfd, reSock;
    int len, ret;
    char BUF[BUFLEN];
    struct timeval tv; //using to set our socket delay
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    //creating our socket here
    cout << "intfmon starting::proc - " << getpid() << ":: INTFNAME - " << intfName << endl;
    memset(&addr,0,sizeof(addr));
    if ((sockfd = socket(AF_UNIX,SOCK_STREAM,0))==-1){
        cout << "Process:: " << getpid() << "::" << strerror(errno) << endl;
        exit(-1);
    }
    setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(const char*) &tv,sizeof(tv));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKPATH, sizeof(addr.sun_path)-1);
    //we start to connect to our local socket
    cout << getpid() << "::attempting to connect" << endl;
    if(connect(sockfd,(struct sockaddr*) &addr, sizeof(addr)) < 0){
        cout << "client: " << getpid() << " error:: " << strerror(errno) << endl;
        close(sockfd);
        exit(-1);
    }
    cout << getpid() << "::CONNECTED" << endl;
    //sending ready command to main program

    sprintf(BUF,"READY");
    len = strlen(BUF)+1;
    ret = write(sockfd,BUF,len); //write our "ready" command to the server

    if(ret < 0){
        cout << "INTFMON::Err Writing \'READY\' to socket - proc:: " << getpid() << " :: err: " << strerror(errno) << endl;
        close(sockfd);
        exit(-1);
    }
    is_running = true;
    while(is_running){
        //several commands -- "GETDATA" | "SETLINK" | "SHUTDOWN"
        //if strcmp == any of these, carry out actions
        ret = read(sockfd,BUF,BUFLEN);
        if(ret>0){
            if(strcmp(BUF,"GETDATA")==0){
                //we start monitoring
                cout << getpid()<<"::Monitoring" << endl;
               //do{
                    monitorStatistics(intfName);
                   len = sprintf(BUF,"READY") + 1;
                   ret = write(sockfd, BUF, len);
                   // ret = read(sockfd,BUF,BUFLEN);
              // }while(is_running && is_up && ret <0);
               if(!is_up){
                   cout << getpid()<<"::LINK DOWN!" << endl;
                   len = sprintf(BUF,"LINKDOWN") + 1;
                   ret = write(sockfd, BUF, len);
               }
            }
        }

        if(strcmp(BUF,"SETLINK")==0){
            cout << getpid() << "::RESTORING LINK" << endl;
            int tmpfd;
            struct ifreq ifr; //used to interact with an interface using netdevice
            tmpfd = socket(AF_INET,SOCK_DGRAM,0);
            if(tmpfd < 0){
                cout << getpid() << "::Err in SETLINK restoring" << endl;
            }
            memset(&ifr, 0, sizeof(ifr));
            strncpy(ifr.ifr_name, intfName.c_str(), IFNAMSIZ);
            ifr.ifr_flags |= IFF_UP;
            ret = ioctl(tmpfd, SIOCSIFFLAGS, &ifr);
            if(ret==-1){
                cout <<getpid() << "::Err in SETLINK (ioctl)" << endl;
                cout << strerror(errno) << endl;
            }else{
                cout <<getpid()<<"::LINK RESTORED" << endl;
                len = sprintf(BUF,"LINKUP") + 1;
                ret = write(sockfd,BUF,len);
            }
            close(tmpfd);

        }
        if(strcmp(BUF,"SHUTDOWN")==0){
            is_running = false;
        }
    }
    cout << getpid() <<"::Shutting Down" << endl;
    memset(BUF,0,BUFLEN);
    len = sprintf(BUF,"DONE") + 1;
    ret = write(sockfd,BUF,len);
    close(sockfd);
    return 0; 
}

void monitorStatistics(string name){
    string operstate, filepath;
    int up, down;
    int rx_bytes,rx_dropped,rx_errors,rx_packets;
    int tx_bytes,tx_dropped,tx_errors,tx_packets;
    ifstream statfile;
    string dir = "/sys/class/net/"+name;
    string bigone;
    //gathering data
    //operstate
    filepath = dir+"/operstate";
    statfile.open(filepath);
    statfile >> operstate;
    statfile.close();

    //up
    filepath = dir+"/carrier_up_count";
    statfile.open(filepath);
    statfile >> up;
    statfile.close();

    //down
    filepath = dir+"/carrier_down_count";
    statfile.open(filepath);
    statfile >> down;
    statfile.close();

    //rx_bytes
    filepath=dir+"/statistics/rx_bytes";
    statfile.open(filepath);
    statfile >> rx_bytes;
    statfile.close();

    //rx_dropped
    filepath=dir+"/statistics/rx_dropped";
    statfile.open(filepath);
    statfile >> rx_dropped;
    statfile.close();

    //rx_errors
    filepath=dir+"/statistics/rx_errors";
    statfile.open(filepath);
    statfile >> rx_errors;
    statfile.close();

    //rx_packets
    filepath=dir+"/statistics/rx_packets";
    statfile.open(filepath);
    statfile >> rx_packets;
    statfile.close();

    //tx_bytes
    filepath=dir+"/statistics/tx_bytes";
    statfile.open(filepath);
    statfile >> tx_bytes;
    statfile.close();

    //tx_dropped
    filepath=dir+"/statistics/tx_dropped";
    statfile.open(filepath);
    statfile >> tx_dropped;
    statfile.close();

    //tx_errors
    filepath=dir+"/statistics/tx_errors";
    statfile.open(filepath);
    statfile>>tx_errors;
    statfile.close();

    //tx_packets
    filepath = dir+"/statistics/tx_packets";
    statfile.open(filepath);
    statfile>>tx_errors;
    statfile.close();

    cout << "Interface: " << name.c_str() <<" state:"<<operstate<<" up_count:"<<up<<" down_count:"<<down<<endl;
    cout<<"rx_bytes:"<<rx_bytes<<" rx_dropped:"<<rx_dropped<<" rx_errors:"<<rx_errors<<" rx_packets:"<<rx_packets<<endl;
    cout<<"tx_bytes:"<<tx_bytes<<" tx_dropped:"<<tx_dropped<<" tx_errors:"<<tx_errors<<" tx_packets:"<<tx_packets<<endl<<endl;
    if(operstate=="down"){
        is_up = false;
    }else{
        is_up = true;
    }
    sleep(1);
}
    void shutdownHandler(int signal){
    switch(signal) {
        case SIGINT:
            cout << "INTFMON::SIGINT CAUGHT" << endl;
            is_running = false;
            break;
    }
}
