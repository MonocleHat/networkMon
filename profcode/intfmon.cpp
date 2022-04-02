//intfMonitor.cpp - monitors the statistics of an ethernet interface
//
// .

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
#include "intfmon.h"

using namespace std;

char socket_path[]="/tmp/netmon";
bool is_running;
bool is_up;
const int BUF_LEN=100;

int main(int argc, char **argv)
{
    if(argc!=2) {
        cout<<"intfMonitor: incorrect argument list"<<endl;
	return -1;
    }
    string intfName = argv[1];
    signal(SIGINT, shutdownHandler);

    //Set up socket communications
    struct sockaddr_un addr;
    char buf[BUF_LEN];
    int len, ret;
    int fd,rc;

    memset(&addr, 0, sizeof(addr));
    //Create the socket
    if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        cout << "intfMonitor: "<<strerror(errno)<<endl;
        exit(-1);
    }

    addr.sun_family = AF_UNIX;
    //Set the socket path to a local socket file
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);

    //Connect to the local socket
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        cout << "client1: " << strerror(errno) << endl;
        close(fd);
        exit(-1);
    }
    sprintf(buf, "Ready");
    len = strlen(buf)+1;
    ret = write(fd, buf, len);
    if(ret<0) {
        cout<<"intfMonitor: Error writing the first socket"<<endl;
        cout<<strerror(errno)<<endl;
        close(fd);
        exit(-1);
    }

    is_running = true;
    while(is_running) {
        ret = read(fd,buf,BUF_LEN);
        if(ret<0) {
            cout<<"intfMonitor: error reading the socket"<<endl;
            cout<<strerror(errno)<<endl;
        }
	if(strcmp(buf, "Monitor")==0) {//Start monitoring
	    memset(buf, 0, BUF_LEN);
            len = sprintf(buf, "Monitoring") + 1;
	    ret = write(fd, buf, len);
	    do{
                monitorStatistics(intfName);
		sleep(1);
	    } while(is_running && is_up);
            if(!is_up) {
                cout<<"intfMonitor: Link Down"<<endl;
                len = sprintf(buf, "Link Down") + 1;
                ret = write(fd, buf, len);
            }
        } else if(strcmp(buf, "Set Link Up")==0) {//Set the link back up
            cout<<"intfMonitor: Setting Link Up"<<endl;
//See https://stackoverflow.com/questions/5858655/linux-programmatically-up-down-an-interface-kernel
            int sockfd;
            struct ifreq ifr;
            sockfd = socket(AF_INET, SOCK_DGRAM, 0);
#ifdef DEBUG
	    cout<<"intfMonitor: sockfd:"<<sockfd<<endl;
#endif
            memset(&ifr, 0, sizeof ifr);
            strncpy(ifr.ifr_name, intfName.c_str(), IFNAMSIZ);
#ifdef DEBUG
	    cout<<"intfMonitor: ifr_name:"<<ifr.ifr_name<<endl;
#endif
            ifr.ifr_flags |= IFF_UP;
            ret=ioctl(sockfd, SIOCSIFFLAGS, &ifr);
#ifdef DEBUG
	    cout<<"intfMonitor: ret:"<<ret<<endl;
#endif
	    if(ret==-1) cout<<strerror(errno)<<endl;
	    else {
                cout<<"intfMonitor: Link Up"<<endl;
                len = sprintf(buf, "Link Up") + 1;
                ret = write(fd, buf, len);
	    }
	    close(sockfd);
        } else if(strcmp(buf, "Shut Down")==0) {//Terminate monitoring
            is_running = false;
        }

    }
    cout<<"intfMonitor: Shutting Down"<<endl;
    memset(buf, 0, BUF_LEN);
    len = sprintf(buf, "Done") + 1;
    ret = write(fd, buf, len);

    close(fd);
    return 0;
}

void monitorStatistics(string intf)
{
    string operstate;
    int up_count, down_count;
    int rx_bytes, rx_dropped, rx_errors, rx_packets;
    int tx_bytes, tx_dropped, tx_errors, tx_packets;
    string filename;
    ifstream iffile;
    string intfDir="/sys/class/net/"+intf;

    //operstate
    filename = intfDir+"/operstate";
    iffile.open(filename);
    iffile>>operstate;
    iffile.close();

    //up_count
    filename = intfDir+"/carrier_up_count";
    iffile.open(filename);
    iffile>>up_count;
    iffile.close();

    //down_count
    filename = intfDir+"/carrier_down_count";
    iffile.open(filename);
    iffile>>down_count;
    iffile.close();

    //rx_bytes
    filename = intfDir+"/statistics/rx_bytes";
    iffile.open(filename);
    iffile>>rx_bytes;
    iffile.close();

    //rx_dropped
    filename = intfDir+"/statistics/rx_dropped";
    iffile.open(filename);
    iffile>>rx_dropped;
    iffile.close();

    //rx_errors
    filename = intfDir+"/statistics/rx_errors";
    iffile.open(filename);
    iffile>>rx_errors;
    iffile.close();

    //rx_packets
    filename = intfDir+"/statistics/rx_packets";
    iffile.open(filename);
    iffile>>rx_packets;
    iffile.close();

    //tx_bytes
    filename = intfDir+"/statistics/tx_bytes";
    iffile.open(filename);
    iffile>>tx_bytes;
    iffile.close();

    //tx_dropped
    filename = intfDir+"/statistics/tx_dropped";
    iffile.open(filename);
    iffile>>tx_dropped;
    iffile.close();

    //tx_errors
    filename = intfDir+"/statistics/tx_errors";
    iffile.open(filename);
    iffile>>tx_errors;
    iffile.close();

    //tx_packets
    filename = intfDir+"/statistics/tx_packets";
    iffile.open(filename);
    iffile>>tx_packets;
    iffile.close();

    cout<<"Interface: "<<intf.c_str()<<" state:"<<operstate<<" up_count:"<<up_count<<" down_count:"<<down_count<<endl;
    cout<<"rx_bytes:"<<rx_bytes<<" rx_dropped:"<<rx_dropped<<" rx_errors:"<<rx_errors<<" rx_packets:"<<rx_packets<<endl;
    cout<<"tx_bytes:"<<tx_bytes<<" tx_dropped:"<<tx_dropped<<" tx_errors:"<<tx_errors<<" tx_packets:"<<tx_packets<<endl<<endl;

    if(iffile.is_open()) iffile.close();
    if(operstate=="down") is_up = false;
    else is_up = true;
    sleep(1);
}

void shutdownHandler(int signal)
{
    switch(signal) {
        case SIGINT://Put shutdown code here
            cout<<"Interface Monitor shutting down"<<endl;
            is_running = false;
            break;
    }
}
