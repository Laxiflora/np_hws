//===================================
#include <iostream>
#include <cstdlib>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "./pj1/npshell.cpp"
#include <string>
#include <bits/stdc++.h>

#define SERV_TCP_PORT 18999
#define LOCAL_HOST "127.0.0.1"
using namespace std;
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
#define MAXCLIENT 30

int getPipeIndexFromIds(int,int);

struct Env{
	int envNum;
	string envTitle[100];
	string envValue[100];
    vector<QueueNode> instTable;
};

struct PipeInfo{
    int pipeSendFrom,pipeSendTo;   //should be user ID instead of FD
    int pipeInFd,pipeOutFd;
    string content;
};

vector<PipeInfo> pipeTable;

void createPipe(int sendFrom,int sendTo,string content){ //ID
    PipeInfo newNode;
    int temp[2];
    pipe(temp);
    newNode.pipeSendFrom = sendFrom;
    newNode.pipeSendTo = sendTo;
    newNode.pipeInFd=temp[1];
    newNode.pipeOutFd = temp[0];
    newNode.content = content;
    pipeTable.push_back(newNode);
    //printf("Pipe from ID %d to %d created, pipeIn=%d,pipeOut=%d\n",sendFrom,sendTo,newNode.pipeInFd,newNode.pipeOutFd);
}

int getPipeIndexFromIds(int sendFrom,int sendTo){
    for(int i=0;i<pipeTable.size();i++){
        if(pipeTable[i].pipeSendFrom==sendFrom && pipeTable[i].pipeSendTo==sendTo){
            return i;
        }
    }
    return -1;
}

void deletePipe(int sendFrom,int sendTo){ //ID
    if(sendFrom == -1){
        for(int i=0;i<pipeTable.size();i++){
            if(pipeTable[i].pipeSendTo == sendTo){
                //printf("Closing fd %d,%d\n",pipeTable[i].pipeInFd,pipeTable[i].pipeOutFd);
                close(pipeTable[i].pipeInFd);
                close(pipeTable[i].pipeOutFd);
                pipeTable[i].pipeSendFrom=0;
                pipeTable[i].pipeSendTo=0;
                pipeTable[i].pipeInFd = 0;
                pipeTable[i].pipeOutFd = 0;
                if(pipeTable.size()==1){
                    pipeTable.clear();
                    return;
                }
                else{
                    pipeTable.erase(pipeTable.begin() + i);
                    i--;
                }
            }
        }
        return;
    }
    if(sendTo == -1){
        for(int i=0;i<pipeTable.size();i++){
            if(pipeTable[i].pipeSendFrom == sendFrom){
                //printf("Closing fd %d,%d\n",pipeTable[i].pipeInFd,pipeTable[i].pipeOutFd);
                close(pipeTable[i].pipeInFd);
                close(pipeTable[i].pipeOutFd);
                pipeTable[i].pipeSendFrom=0;
                pipeTable[i].pipeSendTo=0;
                pipeTable[i].pipeInFd = 0;
                pipeTable[i].pipeOutFd = 0;
                if(pipeTable.size()==1){
                    pipeTable.clear();
                    return;
                }
                else{
                    pipeTable.erase(pipeTable.begin() + i);
                    i--;
                }
            }
        }
        return;
    }
    int index = getPipeIndexFromIds(sendFrom,sendTo);
    //printf("Closing fd %d,%d\n",pipeTable[index].pipeInFd,pipeTable[index].pipeOutFd);
    //close(pipeTable[index].pipeInFd);  //shall already closed.
    close(pipeTable[index].pipeOutFd);
    pipeTable[index].pipeSendFrom=0;
    pipeTable[index].pipeSendTo=0;
    pipeTable[index].pipeInFd = 0;
    pipeTable[index].pipeOutFd = 0;
    if(pipeTable.size()==1){
        pipeTable.clear();
    }
    else{
        pipeTable.erase(pipeTable.begin() + index);
    }
}


