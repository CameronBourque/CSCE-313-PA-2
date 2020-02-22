/*
    Cameron Bourque
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/21/20
 */
#include "common.h"
#include "FIFOreqchannel.h"
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>

using namespace std;


double requestData(int patient, double time, int ecg, FIFORequestChannel* chan){
    //WRITE REQUEST OUT
    datamsg msg(patient, time, ecg);
    chan->cwrite(&msg, sizeof(msg));

    //RECEIVE DATA POINT
    double value;
    int retLen = chan->cread((char*)&value, sizeof(value));

    return value;
}

void requestFile(string filename, int mem, FIFORequestChannel* chan){

    //REQUEST FILE LENGTH
    unsigned char requestBuf[filename.size() + 1 + sizeof(filemsg)];
    filemsg msg(0, 0);
    msg.mtype = FILE_MSG;
    msg.offset = 0;
    msg.length = 0;
    memcpy((char*)requestBuf, (char*)&msg, sizeof(filemsg));
    strcpy( ((char*)requestBuf) + sizeof(filemsg), filename.c_str());
    chan->cwrite(&requestBuf, filename.size() + 1 + sizeof(filemsg));

    //RECEIVE FILE LENGTH
    __int64_t fileSize = 0;
    int rec = chan->cread((char*)&fileSize, sizeof(fileSize));
    if(!fileSize) {
        cout << "Cannot get data from file" << endl;
        return;
    }

    //NOW GET DATA AND WRITE OUT
    ofstream ofile;
    string ofName = "received/" + filename; 
    ofile.open(ofName, ios::binary);

    int runs = ceil((double)fileSize / (double)mem);
    msg.length = mem;
    for(int i = 0; i < runs; i++){
        if(runs - i == 1){
            msg.length = (fileSize-msg.offset) % (__int64_t)mem;
	    if(!msg.length){
                msg.length = (__int64_t)mem;
	    }
        }
        memcpy((char*)requestBuf, (char*)&msg, sizeof(filemsg)); //overwrite the old msg request
        chan->cwrite(&requestBuf, filename.size() + 1 + sizeof(filemsg));

        unsigned char rsp[mem];
        int bytes = chan->cread((char*)rsp, mem);
        if(bytes < 1) {
            cout << "no bytes received" << endl;
            ofile.close();
            return;
        }

        ofile.write((char*)rsp, bytes);

        msg.offset += mem;
    }

    ofile.close();
}

double getRandTime(){
    int preDecimal = rand() % 60;
    int postDecimal = rand() % 250 * 4;
    string mash = to_string(preDecimal) + "." + to_string(postDecimal);
    return atof(mash.c_str());
}

string getTimeDiff(struct timeval* start, struct timeval* end){
    long sec = end->tv_sec - start->tv_sec;
    long musec = end->tv_usec - start->tv_usec;

    if(musec < 0){
        musec += (int)1e6;
        sec--;
    }

    return "sec = " + to_string(sec) + ", musec = " + to_string(musec);
}

string requestNewChannel(FIFORequestChannel* chan){
    //SEND CHANNEL REQUEST OUT
    MESSAGE_TYPE msg = NEWCHANNEL_MSG;
    chan->cwrite(&msg, sizeof(MESSAGE_TYPE));

    //RECEIVE CHANNEL STRING NAME
    unsigned char buf[MAX_MESSAGE];
    int bytes = chan->cread((char*)buf, MAX_MESSAGE);
    if(bytes < 1){
        cout << "no bytes received, channel not created" << endl;
        return "";
    }
    string chanName = (char*)buf;

    return chanName;
}

