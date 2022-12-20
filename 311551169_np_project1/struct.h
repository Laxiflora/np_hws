#include <string>
#define SHELL_INPUT 15001
#define COMMAND_LEN 257

struct QueueNode{
    int round;
    int inputfd;
    int outputfd;
    QueueNode();
};

QueueNode::QueueNode(){
    round = -1;
    inputfd=-1;
    outputfd = -1;
}

// fd[0] = pipeOut
// fd[1] = pipeIn

