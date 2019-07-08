#include "server.h"
#include <thread>
#include <vector>
#include <map>
#include <algorithm>
#include <mutex>
#include <fstream>

class ClockServer{
public:
	ClockServer(uint64_t clockId, int delaySecs)
	    :address(ADDRESS), port(PORT), clockId_(clockId), delaySecs_(delaySecs)
	{
        struct in_addr localInterface;

	    fd = socket(AF_INET, SOCK_DGRAM, 0);
	    std::memset(&groupSock, 0, sizeof(groupSock));
	    groupSock.sin_family = AF_INET;
	    groupSock.sin_addr.s_addr = inet_addr(address);
	    groupSock.sin_port = htons(port);

	}
	void sendMsg()
	{
        cout << "Address  : " << ADDRESS << endl;
	    while(1){
            ClockSyncMessage msg;

            msg.clock_id = clockId_;
            msg.server_ts = get_current_time_ts();
            msg.setCheckSum();
            cout << "sending message with clock_id = " << clockId_ << "server_ts = " << msg.server_ts << endl;
            int nbytes = sendto(
                fd,
                &msg,
                sizeof(msg),
                0,
                (struct sockaddr*) & groupSock,
                sizeof(groupSock)
                );
            if(nbytes <0){
                perror("sendto");
                return;
            }
            sleep(delaySecs_);
        }
	}
	void receiveMsg()
	{
        while(1){
            ClockSyncMessage msg;
            socklen_t addrlen = sizeof(groupSock);
            struct sockaddr_in cliaddr;
            int nbytes = recvfrom(
                fd,
                &msg,
                sizeof(msg),
                0,
                (struct sockaddr *) &cliaddr,
                &addrlen
            );
            if (nbytes < 0) {
                perror("recvfrom");
                return;
            }
            // update final_ts and calculate msg.offset_us
            msg.final_ts = get_current_time_ts();
            msg.setOffset_us();

            // store msg to messageMap
            {
                std::lock_guard<std::mutex> guard(mu);
                it_type it= messageMap_.find(msg.clock_id);
                if(it == messageMap_.end()){
                    vector<ClockSyncMessage> client_vec= {msg};
                    messageMap_[msg.clock_id] = client_vec;
                }
                else
                    it->second.push_back(msg);
            }


            std::cout << "received a msg , clokc_id=" << msg.clock_id << "|server_ts=" << msg.server_ts << "|client_ts="<< msg.client_ts << std::endl;
        }
	}

    void printMinuteLog(fstream & log){
        while(1){
            {
                std::lock_guard<std::mutex> guard(mu);

                for( it_type it = messageMap_.begin(); it != messageMap_.end();){
                    uint64_t timestamp = get_current_time_ts();
                    uint32_t client_id = it->first;
                    vector<ClockSyncMessage>& msgVec = it->second;


                    double sumOffset = 0.;
                    double minOffset = 0.;
                    double maxOffset = 0.;
                    double meanOffset = 0.;
                    double medianOffset = 0.;

                    //sort the vector for median
                    std::sort(
                            msgVec.begin(), 
                            msgVec.end(), 
                            [](ClockSyncMessage& a, ClockSyncMessage &b){
                                return a.offset_us < b.offset_us;});
                
                    typedef vector<ClockSyncMessage>::iterator msgVecIterator;
                    for(msgVecIterator itVec = msgVec.begin(); itVec != msgVec.end(); ++itVec ){
                        if(itVec == msgVec.begin()){
                            minOffset = itVec->offset_us;
                            maxOffset = itVec->offset_us;
                        }
                        else{
                            minOffset = min(minOffset, itVec->offset_us);
                            maxOffset = max(maxOffset, itVec->offset_us);
                        }
                        sumOffset += itVec->offset_us;
                    }
                    
                    size_t len = msgVec.size();
                    if(len){ 
                        meanOffset = sumOffset * 1.0 / len;
                        int index = len/2;
                        medianOffset = len%2==1? msgVec[index].offset_us : 0.5 *(msgVec[index-1].offset_us + msgVec[index].offset_us);
                    }
                    time_t t;
                    struct tm *tmp;
                    char MY_TIME[100];
                    time(&t);

                    tmp = localtime(&t);
                    strftime(MY_TIME, sizeof(MY_TIME), "%Y-%m-%d %I:%M:%S.%s", tmp);

                    cout << MY_TIME << ','<<client_id <<',' << len <<','<<minOffset << ',' << meanOffset<< ',' << medianOffset
                        <<','<< maxOffset<<endl;
                    log << MY_TIME << ','<<client_id << ',' << len << ',' << minOffset<<',' << meanOffset << ',' << medianOffset << ',' << maxOffset << endl;
                    
                    it = messageMap_.erase(it);
                }
            }
            sleep(10);

        }
    }

private:
	struct sockaddr_in groupSock;
	const char* address;
	const int port;
    const uint64_t clockId_;
	const int delaySecs_ = 1;
    std::mutex mu;

    typedef map<uint32_t, vector<ClockSyncMessage>>::iterator it_type;
    map<uint32_t, vector<ClockSyncMessage>> messageMap_;

	int fd;
};

void runSendMsg(ClockServer& server)
{
    server.sendMsg();
    return;
}
void runPrintLog(ClockServer &server)
{   
    fstream fs;
    fs.open("clock_server.out", fstream::in | fstream::out | fstream::app);
    fs << "Log printed" << endl;
    server.printMinuteLog( fs );
    fs.close();
    return;
}
int main(int argc, char *argv[])
{
    if(argc !=3 && argc != 2){
        std::cout << "Please give <clock_id> [interval] parameters" << std::endl;
        return 0;
    }
    int clockId = atoi(argv[1]);
    int delaySecs = 10;
    if(argc == 3)
        delaySecs = atoi(argv[2]);

    ClockServer server(clockId, delaySecs);
    std::thread t1(runSendMsg, std::ref(server));
    std::thread t2(runPrintLog, std::ref(server));
    server.receiveMsg();
    t1.join();
    t2.join();

}
