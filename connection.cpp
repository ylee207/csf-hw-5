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
    rio_readinitb(&m_fdbuf, m_fd);
  // TODO: call rio_readinitb to initialize the rio_t object
}

void Connection::connect(const std::string &hostname, int port) {
  std::string port_str = std::to_string(port);
  int result = open_clientfd(hostname.c_str(), port_str.c_str());
  if (result < 0) {
    m_last_result = EOF_OR_ERROR;
    return;
  }
  rio_readinitb(&m_fdbuf, m_fd);

  // TODO: call open_clientfd to connect to the server
  // TODO: call rio_readinitb to initialize the rio_t object
}

Connection::~Connection() {
  // TODO: close the socket if it is open
  if (is_open()) { 
    Close(m_fd); 
  }
}

bool Connection::is_open() const {
  return m_fd >= 0;
  // TODO: return true if the connection is open
}

void Connection::close() {
  if (is_open()) { 
    Close(m_fd); 
  }  // TODO: close the connection if it is open
}

bool Connection::send(const Message &msg) {
  // TODO: send a message
  if (strlen(msg.data.c_str()) > msg.MAX_LEN) {
    m_last_result = INVALID_MSG;
    return false;
  }
  std::string msg_str = msg.tag + msg.data;
  ssize_t size = rio_writen(m_fd, msg_str.c_str(), strlen(msg_str.c_str()));
  if (size < 0) {
    m_last_result = EOF_OR_ERROR;
    return false;
  }
  else if(size != strlen(msg_str.c_str())) {
    m_last_result = INVALID_MSG;
    return false;
  }
  m_last_result = SUCCESS;
  return true;

  // return true if successful, false if not
  // make sure that m_last_result is set appropriately
}
 std::vector<std::string> divide_data(Message &msg)  {
    std::vector<std::string> result;
    std::istringstream iss(msg.data);
    std::string token;

    while (std::getline(iss, token, ':')) {
        result.push_back(token);
    }
    msg.tag = result[0];
    return result;
}
bool Connection::receive(Message &msg) {
  // TODO: receive a message, storing its tag and data in msg
  // return true if successful, false if not
  // make sure that m_last_result is set appropriately
  char buffer[msg.MAX_LEN];
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
