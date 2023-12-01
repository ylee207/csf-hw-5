#include <sstream>
#include <cctype>
#include <cassert>
#include <string.h>
#include <string>
#include <unistd.h>
#include "csapp.h"
#include "message.h"
#include "connection.h"

Connection::Connection()
  : m_fd(-1)
  , m_last_result(SUCCESS) {
}

Connection::Connection(int fd)
  : m_fd(fd)
  , m_last_result(SUCCESS) {
    // call rio_readinitb to initialize the rio_t object
    rio_readinitb(&m_fdbuf, m_fd);
}

void Connection::connect(const std::string &hostname, int port) {
  // call open_clientfd to connect to the server
  // call rio_readinitb to initialize the rio_t object
  std::string port_str = std::to_string(port);
  m_fd = open_clientfd(hostname.c_str(), port_str.c_str());
  if (m_fd < 0) {
    m_last_result = EOF_OR_ERROR;
    return;
  }
  rio_readinitb(&m_fdbuf, m_fd);
}

Connection::~Connection() {
  // close the socket if it is open
  if (is_open()) { 
    Close(m_fd); 
  }
}

bool Connection::is_open() const {
  // return true if the connection is open
  return m_fd >= 0;
}

void Connection::close() {
  // close the connection if it is open
  if (is_open()) { 
    Close(m_fd); 
  }
}

bool Connection::send(const Message &msg) {
  // send a message
  // return true if successful, false if not
  // make sure that m_last_result is set appropriately
  if (strlen(msg.data.c_str()) > msg.MAX_LEN) {
    m_last_result = INVALID_MSG;
    return false;
  }
  ssize_t size;
  std::string msg_str;
  if (msg.tag == TAG_QUIT || msg.tag == TAG_LEAVE) {
    msg_str = msg.tag + ":" + "bye" + "\n";
    size = rio_writen(m_fd, msg_str.c_str(), strlen(msg_str.c_str()));
  } else {
    msg_str = msg.tag + ":" + msg.data + "\n";
    size = rio_writen(m_fd, msg_str.c_str(), strlen(msg_str.c_str()));
  }

  if (size < 0) {
    m_last_result = EOF_OR_ERROR;
    return false;
  }
  else if(size != (ssize_t) strlen(msg_str.c_str())) {
    m_last_result = INVALID_MSG;
    return false;
  }
  m_last_result = SUCCESS;
  return true;
}

bool Connection::receive(Message &msg) {
  // receive a message, storing its tag and data in msg
  // return true if successful, false if not
  // make sure that m_last_result is set appropriately
  char buffer[msg.MAX_LEN + 1]; // include null terminator space.
  ssize_t d_size = rio_readlineb(&m_fdbuf, buffer, msg.MAX_LEN);
  if (d_size < 0) {
    m_last_result = EOF_OR_ERROR;
    return false;
  }
  std::string s = buffer;
  msg.data = s;
  std::vector<std::string> result = divide_data(msg);
  if (result.size() < 2) {
    m_last_result = INVALID_MSG;
    return false;
  }
  if (msg.tag == TAG_DELIVERY) {
    msg.data = result[1] + ':' + result[2] + ':' + result[3]; 
  }
  else {
    msg.data = result[1];
  }
  m_last_result = SUCCESS;
  return true;
}

// divide the data by ":" and store each segment in the vector
std::vector<std::string> Connection::divide_data(Message &msg)  {
  std::vector<std::string> result;
  std::istringstream iss(msg.data);
  std::string token;

  while (std::getline(iss, token, ':')) {
    result.push_back(token);
  }
  msg.tag = result[0];
  return result;
}