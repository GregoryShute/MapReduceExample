//Gregory Shute
//Compile with g++ lab2.cpp -Wno-write-strings -lpthread

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sstream>
#include <pthread.h>
#include <fstream>
#include <vector>
#include <cstring>
#include <string>
#include <iostream>
#include <algorithm>
using namespace std;
/*Function Prototypes*/

/*End Function Prototypes*/

/* globals */
int n;
int m;
int activeMappers;
pthread_mutex_t invertedIndexLock;
/*End globals*/


/*Buffer Associated Classes*/
class WordFileLine{
string word;
string file;
string line;
public:
	void setWord(string w){ word = w;}
	string getWord(){return word;}

	void setFile(string f){ file = f;}
	string getFile(){return file;}

	void setLine(string l){ line = l;}
	string getLine(){return line;}	
};


class BoundedBuffer{

WordFileLine wordFileLineArray[10];
int front;
int rear; 
int count; 

public:

	pthread_mutex_t bufferLock;
	pthread_cond_t fullCondition;
	pthread_cond_t emptyCondition; 
	

	void initBuffer(){
		front = 0;
		rear = 9; 
		count = 0;
		pthread_cond_init(&fullCondition,NULL);
		pthread_cond_init(&emptyCondition,NULL);
		pthread_mutex_init(&bufferLock,NULL);
	}
		

	WordFileLine getWordFileLineAtIndex(int index){ return (wordFileLineArray)[index];}

	void addWordFileLine(WordFileLine wordFileLine){
		if(isFull() != 1){
			rear = (rear + 1) % 10; 
			wordFileLineArray[rear] = wordFileLine;
			count = count + 1;
		}
	}

	WordFileLine removeWordFileLine(){
		if(isEmpty() != 1){
			WordFileLine wlf = wordFileLineArray[front];
			front = (front + 1) % 10; 
			count = count - 1;
			return wlf;
		}
	}
	
	int isFull(){
		if(count == 10){return 1;}else{return 0;} 
	}

	int isEmpty(){
		if(count == 0){return 1;}else{return 0;};
	}
	

	void setFront(int f){ front = f;}
	int getFront(){return front;}

	void setRear(int r){ rear = r;}
	int getRear(){return rear;}

	void setCount(int c){ count = c;}
	int getCount(){return count;}
};

/*End Buffer Associated Classes*/

/*InvertedIndexEntry Class*/
//This classes is used to structure the entries in the InvertedIndex
class InvertedIndexEntry{

string word;
string entry;

public:

	void initInvertedIndexEntry(){entry = "";}

	string getWord(){return word;}
	string getEntry(){return entry;}
	//when the word isn't already an entry
	void addEntry(WordFileLine wordFileLine){
		word = wordFileLine.getWord();
		string file = wordFileLine.getFile();
		string line = wordFileLine.getLine();
		string openBracket("(");
		string closeBracket(")");
		string colonSpace(": ");
		entry = word + colonSpace + openBracket + file + colonSpace + line + closeBracket;
	}

	//when the word is already an entry and we want to add on another occurrance of the word
	void appendEntry(WordFileLine wordFileLine){
		string file = wordFileLine.getFile();
		string line = wordFileLine.getLine();
		string openBracket("(");
		string closeBracket(")");
		string colonSpace(": ");
		string commaSpace(", ");
		entry = entry + commaSpace + openBracket + file + colonSpace + line + closeBracket;
	}	

};
/*End InvertedIndexEntry Class*/

/*InvertedIndex Class*/
//This class is used for the global invertedIndex to be printed
class InvertedIndex{
vector<InvertedIndexEntry> entries;
int sizeOfIndex;

public:

	void initInvertedIndex(){sizeOfIndex = 0;}

	void addInvertedIndexEntries(vector<InvertedIndexEntry> e){
		
		int i;
		int j = 0;
		int eSize = e.size();
		int index = sizeOfIndex;
		long condition = ((long)sizeOfIndex + (long)e.size());
		
		for(i = index; i < condition; i++){
			entries.insert(entries.end(),e.at(j));
			j++;
			sizeOfIndex = sizeOfIndex + 1;
		}
	}
	
	void printOut(){
		int index;
		for(index = 0; index < (long)entries.size();index++){
			string s;
			s = entries.at(index).getEntry();
			s.erase(remove(s.begin(),s.end(),'\n'),s.end());
			cout << s <<".\n";
		}
	}

};

/*End InvertedIndex Class*/


/*Some more globals*/
InvertedIndex* i;
BoundedBuffer *bb; 
/*End of some more globals*/



/*Reducer Class*/

class Reducer{

BoundedBuffer boundedBuffer;
int bufferID;
vector<InvertedIndexEntry> entries;
int vectorNextSpot;

public:
	vector<InvertedIndexEntry> entrieSize(){return entries;}
	
	void initReducer(int id){bufferID = id; boundedBuffer = bb[id];  vectorNextSpot = 0;}

	int wordIsInIndex(string word){
		int index;
		for(index = 0; index < (long)entries.size(); index++){
			if(entries.at(index).getWord() == word){
				return index;
			}
		}
		return (-1);
	}
	//Inserts an entry into this reducers entries vector.
	void insertEntry(){
		
		WordFileLine wordFileLine;
		boundedBuffer = bb[bufferID];
		wordFileLine = bb[bufferID].removeWordFileLine();

		string word;
		word = wordFileLine.getWord();
		
		int indexOfWord;
		
		if((indexOfWord = wordIsInIndex(word)) >= 0){
			entries.at(indexOfWord).appendEntry(wordFileLine);
		}else{
			InvertedIndexEntry iie;
			iie.initInvertedIndexEntry();
			iie.addEntry(wordFileLine);
			entries.insert(entries.end(),iie);
			vectorNextSpot = vectorNextSpot + 1;
		}
	}

