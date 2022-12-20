#include <iostream>
#include <cstdlib>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include "./pj1/npshell.cpp"
#include <semaphore.h>
#define SERV_TCP_PORT 18789
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
#define MAXCLIENT 32
#define USERNAME_LENGTH 20
#define SHAREMEM_PATH "/311551169_pj2"

int _tableIndex=-1;




struct User{
    public:
        char userName[USERNAME_LENGTH];
        int pid;
        int exist;
        int clientFd;
        int fifoRequest;
        char messageBox[SHELL_INPUT];
        int readPipeFd[MAXCLIENT];
        sem_t semaphore;
        sockaddr_in clientInfo;
        User(){
            exist=0;
            pid=-1;
            sem_init(&semaphore,1,1);
            sprintf(userName,"%s","(no name)");
            sprintf(messageBox,"%s","");
        };
        ~User(){
            sem_destroy(&semaphore); //for safety
        }
};

User* userTable;

static void childSIG(int sig){
    return;
}


static void  SIG_handler(int sig){
    int exitProcessIndex=256;
    switch(sig){
        case SIGCHLD:
            wait(&exitProcessIndex);
            exitProcessIndex = WEXITSTATUS(exitProcessIndex);
            printf("exit status = %d\n",exitProcessIndex);
            if(exitProcessIndex == 256){
                return;
            }
            sem_wait(&userTable[exitProcessIndex].semaphore);
            printf("closing %d\n",userTable[exitProcessIndex].clientFd);
            close(userTable[exitProcessIndex].clientFd);
            userTable[exitProcessIndex].exist = 0;
            userTable[exitProcessIndex].clientFd = -1;
            strcpy(userTable[exitProcessIndex].messageBox,"");
            userTable[exitProcessIndex].pid = -1;
            sem_post(&userTable[exitProcessIndex].semaphore);
            //kill(userTable[exitProcessIndex].pid,SIGKILL); //is this even ok!?
            return;

        case SIGUSR1:  // We expected that the semaphore of this process has already been locked by the signal caller.
            //sem_wait(&userTable[_tableIndex].semaphore);
            if(strcmp(userTable[_tableIndex].messageBox,"") !=0){
                write(STDOUT_FILENO,userTable[_tableIndex].messageBox,sizeof(char)*strlen(userTable[_tableIndex].messageBox));
                strcpy(userTable[_tableIndex].messageBox,"");
            }
            if(userTable[_tableIndex].fifoRequest != -1){
                    char fifoPath[25]={'\0'};
                    sprintf(fifoPath,"./user_pipe/%d_%d",userTable[_tableIndex].fifoRequest , _tableIndex+1);
                    if( (userTable[_tableIndex].readPipeFd[ userTable[_tableIndex].fifoRequest-1] = open(fifoPath, O_RDONLY | O_NONBLOCK)) <0 ){
                        perror("Other Side open failed:");
                        printf("Fifo path = %s\n",fifoPath);
                    }
                    userTable[_tableIndex].fifoRequest = -1;
            }


            //sem_post(&userTable[_tableIndex].semaphore);
            break; 
        case SIGINT: //shall apply for server only
            for(int i=0;i<MAXCLIENT;i++){
                //sem_wait(&userTable[i].semaphore);
                if(userTable[i].exist==1){
                    kill(userTable[i].pid,SIGKILL);  // SIGKILL could be switch if it is too strong
                }
                //sem_post(&userTable[i].semaphore);
            }
            for(int i=0;i<MAXCLIENT;i++){  // only server alive, no need to use semaphore
                sem_destroy(&userTable[i].semaphore);
            }
            if(munmap(userTable, sizeof(User)*MAXCLIENT) !=0 ){
                perror("unmap failed:");
            }
            if(shm_unlink(SHAREMEM_PATH) != 0){
                perror("unlink failed");
            }
            exit(0);
            break;
        default:
            strcpy(userTable[1].userName,"default");
            break;
    }
}

int clientSizeFull(){
    for(int i=0;i<MAXCLIENT;i++){
        sem_wait(&userTable[i].semaphore);
        if(userTable[i].exist==0){
            sem_post(&userTable[i].semaphore);
            return i;
        }
        sem_post(&userTable[i].semaphore);
    }
    return -1;
}

