#include <string>
#include <iostream>
#include <filesystem>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <vector>
#include "console_for_windows.cpp"
#include <stdlib.h>

#ifndef STDIN_FILENO
  #define STDIN_FILENO  0
  #define STDOUT_FILENO 1
  #define STDERR_FILENO 2
#endif



#ifndef __has_include
  static_assert(false, "__has_include not supported");
#else
#  if __cplusplus >= 201703L && __has_include(<filesystem>)
#    include <filesystem>
     namespace fs = std::filesystem;
#  elif __has_include(<experimental/filesystem>)
#    include <experimental/filesystem>
     namespace fs = std::experimental::filesystem;
#  elif __has_include(<boost/filesystem.hpp>)
#    include <boost/filesystem.hpp>
     namespace fs = boost::filesystem;
#  endif
#endif




#define ENV_LENGTH 250

using boost::asio::ip::tcp;
using namespace std;
namespace fs = std::filesystem;

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
    _putenv_s("REQUEST_METHOD",this->REQUEST_METHOD);
    _putenv_s("REQUEST_URI",this->REQUEST_URI);
    _putenv_s("QUERY_STRING",this->QUERY_STRING);
    _putenv_s("SERVER_PROTOCOL",this->SERVER_PROTOCOL);
    _putenv_s("HTTP_HOST",this->HTTP_HOST);
    _putenv_s("SERVER_ADDR",this->SERVER_ADDR);
    _putenv_s("SERVER_PORT",this->SERVER_PORT);
    _putenv_s("REMOTE_ADDR",this->REMOTE_ADDR);
    _putenv_s("REMOTE_PORT",this->REMOTE_PORT);
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



    void doConsole(){
        forWindows(&this->socket_);
    }

    void doPanel(){
        char wholeHtml[50000];
        char tem[10000];
        string test_cases_string="";
        vector<string> test_case;
        string path = ".\\test_case";
        for (const auto & entry : fs::directory_iterator(path)){
                printf("path = %s",entry.path().string().c_str() );
            string a = split(entry.path().string(),'\\')[2];
            test_case.push_back(a);
        }
        for(int i=0;i<test_case.size();i++){
            char temp[1000];
            sprintf(temp,"<option value=\"%s\">%s</option>",test_case[i].c_str(),test_case[i].c_str());
            test_cases_string = test_cases_string+temp;
        }
        printf("%s",test_cases_string.c_str());

        string hosts_string = "";
        for(int i=0;i<12;i++){
            char temp[1000];
            char _temp[20];
            sprintf(_temp,"nplinux%d",i+1);
            sprintf(temp,"<option value=\"%s.cs.nctu.edu.tw\">%s</option>",_temp,_temp);
            hosts_string = hosts_string + temp;
        }

        int N_SERVERS = MAXCLIENT;
//console.cgi?h0=nplinux1.cs.nctu.edu.tw&p0=2323&f0=.%2Ftest_case%2Ft2.txt&h1=&p1=&f1=&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=
//console.cgi?h0=nplinux1.cs.nctu.edu.tw&p0=2323&f0=.%2Ftest_case%2Ft1.txt&h1=&p1=&f1=&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=
//console.cgi?h0=nplinux1.cs.nctu.edu.tw&p0=2323&f0=t2.txt&h1=&p1=&f1=&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=

        sprintf(tem,"HTTP/1.1 200 OK\nContent-type: text/html\r\n\r\n");
        strcat(wholeHtml,tem);
        sprintf(tem,"<!DOCTYPE html>\
        <html lang=\"en\">\
        <head>\
            <title>NP Project 3 Panel</title>\
            <link\
            rel=\"stylesheet\"\
            href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\"\
            integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\"\
            crossorigin=\"anonymous\"\
            />\
            <link\
            href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\
            rel=\"stylesheet\"\
            />\
            <link\
            rel=\"icon\"\
            type=\"image/png\"\
            href=\"https://cdn4.iconfinder.com/data/icons/iconsimple-setting-time/512/dashboard-512.png\"\
            />\
            <style>\
            * {\
                font-family: 'Source Code Pro', monospace;\
            }\
            </style>\
        </head>\
        <body class=\"bg-secondary pt-5\">");
        strcat(wholeHtml,tem);
    sprintf(tem,"<form action=\"console.cgi\" method=\"GET\">\
        <table class=\"table mx-auto bg-light\" style=\"width: inherit\">\
            <thead class=\"thead-dark\">\
            <tr>\
                <th scope=\"col\">#</th>\
                <th scope=\"col\">Host</th>\
                <th scope=\"col\">Port</th>\
                <th scope=\"col\">Input File</th>\
            </tr>\
            </thead>\
            <tbody>");
            strcat(wholeHtml,tem);


//http://nplinux1.cs.nctu.edu.tw:5555/console.cgi?h0=nplinux1.cs.nctu.edu.tw&p0=2323&f0=.%2Ftest_case%2Ft1.txt&h1=&p1=&f1=&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=

    for(int i=0;i<N_SERVERS;i++){
        
        sprintf(tem,"<tr>\
                <th scope=\"row\" class=\"align-middle\">Session %d</th>\
                <td>\
                <div class=\"input-group\">\
                    <select name=\"h%d\" class=\"custom-select\">\
                    <option></option>%s\
                    </select>\
                    <div class=\"input-group-append\">\
                    <span class=\"input-group-text\">.cs.nctu.edu.tw</span>\
                    </div>\
                </div>\
                </td>\
                <td>\
                <input name=\"p%d\" type=\"text\" class=\"form-control\" size=\"5\" />\
                </td>\
                <td>\
                <select name=\"f%d\" class=\"custom-select\">\
                    <option></option>%s\
                </select>\
                </td>\
            </tr>",i+1,i,hosts_string.c_str(),i,i,test_cases_string.c_str());
            strcat(wholeHtml,tem);
    }
    sprintf(tem,"<tr>\
            <td colspan=\"3\"></td>\
            <td>\
              <button type=\"submit\" class=\"btn btn-info btn-block\">Run</button>\
            </td>\
                    </tr>\
                    </tbody>\
                </table>\
                </form>\
            </body>\
            </html>");
            strcat(wholeHtml,tem);
            //printf("\n========================\n%s",wholeHtml);
        boost::asio::write(this->socket_,boost::asio::buffer(wholeHtml, strlen(wholeHtml)));
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
                //io_context.notify_fork(boost::asio::io_context::fork_prepare);
                // This is the child process.
                //io_context.notify_fork(boost::asio::io_context::fork_child);
                processHeader();
                setEnvs();
                printEnvs();
                char* argv[3];
                argv[0] = getenv("REQUEST_URI");
                argv[1] = getenv("REQUEST_URI");
                argv[2]=NULL;

                int childsock = socket_.native_handle();
                printf("URI = %s\n",argv[0]);
      
                
                if(strcmp(argv[0] , "./console.cgi") == 0){
                    doConsole();
                    socket_.close();
                }
                else if(strcmp(argv[0],"./panel.cgi")==0){
                    doPanel();
                }
                //do_write(strlen(data_));
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
