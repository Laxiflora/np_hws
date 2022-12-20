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


boost::asio::io_context io_context;
string proxyHostName="";
string proxyPort="";

void printToHtml(string data,int session,const char* type){
  char out[10000];
  if( strcmp(type,"command") == 0){
    sprintf(out,"<script>document.getElementById('s%d').innerHTML += '<b>%s</b>';</script>",session,data.c_str());
  }
  else{
    sprintf(out,"<script>document.getElementById('s%d').innerHTML += '%s';</script>",session,data.c_str());
  }
  printf("%s",out);
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

class Client {
    public:
    Client(boost::asio::io_context& ioc,
            const string& host, const string& port, const string& filePath)
        : socket_(ioc), resolver_(ioc) {
          exit = 0;
          if(proxyHostName != "" && proxyPort != ""){
            unsigned char reply[10+host.length()];
            reply[0] = 0x04;
            reply[1] = 0x01;
            reply[2] = stoi(port)/256;
            reply[3] = stoi(port)%256;
            reply[4] = 0x00;
            reply[5] = 0x00;
            reply[6] = 0x00;
            reply[7] = 0x01;
            reply[8] = 0;
            for(int i=9,j=0;j<(int)host.length();i++,j++){
              reply[i] = host[j];
            }
            reply[9+host.length()]=0;

            auto endpoints = resolver_.resolve(tcp::v4(), proxyHostName, proxyPort);
            connect(socket_,endpoints);
            boost::asio::write(socket_,boost::asio::buffer(reply,10+host.length()));
          }
          else{
            auto endpoints = resolver_.resolve(tcp::v4(), host, port);
            connect(socket_,endpoints);
          }
          if( (file = fopen(filePath.c_str(),"r")) == NULL){
              perror("open failed");
          }
          exit=0;
    }
    string host;
    string port;
    int session;

    void readData(){
        //std::cout << "Reply is: ";
        memset(cin_buf_, 0, 20480);
        socket_.async_read_some(boost::asio::buffer(cin_buf_,20480),
            [this](boost::system::error_code ec, std::size_t len){

            string data_ = cin_buf_;
            int index;
            data_ = htmlEscape(data_);
            //cout<<"<p style=\"color:red\">WAS HERE.</p> ";
            printToHtml(data_,this->session,"shell");
            //cout<<data_;
            if(data_.find("%") == string::npos){
                memset(cin_buf_,0,20480);
                readData();
            }
            else if(exit ==1){
                return;
            }
            else{
                sendClientData();
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
          if(strcmp(buffer,"exit")==0){
              exit = 1;
          }
          string temp_(buffer);
          temp_ = htmlEscape(temp_);
          printToHtml(temp_,this->session,"command");
          // printf("%s",cin_buf_);
          // printf("<br>");
          boost::asio::async_write(socket_, boost::asio::buffer(buffer, len),
              [this](boost::system::error_code ec, std::size_t len){
                  if(len!=0){
                      readData();
                  }
                  else{
                      return;
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

//http://nplinux1.cs.nctu.edu.tw/~ycliu0113/hw4.cgi?h0=nplinux1.cs.nctu.edu.tw&p0=2323&f0=t1.txt & h1= &p1=&f1=&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=
// & sh=nplinux1.cs.nctu.edu.tw&sp=2328
vector<Client> cliTable;
void registerTable(string query){
    vector<string> token = split(query,'&');
    if(split(token[token.size()-1],'=').size() != 1 ){  //has proxy server
      proxyHostName = split(token[token.size()-2],'=')[1];
      proxyPort = split(token[token.size()-1],'=')[1];
    }
    for(int j=0;j<token.size()-2;j+=3){
      vector<string> sli = split(token[j],'=');
      if(sli.size()==1){
          continue;
      }
      else{
        int index = max(0,j/3);
        //Client temp(io_context,split(token[j],'=')[1],split(token[j+1],'=')[1],split(token[j+2],'=')[1]);
        char path[100];
        sprintf(path,"./test_case/%s",split(token[j+2],'=')[1].c_str());

        // dest_socket = new tcp::socket(io_context);
        // tcp::endpoint endpoint;
        // endpoint = tcp::endpoint(boost::asio::ip::address::from_string(D_ADDR), atoi(D_PORT));
        // (*dest_socket).connect(endpoint);
        // onRead_Web();		//send everything web wants to send in this turn
        // onRead_Client();  // send everything client wants to respond in this turn (thus will )



        cliTable.push_back( Client(io_context,split(token[j],'=')[1],split(token[j+1],'=')[1],path));
        cliTable[index].host = split(token[j],'=')[1];
        cliTable[index].port = split(token[j+1],'=')[1];
        cliTable[index].session = index;
      }
    }
}


void runConsole(){
    for(int i=0;i<cliTable.size();i++){
        cliTable[i].readData();  //因為會先送welcome message
    }
}


void printInitHtml(){
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

    printf("<!DOCTYPE html>\
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
}


int main(int argc, char* argv[]){
    setbuf(stdout, NULL);
    printf("HTTP/1.1 200 OK\n");
    cout << "Content-type:text/html\r\n\r\n<h1>Debug site</h1>";
    string query = getenv("QUERY_STRING");
    registerTable(query); // sockets connected, cliTable initialized.
    printInitHtml();
    runConsole();
    //printTable();
    io_context.run();

    return 0;
}