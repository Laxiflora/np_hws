#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <vector>
#include <fstream>
#include <stdlib.h>
#include <regex>
#define ENV_LENGTH 250
#define MAX_CONNECTIONS 20

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


    void do_read(){
      auto self(shared_from_this());
      socket_.async_read_some(boost::asio::buffer(data_, max_length),
          [this, self](boost::system::error_code ec, size_t length){
            if (!ec){
                io_context.notify_fork(boost::asio::io_context::fork_prepare);
                if (fork() == 0){
                  // This is the child process.
                  io_context.notify_fork(boost::asio::io_context::fork_child);
                  processHeader(length);
                  checkFireWall();
                  printHeader();
                  if(command == "Connect"){
                    makeConnectReply();
                  }
                  else{
                    makeBindReply();
                  }
                  io_context.run();
                  exit(0);
                }
                else{
                  // This is the parent process.
                  io_context.notify_fork(boost::asio::io_context::fork_parent);
                  socket_.close();
                }
            }
          });
  }
//curl -x socks4://nplinux1.cs.nctu.edu.tw:8988 http://98.139.183.24


void onRead_Web(){
  (*dest_socket).async_read_some(boost::asio::buffer(message_),
  [this](boost::system::error_code ec, std::size_t length){
    if(ec){
      if(ec == boost::asio::error::eof){
        return;
      }
    }
    onWrite_Client(length);
  }
  );
}

void onWrite_Client(int length){
  boost::asio::async_write(socket_, boost::asio::buffer(message_,length),
  [this](boost::system::error_code ec, std::size_t){
    onRead_Web();
  }
  );
}


void onRead_Client(){
  socket_.async_read_some(boost::asio::buffer(data_),
  [this](boost::system::error_code ec, std::size_t length){
    if(ec){
      if(ec == boost::asio::error::eof){
        return;
      }
    }
    onWrite_Web(length);
  }
  );
}


void onWrite_Web(int length){
  boost::asio::async_write(*dest_socket, boost::asio::buffer(data_,length),
  [this](boost::system::error_code ec, std::size_t){
    onRead_Client();
  }
  );
}


void makeBindReply(){
    char rep[8];
    rep[0] = 0x00;
    if(this->reply == "Reject"){
      rep[1] = 0x5b; //91
    }
    else if(this->reply == "Accept"){
      rep[1] = 0x5a;
    }

	int tem;
	tcp::endpoint toBind(boost::asio::ip::address::from_string("0.0.0.0"), 0);

    dest_acceptor_ = new tcp::acceptor(io_context);

    (*dest_acceptor_).open(tcp::v4());
    (*dest_acceptor_).set_option(tcp::acceptor::reuse_address(true));
    (*dest_acceptor_).bind(toBind);
    (*dest_acceptor_).listen(boost::asio::socket_base::max_connections);

    tem = (*dest_acceptor_).local_endpoint().port() / 256;
    rep[2] = (tem > 128 ? tem - 256 : tem);
    tem = (*dest_acceptor_).local_endpoint().port() % 256;
    rep[3] = (tem > 128 ? tem - 256 : tem);
	  for (int i = 4; i < 8; ++i) rep[i] = 0;

	boost::asio::async_write(socket_, boost::asio::buffer(rep, 8),
		[this,rep](boost::system::error_code ec, std::size_t){
			if (ec) return;
      if(reply=="Reject"){
        delete dest_acceptor_;
        return;
      }
      dest_socket = new tcp::socket(io_context);
			(*dest_acceptor_).accept(*dest_socket);
			boost::asio::write(socket_, boost::asio::buffer(rep, 8));  // resend to reply to client
			onRead_Web();		//send everything web wants to send in this turn
			onRead_Client();  // send everything client wants to respond in this turn (thus will )
		});
}