class ClientTable{
    public:
        int clientFd[MAXCLIENT];
        sockaddr_in clientInfo[MAXCLIENT];
        string clientName[MAXCLIENT];
        Env clientEnv[MAXCLIENT];
        int clientSize;
        int clientId[MAXCLIENT];
        ClientTable(){
            for(int i=0;i<MAXCLIENT;i++){
                clientFd[i]=0;
                clientId[i]=0;
            }
            clientSize=0;
        };

};

ClientTable cliTable;





int getTableIndexByFd(int fd){
    for(int i=0;i<cliTable.clientSize;i++){
        if(cliTable.clientFd[i]==fd){
            return i;
        }
    }
    return -1;
}

int getTableIndexById(int id){
    for(int i=0;i<cliTable.clientSize;i++){
        if(cliTable.clientId[i]==id){
            return i;
        }
    }
    return -1;
}



void recordClientInfo(int clifd,sockaddr_in info,string name){
    for(int i=0;i<=cliTable.clientSize;i++){
        if(cliTable.clientSize==i){  // no users logout yet.
            cliTable.clientSize++;
        }
        if(cliTable.clientFd[i]==0){ // this slot has not been used yet.
            cliTable.clientFd[i]=clifd;
            cliTable.clientName[i] = "(no name)";
            cliTable.clientInfo[i] = info;
            cliTable.clientEnv[i].envTitle[0] = "PATH";
            cliTable.clientEnv[i].envValue[0] = "bin:.";
            cliTable.clientEnv[i].envNum = 1;
            cliTable.clientId[i] = i+1;  //ID start from 1
            break;
        }
    }
}

void broadCast(int callerFd,char* msg){
	int stdout = dup(STDOUT_FILENO);
    if(callerFd!=0){
        dup2(callerFd, STDOUT_FILENO);
        printf("%s",msg);
    }
	for(int i = 0; i < cliTable.clientSize; ++i){
		if (cliTable.clientFd[i] == callerFd) continue;
		if (cliTable.clientFd[i] == 0) continue;
		dup2(cliTable.clientFd[i], STDOUT_FILENO);
		printf("%s",msg);
	}
	dup2(stdout, STDOUT_FILENO);
    //printf("Closing fd %d\n",stdout);
	close(stdout);
}

int isUserPipeIn(string in){
    return ((in.find("<")!=-1)&&(in.length()>1)) ;
}


int isUserPipeOut(string in){
    return ((in.find(">")!=-1)&&(in.length()>1));
}


