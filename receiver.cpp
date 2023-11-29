#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include "csapp.h"
#include "message.h"
#include "connection.h"
#include "client_util.h"

int main(int argc, char **argv) {
  if (argc != 5) {
    std::cerr << "Usage: ./receiver [server_address] [port] [username] [room]\n";
    return 1;
  }

  std::string server_hostname = argv[1];
  int server_port = std::stoi(argv[2]);
  std::string username = argv[3];
  std::string room_name = argv[4];

  Connection conn;

  // TODO: connect to server
  // Connect to server
  conn.connect(server_hostname, server_port);
  // If connection is not open, then it means we have error with connecting to server
  if (!conn.is_open()) {
    std::cerr << "Couldn't connect to server.\n";
    exit(1);
  }

  // TODO: send rlogin and join messages (expect a response from
  //       the server for each one)

  // send rlogin
  Message rlogin_msg(TAG_RLOGIN, username);
  bool send_status = conn.send(rlogin_msg);
  if (!send_status) {
    std::cerr << "Send rlogin not successful.\n";
    exit(1);
  }

  // check response from the server
  Message rlogin_response;
  bool receive_status = conn.receive(rlogin_response);
  if (!receive_status) {
    std::cerr << "Receive from server not successful.\n";
    exit(1);
  }
  if (rlogin_response.tag == TAG_ERR) {
    std::string err_msg = rlogin_response.data;
    std::cerr << err_msg.c_str() << "\n";
    exit(1);
  }

  // join messages
  Message join_msg(TAG_JOIN, room_name);
  send_status = conn.send(join_msg);
  if (!send_status) {
    std::cerr << "join messages not successful.\n";
    exit(1);
  }

  // check response from the server
  Message join_response;
  receive_status = conn.receive(join_response);
  if (!receive_status) {
    std::cerr << "Receive from server not successful.\n";
    exit(1);
  }
  if (join_response.tag == TAG_ERR) {
    std::string err_msg = join_response.data;
    std::cerr << err_msg.c_str() << "\n";
    exit(1);
  }

  // TODO: loop waiting for messages from server
  //       (which should be tagged with TAG_DELIVERY)
  // We assume that the delivery from the server will be in the format 'delivery:[room]:[sender]:[message]'. (except for error cases)
  while (1) {
    Message msg_from_server;
    receive_status = conn.receive(msg_from_server);
    if (!receive_status) {
      std::cerr << "Receive from server not successful.\n";
      exit(1);
    }
    if (msg_from_server.tag == TAG_ERR) {
      std::string err_msg = msg_from_server.data;
      std::cerr << err_msg.c_str() << "\n";
      exit(1);
    }
    if (msg_from_server.tag == TAG_DELIVERY) {
      // print to stdout with [username of sender]: [message text]
      std::string received_data = msg_from_server.data;

      int data_length = received_data.size(); // length of the data received
      int prev_colon_pos = 0; // pointer to track whenever we find a new colon.
      std::vector<std::string> data_storage;

      // Iterate through the data to separate the pieces of data by ':'
      for (int i = 0; i < data_length; i++) {
        char curr_char = received_data[i];
        if (curr_char == ':') {
          std::string data_piece = received_data.substr(prev_colon_pos, i - prev_colon_pos);
          prev_colon_pos = i + 1;
          data_storage.push_back(data_piece);
        }
      }

      // Handle the last piece of data
      if (prev_colon_pos < data_length) {
        data_storage.push_back(received_data.substr(prev_colon_pos));
      }
      
      // Print data_storage[1] and data_storage[2] if they exist
      if (data_storage.size() > 2) {
        fprintf(stdout, "%s: %s\n", data_storage[1].c_str(), data_storage[2].c_str());
      }
    }
  }

  return 0;
}
