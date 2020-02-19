/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/20
 */
#include "common.h"
#include "FIFOreqchannel.h"
#include <sys/wait.h>
#include <unistd.h>


using namespace std;


double requestData(int patient, double time, int ecg, FIFORequestChannel* chan){
    datamsg msg(patient, time, ecg);
    chan->cwrite(&msg, sizeof(msg));

    double value;
    int retLen = chan->cread((char*)&value, sizeof(value));

    return value;
}

void requestFile(string filename, int mem, FIFORequestChannel* chan){

    //REQUEST FILE LENGTH
    cout << "creating request" << endl;
    unsigned char requestBuf[filename.size() + 1 + sizeof(filemsg)];
    filemsg msg(0, 0);
    msg.mtype = FILE_MSG;
    msg.offset = 0;
    msg.length = 0;
    memcpy((char*)requestBuf, (char*)&msg, sizeof(filemsg));
    strcpy( ((char*)requestBuf) + sizeof(filemsg), filename.c_str());

    cout << "sizeof filemsg: " << sizeof(filemsg) << endl;
    cout << "mtype: " << ((filemsg*)requestBuf)->mtype << endl;
    cout << "offset: " << ((filemsg*)requestBuf)->offset << endl;
    cout << "length: " << ((filemsg*)requestBuf)->length << endl;
    cout << "sizeof mtype: " << sizeof(msg.mtype) << endl;
    cout << "sizeof offset: " << sizeof(msg.offset) << endl;
    cout << "sizeof length: " << sizeof(msg.length) << endl;
    for(int i = 0; i < sizeof(filemsg) + filename.size() + 1; i++){
	cout << hex << (int)requestBuf[i] << " ";
    }
    cout << dec << endl;
    
    chan->cwrite(&requestBuf, filename.size() + 1 + sizeof(filemsg));

    //RECEIVE FILE LENGTH
    cout << "receiving file size" << endl;
    __int64_t fileSize = 0;
    int rec = chan->cread((char*)&fileSize, sizeof(fileSize));
    if(!fileSize) { cout << "Cannot get data from file" << endl; return; }

    //NOW GET DATA AND WRITE OUT
    cout << "getting data from file" << endl;
    ofstream ofile;
    string ofName = "received/" + filename; 
    ofile.open(ofName);

    cout << "file size: " << fileSize << endl;

    int runs = ceil((double)fileSize / (double)mem);
    cout << ((double)fileSize / (double)mem) << " = " << runs << endl;

    msg.length = mem;
    for(int i = 0; i < runs; i++){
	cout << "offset: " << msg.offset << endl;
	if(runs - i == 1){
            msg.length = (fileSize-msg.offset) % (__int64_t)mem;
	}
	cout << "length: " << msg.length << endl;
        memcpy((char*)requestBuf, (char*)&msg, sizeof(filemsg)); //overwrite the old msg request
	chan->cwrite(&requestBuf, filename.size() + 1 + sizeof(filemsg));

	unsigned char rsp[mem];
	int bytes = chan->cread((char*)rsp, mem);
	//cout << "received " << bytes << " bytes: " << endl;
	if(bytes < 1) { cout << "no bytes received" << endl; ofile.close(); return; }
        ofile << rsp;
	msg.offset += mem;
    }

    ofile.close();
    cout << "file request complete" << endl;
}

int main(int argc, char *argv[]){
    int n = 100;    // default number of requests per "patient"
	int p = 15;		// number of patients
    srand(time_t(NULL));

    //GET ARGUMENTS
    int patient, ecg;
    int mem = MAX_MESSAGE;
    double time;
    bool channel;
    string filename;
    int c;

    while((c = getopt(argc, argv, "t:p:e:m:f:c:")) != -1){
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
	    case 'c':
		break;
	}
    }

    //FORK AND SPLIT SERVER HERE
    string memtoa = to_string(mem);
    char* args[] = {"./server", "-m", (char*)memtoa.c_str(), NULL};
    int childP = fork();
    if(!childP){
	cout << "Starting Server" << endl;
        int out = execvp("./server", args);
	if(out){ cout << "FAILED TO START SERVER" << endl; }
    }
    
    FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);
#if 0 
    //SINGLE DATA POINT REQUEST
    if(patient > 0 && time < 59.999 && ecg > 0){
        double value;
	value = requestData(patient, time, ecg, &chan);
	cout << "person " << patient << " time " << time << " ecg " << ecg << ": " << value << endl;
    }

    //X1 FILE WRITE
    ofstream ofile;
    ofile.open("x1.csv");

    for(double tm = 0.000; tm < 59.999; tm += 0.004){
	double ret1 = requestData(1, tm, 1, &chan);
	double ret2 = requestData(1, tm, 2, &chan);
        ofile << tm << "," << ret1 << "," << ret2 << endl;
    }

    ofile.close();
#endif	//THIS WORKS

    //FILE REQUEST
    if(filename.size() > 0){
	cout << "requesting file " << filename << " in chunks of " << mem << endl;
	requestFile(filename, mem, &chan);
    }

    // closing the channel   
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite (&m, sizeof (MESSAGE_TYPE));

    wait(&childP);
}
