/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/20
 */
#include "common.h"
#include "FIFOreqchannel.h"

using namespace std;

//Send a data message to the server
//Send a file message to the server

int main(int argc, char *argv[]){
    int n = 100;    // default number of requests per "patient"
    int p = 15;		// number of patients
    srand(time_t(NULL));

    FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);


//+-------------------------------------------------------------+

    //getopt
    int patient, ecg, mem; //do mem later
    double time;
    string filename; //do this later
    int c;
    while((c = getopt(argc, argv, "t:p:e:m:f:")) != -1){
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
		break;
	}
    }

    //Data request
    char buf[MAX_MESSAGE];

    //File request
    
    //New Channel request
    
    //DO THESE LAST
    //Fork Server (will occur at top)
    //Channel close (all channels MUST close)

//+-------------------------------------------------------------+



    // sending a non-sense message, you need to change this
    char buf [MAX_MESSAGE];
    char x = 55;
    chan.cwrite (&x, sizeof (x));
    int nbytes = chan.cread (buf, MAX_MESSAGE);

//:::::::::::::::::::::::::::::::::::::::::
#if 0
    //THIS WON'T COMPILE:
    //sending example:
    datamsg d(14, 59.00, 2);
    chan.cwrite(&d, sizeof(datamsg));

    //receive example:
    double rsp = 0;
    chan.cread(&rsp, sizeof(double));
    cout << "Response " << rsp << endl;


    //file_msg
    string filename = "akhhdkjhaksdhf";
    //needs to contain the filename with a nullbyte after and then payload
    char * buf = new char[filename.size() + 1 + sizeof(filemsg)];
    filemsg* f = (filemsg*) msg;
    f->mtype = FILE_MSG;
    f->offset = 0; f->length = 5000;
    //this will copy the string into the buffer and add a null pointer to the end of it
    strcpy(buf + sizeof(filemsg), filename.c_str());
    //writes out the buffer of correct size
    chan.cwrite(buf, sizeof(filemsg) + filename.size()+1);
#endif
//:::::::::::::::::::::::::::::::::::::::::

    // closing the channel    
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite (&m, sizeof (MESSAGE_TYPE));

   
}