/***
* Function that handles cat <4 >5
***/
int handleSpecitalCase(vector<string> token,int newsockFd,string input){
    int hasPipeIn=0,hasPipeOut=0;
    for(int i=0;i<token.size();i++){
        if(isOrdinPipe(token[i])||isExclamation(token[i])||isNumberPipe(token[i])||isWriteFile(token[i]))
            return 1;
        if(isUserPipeIn(token[i]))
            hasPipeIn = i;
        if(isUserPipeOut(token[i]))
            hasPipeOut=i;
    }
    if(!hasPipeIn || !hasPipeOut)
        return 1;
    //is a special case
    int pipeFromId = stoi(token[hasPipeIn].substr(1,token[hasPipeIn].length()) );
    int pipeToId = stoi(token[hasPipeOut].substr(1,token[hasPipeOut].length()));
    int inputFd=999;
    int outputFd=999;
    int myId = cliTable.clientId[getTableIndexByFd(newsockFd)];
    int isNotNull=0;
    //check fromId exist

    // get information
    int cliFromIndex=0;
    if((cliFromIndex = getTableIndexById(pipeFromId)) == -1){ //sender not exist
        printf("*** Error: user #%d does not exist yet. ***\n",pipeFromId);
        inputFd = open("/dev/null",O_RDWR); // read from blackhole!
    }
    else{
        if(getPipeIndexFromIds(pipeFromId,myId)==-1){//no pipe exist
            printf("*** Error: the pipe #%d->#%d does not exist yet. ***\n",pipeFromId,myId);
            inputFd = open("/dev/null",O_RDWR); // read from blackhole!
            
            //handle pipeError                
        }
        else{
            int index = getPipeIndexFromIds(pipeFromId,myId);
            inputFd =  pipeTable[index].pipeOutFd;
            char msg[512];
            sprintf(msg,"*** %s (#%d) just received from %s (#%d) by '%s' ***\n",cliTable.clientName[getTableIndexById(myId)].c_str(),myId,cliTable.clientName[getTableIndexById(pipeFromId)].c_str(),pipeFromId,input.c_str());
            broadCast(newsockFd,msg);
            isNotNull=1;
        }
    }


    //send information
    int isgarbage=1;
    if(getPipeIndexFromIds(myId,pipeToId)!=-1){ // the pipe already exists
        printf("*** Error: the pipe #%d->#%d already exists. ***\n",myId,pipeToId);
        outputFd = open("/dev/null",O_RDWR); // write to blackhole!
        //handle pipeError
    }
    else{
        //cout<<"PIpeto = "<<pipeTo<<", pipeFrom = "<<newsockFd;
        int cliFromIndex = getTableIndexById(myId);
        int cliToIndex;
        if((cliToIndex =getTableIndexById(pipeToId)) == -1){  //this user does not exist yet.
            printf("*** Error: user #%d does not exist yet. ***\n",pipeToId);
            outputFd = open("/dev/null",O_RDWR); // write to blackhole!
            //cout<<"command[0] = "<< argv[0]<<" ,command[1]="<<argv[1]<<" , inFd="<<pipeEntry_FD<<", outFd="<<noop<<" , commandInFd = "<<commandInFd<<", End of "<< argv[0]<<endl;
            //return;
        }
        else{
            createPipe(myId,pipeToId,input);
            int pipeindex = getPipeIndexFromIds(myId,pipeToId);
            outputFd = pipeTable[pipeindex].pipeInFd;
            char msg[1000]={'\0'};
            sprintf(msg,"*** %s (#%d) just piped '%s' to %s (#%d) ***\n",cliTable.clientName[cliFromIndex].c_str() , cliTable.clientId[cliFromIndex] , input.c_str() , cliTable.clientName[cliToIndex].c_str() , cliTable.clientId[cliToIndex]);
            broadCast(newsockFd,msg);
            isgarbage=0;
        }
    }
    token.erase(token.begin() + hasPipeIn);
    token.erase(token.begin() + hasPipeOut);
    int argc = token.size();
    char* argv[argc+1];
    parseArgument(token,argc,0,argv);
    // int stdin = dup(STDIN_FILENO);
    // int stdout = dup(STDOUT_FILENO);
    // dup2(inputFd,STDIN_FILENO);
    // dup2(outputFd,STDOUT_FILENO);
    // int temp=STDOUT_FILENO;
    int noop = -1;
    execCommand(argv,0,inputFd,outputFd,noop,0);
    // dup2(stdin,STDIN_FILENO);
    // dup2(stdout,STDOUT_FILENO);
    // close(stdin);
    // close(stdout);
    //printf("closing %d\t",inputFd);
    close(inputFd);
    if(isNotNull==1){
        deletePipe(pipeFromId,myId);
    }
    //printf("closing %d\n",outputFd);
    close(outputFd);
    updateTable();
    return 0;
}

