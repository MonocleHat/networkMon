//networkMonitor.cpp - monitors network devices
//
// 16-Feb-19  M. Watler         Created.
// 19-Feb-20  M. Watler         Modified to use exec() to start the interface monitors,
//                              and to control all socket connections from here.
// 22-Oct-20  M. Watler         Shut down the children via kill signal as well.

#include <iostream>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>
#include "netmon.h"

using namespace std;

char socket_path[]="/tmp/netmon";
bool is_running;
const int MAX_CONNECTIONS=16;
const int BUF_LEN=100;

int main()
{
    int numIntf;
    int numClients;
    int childId[MAX_CONNECTIONS];
    int master_fd, max_fd, cl[MAX_CONNECTIONS];
    fd_set activefds;
    fd_set readfds;
    int ret;

    vector<string> intfName;
    cout<<"How many interfaces do you want to monitor: ";
    cin>>numIntf;
    for(int i=0; i<numIntf; ++i) {
        string interface;
        cout<<"Enter interface "<<to_string(i+1)<<": ";
        cin>>interface;
	intfName.push_back(interface);
    }
    //Intercept ctrl-C
    struct sigaction action;

    action.sa_handler = shutdownHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, NULL);

    //Create the socket for inter-process communications
    struct sockaddr_un addr;
    char buf[BUF_LEN];
    int len;

    //Create the socket
    memset(&addr, 0, sizeof(addr));
    if ( (master_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0) {
#ifdef DEBUG
        cout << "server: " << strerror(errno) << endl;
#endif
        exit(-1);
    }

    addr.sun_family = AF_UNIX;
    //Set the socket path to a local socket file
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
    unlink(socket_path);

    //Bind the socket to this local socket file
    if (bind(master_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
#ifdef DEBUG
        cout << "server: " << strerror(errno) << endl;
#endif
        close(master_fd);
        exit(-1);
    }

    cout<<"Waiting for the interfaces..."<<endl;
    //Listen for a client to connect to this local socket file
    if (listen(master_fd, 5) == -1) {
#ifdef DEBUG
        cout << "server: " << strerror(errno) << endl;
#endif
        unlink(socket_path);
        close(master_fd);
        exit(-1);
    }

    //Start the interface monitors
    for(int i=0; i<numIntf; ++i) {
        childId[i] = fork();
	if(childId[i]==0) {
            cout<<"Starting the monitor for the interface "<<intfName.at(i)<<endl;
	    execlp("./intfMonitor", "./intfMonitor", intfName.at(i).c_str(), NULL); 
	    cout<<"Error starting the interface monitor for "<<intfName.at(i)<<endl;
	    cout<<strerror(errno)<<endl;
	    return -1;
        }
    }

    FD_ZERO(&readfds);
    FD_ZERO(&activefds);
    //Add the master_fd to the socket set
    FD_SET(master_fd, &activefds);
    max_fd = master_fd;
#ifdef DEBUG
    cout<<"Accept all connections"<<endl;
#endif
    is_running=true;
    numClients=0;

    while(is_running) {
        //Block until an input arrives on one or more sockets
        readfds = activefds;
	ret = select(max_fd+1, &readfds, NULL, NULL, NULL);
        if (ret < 0) {
            cout << "server: " << strerror(errno) << endl;
        } else {//Service all the sockets with input pending
            if (FD_ISSET (master_fd, &readfds))
            {//Connection request on the master socket
                cl[numClients] = accept(master_fd, NULL, NULL);//Accept the new connection
                if (cl[numClients] < 0) {
                    cout << "server: " << strerror(errno) << endl;
                } else {
                    cout<<"Server: incoming connection "<<cl[numClients]<<endl;
                    FD_SET (cl[numClients], &activefds);//Add the new connection to the set
                    if(max_fd<cl[numClients]) max_fd=cl[numClients];//Update the maximum fd
                    ++numClients;
                }
            }
            else//Data arriving on an already-connected socket
            {
                for (int i = 0; i < numClients; ++i) {//Find which client sent the data
                    if (FD_ISSET (cl[i], &readfds)) {
                        ret = read(cl[i],buf,BUF_LEN);//Read the data from that client
                        if(ret==-1) {
                            cout<<"server: Read Error"<<endl;
                            cout<<strerror(errno)<<endl;
                        }

                        if(strcmp(buf, "Ready")==0) {//Start monitoring
                            len = sprintf(buf, "Monitor") + 1;
                            ret = write(cl[i], buf, len);//TODO: Error handling
                        } else if(strcmp(buf, "Monitoring")==0) {//Do nothing
                        } else if(strcmp(buf, "Link Down")==0) {//Set the Link back up
                            cout<<"networkMonitor: Link Down"<<endl;
                            len = sprintf(buf, "Set Link Up") + 1;
#ifdef DEBUG
                            cout<<"networkMonitor: Set Link Up"<<endl;
#endif
                            ret = write(cl[i], buf, len);//TODO: Error handling
                	} else if(strcmp(buf, "Link Up")==0) {//Start monitoring
                            len = sprintf(buf, "Monitor") + 1;
                            ret = write(cl[i], buf, len);//TODO: Error handling
                        } else if(strcmp(buf, "Done")==0) {//The interface monitor has shut down
                            cout<<"networkMonitor: connection "<<cl[i]<<" has terminated"<<endl;
                	    close(cl[i]);
                        }
                    }
                }
            }
        }
    }

    cout<<"networkMonitor: closing connections"<<endl;
    for(int i=0; i<numClients; ++i) {
        len = sprintf(buf, "Shut Down") + 1;//Shut down the interface monitor
        ret = write(cl[i], buf, len);//TODO: Error handling
        close(cl[i]);
	kill(childId[i], SIGINT);
    }
    close(master_fd);
    unlink(socket_path);
    return 0;
}

void shutdownHandler(int signal)
{
    switch(signal) {
        case SIGINT://Put shutdown code here
	    cout<<"Network Monitor shutting down"<<endl;
	    is_running = false;
            break;
    }
}
