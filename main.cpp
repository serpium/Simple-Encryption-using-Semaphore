#include <iostream>
#include <string>
#include <fstream>
#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>

using namespace std;

//Msg is the class that has information about the data.
class Msg{
  public:
  	char origin;
   	bool crypto;
   	bool counted;	
};



//declare the shared variables
sem_t infull, inempty, outfull, outempty, incount, incountempty, outcount, outcountempty;

int buffsize, in, out, outin, outout, incounterbuff, incounterout, outcounterbuff, outcounterout;

//counted storage
int inpcounter[26];
int outpcounter[26];

//shared buffer
Msg *inbuff, *outbuff;

//Threads and function
void *reader(void *param);
void *writer(void *param);
void *encryption(void *param);
void *incounter(void *param);
void *outcounter(void *param);

int main(int argc, char* argv[]) {
  // program start
  if(argc!=3) {
        cout << "USAGE: encrypt inputfilename outputfilename" << endl;
	return -1;
  }

  // and read file
  //ofstrream out(argv[2]);
  char *infile = argv[1];
  char *outfile = argv[2];

  //Ask the buffer size

  cout << "Enter buffer size: ";
  cin >> buffsize;

  //initialize the int variables and buffer.
  in = 0;
  out = 0;
  outin = 0;
  outout = 0;
  incounterbuff = 0;
  incounterout = 0;
  outcounterbuff = 0;
  outcounterout = 0;

  inbuff = new Msg[buffsize];
  outbuff = new Msg[buffsize];
  
  //declare the threads.
  pthread_t prod, cons, encrypt, incounts, outcounts;

  //initializing
  sem_init(&infull, 0, 0);
  sem_init(&inempty, 0, buffsize);
  sem_init(&outfull, 0, 0);
  sem_init(&outempty, 0, buffsize);
  sem_init(&incount, 0, 0);
  sem_init(&incountempty, 0, buffsize);
  sem_init(&outcount, 0, 0);
  sem_init(&outcountempty, 0, buffsize);

  pthread_create(&outcounts, NULL, outcounter, NULL);
  pthread_create(&incounts, NULL, incounter, NULL);
  pthread_create(&prod, NULL, writer, outfile);
  pthread_create(&encrypt, NULL, encryption, NULL);
  pthread_create(&cons, NULL, reader, infile);


  pthread_join(incounts, NULL);
  pthread_join(prod, NULL);
  pthread_join(outcounts, NULL);
  pthread_join(encrypt, NULL);
  pthread_join(cons, NULL);

  
  //print input counter
  //
  //print_in_counter();
   cout << "Input file contains" << endl;
     int k = 0;
     for(int i=0; i<26; i++) {
	k = i+65;
	if(inpcounter[i]!=0) {
		cout << (char) k << ": " << inpcounter[i] << endl;
	}
     }

  //print_out_counter();

   cout << "Output file Contains" << endl;
  
     for(int i2=0; i2<26; i2++) {
	k = i2+65;
	if(outpcounter[i2]!=0) {
		cout << (char) k << ": " << outpcounter[i2] << endl;
	}
     }

  //cleanup
  sem_destroy(&infull);
  sem_destroy(&inempty);
  sem_destroy(&outfull);
  sem_destroy(&outempty);
  sem_destroy(&incount);
  sem_destroy(&incountempty);
  sem_destroy(&outcount);
  sem_destroy(&outcountempty);

  //do main work

 return 0; 
}

//reader thread read the file and put the character in the buffer.
//
//This thread will wait untill inpuf buffer is available, and the items are all read.
void *reader(void *param) {
   char c;
   char *filename = (char *)param;

   FILE *file;
   file = fopen(filename, "r");
//   cout << "I am in reader thread." << endl;
   while(c = (char) fgetc(file)) {
	sem_wait(&inempty);
	sem_wait(&incountempty);
//	cout << "put" << c << " in buffer " << in << endl;

	Msg item;
	item.origin = c;
	item.crypto = false;
	item.counted = false;
	inbuff[in] = item;

	in = (in + 1) % buffsize;

    	sem_post(&incount);
	sem_post(&infull);
	if(c==EOF) {
                break;
        }
	
   }
//   cout << "lol" << endl;
   fclose(file);
  pthread_exit(0);

} 