void recordClientInfo_serv3(int clientfd, int pid,sockaddr_in info,string name){
    sem_wait(&userTable[_tableIndex].semaphore);
    sprintf(userTable[_tableIndex].userName,"%s",name.c_str());
    // printf("USER NAME = %s\n",userTable[_tableIndex].userName);
    // printf("client fd = %d\n",clientfd);
    userTable[_tableIndex].pid = pid;
    userTable[_tableIndex].clientInfo = info;
    userTable[_tableIndex].exist = 1;
    userTable[_tableIndex].clientFd=clientfd;  // for server to close fd
    sem_post(&userTable[_tableIndex].semaphore);
}

void broadCast_serv3(int leave,char* msg){
	if(leave !=-1){
        printf("%s",msg);
    }
	for(int i = 0; i < MAXCLIENT; ++i){
        sem_wait(&userTable[i].semaphore);
		if (userTable[i].exist==0) {
            sem_post(&userTable[i].semaphore);
            continue;
        }
        if(i==_tableIndex){
            sem_post(&userTable[i].semaphore);
            continue;
        }
        strcat(userTable[i].messageBox,msg);
		int destpid = userTable[i].pid;
        kill(destpid,SIGUSR1);
        sem_post(&userTable[i].semaphore);
	}
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
int handleSpecitalCase_serv3(vector<string> token,string input){
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
    int pipeToIndex = stoi(token[hasPipeOut].substr(1,token[hasPipeOut].length()))-1;
    int fromIndex= stoi(token[hasPipeIn].substr(1,token[hasPipeIn].length()) )-1;
    int inputFd=999;
    int outputFd=999;
    int isNotNull=0;
    char fifoPath[25];
    //check fromId exist

    sem_wait(&userTable[_tableIndex].semaphore);
    char myName[USERNAME_LENGTH+1];
    sprintf(myName,"%s",userTable[_tableIndex].userName);
    sem_post(&userTable[_tableIndex].semaphore);

    // get information
    int pipeEntry_FD; // 這是要user pipe透過pipe()得到的fd，非socket fd!
    int targetExistence=0;
    if(fromIndex >= MAXCLIENT){
        printf("*** Error: user #%d does not exist yet. ***\n",fromIndex+1);
        pipeEntry_FD = open("/dev/null",O_RDWR); // read from blackhole!   
    }
    else{
        sem_wait(&userTable[fromIndex].semaphore);
        targetExistence = userTable[fromIndex].exist;
        char fromName[USERNAME_LENGTH+1]={'\0'};
        sprintf(fromName,"%s",userTable[fromIndex].userName);
        int sendFromPid = userTable[fromIndex].pid;
        sem_post(&userTable[fromIndex].semaphore);
        sprintf(fifoPath,"./user_pipe/%d_%d",fromIndex+1, _tableIndex+1);

        if(targetExistence == 0){
            printf("*** Error: user #%d does not exist yet. ***\n",fromIndex+1);
            pipeEntry_FD = open("/dev/null",O_RDWR); // read from blackhole!                
        }
        else if((access(fifoPath, F_OK)) == -1) {
            printf("*** Error: the pipe #%d->#%d does not exist yet. ***\n",fromIndex+1,_tableIndex+1);
            pipeEntry_FD = open("/dev/null",O_RDWR); // read from blackhole!
        }

        else{
            if( (pipeEntry_FD = open(fifoPath, O_NONBLOCK | O_RDONLY)) <0){
                perror("read fifo failed:");
            }
            isNotNull = 1;
            char msg[1000]={'\0'};
            sprintf(msg,"*** %s (#%d) just received from %s (#%d) by '%s' ***\n",myName , _tableIndex+1 , fromName , fromIndex+1 , input.c_str());
            broadCast_serv3(0,msg);
        }
    }
        inputFd = pipeEntry_FD;

        // To information     
        char fifoPath_out[25];
        targetExistence=0;
        if(pipeToIndex >= MAXCLIENT){
            printf("*** Error: user #%d does not exist yet. ***\n",pipeToIndex+1);
            pipeEntry_FD = open("/dev/null",O_RDWR); // read from blackhole!   
        }
        else{
            sem_wait(&userTable[pipeToIndex].semaphore);
            targetExistence = userTable[pipeToIndex].exist;
            char toName[USERNAME_LENGTH+1]={'\0'};
            sprintf(toName,"%s",userTable[pipeToIndex].userName);
            int tellToPid = userTable[pipeToIndex].pid;
            sem_post(&userTable[pipeToIndex].semaphore);

            sprintf(fifoPath_out,"./user_pipe/%d_%d",_tableIndex+1,pipeToIndex+1);

            if(targetExistence == 0){
                printf("*** Error: user #%d does not exist yet. ***\n",pipeToIndex+1);
                pipeEntry_FD = open("/dev/null",O_RDWR); // write to blackhole!                
            }
            else if((access(fifoPath_out, F_OK)) != -1) {
                printf("*** Error: the pipe #%d->#%d already exists. ***\n",_tableIndex+1,pipeToIndex+1);
                pipeEntry_FD = open("/dev/null",O_RDWR); // write to blackhole!
            }
            else{
        
                if(mkfifo(fifoPath_out,0666) <0){
                    perror("make fifo failed:");
                }
                // make other side open the fifo read side first.
                sem_wait(&userTable[pipeToIndex].semaphore);
                userTable[pipeToIndex].fifoRequest = _tableIndex+1;
                kill(tellToPid,SIGUSR1);
                sem_post(&userTable[pipeToIndex].semaphore);

                // hold until we can write in to fifo
                while( (pipeEntry_FD = open(fifoPath_out,O_WRONLY)) ==-1){
                    perror("open fifo failed:");
                }
                char msg[1000]={'\0'};
                sprintf(msg,"*** %s (#%d) just piped '%s' to %s (#%d) ***\n",myName , _tableIndex+1 , input.c_str() , toName , pipeToIndex+1);
                broadCast_serv3(0,msg);
            }

        }
        outputFd = pipeEntry_FD;

        token.erase(token.begin() + hasPipeIn);
        token.erase(token.begin() + hasPipeOut);
        int argc = token.size();
        char* argv[argc+1];
        parseArgument(token,argc,0,argv);
        //cout<<"command = "<<argv[0]<<" , inFd="<<outputFd<<", outFd= -1"<<" , commandInFd = "<<inputFd<<", End of "<< argv[0]<<endl;
        int noop=-1;
        execCommand(argv,0,inputFd,outputFd,noop,0);
        if(isNotNull){
            if(unlink(fifoPath)<0)
                perror("unlink failed:");
        }
        //printf("Closing fd %d , %d\n\n",stdout,pipeEntry_FD);
        close(outputFd);
        close(inputFd);
        updateTable();
        return 0; //UserPipeOut必定為整行最後的指令，同write
}

/*** *
* Function that handles cat <4 |1
***/
int handleNumpipeSpecialCase_serv3(vector<string> token,string input){
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
    sem_wait(&userTable[_tableIndex].semaphore);
    char myName[USERNAME_LENGTH+1];
    sprintf(myName,"%s",userTable[_tableIndex].userName);
    sem_post(&userTable[_tableIndex].semaphore);
    int pipeFromIndex = stoi(token[hasPipeIn].substr(1,token[hasPipeIn].length())) -1;
    int round = stoi(token[hasNumpipe].substr(1,token[hasNumpipe].length()));
    int inputFd=999;
    int outputFd=999;
    int isNotNull=0;
    int targetExistence=0;
    int pipeEntry_FD=0;
    char fifoPath[25];
    //check fromId exist

    // get information
    int cliFromIndex=0;
    if(pipeFromIndex >= MAXCLIENT){
        printf("*** Error: user #%d does not exist yet. ***\n",pipeFromIndex+1);
        pipeEntry_FD = open("/dev/null",O_RDWR); // read from blackhole!   
    }
    else{
        sem_wait(&userTable[pipeFromIndex].semaphore);
        targetExistence = userTable[pipeFromIndex].exist;
        char fromName[USERNAME_LENGTH+1]={'\0'};
        sprintf(fromName,"%s",userTable[pipeFromIndex].userName);
        int sendFromPid = userTable[pipeFromIndex].pid;
        sem_post(&userTable[pipeFromIndex].semaphore);
        sprintf(fifoPath,"./user_pipe/%d_%d",pipeFromIndex+1, _tableIndex+1);

        if(targetExistence == 0){
            printf("*** Error: user #%d does not exist yet. ***\n",pipeFromIndex+1);
            pipeEntry_FD = open("/dev/null",O_RDWR); // read from blackhole!                
        }
        else if((access(fifoPath, F_OK)) == -1) {
            printf("*** Error: the pipe #%d->#%d does not exist yet. ***\n",pipeFromIndex+1,_tableIndex+1);
            pipeEntry_FD = open("/dev/null",O_RDWR); // read from blackhole!
        }

        else{
            if( (pipeEntry_FD = open(fifoPath, O_NONBLOCK | O_RDONLY)) <0){
                perror("read fifo failed:");
            }
            isNotNull = 1;
            char msg[1000]={'\0'};
            sprintf(msg,"*** %s (#%d) just received from %s (#%d) by '%s' ***\n",myName , _tableIndex+1 , fromName , pipeFromIndex+1 , input.c_str());
            broadCast_serv3(0,msg);
        }
    }
    inputFd = pipeEntry_FD;
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
        unlink(fifoPath);
    }
    return 0;
}






