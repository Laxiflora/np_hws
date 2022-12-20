#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include "struct.h"
#include <fcntl.h>

#define SHELL_INPUT 15001
#define COMMAND_LEN 257
#ifndef STDIN_NO
    #define STDIN_NO 0
#endif
#ifndef STDOUT_NO
    #define STDOUT_NO 1
#endif
#ifndef STDERR_NO
    #define STDERR_NO 2
#endif
#define MAXQUEUE 1000
#define MAXARGUMENT 100

using namespace std;

vector<int> pendingPid;
//map<string, string> mapStudent;  //for setenv
vector<QueueNode> instTable;
int rootPid;

void updateTable(){
    for(int i=0;i<instTable.size();i++){
        instTable[i].round-=1;
        if(instTable[i].round<0){
            instTable.erase(instTable.begin() + i);
            i--;
        }
    }
}


vector<string> split(const string& s, char delimiter){
   vector<string> tokens;
   string token;
   istringstream tokenStream(s);
   while (getline(tokenStream, token, delimiter))
   {
      tokens.push_back(token);
   }
   return tokens;
}

void setenv(vector<string> token){
    if(token.size()<3){
        string err_message = "Missing parameter: "+token[0]+"\n";
        char error[err_message.length() + 1];
        strcpy(error, err_message.c_str());
        write(STDERR_NO,error,strlen(error));
        return;
    }
    setenv(token[1].c_str(),token[2].c_str(),1);
}

void printenv(vector<string> token) {
    if(token.size()==1){
        string err_message = "Missing parameter: "+token[0]+"\n";
        char error[err_message.length() + 1];
        strcpy(error, err_message.c_str());
        write(STDERR_NO,error,strlen(error));
        return;
    }
    const char * val = getenv( token[1].c_str() );
    if ( val == nullptr ) {  // no matched result
        return;
    }
    else {
        printf("%s\n",val);
    }
}

int checkTable(int inRound,int& pipeInfd , int& pipeOutfd){
    for(int i=0;i<instTable.size();i++){
        //cout<<endl<<"InstTable round = "<<instTable[i].round<<" infd = "<<instTable[i].inputfd<<endl<<endl;
        if(instTable[i].round== inRound){
            pipeInfd = instTable[i].inputfd;
            pipeOutfd = instTable[i].outputfd;
            return 1;
        }
    }
    return -1;
}


int checkCommand(char* const argv[]){
    int exist = -1;
    string temp = getenv("PATH");
    vector<string> list = split(temp,':');
    for(int i=0;i<list.size() && exist==-1;i++){
        string path = list[i];
        path += "/";
        path+=argv[0];
        char* ch = (char*) malloc(sizeof(path.c_str()+1));
        strcpy(ch,path.c_str());
        exist = access(ch,X_OK);
        free(ch);
    }
    return exist;
}

void waitProcess(pid_t pid){
	int status;
	int waitPID;
	while(1){
		waitPID = waitpid(pid, &status, WNOHANG);
		if (waitPID == pid) break;
	}
}


void execCommand(char* const argv[], int isExclamationMark,int commandInFd,int& pipeInFd,int& pipeOutFd, int isNumbered){
    if(checkCommand(argv) == -1){
        //execvp(argv[0],argv);
        string temp = "Unknown command: [";
        temp += argv[0];
        temp += "].";
        char mesg[temp.length()];
        strcpy(mesg,temp.c_str());
        mesg[temp.length()] = '\0';
        write(STDERR_NO,mesg,sizeof(mesg));
        printf("\n");
        return;
    }
    int newPipeFd[2] = {-1};
    if(pipeOutFd == -1 && pipeInFd == -1){  //if is not going to output to STDOUT
        pipe(newPipeFd);
        pipeOutFd = newPipeFd[0]; //redirect out
        pipeInFd = newPipeFd[1];
    }
    else{
        isNumbered = 1;
        newPipeFd[0] = pipeOutFd;
        newPipeFd[1] = pipeInFd;
    }
    int pid;
    //cout<<"command = "<< argv[0]<<" , inFd="<<pipeInFd<<", outFd="<<pipeOutFd<<" , commandInFd = "<<commandInFd<<", End of "<< argv[0]<<endl;
    
    if( (pid= fork()) == -1){
        waitProcess(pid);
        pid = fork();
    }
    
    
    if(pid == 0){
        close(STDIN_NO);
        dup2(commandInFd,STDIN_NO);
        close(pipeOutFd);
        dup2(pipeInFd,STDOUT_NO);
        if(isExclamationMark){
            dup2(pipeInFd,STDERR_NO);
        }
        if(execvp(argv[0],argv) == -1){
     //       perror("EXEC failed");
        }
        close(pipeInFd);
        exit(0);
    }
    else{
        if(pipeInFd == 1){
    //        cout<<"waiting for"<<endl;
            waitProcess(pid);
        }
        pendingPid.push_back(pid);
    //    cout<<"closing "<<commandInFd<<endl;
        close(commandInFd);
        if(!isNumbered && pipeInFd != 1){
    //        cout<<"closing "<<pipeInFd<<endl;
            close(pipeInFd);
        }
    }

}

void parseArgument(vector<string> token, int argc,int start,char** argv){
    for(int i=0;i<argc;i++,start++){
        argv[i] = (char*)malloc(sizeof(char)*COMMAND_LEN);
        strcpy(argv[i],token[start].c_str());
    }
    argv[argc] = NULL;
}

int isOrdinPipe(string in){
    return in=="|";
}

int isExclamation(string in){
    return (in.find("!") != -1);
}

int isNumberPipe(string in){
    return ((in.find("|")!=-1) && (in.length() >1) );
}

