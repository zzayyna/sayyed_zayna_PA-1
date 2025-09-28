/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Zayna Sayyed
	UIN: 833009541
	Date: Sep 25 2025
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <iostream>
#include <fstream>
#include <cstring> 
#include <cstdlib>
#include <unistd.h>


using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = -1;
	double t = -1.0;
	int e = -1;
	int m = MAX_MESSAGE;
	bool new_chan = false;
	vector<FIFORequestChannel*> channels;

	
	string filename = "";

	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm':
				m = atoi(optarg);
				break;
			case 'c':
				new_chan = true;
				break;

		}
	}

	//give arguements for the server
	//server needs './server','-m','<val for -m arg>','NULL'
	//fork
	//in the child, run execvp using the server arguments
	pid_t id = fork();
	if(id==0){
		char m_str[20];
		sprintf(m_str, "%d", m);
		char *args[] = {(char*)"./server", (char*)"-m", m_str, NULL};
		execvp("./server", args);
	}
	
    FIFORequestChannel cont_chan("control", FIFORequestChannel::CLIENT_SIDE);
	channels.push_back(&cont_chan);

	if (new_chan){
		//send newchannel request to the server
		MESSAGE_TYPE nc = NEWCHANNEL_MSG;
    	cont_chan.cwrite(&nc, sizeof(MESSAGE_TYPE));
		// create a variable to hold the name
		char new_chan_name[MAX_MESSAGE];
		// cread the response from the server
		cont_chan.cread(new_chan_name, MAX_MESSAGE);
		// call the FIFORequestChannel constructor with the name from the server // dynamically create the channel (call new with the constructor so u can call it outside the if statement)
		FIFORequestChannel* new_channel = new FIFORequestChannel(new_chan_name, FIFORequestChannel::CLIENT_SIDE);
		//push the new channel into the vector
		channels.push_back(new_channel);
	}

	FIFORequestChannel chan = *(channels.back());

	//single data point, run p,t,e != -1
	// example data point request
	if ( p!= -1 && t != -1.0 && e != -1 ){
		char buf[MAX_MESSAGE]; // 256
    	datamsg x(p, t, e); //change from hard coding to user's values
	
		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg)); // question
		double reply;
		chan.cread(&reply, sizeof(double)); //answer
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}
	
	// else if p!=-1, request 1000 datapoints
	// loop over 1st 1000 lines
	// send request for ecg 1
	// send request for ecg 1
	// write line to recieved/x1.csv
	else if (p!= -1){ //use filestream?
		ofstream outfile("received/x" + to_string(p) + ".csv");
		// Loop over first 1000 lines
		for (int i = 0; i < 1000; i++){
			double time = i * 0.004; // Time increments by 0.004
			
			// Send request for ecg 1
			char buf1[MAX_MESSAGE];
			datamsg ecg1_req(p, time, 1);
			memcpy(buf1, &ecg1_req, sizeof(datamsg));
			chan.cwrite(buf1, sizeof(datamsg));
			double ecg1_val;
			chan.cread(&ecg1_val, sizeof(double));
			
			// Send request for ecg 2
			char buf2[MAX_MESSAGE];
			datamsg ecg2_req(p, time, 2);
			memcpy(buf2, &ecg2_req, sizeof(datamsg));
			chan.cwrite(buf2, sizeof(datamsg));
			double ecg2_val;
			chan.cread(&ecg2_val, sizeof(double));
			
			// Write line to received/x{p}.csv
			outfile << time << "," << ecg1_val << "," << ecg2_val << endl;
		}
		outfile.close();
	}

    // sending a non-sense message, you need to change this
	else if (!filename.empty()){
		filemsg fm(0, 0);
		string fname = filename; //use -f argument from user
		
		int len = sizeof(filemsg) + (fname.size() + 1);
		char* buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), fname.c_str());
		chan.cwrite(buf2, len);  // I want the file length;

		int64_t filesize = 0;
		chan.cread(&filesize, sizeof(int64_t));

		char* buf3 = new char[m];//create buffer of size buff capacity (m)
		ofstream outfile("received/" + fname, ios::binary);

		//loop over the segments in the file filesize/ buff capacity
		// create filemsg instance
		int64_t bytes_transferred = 0;
		while (bytes_transferred < filesize) {
			filemsg* file_req = (filemsg*)buf2;
			file_req-> offset = bytes_transferred; //set offset in the file
			int64_t remaining = filesize - bytes_transferred;
			if (remaining < m) {
				file_req->length = remaining;
			} else {
				file_req->length = m;
			} 
			//set the length. Be careful of the last segment
			//send the request (buf2)
			chan.cwrite(buf2, len);
			chan.cread(buf3, file_req->length);
			outfile.write(buf3, file_req->length);
			
			bytes_transferred += file_req->length;
		}
		outfile.close();
		// recieve the response. response buffer needs to be diff form the request buffer
		// cread into buf3 lenfth file_req->len
		// qrite buf3 into file: recieved/filename

		delete[] buf2;
		delete[] buf3;

		if (new_chan){
		//do your close and deletes
			MESSAGE_TYPE quit_msg = QUIT_MSG;
			channels.back()->cwrite(&quit_msg, sizeof(MESSAGE_TYPE));
			delete channels.back();
			channels.pop_back();
		}

	}

	//if necessary, close and delete the new channel
	
	
	// closing the channel    
    MESSAGE_TYPE mq = QUIT_MSG;
    chan.cwrite(&mq, sizeof(MESSAGE_TYPE));
}