/***
* Function that handles cat <4 |1
***/
int handleNumpipeSpecialCase(vector<string> token,int newsockFd,string input){
    int hasPipeIn=0,hasNumpipe=0;
    for(int i=0;i<token.size();i++){
        if(isOrdinPipe(token[i])||isExclamation(token[i])||isWriteFile(token[i]) || isUserPipeOut(token[i]) )
            return 1;
        if(isUserPipeIn(token[i]))
            hasPipeIn = i;
        if(isNumberPipe(token[i])){
            hasNumpipe=i;
        }
    }
    if(!hasPipeIn || !hasNumpipe)
        return 1;

    // is spetial case
    int pipeFromId = stoi(token[hasPipeIn].substr(1,token[hasPipeIn].length()) );
    int round = stoi(token[hasNumpipe].substr(1,token[hasNumpipe].length()));
    int inputFd=999;
    int outputFd=999;
    int myId = cliTable.clientId[getTableIndexByFd(newsockFd)];
    int isNotNull=0;
    //check fromId exist

    // get information
    int cliFromIndex=0;
    if((cliFromIndex = getTableIndexById(pipeFromId)) == -1){ //sender not exist
        printf("*** Error: user #%d does not exist yet. ***\n",pipeFromId);
        inputFd = open("/dev/null",O_RDWR); // read from blackhole!
    }
    else{
        if(getPipeIndexFromIds(pipeFromId,myId)==-1){//no pipe exist
            printf("*** Error: the pipe #%d->#%d does not exist yet. ***\n",pipeFromId,myId);
            inputFd = open("/dev/null",O_RDWR); // read from blackhole!
            
            //handle pipeError                
        }
        else{
            int index = getPipeIndexFromIds(pipeFromId,myId);
            inputFd =  pipeTable[index].pipeOutFd;
            char msg[512];
            sprintf(msg,"*** %s (#%d) just received from %s (#%d) by '%s' ***\n",cliTable.clientName[getTableIndexById(myId)].c_str(),myId,cliTable.clientName[getTableIndexById(pipeFromId)].c_str(),pipeFromId,input.c_str());
            broadCast(newsockFd,msg);
            isNotNull=1;
        }
    }

    token.erase(token.begin() + hasPipeIn);
    token.erase(token.begin() + hasNumpipe);
    int argc = token.size();
    char* argv[argc+1];
    parseArgument(token,argc,0,argv);

    int pipeInFd=-1,pipeOutFd=-1; //if no exist number pipe, create one.
    checkTable(round,pipeInFd,pipeOutFd);
    if(pipeOutFd == -1){                  // means no same round number pipe is found, pipe in fd not specified
        execCommand(argv,0,inputFd,pipeInFd,pipeOutFd,1);
        registerTable(pipeOutFd,pipeInFd,round);
    }
    else{
        execCommand(argv,0,inputFd,pipeInFd,pipeOutFd,1);
    }
    updateTable();
    close(inputFd);
    if(isNotNull==1){
        deletePipe(pipeFromId,myId);
    }
    return 0;
}