int isRedirection(string in){
    return ( (in.find(">")!=-1) );
}

void writeFile(char* fileName,int inFd){
    FILE* out;
    FILE* in = fdopen(inFd,"r");
    if( (out = fopen(fileName,"w")) == NULL){
        perror("Failed open: ");
    }
    char reading_buf[1000]={'\0'};
    //cout<<"Writing , reading "<<inFd<<" fd \n";
    while(fgets(reading_buf,1000,in) != NULL){
        fprintf(out,"%s",reading_buf);
    }
    close(inFd);
    fclose(out);
}

void registerTable(int pipeOutFd,int pipeInFd,int inRound){
    //std::vector<T>::push_back() creates a copy of the argument and stores it in the vector.
    QueueNode temp;
    temp.outputfd = pipeOutFd;
    temp.inputfd = pipeInFd;
    temp.round = inRound;
    instTable.push_back(temp);
}

void parseCommand(vector<string> token){
    int start=0;
    int tail=0;
    int argc=0;
    int isFirst=1;
    int noop= -1;  // a useless integer.
    int commandInFd = -1; // this is the fd that is going to input to this command
    int pipeInFd = -1;  //this is the pipe input fd that this command going to output to
    int pipeOutFd=-1; // this is the information that the established pipe is going to output to
    //char relation = '-';
    while(tail<token.size()){
        //cout<<"tail="<<token[tail]<<endl;
        while(tail<token.size()&&!isOrdinPipe(token[tail])&& !isExclamation(token[tail])&&!isNumberPipe(token[tail])&&!isRedirection(token[tail])){
            tail++;
        }
        if(isFirst){
            if( checkTable(0,pipeInFd,commandInFd) != -1){
               // cout<<"CHECK,command "<<token[start]<<" has pipeIn = "<< pipeInFd << " . commandInFd = "<<commandInFd<<endl;
           //     cout<<"closing "<<pipeInFd<<endl;
                close(pipeInFd);
                pipeInFd = -1;
            }
            isFirst=0;
        }
        if(tail>=token.size())
            break;
        argc = tail-start;
        char* argv[argc+1];
        parseArgument(token,argc,start,argv);
        //cout<<"argv="<<argv[0]<<"\n";
        if(tail<token.size() && isExclamation(token[tail])){  // we assume exclamation only has numbered one
            string temp = token[tail].substr(1,token[tail].length()); // remove |
            int round = stoi(temp);
            checkTable(round,pipeInFd,pipeOutFd);
            if(pipeOutFd == -1){                  // means no same round number pipe is found, pipe in fd not specified
                execCommand(argv,1,commandInFd,pipeInFd,pipeOutFd,1);
                registerTable(pipeOutFd,pipeInFd,round);
            }
            else{
                execCommand(argv,1,commandInFd,pipeInFd,pipeOutFd,1);
            }
            updateTable();
            tail++;
            start = tail;
            isFirst = 1;
            continue;

        }
        else if(tail<token.size() && isNumberPipe(token[tail]) ){
            string temp = token[tail].substr(1,token[tail].length()); // remove |
            int round = stoi(temp);
            checkTable(round,pipeInFd,pipeOutFd);
            if(pipeOutFd == -1){                  // means no same round number pipe is found, pipe in fd not specified
                execCommand(argv,0,commandInFd,pipeInFd,pipeOutFd,1);
                registerTable(pipeOutFd,pipeInFd,round);
            }
            else{
                execCommand(argv,0,commandInFd,pipeInFd,pipeOutFd,1);
            }
            updateTable();
            tail++;
            start = tail;
            isFirst = 1;
            continue;
        }
        else{
            pipeInFd = -1;
            pipeOutFd = -1;
            execCommand(argv,0,commandInFd,pipeInFd,pipeOutFd,0);
        }
        if(isOrdinPipe(token[tail])){
            commandInFd = pipeOutFd;
            pipeOutFd = -1;
            pipeInFd = -1;
            tail++;
        }
        else if(isRedirection(token[tail])){
            commandInFd = pipeOutFd;
            pipeOutFd = -1;
            char* fileName= (char*) malloc(sizeof(char)*strlen(token[tail+1].c_str()) );
            strcpy(fileName,token[tail+1].c_str());
            writeFile(fileName,commandInFd);
            updateTable();
            return;  //redirect sign should be the last command in a line
        }
        start=tail;
    }
    if(start < tail && tail>=token.size() && !isNumberPipe(token[tail-1]) ){ //got some remaining, not a number pipe
        argc = tail-start;
        char* argv[argc+1];
        int temp = STDOUT_NO;
        parseArgument(token,argc,start,argv);
        execCommand(argv,0,commandInFd,temp,noop,0);
        noop = -1;
        pipeInFd = -1;
        pipeOutFd = -1;
        updateTable();
    }
}

static void handler(int signo){
    switch(signo){
        case SIGCHLD:
            break;
    }
}


int main(int argc, char* argv[]){
    setenv("PATH","bin:.",1);
    rootPid = getpid();
    signal(SIGCHLD,handler);
    string input;
    printf("%% ");
    while(1){
        getline(cin,input);
        vector<string> token;
        if(input==""){
            printf("%% ");
            continue;
        }
        else{
            token = split(input,' ');
        }
        if(token[0]=="exit"){
            break;
        }
        else if(token[0] == "printenv"){
            updateTable();
            printenv(token);
            printf("%% ");
            continue;
        }
        else if(token[0]== "setenv"){
            updateTable();
            setenv(token);
            printf("%% ");
            continue;
        }
        else{
            parseCommand(token);
            printf("%% ");
        }
    }
}