//writer thread will get a item from output buffer and put in the output file.
//If output file is not founded, it will show the error message.
//
void *writer(void *param) { 
   char c;
   char *filename = (char *)param;
   FILE *file;
   file = fopen(filename, "w");

//   cout << "start writing" << endl;

   if (file == NULL) {
	cout<< "No file found!!!" << endl;
   }

   do {
	sem_wait(&outfull);
	c = outbuff[outout].origin;
	if(c==EOF) {
		break;
	}
	fputc(c, file);
	outout = (outout + 1) % buffsize;
        sem_post(&outempty);
  } while (c != EOF);

  fclose(file);
  pthread_exit(0);
//  cout << "lol2" << endl;
}

//Encryption thread is consumer for input buffer and producer for output buffer. 
//Also, it will wait that output items are counted
//
void *encryption(void *param) {
    int s = 1;
    char c;
 //   cout << "start encrypting" << endl;
    do {
    sem_wait(&infull);
    sem_wait(&outempty);
    sem_wait(&outcountempty);
    
    c = inbuff[out].origin;
//    cout << "Origin val is.. " << c << endl;

    int asciic = (int) c;

    if((asciic >= 65 && asciic <= 90) || (asciic >= 97 && asciic <= 122)){
	if(s == 1) {
		asciic ++;
		if(asciic == 91) {
			asciic = 65;
		}
		else if(asciic == 123) {
			asciic = 97;
		}
	s = -1;
    	}
	else if (s == -1) {
		asciic --;
		if(asciic == 64) {
			asciic = 90;
		}
		else if(asciic == 96) {
			asciic = 122;
		}
	s = 0;
	}
	else if (s == 0) {
		s = 1;
	} 
   }
//   cout << "encrypted as " << (char) asciic << endl;
   if(c!=EOF) {
	Msg oitem;
	oitem.origin = asciic;
	oitem.counted = false;
	oitem.crypto = false;
  	outbuff[outin] = oitem;
   }
   else {
	Msg oiteme;
	oiteme.origin = c;
	oiteme.counted = false;
	oiteme.crypto = false;
  	outbuff[outin] = oiteme;  
   }

   out = (out + 1) % buffsize;
   outin = (outin +1) % buffsize;
   
   sem_post(&outcount);
   sem_post(&outfull);
   sem_post(&inempty);

   } while(c != EOF);

     pthread_exit(0);
//   cout << "lol3" << endl;

}


//incounter thread will count the input alphabet. Small letter will be counted with capital letter.
//It will wait until the input buffer is not completely empty.
//
void *incounter(void *param) {
   char c;
   int loc;

//   cout<< "start incounter" << endl;
   while(c!=EOF) {
	sem_wait(&incount);

	c = inbuff[incounterbuff].origin;
	if(!inbuff[incounterbuff].counted) {
		loc = (int) c;
		
		if(loc >= 65 && loc <= 90) {
			loc = loc - 65;
			inpcounter[loc] = inpcounter[loc] + 1;
//            		cout << c << " is added in inpcounter index " << loc << " and number is.." << inpcounter[2] << endl;
		}
		else if(loc >= 97 && loc <= 122) {
			loc = loc - 97;
			inpcounter[loc] = inpcounter[loc] + 1;
//                	cout << c << " is added in inpcounter index " << loc << " and number is.." << inpcounter[2] << endl;
		}
		else { 

		}
		inbuff[incounterbuff].counted = true;
   		}
	incounterbuff = (incounterbuff + 1) % buffsize;
	sem_post(&incountempty);
	}
	
  pthread_exit(0);
//   cout << "finish incounter" << endl;
}


//Output counter thread will count the encrypted message and count the character. 
//It will also wait until output buffer is available.
//
void *outcounter(void *param) {
   char c;
   int loc;

//   cout<< "start incounter" << endl;
   while(c!=EOF) {
	sem_wait(&outcount);

	c = outbuff[outcounterbuff].origin;
	if(!outbuff[outcounterbuff].counted) {
		loc = (int) c;

		if(loc >= 65 && loc <= 90) {
			loc = loc - 65;
			outpcounter[loc]++;
 //              		cout << c << " is added in outpcounter index " << loc << endl;
		}
		else if(loc >= 97 && loc <= 122) {
			loc = loc - 97;
			outpcounter[loc]++;
 //               	cout << c << " is added in outpcounter index " << loc << endl;
		}
		else { 
		
		}
		outbuff[outcounterbuff].counted = true;
   		}
	outcounterbuff = (outcounterbuff + 1) % buffsize;
	sem_post(&outcountempty);
	}
     
  pthread_exit(0);
//   cout << "finish incounter" << endl;
}

