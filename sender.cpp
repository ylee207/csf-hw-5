#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include "csapp.h"
#include "message.h"
#include "connection.h"
#include "client_util.h"

int main(int argc, char **argv) {
  if (argc != 4) {
    std::cerr << "Usage: ./sender [server_address] [port] [username]\n";
    return 1;
  }

  std::string server_hostname;
  int server_port;
  std::string username;

  server_hostname = argv[1];
  server_port = std::stoi(argv[2]);
  username = argv[3];

  // TODO: connect to server
  Connection conn;
  conn.connect(server_hostname, server_port);
  if (!conn.is_open()) {
    std::cerr << "Couldn't connect to server.";
    exit(1);
  }


  Message slogin_msg(TAG_SLOGIN, username);
  bool send_status = conn.send(slogin_msg);
  if (!send_status) {
    std::cerr << "Send rlogin not successful.";
    exit(1);
  }

  Message recived_msg;
  bool recived_status = conn.receive(recived_msg);
  if (!recived_status) {
    std::cerr << "Recived rlogin not successful.";
    exit(1);
  }
  if (recived_msg.tag == TAG_ERR) {
    std::string err_msg = recived_msg.data;
    std::cerr << err_msg.c_str();
    exit(1);
  }

  // TODO: send slogin message

  // TODO: loop reading commands from user, sending messages to
  //       server as appropriate

  return 0;
}
