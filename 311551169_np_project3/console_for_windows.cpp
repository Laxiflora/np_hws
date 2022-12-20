#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <vector>
#ifndef ENV_LENGTH
    #define ENV_LENGTH 250
#endif
#define MAXCLIENT 5
#define BUF_SIZE 20480
using namespace std;
using boost::asio::ip::tcp;

boost::asio::io_context io_context;   //already defied in console.cgi

tcp::socket* sock;

void printToHtml(string data,int session,const char* type){
  char out[10000];
  if( strcmp(type,"command") == 0){
    sprintf(out,"<script>document.getElementById('s%d').innerHTML += '<b>%s</b>';</script>",session,data.c_str());
  }
  else{
    sprintf(out,"<script>document.getElementById('s%d').innerHTML += '%s';</script>",session,data.c_str());
  }
  boost::asio::write((*sock),boost::asio::buffer(out, strlen(out)));
}


void replace_all(string& str, const string& old, const string& repl) {
    size_t pos = 0;
    while ((pos = str.find(old, pos)) != string::npos) {
        str.replace(pos, old.length(), repl);
        pos += repl.length();
    }
}

string htmlEscape(string str) {
    replace_all(str, string("&"), string("&amp;"));
    replace_all(str, string("'"), string("&apos;"));
    replace_all(str, string("\""), string("&quot;"));
    replace_all(str, string(">"), ("&gt;"));
    replace_all(str, ("<"), ("&lt;"));
    replace_all(str,string("\r\n"),string("<br>"));
    replace_all(str,string("\n"),string("<br>"));

    return str;
}
class Client;
vector<Client> cliTable;
class Client {
    public:
    Client(boost::asio::io_context& ioc,
            const string& host, const string& port, const string& filePath)
        : socket_(ioc), resolver_(ioc) {
            if( (file = fopen(filePath.c_str(),"r")) == NULL){
                perror("open failed");
            }
            exit=0;
            auto endpoints = resolver_.resolve(tcp::v4(), host, port);
            connect(socket_,endpoints);
    }
    string host;
    string port;
    int session=0;
    int outputTag = 0;

    void readData(){
        //std::cout << "Reply is: ";
        memset(cin_buf_, 0, 20480);
        cliTable[this->session].socket_.async_read_some(boost::asio::buffer(cin_buf_,20480),
            [this](boost::system::error_code ec, std::size_t len){
            if(!ec){
              string data_ = cin_buf_;
              data_ = htmlEscape(data_);
              //cout<<"<p style=\"color:red\">WAS HERE.</p> ";
              printToHtml(data_,this->outputTag,"shell");
              //cout<<data_;
              if(data_.find("%") == string::npos){
                  memset(cin_buf_,0,20480);
                  readData();
              }
              else{
                  sendClientData();
              }
            }
            else{
              printf("%s\n",ec.message().c_str());
              printf("socket normally closed.\n");
              printf("client %d leave\n",this->session);
              cliTable[this->session].socket_.close();
              int leaveIndex = this->session;
              // cliTable.erase(cliTable.begin() + leaveIndex);
              // for(int i=leaveIndex;i<cliTable.size();i++){
              //   cliTable[i].session-=1;
              // }
              for(int i=0;i<cliTable.size();i++){
                printf("cli[i].session = %d, origin pos = %d\n",cliTable[i].session,cliTable[i].outputTag);
              }
              return;
            }
        });
    }


    private:
        FILE* file;
        tcp::socket socket_;
        tcp::resolver resolver_;
        int exit;
        char cin_buf_[BUF_SIZE];
        std::array<char, BUF_SIZE> buf_;


        void sendClientData() {
          std::size_t len = 0;
          char buffer[20480]= {'\0'};
          //std::cout << "Enter message: ";
          fgets(buffer, BUF_SIZE,this->file);
          len = strlen(buffer);
          printf("buffer = %s\n=============",buffer);
          string temp_(buffer);
          string compare_ = temp_;
          if(compare_.find("\r\n")!=-1){
              compare_[compare_.find("\r\n")]='\0';
          }
          if(compare_.find("\n")!=-1){
              compare_[compare_.find("\n")]='\0';
          }
          if(compare_.find("\r")!=-1){
              compare_[compare_.find("\r")]='\0';
          }
          printf("temp = |%s|",temp_.c_str());
          if(strcmp(compare_.c_str(),"exit")==0){
            printf("Enter the exit\n");
            exit = 1;
          }
          temp_ = htmlEscape(temp_);
          printToHtml(temp_,this->outputTag,"command");
          boost::asio::async_write(cliTable[this->session].socket_, boost::asio::buffer(buffer, len),
              [this](boost::system::error_code ec, std::size_t len){
                if(!ec){
                  if(exit ==1){
                    printf("I am closing socket\n");
                    cliTable[this->session].socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
                  }
                  if(len!=0){
                      readData();
                  }
                  else{
                      return;
                  }
                }
                else{
                  printf("write error(which should not orrer in %d, origin pos = %d: %s\n",this->session,this->outputTag,ec.message().c_str());
                }
              });
        }

};


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