// Reply:
// VN	CD	DSTPORT	DSTIP
// 1	1	  2	      4
  void makeConnectReply(){
    char rep[8];
    rep[0] = 0x00;
    if(this->reply == "Reject"){
      rep[1] = 0x5b; //91
    }
    else if(this->reply == "Accept"){
      rep[1] = 0x5a;
    }
    for (int i = 2; i < 8; i++) rep[i] = data_[i];
    
	boost::asio::async_write(socket_, boost::asio::buffer(rep, 8),
		[this](boost::system::error_code ec, std::size_t){
			if (ec) {
        printf("ERROR\n");
        return;
      }
      if(reply=="Reject") return;
			dest_socket = new tcp::socket(io_context);
			tcp::endpoint endpoint;
			endpoint = tcp::endpoint(boost::asio::ip::address::from_string(D_ADDR), atoi(D_PORT));
			(*dest_socket).connect(endpoint);
			onRead_Web();		//send everything web wants to send in this turn
			onRead_Client();  // send everything client wants to respond in this turn (thus will )
		});
  }


  int checkFireWall(){
    ifstream fout;
	  fout.open("socks.conf");
    if(!fout){
      printf("Firewall config not found!\n");
      return -1;
    }

    string entry;
    while(getline(fout, entry)){
      vector<string> slice = split(entry,' ');
      if( (CD == 1 && slice[1] == "c") || (CD == 2 && slice[1] == "b")){  //we assume all entries are whitelist a.k.a permit
        command = CD==1?"Connect":"Bind";
        string permitIP = slice[2];
        vector<string> segment = split(permitIP,'.');
        string reg = "";
        for(int i=0;i<(int)segment.size();i++){
          if(segment[i]=="*"){
            reg = reg + "[0-9]+";
            if(i+1<(int)segment.size()){
              reg = reg + ".";
            }
          }
          else{
            reg = reg + segment[i];
            if(i+1<(int)segment.size()){
              reg = reg + ".";
            }
          }
        }
        regex reg_(reg);
        if(regex_match(D_ADDR, reg_)){
          fout.close();
          reply = "Accept";
          return 1;
        }
      }
    }

    fout.close();
    reply = "Reject";
    return 0;
  }

  void processHeader(int length){
    strcpy(S_ADDR, socket_.remote_endpoint().address().to_string().c_str());
    sprintf(S_PORT, "%u", socket_.remote_endpoint().port());

    // for(int i=0;i<length;i++){
    //   cout<<(int)data_[i]<<" ";
    // }
    // cout<<"\n";

    int sockVersion=0;
    if(data_[0]<0){
      sockVersion = (int)data_[0] +256;
    }
    else{
      sockVersion = (int)data_[0];
    }
    if(sockVersion != 4){   //wrong version
      //printf("Version wrong! version = %d\n",sockVersion);
      reply = "Reject";
      return;
    }

    if(data_[1]<0){
      CD = (int)data_[1] +256;
    }
    else{
      CD = (int)data_[1];
    }

    int d_port = 0;
    if(data_[2]<0){// need to switch to positive
      d_port += (((int)data_[2]+256) << 8);
    }
    else{
      d_port += ((int)data_[2] << 8);
    }
    if(data_[3]<0){
      d_port += (((int)data_[3]+256)%256 );
    }
    else{
      d_port +=((int)data_[3]);
    }
    sprintf(D_PORT,"%d",d_port);


    string d_ip = "";
    for(int i =4;i<8;i++){
      if(i!=4){
        d_ip = d_ip+".";
      }
      int temp=0;
      if(data_[i]<0){
        temp = (int)data_[i]+256;
      }
      else{
        temp = (int)data_[i];
      }
      d_ip = d_ip+ to_string(temp);
    }
    strcpy(D_ADDR,d_ip.c_str());

    //dport = dport+ (data_[2])
    // 443 = (1BB)hex
    // (1BB)hex = 1 in data[2], ffffffbb in data[3];
    int isSOCK4a=1;
    int currentIndex = 8;
    if(!data_[currentIndex]){
      //no user name exist!
      if(currentIndex+1 != length){ //isSOCK4a
        currentIndex++;
        for(int i=currentIndex,j=0;i<length;i++,j++){
          HOST_NAME[j] = data_[i];
          if(!data_[i+1]){
            HOST_NAME[j+1] = '\0';
            break;
          }
        }
      }
    }
    else{
      for(int j=0;data_[currentIndex];currentIndex++,j++){
        USER_ID[j] = data_[currentIndex];
        if(!data_[currentIndex+1]){
          USER_ID[j+1] = '\0';
          if(currentIndex+2 == length){ // is SOCK4
            isSOCK4a = 0;
          }
          currentIndex++;
        }
      }
      if(isSOCK4a){
        for(int i=currentIndex,j=0;i<length;i++,j++){
          HOST_NAME[j] = data_[i];
          if(!data_[i+1]){
            HOST_NAME[j+1] = '\0';
            break;
          }
        }
      }
    }
    if(D_ADDR[0] == '0'){ // undefined ip, needs to resolve
      resolveHostName();
    }
  }

  void resolveHostName(){
    //curl --socks4a nplinux1.cs.nctu.edu.tw:5678 https://www.google.com/
    boost::asio::ip::tcp::resolver resolver(io_context);
    boost::asio::ip::tcp::resolver::query query(HOST_NAME, D_PORT);
    boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);
    tcp::resolver::iterator end;
    boost::system::error_code error = boost::asio::error::host_not_found;
    while(iter!=end && error){ //how to use for each...
      if(iter->endpoint().address().is_v4()){
        strcpy(D_ADDR,iter->endpoint().address().to_v4().to_string().c_str());
        sprintf(D_PORT,"%hu",iter->endpoint().port());
        break;
      }
      iter++;
    }
    return;
  }

  void printHeader(){
    printf("<S_IP>: %s\n\
    <S_PORT>: %s\n\
    <D_IP>: %s\n\
    <D_PORT>: %s\n\
    <Command>: %s\n\
    <Reply>: %s\n\
    <HOST_NAME>: %s\n\
    ", S_ADDR,S_PORT,D_ADDR,D_PORT,command.c_str(),reply.c_str(),HOST_NAME);
  }


  tcp::socket socket_;
  enum { max_length = 1024 };
  unsigned char data_[max_length];
  unsigned char message_[max_length];
  private:
      char HOST_NAME[ENV_LENGTH]={'\0'};
      char USER_ID[ENV_LENGTH]={'\0'};
      char D_ADDR[ENV_LENGTH]={'\0'};
      tcp::socket* dest_socket;
      char D_PORT[ENV_LENGTH]={'\0'};
      char S_ADDR[ENV_LENGTH]={'\0'};
      char S_PORT[ENV_LENGTH]={'\0'};
      string command = "";
      string reply = "";
      int CD;
      tcp::acceptor* dest_acceptor_;

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
//curl --proxy socks4a://nplinux1.cs.nctu.edu.tw:8988 https://www.google.com/
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