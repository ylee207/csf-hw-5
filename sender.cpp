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

  // connect to server
  Connection conn;
  conn.connect(server_hostname, server_port);
  if (!conn.is_open()) {
    std::cerr << "Could not connect to server\n";
    exit(1);
  }

  // send slogin message
  Message slogin_msg(TAG_SLOGIN, username);
  bool send_status = conn.send(slogin_msg);
  if (!send_status) {
    std::cerr << "Send slogin not successful.\n";
    exit(1);
  }

  // handle error case when receiving slogin is not successful.
  Message received_msg;
  bool recived_status = conn.receive(received_msg);
  if (!recived_status) {
    std::cerr << "Received slogin not successful.\n";
    exit(1);
  }
  if (received_msg.tag == TAG_ERR) {
    std::string err_msg = received_msg.data;
    std::cerr << err_msg.c_str();
    exit(1);
  }


  // loop reading commands from user, sending messages to server as appropriate
  while (1) {
    bool send_status;
    bool receive_status;
    std::string user_command;
    std::getline(std::cin, user_command);

    // join:[room]
    std::string command_type = "/join ";
    size_t pos = user_command.find(command_type);
    // if '/join' is in the user_command
    if (pos != std::string::npos) {
      // extract the [room name] which starts right after the command_type with a space
      std::string room_name = user_command.substr(pos + command_type.length());
      // create the message with the extracted room name
      Message msg_to_server(TAG_JOIN, room_name);
      // check the status of sending to the server
      send_status = conn.send(msg_to_server);
      if (!send_status) {
        std::cerr << "Sending join not successful.\n";
        exit(1);
      }
      Message join_receive_response;
      receive_status = conn.receive(join_receive_response);
      if (!receive_status) {
        std::cerr << "Receiving join not successful.\n";
        exit(1);
      }
      if (join_receive_response.tag == TAG_ERR) {
        std::string err_msg = join_receive_response.data;
        std::cerr << err_msg.c_str();
      }
      continue;
    }


    // leave:
    command_type = "/leave";
    pos = user_command.find(command_type);
    //if '/leave' is found
    if (pos != std::string::npos) {
      Message msg_leave(TAG_LEAVE, "");
      send_status = conn.send(msg_leave);
      if (!send_status) {
        std::cerr << "Sending leave not successful.\n";
        exit(1);
      }
      Message leave_response;
      receive_status = conn.receive(leave_response);
      if (!receive_status) {
        std::cerr << "Receiving after leave not successful.\n";
        exit(1);
      }
      if (leave_response.tag == TAG_ERR) {
        std::string err_msg = leave_response.data;
        std::cerr << err_msg.c_str() << "\n";
      }
      continue;
    }


    // quit:
    command_type = "/quit";
    pos = user_command.find(command_type);
    // if '/quit' is found
    if (pos != std::string::npos) {
      Message msg_quit(TAG_QUIT, "");
      send_status = conn.send(msg_quit);
      if (!send_status) {
        std::cerr << "Sending quit not successful.\n";
        exit(1);
      }
      Message quit_response;
      receive_status = conn.receive(quit_response);
      if (!receive_status) {
        std::cerr << "Receiving after quit not successful.\n";
        exit(1);
      }
      if (quit_response.tag == TAG_ERR) {
        std::string err_msg = quit_response.data;
        std::cerr << err_msg.c_str() << "\n";
      }
      break;
    }

    // sendall:[message text]
    Message msg_sendall(TAG_SENDALL, user_command);
    send_status = conn.send(msg_sendall);
    if (!send_status) {
      std::cerr << "Sending sendall not successful.\n";
      exit(1);
    }
    Message sendall_response;
    receive_status = conn.receive(sendall_response);
    if (!receive_status) {
      std::cerr << "Receiving after sendall not successful.\n";
      exit(1);
    }
    if (sendall_response.tag == TAG_ERR) {
      std::string err_msg = sendall_response.data;
      std::cerr << err_msg.c_str();
    }
  }

  return 0;
}