// h0=nplinux1.cs.nctu.edu.tw & p0=1234&f0=t1.txt & h1=nplinux2.cs.nctu.edu.tw &
// p1=5678 & f1=t2.txt & h2= & p2= & f2= & h3= & p3= & f3= & h4= & p4= & f4=




// h0=nplinux1.cs.nctu.edu.tw & p0=2323 & f0=t1.txt & h1= & p1= & f1=&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=open failed: Invalid argument Exception: connect: Connection refused
void registerTable(string query){
    vector<string> token = split(query,'&');
    for(int j=0;j<token.size();j+=3){
    if(split(token[j],'=').size()==1){
        break;
    }
      int index = max(0,j/3);
      //Client temp(io_context,split(token[j],'=')[1],split(token[j+1],'=')[1],split(token[j+2],'=')[1]);
      char path[100];
      sprintf(path,"./test_case/%s",split(token[j+2],'=')[1].c_str());
      cliTable.push_back( Client(io_context,split(token[j],'=')[1],split(token[j+1],'=')[1],path));
      cliTable[index].host = split(token[j],'=')[1];
      cliTable[index].port = split(token[j+1],'=')[1];
      cliTable[index].session = index;
      cliTable[index].outputTag = index;
    }
}


void runConsole(){
    for(int i=0;i<cliTable.size();i++){
      printf("start read data\n");
        cliTable[i].readData();  //因為會先送welcome message
    }
}


void printHtmlBody(){
    string headHtml="";
    for(int i=0;i<cliTable.size();i++){
      char temp[100] = {'\0'};
      sprintf(temp,"<th scope=\"col\">%s:%s</th>",cliTable[i].host.c_str(),cliTable[i].port.c_str());
      headHtml +=temp;
    }
    headHtml += "</tr> </thead> <tbody> <tr>";
    for(int i=0;i<cliTable.size();i++){
      char temp[100] = {'\0'};
      sprintf(temp,"<td><pre id=\"s%d\" class=\"mb-0\"></pre></td>",i);
      headHtml +=temp;
    }
    char tem[10000];
    sprintf(tem,"HTTP/1.1 200 OK\n\
    Content-type:text/html\r\n\r\n\
    <!DOCTYPE html>\
<html lang=\"en\">\
  <head>\
    <meta charset=\"UTF-8\" />\
    <title>NP Project 3 Sample Console</title>\
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
      href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\
    />\
    <style>\
      * {\
        font-family: 'Source Code Pro', monospace;\
        font-size: 1rem !important;\
      }\
      body {\
        background-color: #212529;\
      }\
      pre {\
        color: #cccccc;\
      }\
      b {\
        color: #01b468;\
      }\
    </style>\
  </head>\
  <body>\
    <table class=\"table table-dark table-bordered\">\
      <thead>\
        <tr>\
        %s\
        </tr>\
      </tbody>\
    </table>\
  </body>\
</html>",headHtml.c_str());
boost::asio::write((*sock),boost::asio::buffer(tem, strlen(tem)));
}

// h0=nplinux1.cs.nctu.edu.tw&p0=2323&f0=t1.txt&h1=&p1=&f1=&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=open failed: Invalid argument Exception: connect: Connection refused

void forWindows(tcp::socket* socket_){
    cliTable.clear();
    sock = socket_;
    string query = getenv("QUERY_STRING");
    registerTable(query); // sockets connected, cliTable initialized.
                  for(int i=0;i<cliTable.size();i++){
                printf("cli[i].session = %d, origin pos = %d\n",cliTable[i].session,cliTable[i].outputTag);
              }
    printHtmlBody();
    printf("\nClient count = %d",cliTable.size());
    runConsole();
    //printTable();
    io_context.run();
    printf("\nLeaving\n");
    return;
}
