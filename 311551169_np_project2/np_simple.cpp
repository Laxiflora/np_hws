#include <iostream>
#include <cstdlib>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "./pj1/npshell.cpp"
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

static void handler(int signo){
    switch(signo){
        case SIGCHLD:
            break;
    }
}


int main(int argc,char* argv[]){
    signal (SIGCHLD, handler);
    int sockfd, newsockfd , childpid;
    unsigned int clilen;
    struct sockaddr_in cli_addr, serv_addr;
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
    listen(sockfd, 5);

    for ( ; ; ) {
        clilen = sizeof(cli_addr);
        if( (newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) <0 ){
            printf("accept error\n");
            continue;
        }
        if ( (childpid = fork()) == -1){ // process limit exceed
            waitProcess(childpid);
            childpid = fork();
        }
        else if (childpid == 0) { /* child process */
            /* close original socket */
            close(sockfd);
            /* process the request */
			dup2(newsockfd, STDERR_FILENO);
			dup2(newsockfd, STDOUT_FILENO);
            dup2(newsockfd, STDIN_FILENO);
            exec_npshell();
            exit(0);
        }
        else{  // parent will wait until child leave custom shell
            waitpid(childpid,NULL,0);
        }
        close(newsockfd); /* parent process */
    }


}






	// all_users = (User *) mmap(NULL, sizeof(User) * (MAX_USER + 1), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	// if (all_users == MAP_FAILED) {
	// 	fputs("Cannot create shared memory\n", stderr);
	// 	exit(2);
	// }
	// memset(all_users, 0x00, sizeof(User) * (MAX_USER + 1));
	// char filename[PATH_MAX];
	// snprintf(filename, PATH_MAX, "%s/broadcast", MSG_BOX_DIR);
	// mkfifo(filename, 0777);