	void giveInvertedIndexEntriesToInvertedIndex(InvertedIndex* invertedIndex){
		(*invertedIndex).addInvertedIndexEntries(entries);
		
	}

	

	BoundedBuffer* getBoundedBuffer(){boundedBuffer = bb[bufferID];return &boundedBuffer;}

	

};

/*End Reducer Class*/


/*Mapper Class*/

class Mapper{
string filename;
int numReducers;

public:

	void initMapper(string file,int num){
		filename = file;
		numReducers = num;
	}
		
	void readLines(char* filename){

		FILE* fp;
		ssize_t read;
		char* line =NULL;
		size_t len = 0;
		int lineNumber = 1;
		fp = fopen(filename,"r");
		string fileString(filename);
		
		
		while((read = getline(&line,&len,fp)) != -1){
			
			string wordString(line);
			int hash = myhash(wordString);
			stringstream ss;
			ss << lineNumber;
			string num = ss.str();
			WordFileLine* wlf = new WordFileLine();
			(*wlf).setWord(wordString);
			(*wlf).setFile(fileString);
			(*wlf).setLine(num);
			
			pthread_mutex_lock(&(bb[hash].bufferLock));
			
			while(bb[hash].isFull() == 1){
				pthread_cond_wait(&(bb[hash].fullCondition),(&bb[hash].bufferLock));
			}
			
			bb[hash].addWordFileLine((*wlf));
			pthread_cond_signal(&(bb[hash].emptyCondition));
			pthread_mutex_unlock(&(bb[hash].bufferLock));
			
			lineNumber++;
			
			
		}
		fclose(fp);
	}

	int myhash(string word){
		int w = (int)word.at(0);
		int h = w%numReducers;
		return h;
	}

};

/*End Mapper Class*/


void* reduce(void* threadID){
	
	Reducer* r = new Reducer();
	int id = (int)threadID;
	
	(*r).initReducer(id);

	//while there are active mappers or your buffer is not empty	
	while((activeMappers > 0) || (bb[id].getCount() != 0)    ){ 
		
		pthread_mutex_lock(&(bb[id].bufferLock));
		
		while( (bb[id].isEmpty() == 1) ){
			
			pthread_cond_wait(&(bb[id].emptyCondition),&(bb[id].bufferLock)); //waiting for something to come into my buffer
			while((activeMappers == 0) && (bb[id].isEmpty() == 1)){
			/*Check that reducers with empty BoundedBuffers, yet non-empty InvertedIndexEntry vectors give entries over to
			global IndvertedIndex before exiting*/
				if(!((*r).entrieSize().empty())){
					pthread_mutex_lock(&invertedIndexLock);
					(*r).giveInvertedIndexEntriesToInvertedIndex(i);
					pthread_mutex_unlock(&invertedIndexLock);
				}
			pthread_exit(NULL);}
			
		}
		
		
		(*r).insertEntry();

		pthread_cond_signal(&(bb[id].fullCondition)); 
		pthread_mutex_unlock(&(bb[id].bufferLock));
		
		
		
	}
	
	
	pthread_mutex_lock(&invertedIndexLock);
	
	(*r).giveInvertedIndexEntriesToInvertedIndex(i);
	pthread_mutex_unlock(&invertedIndexLock);
	
	pthread_exit(NULL);

}

void* map(void* threadID){

	int id = (int)threadID;
	id = id+1;
	int line = 1;
	string foo("foo");
	string txt(".txt");
	string file;
	stringstream ss;
	ss << id;
	string num = ss.str();
	file = foo + num + txt;
	Mapper* m = new Mapper();
	(*m).initMapper(file,n);
	char* copy = (char*)file.c_str();
	(*m).readLines(copy);
	pthread_exit(NULL);
}


int main(int argc, char *argv[]){

i = new InvertedIndex(); //Global InvertedIndex
(*i).initInvertedIndex();
pthread_mutex_init(&invertedIndexLock,NULL);
istringstream ss_2(argv[2]);
ss_2 >> n;

istringstream ss_1(argv[1]);
ss_1 >> m;
activeMappers = m;

bb = new BoundedBuffer[n]; // Shared Array of BoundedBuffers

int ii;
for(ii = 0; ii < n; ii++){ 
	BoundedBuffer b;
	b.initBuffer();
	bb[ii] = b; 
	}

pthread_t reducers[n];
int z;
for(z = 0; z < n; z++){ pthread_create(&reducers[z],NULL,reduce, (void*) z);}

pthread_t mappers[m];
int t;
for(t = 0; t < m; t++){ pthread_create(&mappers[t],NULL,map, (void*) t);}

void *status;
int k;
for(k = 0; k < m; k++){pthread_join(mappers[k], &status);activeMappers = activeMappers - 1;}

for(k = 0;k < n;k++){pthread_cond_signal(&(bb[k].emptyCondition));}

for(k = 0; k < n; k++){pthread_join(reducers[k], &status);}

(*i).printOut();

}