int main(int argc, char *argv[]){
    int n = 100;    // default number of requests per "patient"
    int p = 15;		// number of patients
    srand(time_t(NULL));

    struct timeval seedGen; //ACTUAL RANDOM NUMBER GENERATION
    gettimeofday(&seedGen, NULL);
    srand(seedGen.tv_usec * seedGen.tv_sec);

    //GET ARGUMENTS
    int patient, ecg;
    int mem = MAX_MESSAGE;
    double time;
    bool channel = false;
    string filename;
    int c;

    while((c = getopt(argc, argv, ":c:t:p:e:m:f:")) != -1){
        switch(c){
            case 't':
                time = atof(optarg);
                break;
            case 'p':
                patient = atoi(optarg);
                break;
            case 'e':
                ecg = atoi(optarg);
                break;
            case 'm':
                mem = atoi(optarg);
                break;
            case 'f':
                filename = optarg;
                break;
            case ':':
                switch(optopt){
                    case 'c':
                        channel = true;
                        break;
                }
                break;
        }
    }

    //FORK AND SPLIT SERVER HERE
    char* args[] = {"./server", "-m", (char*)to_string(mem).c_str(), NULL};
    int childP = fork();
    if(!childP){
        cout << "Starting server" << endl;
        int out = execvp("./server", args);
        if(out){
            cout << "FAILED TO START SERVER" << endl;
        }
    }
    
    FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);

    struct timeval start, end;

    //SINGLE DATA POINT REQUEST
    if(patient > 0 && patient <= p && time >= 0.000 && time < 59.999 && ecg <= 2 && ecg > 0){
        cout << "Requesting data point from input arguments" << endl;
        double value;
        gettimeofday(&start, NULL);
        value = requestData(patient, time, ecg, &chan);
        gettimeofday(&end, NULL);
        cout << "person " << patient << " time " << time << " ecg " << ecg << ": " << value << endl;
        cout << "Single Data Point Request Time: " << getTimeDiff(&start, &end) << endl;
    }

    //Xp FILE WRITE (only patient is given)
    else if(patient > 0 && patient <= p){
        cout << "Copying " << patient << ".csv into x" << patient << ".csv" << endl;
        ofstream ofile;
	string filestr = "received/x" + to_string(patient) + ".csv";
        ofile.open(filestr);
        gettimeofday(&start, NULL);

        for(double tm = 0.000; tm < 59.999; tm += 0.004){
            double ret1 = requestData(patient, tm, 1, &chan);
            double ret2 = requestData(patient, tm, 2, &chan);
            ofile << tm << "," << ret1 << "," << ret2 << endl;
        }

        gettimeofday(&end, NULL);
        ofile.close();
        cout << "Data Point File Request Time: " << getTimeDiff(&start, &end) << endl;
    }


    //FILE REQUEST
    if(filename.size() > 0){
        cout << "Duplicating file " << filename << " from input arguments" << endl;
        gettimeofday(&start, NULL);
	    requestFile(filename, mem, &chan);
        gettimeofday(&end, NULL);
        cout << "File Request Time: " << getTimeDiff(&start, &end) << endl;
    }

    //NEW CHANNEL REQUEST
    if(channel) {
        string chanName = requestNewChannel(&chan);

        if(chanName.size() > 0){
            FIFORequestChannel newChan(chanName, FIFORequestChannel::CLIENT_SIDE);

            cout << "Testing data requests on new channel:" << endl;

            int person = rand() % 15 + 1;
            int ecgno = rand() % 2 + 1;
            double timePt = getRandTime();
            double dataPoint = requestData(person, timePt, ecgno, &newChan);
            cout << "person " << person << " time " << timePt << " ecg " << ecgno << ": " << dataPoint << endl;

            person = rand() % 15 + 1;
            ecgno = rand() % 2 + 1;
            timePt = getRandTime();
            dataPoint = requestData(person, timePt, ecgno, &newChan);
            cout << "person " << person << " time " << timePt << " ecg " << ecgno << ": " << dataPoint << endl;

            person = rand() % 15 + 1;
            ecgno = rand() % 2 + 1;
            timePt = getRandTime();
            dataPoint = requestData(person, timePt, ecgno, &newChan);
            cout << "person " << person << " time " << timePt << " ecg " << ecgno << ": " << dataPoint << endl;

            MESSAGE_TYPE quit = QUIT_MSG;
            newChan.cwrite(&quit, sizeof(MESSAGE_TYPE));
        }
    }

    // closing the channel   
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite (&m, sizeof (MESSAGE_TYPE));

    wait(&childP);
}