//=====================
/*
char* myfifo = "userpipe/in_out"
mkfifo(myfifo,0666);
fd = open(myfifo,O_WRONLY);

write(fd,data,sizeof(data) )
close(fd);

if (unlink(fifoPath) < 0) err_sys("client: can't unlink %s", FIFO2);

*/


void parseCommand_serv3(vector<string> token,string input){
    int start=0;
    int tail=0;
    int argc=0;
    int isFirst=1;
    int noop= -1;  // a useless integer.
    int commandInFd = -1; // this is the fd that is going to input to this command
    int pipeInFd = -1;  //this is the pipe input fd that this command going to output to
    int pipeOutFd=-1; // this is the information that the established pipe is going to output to


    char myName[USERNAME_LENGTH+1]={'\0'};
    sem_wait(&userTable[_tableIndex].semaphore);
    sprintf(myName,"%s",userTable[_tableIndex].userName);
    sem_post(&userTable[_tableIndex].semaphore);

    //char relation = '-';
    for(int i=0;i<token.size();i++){
        if(isUserPipeIn(token[i])){ // exist input
            int firstCutPos=0;
            while(firstCutPos<token.size()&&!isOrdinPipe(token[firstCutPos])&& !isExclamation(token[firstCutPos])&&!isNumberPipe(token[firstCutPos])&&!isWriteFile(token[firstCutPos])&& !isUserPipeIn(token[firstCutPos]) && !isUserPipeOut(token[firstCutPos])){
                firstCutPos++;
            }
            string temp = token[i];
            if(firstCutPos <= i){
                token.erase(token.begin()+i);
                token.insert(token.begin()+firstCutPos,temp);
            }
            break;
        }
    }
    if(handleSpecitalCase_serv3(token,input)==0)
         return;
    if(handleNumpipeSpecialCase_serv3(token,input)==0)
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
            int fromIndex= stoi(token[tail].substr(1,token[tail].length())) -1;
            int pipeEntry_FD; // 這是要user pipe透過pipe()得到的fd，非socket fd!
            int targetExistence=0;
            int isNotNull=0;
            char fifoPath[20];
            if(fromIndex >= MAXCLIENT){
                printf("*** Error: user #%d does not exist yet. ***\n",fromIndex+1);
                pipeEntry_FD = open("/dev/null",O_RDWR); // read from blackhole!   
            }
            else{
                //sem_wait(&userTable[fromIndex].semaphore);
                targetExistence = userTable[fromIndex].exist;
                char fromName[USERNAME_LENGTH+1]={'\0'};
                sprintf(fromName,"%s",userTable[fromIndex].userName);
                //int sendFromPid = userTable[fromIndex].pid;
                //sem_post(&userTable[fromIndex].semaphore);
                sprintf(fifoPath,"./user_pipe/%d_%d",fromIndex+1, _tableIndex+1);

                if(targetExistence == 0){
                    printf("*** Error: user #%d does not exist yet. ***\n",fromIndex+1);
                    pipeEntry_FD = open("/dev/null",O_RDWR); // read from blackhole!                
                }
                else if((access(fifoPath, F_OK)) == -1) {
                    printf("*** Error: the pipe #%d->#%d does not exist yet. ***\n",fromIndex+1,_tableIndex+1);
                    pipeEntry_FD = open("/dev/null",O_RDWR); // read from blackhole!
                }

                else{
                    if( (pipeEntry_FD = open(fifoPath, O_NONBLOCK | O_RDONLY)) <0){
                        perror("read fifo failed:");
                    }
                    isNotNull = 1;
                    char msg[1000]={'\0'};
                    sprintf(msg,"*** %s (#%d) just received from %s (#%d) by '%s' ***\n",myName , _tableIndex+1 , fromName , fromIndex+1 , input.c_str());
                    broadCast_serv3(0,msg);
                }
            }
            commandInFd = pipeEntry_FD;
            //cout<<"command = "<<argv[0]<<" , inFd="<<pipeInFd<<", outFd="<<noop<<" , commandInFd = "<<commandInFd<<", End of "<< argv[0]<<endl;
            if(tail+1 >= token.size()){
                int temp = STDOUT_NO;
                int noop=-1;
                execCommand(argv,0,commandInFd,temp,noop,0);
                if(isNotNull){
                    if(unlink(fifoPath)<0)
                        perror("unlink failed:");
                }
                else 
                    close(pipeEntry_FD); //printf("Closing fd %d",pipeEntry_FD);
                return;
            }
            else{
                execCommand(argv,0,commandInFd,pipeInFd,pipeOutFd,0);
                if(isNotNull){
                    if(unlink(fifoPath)<0){
                        perror("unlink failed:");
                    }
                }
                else {
                    close(pipeEntry_FD); //printf("Closing fd %d",pipeEntry_FD);
                }
                start=tail;
                tail++;
            }
            commandInFd = -1;

        }

        else if(tail<token.size() && isUserPipeOut(token[tail]) ) {
            int toIndex= stoi(token[tail].substr(1,token[tail].length())) -1;
            int pipeEntry_FD; // 這是要user pipe透過pipe()得到的fd，非socket fd!
            
            int targetExistence=0;
            if(toIndex >= MAXCLIENT){
                printf("*** Error: user #%d does not exist yet. ***\n",toIndex+1);
                pipeEntry_FD = open("/dev/null",O_RDWR); // read from blackhole!   
            }
            else{
                //sem_wait(&userTable[toIndex].semaphore);
                targetExistence = userTable[toIndex].exist;
                char toName[USERNAME_LENGTH+1]={'\0'};
                sprintf(toName,"%s",userTable[toIndex].userName);
                int tellToPid = userTable[toIndex].pid;
                //sem_post(&userTable[toIndex].semaphore);

                char fifoPath[20];
                sprintf(fifoPath,"./user_pipe/%d_%d",_tableIndex+1,toIndex+1);

                if(targetExistence == 0){
                    printf("*** Error: user #%d does not exist yet. ***\n",toIndex+1);
                    pipeEntry_FD = open("/dev/null",O_RDWR); // write to blackhole!                
                }
                else if((access(fifoPath, F_OK)) != -1) {
                    printf("*** Error: the pipe #%d->#%d already exists. ***\n",_tableIndex+1,toIndex+1);
                    pipeEntry_FD = open("/dev/null",O_RDWR); // write to blackhole!
                }
                else{
            
                    if(mkfifo(fifoPath,0666) <0){
                        perror("make fifo failed:");
                    }
                    // make other side open the fifo read side first.
                    sem_wait(&userTable[toIndex].semaphore);
                    userTable[toIndex].fifoRequest = _tableIndex+1;
                    kill(tellToPid,SIGUSR1);
                    sem_post(&userTable[toIndex].semaphore);

                    // hold until we can write in to fifo
                    while( (pipeEntry_FD = open(fifoPath,O_WRONLY)) ==-1){
                        perror("open fifo failed:");
                    }
                    char msg[1000]={'\0'};
                    sprintf(msg,"*** %s (#%d) just piped '%s' to %s (#%d) ***\n",myName , _tableIndex+1 , input.c_str() , toName , toIndex+1);
                    broadCast_serv3(0,msg);
                }

            }
            //cout<<"command = "<<argv[0]<<" , inFd="<<pipeEntry_FD<<", outFd="<<noop<<" , commandInFd = "<<commandInFd<<", End of "<< argv[0]<<endl;
            noop=-1;
            execCommand(argv,0,commandInFd,pipeEntry_FD,noop,0);
            commandInFd = -1;
            //printf("Closing fd %d , %d\n\n",stdout,pipeEntry_FD);
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


void deleteFifo(){ //delete all fifo related to me
    char path[25];
    for(int i=0;i<MAXCLIENT;i++){
        sprintf(path,"./user_pipe/%d_%d",i,_tableIndex+1);
        if((access(path, F_OK)) != -1) {
            unlink(path);
        }
        sprintf(path,"./user_pipe/%d_%d",_tableIndex+1,i);
        if((access(path, F_OK)) != -1) {
            unlink(path);
        }
    }
}

//=================================

void exec_npshell_serv3(int newsockfd){
    setenv("PATH","bin:.",1);
    string input="";
    printf("%% ");
    while(1){
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
        if(input==""){
            printf("%% ");
            continue;
        }
        else{
            token = split(input,' ');
        }
        if(strcmp(token[0].c_str(),"exit")==0){
            int wpid;
            while ((wpid = wait(NULL)) > 0){}
            //clear fifo , messages, etc;.
            char msg[512];
            sem_wait(&userTable[_tableIndex].semaphore);
            sprintf(msg,"*** User '%s' left. ***\n",userTable[_tableIndex].userName);
            sem_post(&userTable[_tableIndex].semaphore);
            broadCast_serv3(-1,msg);
            deleteFifo();
            break;
        }
        else if(strcmp(token[0].c_str(),"tell")==0){
            int tellTo = stoi(token[1])-1;
            int tellToPid=-1;
            int tellFd;
            input.erase(0,token[0].length()+token[1].length()+2);
            updateTable();
            char myName[USERNAME_LENGTH+1]={'\0'};
            sem_wait(&userTable[_tableIndex].semaphore);
            sprintf(myName,"%s",userTable[_tableIndex].userName);
            sem_post(&userTable[_tableIndex].semaphore);

            sem_wait(&userTable[tellTo].semaphore);
            if(userTable[tellTo].exist==0){
                printf("*** Error: user #%d does not exist yet. ***\n",tellTo+1);
                printf("%% ");
                sem_post(&userTable[tellTo].semaphore);
                continue;
            }
            else{
                tellToPid = userTable[tellTo].pid;
                sprintf(userTable[tellTo].messageBox,"*** %s told you ***: %s\n",myName,input.c_str());
                kill(tellToPid,SIGUSR1);
                sem_post(&userTable[tellTo].semaphore);
                printf("%% ");
                continue;
            }
        }
        else if (strcmp(token[0].c_str(),"name")==0){
            int isNamed=0;
            updateTable();
            char msg[512];
            for(int i=0;i<MAXCLIENT;i++){
                sem_wait(&userTable[i].semaphore);
                if( strcmp(userTable[i].userName,token[1].c_str()) == 0){
                    printf("*** User '%s' already exists. ***\n",token[1].c_str());
                    printf("%% ");
                    sem_post(&userTable[i].semaphore);
                    isNamed=1;
                    break;
                }
                sem_post(&userTable[i].semaphore);
            }
            if(!isNamed){
                sem_wait(&userTable[_tableIndex].semaphore);
                char IP[20];
                inet_ntop(AF_INET,&userTable[_tableIndex].clientInfo.sin_addr,IP,sizeof(struct sockaddr));
                int port = ntohs(userTable[_tableIndex].clientInfo.sin_port);
                sprintf(userTable[_tableIndex].userName,"%s",token[1].c_str());
                sem_post(&userTable[_tableIndex].semaphore);


                sprintf(msg,"*** User from %s:%d is named '%s'. ***\n",IP,port,token[1].c_str());
                broadCast_serv3(0,msg);
                printf("%% ");
            }
        }
        else if (strcmp(token[0].c_str(),"who")==0){
            updateTable();
            printf("<ID>\t<nickname>\t<IP:port>\t<indicate me>\n");
            for(int i=0;i<MAXCLIENT;i++){
                sem_wait(&userTable[i].semaphore);
                if(userTable[i].exist==0){
                    sem_post(&userTable[i].semaphore);
                    continue;
                }
                int ID = i+1;
                char IP[20];
                inet_ntop(AF_INET,&userTable[i].clientInfo.sin_addr,IP,sizeof(struct sockaddr));
                int port = ntohs(userTable[i].clientInfo.sin_port);
                printf("%d\t%s\t%s:%d\t",ID,userTable[i].userName,IP,port);
                if(i==_tableIndex){
                    printf("<-me\n");
                }
                else{
                    printf("\n");
                }
                sem_post(&userTable[i].semaphore);
            }
            printf("%% ");
        }
        else if(strcmp(token[0].c_str(),"yell")==0){
            updateTable();
            input.erase(0,5);
            char yell[1030];

            char myName[USERNAME_LENGTH+1]={'\0'};
            sem_wait(&userTable[_tableIndex].semaphore);
            sprintf(myName,"%s",userTable[_tableIndex].userName);
            sem_post(&userTable[_tableIndex].semaphore);

            sprintf(yell,"*** %s yelled ***: %s\n",myName,input.c_str());
            broadCast_serv3(0,yell);
            printf("%% ");
        }
        else if(strcmp(token[0].c_str(),"printenv")==0){
            updateTable();
            printenv(token);
            printf("%% ");
            continue;
        }
        else if(strcmp(token[0].c_str(),"setenv")==0){
            updateTable();
            setenv(token);
            printf("%% ");
            continue;
        }
        else{
            parseCommand_serv3(token,input);
            printf("%% ");
        }
    }
}


void initTable(){
    for(int i=0;i<MAXCLIENT;i++){
        for(int j=0;j<MAXCLIENT;j++){
            userTable[i].readPipeFd[j]=-1;
        }
        sem_destroy(&userTable[i].semaphore);
        userTable[i].exist=0;
        userTable[i].pid=-1;
        userTable[i].clientFd=-1;
        userTable[i].fifoRequest = -1;
        sem_init(&userTable[i].semaphore,1,1);
        sprintf(userTable[i].userName,"%s","(no name)");
        sprintf(userTable[i].messageBox,"%s","");
    }
}



void sendWelcomeMessage_serv3(int _tableIndex){
	printf("****************************************\n"
	       "** Welcome to the information server. **\n" 
	       "****************************************\n");
    char newComerAddr[INET_ADDRSTRLEN];
    char msg[512];
    int newComerPort;
    sem_wait(&userTable[_tableIndex].semaphore);
    inet_ntop(AF_INET, &userTable[_tableIndex].clientInfo.sin_addr, newComerAddr, INET_ADDRSTRLEN);
    newComerPort = ntohs(userTable[_tableIndex].clientInfo.sin_port);

    sem_post(&userTable[_tableIndex].semaphore);

    sprintf(msg,"*** User '(no name)' entered from %s:%d. ***\n", newComerAddr,newComerPort);
    broadCast_serv3(0,msg);
}


int main(int argc, char **argv){
    setbuf(stdout, NULL);
    /* Allocate shared memory and install SIGNAL handlers*/
    int fd = shm_open(SHAREMEM_PATH,  O_CREAT | O_RDWR | O_TRUNC, 0666);
    ftruncate(fd, sizeof(User)*MAXCLIENT);
    userTable = (User *)mmap(NULL,  sizeof(User)*MAXCLIENT, O_RDWR, MAP_SHARED | O_CREAT, fd, 0);
    close(fd);
    initTable();
    signal (SIGCHLD, SIG_handler);
    signal(SIGUSR1,SIG_handler);
    signal(SIGINT,SIG_handler);
    // return 0;
    int sockfd, newsockfd , childpid;
    unsigned int clilen;
    struct sockaddr_in cli_addr, serv_addr;
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
    listen(sockfd, 5);

    for ( ; ; ) {
        clilen = sizeof(cli_addr);
        if( (newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) <0 ){
            printf("accept error\n");
            continue;
        }
        if ( (childpid = fork()) == -1){
            waitProcess(childpid);
            childpid = fork();
        }
        if (childpid == 0) { /* child process */
            signal(SIGCHLD,childSIG);
            /* close original socket */
            close(sockfd);
            int pid = getpid();
            if( (_tableIndex = clientSizeFull()) == -1 ){
                // client amount exceed!
                exit(-1);
            }
            recordClientInfo_serv3(newsockfd,pid,cli_addr,"(no name)");
            /* process the request */
			dup2(newsockfd, STDERR_FILENO);
			dup2(newsockfd, STDOUT_FILENO);
            dup2(newsockfd, STDIN_FILENO);
            
            sendWelcomeMessage_serv3(_tableIndex);
            exec_npshell_serv3(newsockfd);
            exit(_tableIndex);
        }
    }


}

/*
char* myfifo = "userpipe/in_out"
mkfifo(myfifo,0666);
fd = open(myfifo,O_WRONLY);

write(fd,data,sizeof(data) )
close(fd);

if (unlink(fifoPath) < 0) err_sys("client: can't unlink %s", FIFO2);

*/