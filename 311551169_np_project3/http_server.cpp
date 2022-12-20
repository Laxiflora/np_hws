#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <vector>
#include <stdlib.h>
#define ENV_LENGTH 250

using boost::asio::ip::tcp;
using namespace std;

boost::asio::io_context io_context;

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



class session: public enable_shared_from_this<session>{
  public:
    session(tcp::socket socket): socket_(move(socket)){}
    void start(){
      do_read();
    }

  private:

    char REQUEST_METHOD[ENV_LENGTH]={'\0'};
    char REQUEST_URI[ENV_LENGTH]={'\0'};
    char QUERY_STRING[ENV_LENGTH]={'\0'};
    char SERVER_PROTOCOL[ENV_LENGTH]={'\0'};
    char HTTP_HOST[ENV_LENGTH]={'\0'};
    char SERVER_ADDR[ENV_LENGTH]={'\0'};
    char SERVER_PORT[ENV_LENGTH]={'\0'};
    char REMOTE_ADDR[ENV_LENGTH]={'\0'};
    char REMOTE_PORT[ENV_LENGTH]={'\0'};
// GET /aaa.cgi HTTP/1.1
// Host: nplinux1.cs.nctu.edu.tw:8899
// Connection: keep-alive
// Cache-Control: max-age=0
// Upgrade-Insecure-Requests: 1
// User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/107.0.0.0 Safari/537.36 Edg/107.0.1418.56
// Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9
// Accept-Encoding: gzip, deflate
// Accept-Language: zh-TW,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6
// Cookie: _ga=GA1.3.1338010182.1648486440

  void setEnvs(){
    setenv("REQUEST_METHOD",this->REQUEST_METHOD,1);
    setenv("REQUEST_URI",this->REQUEST_URI,1);
    setenv("QUERY_STRING",this->QUERY_STRING,1);
    setenv("SERVER_PROTOCOL",this->SERVER_PROTOCOL,1);
    setenv("HTTP_HOST",this->HTTP_HOST,1);
    setenv("SERVER_ADDR",this->SERVER_ADDR,1);
    setenv("SERVER_PORT",this->SERVER_PORT,1);
    setenv("REMOTE_ADDR",this->REMOTE_ADDR,1);
    setenv("REMOTE_PORT",this->REMOTE_PORT,1);
  }


  void printEnvs(){
    printf("========================================\n");
    printf("REQUEST_METHOD %s\n",getenv( "REQUEST_METHOD" ) );
    printf("REQUEST_URI %s\n",getenv("REQUEST_URI"));
    printf("QUERY_STRING %s\n",getenv("QUERY_STRING"));
    printf("SERVER_PROTOCOL %s\n",getenv("SERVER_PROTOCOL"));
    printf("HTTP_HOST %s\n",getenv("HTTP_HOST"));
    printf("SERVER_ADDR %s\n",getenv("SERVER_ADDR"));
    printf("SERVER_PORT %s\n",getenv("SERVER_PORT"));
    printf("REMOTE_ADDR %s\n",getenv("REMOTE_ADDR"));
    printf("REMOTE_PORT %s\n",getenv("REMOTE_PORT"));
    printf("=======================================\n");
  }


    void processHeader(){
    char noop[ENV_LENGTH];
    char temp[ENV_LENGTH];
    // 'GET' '/aaa.cgi?i=1' 'HTTP/1.1' \r\n 'Host:' 'nplinux1.cs.nctu.edu.tw:8899'
    printf("data = %s\n",data_);
    sscanf(data_, "%s %s %s %s %s", REQUEST_METHOD, temp, SERVER_PROTOCOL, noop, HTTP_HOST);
    string slide = temp;
    vector<string> _slide = split(slide,'?');
    if(_slide.size()>1){
        strcpy(QUERY_STRING,_slide[1].c_str());
    }
    strcpy(SERVER_ADDR, socket_.local_endpoint().address().to_string().c_str());
    sprintf(SERVER_PORT, "%u", socket_.local_endpoint().port());
    sprintf(REQUEST_URI,"./%s",_slide[0].erase(0,1).c_str());
    //strcpy(REQUEST_URI,_slide[0].erase(0,1).c_str());
    strcpy(REMOTE_ADDR, socket_.remote_endpoint().address().to_string().c_str());
    sprintf(REMOTE_PORT, "%u", socket_.remote_endpoint().port());
    }

    void do_read(){
      auto self(shared_from_this());
      socket_.async_read_some(boost::asio::buffer(data_, max_length),
          [this, self](boost::system::error_code ec, size_t length){
            if (!ec){
                io_context.notify_fork(boost::asio::io_context::fork_prepare);
                if (fork() == 0){
                // This is the child process.
                io_context.notify_fork(boost::asio::io_context::fork_child);
                processHeader();
                setEnvs();
                printEnvs();
                char* argv[3];
                argv[0] = getenv("REQUEST_URI");
                argv[1] = getenv("REQUEST_URI");
                argv[2]=NULL;

                int childsock = socket_.native_handle();
                printf("URI = %s\n",argv[0]);
                dup2(childsock, STDERR_FILENO);
                dup2(childsock, STDIN_FILENO);
                dup2(childsock, STDOUT_FILENO);

                printf("HTTP/1.1 200 OK\n");
                if(execvp(argv[0],argv) <0){
                    cout << "Content-type:text/html\r\n\r\n<h1>Debug site</h1>";
                    perror("exec failed: ");
                }
                exit(0);
                }
                else{
                // This is the parent process.
                sleep(1);
                io_context.notify_fork(boost::asio::io_context::fork_parent);
                socket_.close();
                }
            }
          });
  }

  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
};

class server{
public:
  server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)){
    do_accept();
  }

private:
  void do_accept(){
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket){
          if (!ec){
            make_shared<session>(move(socket))->start();
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
};

int main(int argc, char* argv[]){
  setbuf(stdout, NULL);
  try{
    if (argc != 2){
      cerr << "Usage: async_tcp_echo_server <port>\n";
      return 1;
    }

    //boost::asio::io_context io_context;

    server s(io_context, atoi(argv[1]));

    io_context.run();
  }
  catch (exception& e){
    cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

// GET / HTTP/1.1
// Host: nplinux1.cs.nctu.edu.tw:8899
// Connection: keep-alive
// Cache-Control: max-age=0
// Upgrade-Insecure-Requests: 1
// User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/107.0.0.0 Safari/537.36 Edg/107.0.1418.56
// Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9
// Accept-Encoding: gzip, deflate
// Accept-Language: zh-TW,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6
// Cookie: _ga=GA1.3.1338010182.1648486440


// GET /aaa.cgi HTTP/1.1
// Host: nplinux1.cs.nctu.edu.tw:8899
// Connection: keep-alive
// Cache-Control: max-age=0
// Upgrade-Insecure-Requests: 1
// User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/107.0.0.0 Safari/537.36 Edg/107.0.1418.56
// Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9
// Accept-Encoding: gzip, deflate
// Accept-Language: zh-TW,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6
// Cookie: _ga=GA1.3.1338010182.1648486440