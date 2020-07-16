#include <time.h>
#include "server.h"

using namespace std;

int main(int argc, char *argv[])
{
    if(argc !=3 && argc != 2){
        std::cout << "Please give <client_id> [clock_id] parameters" << std::endl;
        return 0;
    }
    int clientId = atoi(argv[1]);
    int clockId  = -1;
    if(argc == 3)
        clockId = atoi(argv[2]);

    // create what looks like an ordinary UDP socket
    //
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    // allow multiple sockets to use the same PORT number
    u_int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*) &yes, sizeof(yes)) < 0){
       perror("Reusing ADDR failed");
       return 1;
    }

    // set up destination address
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // differs from sender
    addr.sin_port = htons(PORT);

    // bind to receive address
    if (::bind(fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }
    
    cout<< " checking address: "<< addr.sin_addr.s_addr << endl;

    // use setsockopt() to request that the kernel join a multicast group
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(ADDRESS);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq) ) < 0){
        perror("setsockopt");
        return 1;
    }

    cout << "ADDRESS : " << ADDRESS << endl;
    // now just enter a read-print loop
    //
    while (1) {
        ClockSyncMessage message;
	struct sockaddr_in server_addr;
        socklen_t addrlen = sizeof(server_addr);
        int nbytes = recvfrom(
            fd,
            &message,
            sizeof(message),
            0,
            (struct sockaddr *) & server_addr,
            &addrlen
        );
        if (nbytes < 0) {
            perror("recvfrom");
            return 1;
        }
        cout <<" received one message from server, clock_id = " << message.clock_id <<"|address=" <<server_addr.sin_addr.s_addr<< endl;
        
        message.clock_id = clientId;
        message.client_ts = get_current_time_ts();
        message.setCheckSum();
        cout << "send message with client_id = " << clientId << endl;
        nbytes = sendto(
		    fd,
		    &message,
		    sizeof(message),
		    0,
		    (struct sockaddr*) & server_addr,
		    sizeof(server_addr)
		    );
	    if(nbytes <0){
		perror("sendto");
		    return 1;
	    }

     }


    return 0;
}