void parseCommand_serv2(vector<string> token,int newsockFd,string input){
    int start=0;
    int tail=0;
    int argc=0;
    int isFirst=1;
    int noop= -1;  // a useless integer.
    int commandInFd = -1; // this is the fd that is going to input to this command
    int pipeInFd = -1;  //this is the pipe input fd that this command going to output to
    int pipeOutFd=-1; // this is the information that the established pipe is going to output to
    //char relation = '-';
    for(int i=0;i<token.size();i++){
        if(isUserPipeIn(token[i])){ // exist input
            int firstCutPos=0;
            while(firstCutPos<token.size()&&!isOrdinPipe(token[firstCutPos])&& !isExclamation(token[firstCutPos])&&!isNumberPipe(token[firstCutPos])&&!isWriteFile(token[firstCutPos])&& !isUserPipeIn(token[firstCutPos]) && !isUserPipeOut(token[firstCutPos])){
                firstCutPos++;
            }
            // printf("before swap\n");
            // for(int i=0;i<token.size();i++){
            //     cout<<token[i]<<" ";
            // }
            string temp = token[i];
            if(firstCutPos <= i){
                token.erase(token.begin()+i);
                token.insert(token.begin()+firstCutPos,temp);
            }
            // printf("\nafter swap\n");
            // for(int i=0;i<token.size();i++){
            //     cout<<token[i]<<" ";
            // }
            //printf("Load success, commandInfd = %d",commandInFd);
            break;
        }
    }
    if(handleSpecitalCase(token,newsockFd,input)==0)
        return;
    if(handleNumpipeSpecialCase(token,newsockFd,input)==0)
        return;
    while(tail<token.size()){
        //cout<<"tail="<<token[tail]<<endl;
        while(tail<token.size()&&!isOrdinPipe(token[tail])&& !isExclamation(token[tail])&&!isNumberPipe(token[tail])&&!isWriteFile(token[tail])&& !isUserPipeIn(token[tail]) && !isUserPipeOut(token[tail])){
            tail++;
        }
        if(isFirst){
            if( checkTable(0,pipeInFd,commandInFd) != -1){
               // cout<<"CHECK,command "<<token[start]<<" has pipeIn = "<< pipeInFd << " . commandInFd = "<<commandInFd<<endl;
           //     cout<<"closing "<<pipeInFd<<endl;
           //printf("Closing fd %d\n",pipeInFd);
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
        string wtf[argc];
        for(int i=0;i<argc;i++){
            wtf[i] = token[start+i];
        }
        //printf("start = %d,argv[0]=%s,argc = %d , tail = %d\n",start,argv[0],argc);
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
        else if(tail<token.size() && isUserPipeIn(token[tail])){
            //cout<<"DIFF\n";
            int fromId = stoi(token[tail].substr(1,token[tail].length()) ) ;
            int pipeFrom = cliTable.clientFd[getTableIndexById(fromId)];
            int pipeEntry_FD;
            int cliFromIndex;
            int isNotNull=0;
            int index=0;
            int myId = cliTable.clientId[getTableIndexByFd(newsockFd)];
            pipeInFd = -1;
            pipeOutFd = -1;
            if(cliFromIndex = getTableIndexById(fromId) == -1){ //sender not exist
                printf("*** Error: user #%d does not exist yet. ***\n",fromId);
                pipeEntry_FD = open("/dev/null",O_RDWR); // read from blackhole!
                pipeInFd = STDOUT_NO;
                pipeOutFd = -1;
            }
            else{
                if(getPipeIndexFromIds(fromId,myId)==-1){//no pipe exist
                    printf("*** Error: the pipe #%d->#%d does not exist yet. ***\n",fromId,myId);
                    pipeEntry_FD = open("/dev/null",O_RDWR); // read from blackhole!
                    pipeInFd = STDOUT_NO;
                    pipeOutFd = -1;
                    
                    //handle pipeError                
                }
                else{
                    index = getPipeIndexFromIds(fromId,myId);
                    pipeEntry_FD =  pipeTable[index].pipeOutFd;
                    char msg[512];
                    sprintf(msg,"*** %s (#%d) just received from %s (#%d) by '%s' ***\n",cliTable.clientName[getTableIndexById(myId)].c_str(),myId,cliTable.clientName[getTableIndexById(fromId)].c_str(),fromId,input.c_str());
                    broadCast(newsockFd,msg);
                    isNotNull=1;
                }
            }
            commandInFd = pipeEntry_FD;
            if(tail+1 >= token.size()){
                int temp = STDOUT_NO;
                int noop=-1;
                execCommand(argv,0,commandInFd,temp,noop,0);
                if(isNotNull)  {deletePipe(fromId,myId);}
                
                else {
                    close(pipeEntry_FD); //printf("Closing fd %d",pipeEntry_FD);
                }
                return;
            }
            else{
                execCommand(argv,0,commandInFd,pipeInFd,pipeOutFd,0);
                if(isNotNull){    deletePipe(fromId,myId);}
                else {
                    close(pipeEntry_FD); //printf("Closing fd %d",pipeEntry_FD);
                    }
                start=tail;
                tail++;
            }
            commandInFd = -1;
        }
        else if(tail<token.size() && isUserPipeOut(token[tail]) ) {
            int ToId= stoi(token[tail].substr(1,token[tail].length()));
            int pipeTo = cliTable.clientFd[getTableIndexById(ToId)];
            int pipeEntry_FD; // 這是要user pipe透過pipe()得到的fd，非socket fd!
            int myId = cliTable.clientId[getTableIndexByFd(newsockFd)];
            if(getPipeIndexFromIds(myId,ToId)!=-1){ // the pipe already exists
                printf("*** Error: the pipe #%d->#%d already exists. ***\n",myId,ToId);
                pipeEntry_FD = open("/dev/null",O_RDWR); // write to blackhole!
                //handle pipeError
            }
            else{
                //cout<<"PIpeto = "<<pipeTo<<", pipeFrom = "<<newsockFd;
                int cliFromIndex = getTableIndexById(myId);
                int cliToIndex;
                if((cliToIndex =getTableIndexById(ToId)) == -1){  //this user does not exist yet.
                    printf("*** Error: user #%d does not exist yet. ***\n",ToId);
                    pipeEntry_FD = open("/dev/null",O_RDWR); // write to blackhole!
                    //cout<<"command[0] = "<< argv[0]<<" ,command[1]="<<argv[1]<<" , inFd="<<pipeEntry_FD<<", outFd="<<noop<<" , commandInFd = "<<commandInFd<<", End of "<< argv[0]<<endl;
                    //return;
                }
                else{
                    createPipe(cliTable.clientId[cliFromIndex],cliTable.clientId[cliToIndex],input);
                    int pipeindex = getPipeIndexFromIds(myId,ToId);
                    pipeEntry_FD = pipeTable[pipeindex].pipeInFd;
                    char msg[1000]={'\0'};
                    sprintf(msg,"*** %s (#%d) just piped '%s' to %s (#%d) ***\n",cliTable.clientName[cliFromIndex].c_str() , cliTable.clientId[cliFromIndex] , input.c_str() , cliTable.clientName[cliToIndex].c_str() , cliTable.clientId[cliToIndex]);
                    broadCast(newsockFd,msg);

                }
                
            }
            int stdout = dup(STDOUT_FILENO);
            dup2(pipeEntry_FD,STDOUT_FILENO);
            //cout<<"command = "<<argv[0]<<" , inFd="<<pipeEntry_FD<<", outFd="<<noop<<" , commandInFd = "<<commandInFd<<", End of "<< argv[0]<<endl;
            int out = STDOUT_FILENO;
            noop=-1;
            execCommand(argv,0,commandInFd,out,noop,0);
            commandInFd = -1;
            dup2(stdout,STDOUT_FILENO);
            //printf("Closing fd %d , %d\n\n",stdout,pipeEntry_FD);
            close(stdout);
            close(pipeEntry_FD);
            noop = -1;
            pipeInFd = -1;
            pipeOutFd = -1;
            updateTable();
            return; //UserPipeOut必定為整行最後的指令，同write
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
        else if(isWriteFile(token[tail])){
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
    if(start < tail && tail>=token.size() && !isNumberPipe(token[tail-1]) && !isUserPipeOut(token[tail-1]) ){ //got some remaining, not a number pipe
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



void exec_npshell_serv2(int newsockFd,fd_set& readfds){
    rootPid = getpid();
    string input="";
    getline(cin,input);
    if(input.find("\r\n")!=-1){
        input[input.find("\r\n")]='\0';
    }
    if(input.find("\n")!=-1){
        input[input.find("\n")]='\0';
    }
    if(input.find("\r")!=-1){
        input[input.find("\r")]='\0';
    }
    vector<string> token;
    if(input=="\0"){
        printf("%% ");
    }
    else{
        token = split(input,' ');
    }
    if(strcmp(token[0].c_str(),"printenv")==0){
        updateTable();
        printenv(token);
        printf("%% ");
    }
    else if(strcmp(token[0].c_str(),"setenv")==0){
        updateTable();
        setenv(token);
        printf("%% ");
    }
    else if(strcmp(token[0].c_str(),"yell")==0){
        updateTable();
        input.erase(0,5);
        int index = getTableIndexByFd(newsockFd);
        char yell[1030];
        sprintf(yell,"*** %s yelled ***: %s\n",cliTable.clientName[index].c_str(),input.c_str());
        broadCast(newsockFd,yell);
        printf("%% ");
    }
    else if (strcmp(token[0].c_str(),"who")==0){
        printf("<ID>\t<nickname>\t<IP:port>\t<indicate me>\n");
        for(int i=0;i<cliTable.clientSize;i++){
            if(cliTable.clientId[i]==0 || cliTable.clientFd[i]==0){
                continue;
            }
            int ID = cliTable.clientId[i];
            char IP[20];
            inet_ntop(AF_INET,&cliTable.clientInfo[i].sin_addr,IP,sizeof(struct sockaddr));
            int port = ntohs(cliTable.clientInfo[i].sin_port);
            printf("%d\t%s\t%s:%d\t",ID,cliTable.clientName[i].c_str(),IP,port);
            if(newsockFd==cliTable.clientFd[i]){
                printf("<-me\n");
            }
            else{
                printf("\n");
            }
        }
        printf("%% ");
    }
    else if (strcmp(token[0].c_str(),"exit")==0){
        int index = getTableIndexByFd(newsockFd);
        sockaddr_in temp;
        deletePipe(-1,cliTable.clientId[index]);
        deletePipe(cliTable.clientId[index],-1);
        char msg[256];
        sprintf(msg,"*** User '%s' left. ***\n",cliTable.clientName[index].c_str());
        cliTable.clientFd[index] = 0;
        cliTable.clientEnv[index].envNum=0;
        cliTable.clientName[index]="";
        cliTable.clientInfo[index]=temp;
        cliTable.clientId[index] = 0;
        //cliTable.clientFd[index].cli_info = NULL;
        FD_CLR(newsockFd,&readfds);
        close(newsockFd);
        broadCast(0,msg);
    }
    else if (strcmp(token[0].c_str(),"name")==0){
        char IP[20];
        int index = getTableIndexByFd(newsockFd);
        inet_ntop(AF_INET,&cliTable.clientInfo[index].sin_addr,IP,sizeof(struct sockaddr));
        int port = ntohs(cliTable.clientInfo[index].sin_port);
        char msg[512];
        for(int i=0;i<cliTable.clientSize;i++){
            if( strcmp(cliTable.clientName[i].c_str(),token[1].c_str()) == 0){
                printf("*** User '%s' already exists. ***\n",token[1].c_str());
                printf("%% ");
                return;
            }
        }
        sprintf(msg,"*** User from %s:%d is named '%s'. ***\n",IP,port,token[1].c_str());
        cliTable.clientName[index] = token[1];
        broadCast(newsockFd,msg);
        printf("%% ");
    }
    else if(strcmp(token[0].c_str(),"tell")==0){
        int tellTo = stoi(token[1]);
        int tellFd;
        int index = getTableIndexByFd(newsockFd);
        input.erase(0,token[0].length()+token[1].length()+2);
        if(getTableIndexById(tellTo)==-1){
            printf("*** Error: user #%d does not exist yet. ***\n",tellTo);
        }
        else{
            char msg[120];
            tellFd = cliTable.clientFd[getTableIndexById(tellTo)];
            // char fromName[120];
            // char content[512];
            // strcpy(content,input.c_str());
            // strcpy(fromName,cliTable.clientName[index].c_str());
            sprintf(msg,"*** %s told you ***: %s\n",cliTable.clientName[index].c_str(),input.c_str());
            send(tellFd,msg,sizeof(char)*strlen(msg),0);
        }
        printf("%% ");
    }
    else{
        parseCommand_serv2(token,newsockFd,input);
        printf("%% ");
    }
}



void sendWelcomeMessage(int fd){
    int index = getTableIndexByFd(fd);
    sockaddr_in client_info = cliTable.clientInfo[index]; 
	char newComerAddr[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &client_info.sin_addr, newComerAddr, INET_ADDRSTRLEN);
	int stdout = dup(STDOUT_FILENO);
	dup2(fd, STDOUT_FILENO);
	printf("****************************************\n"
	       "** Welcome to the information server. **\n" 
	       "****************************************\n");
    dup2(stdout, STDOUT_FILENO);

    char msg[256];
    sprintf(msg,"*** User '(no name)' entered from %s:%d. ***\n", newComerAddr, ntohs(client_info.sin_port));
    broadCast(fd,msg);
    dup2(fd, STDOUT_FILENO);
    printf("%% ");
    dup2(stdout, STDOUT_FILENO);
	close(stdout);
}

void swapClientEnv(int index,vector<QueueNode>& instTable){
    clearenv();
    for(int i=0;i<cliTable.clientEnv[index].envNum;i++){
        setenv(cliTable.clientEnv[index].envTitle[i].c_str(),cliTable.clientEnv[index].envValue[i].c_str(),1);
    }
    instTable = cliTable.clientEnv[index].instTable;
}

void saveClientEnv(int index,vector<QueueNode>& instTable){
    for(int i=0;i<100;i++){
        cliTable.clientEnv[index].envTitle[i] = "";
        cliTable.clientEnv[index].envValue[i] = "";
    }
    char ** s = environ;
    int i=0;
    cliTable.clientEnv[index].envNum=0;
    for (char **env = environ,i=0; *env != 0; env++,i++){
        string data = *env;
        vector<string> envi = split(data,'=');
        cliTable.clientEnv[index].envTitle[i] = envi[0];
        cliTable.clientEnv[index].envValue[i] = envi[1];
        cliTable.clientEnv[index].envNum++;
    }
    cliTable.clientEnv[index].instTable = instTable;
    instTable.clear();
}


int main(int argc,char* argv[]){
    instTable.clear();
    pipeTable.clear();
    setbuf(stdout, NULL);
    int sockfd, newsockfd , childpid,sockMaxFd=0;
    int clientSockfd;
    sockaddr_in cli_info, serv_addr;
    unsigned int clilen = sizeof(cli_info); // need to init, https://stackoverflow.com/questions/32054055/why-does-it-show-received-a-connection-from-0-0-0-0-port-0
    fd_set readfds;
    //pname = argv[0];

    /*
    * Open a TCP socket (an Internet stream socket).
    */
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        printf("server: can't open stream socket");

    /*
    * set the server as SO_REUSEADDR
    */
    const int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");    
    
    
    /*
    * Bind our local address so that the client can send to us.
    */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int port = SERV_TCP_PORT;
    if(argc>1){
        port = stoi(argv[1]);
    }
    serv_addr.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        printf("server: can't bind local address");
    //cout<<"Listening on"<<serv_addr.sin_addr.s_addr<<" , port "<<SERV_TCP_PORT<<endl;
    listen(sockfd, 30);

    for ( ; ; ) {

        FD_ZERO(&readfds);
        FD_SET(sockfd,&readfds);
        sockMaxFd = sockfd;
        for(int i=0;i<cliTable.clientSize;i++){
            if(cliTable.clientFd[i]==0)
                continue;
            FD_SET(cliTable.clientFd[i],&readfds);
            if (sockMaxFd < cliTable.clientFd[i]) sockMaxFd = cliTable.clientFd[i]; 
        }
        if(select(sockMaxFd + 1, &readfds, NULL, NULL, NULL)<0){
            //handle error
        }
        if (FD_ISSET(sockfd, &readfds)){ // someone enter the server, check ISSET
			if( (clientSockfd = accept(sockfd, (struct sockaddr *) &cli_info, &clilen)) <0 ){
                printf("Accept error\n");
            }
			else{
                string temp = "(no name)";
                recordClientInfo(clientSockfd,cli_info,temp);
            }
                sendWelcomeMessage(clientSockfd);
		}
        
        for(int i=0;i<cliTable.clientSize;i++){
            if(cliTable.clientFd[i]==0) // vacant
                continue;
            if (FD_ISSET(cliTable.clientFd[i], &readfds)){	 //if this socket has something to say
				int newsockFd = cliTable.clientFd[i];
				swapClientEnv(i,instTable);
				int fd_err = dup(STDERR_FILENO);
				int fd_out = dup(STDOUT_FILENO);
                int fd_in = dup(STDIN_FILENO);
				dup2(cliTable.clientFd[i], STDERR_FILENO);
				dup2(cliTable.clientFd[i], STDOUT_FILENO);
                dup2(cliTable.clientFd[i],STDIN_FILENO);
			
				exec_npshell_serv2(newsockFd,readfds);
				dup2(fd_err, STDERR_FILENO);
				dup2(fd_out, STDOUT_FILENO);
                dup2(fd_in,STDIN_FILENO);
				saveClientEnv(i,instTable);
				close(fd_in);
				close(fd_out);
                close(fd_err);
			}

        }
    }
}