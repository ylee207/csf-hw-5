#include <pthread.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <set>
#include <vector>
#include <cctype>
#include <cassert>
#include "csapp.h"
#include "message.h"
#include "connection.h"
#include "user.h"
#include "room.h"
#include "guard.h"
#include "server.h"

////////////////////////////////////////////////////////////////////////
// Server implementation data types
////////////////////////////////////////////////////////////////////////

struct ServerConnection {
  Server *server;
  Connection *connection;
};

////////////////////////////////////////////////////////////////////////
// Client thread functions
////////////////////////////////////////////////////////////////////////

namespace {

void sendLoginSuccessResponse(ServerConnection *serverConn, const std::string& tag) {
  std::string responseText;
  if (tag == TAG_SLOGIN) {
    responseText = "Sender logged in";
  } else if (tag == TAG_RLOGIN) {
    responseText = "Receiver logged in";
  } else {
    // Handle unexpected tag value
    responseText = "Unknown login type";
  }
  serverConn->connection->send(Message(TAG_OK, responseText));
}


void sendLoginErrorResponse(ServerConnection *serverConn) {
  serverConn->connection->send(Message(TAG_ERR, "Login unsuccessful"));
}

void handleClientCommunication(ServerConnection *serverConn, const std::string& userType, const std::string& messageData) {
  User *user = new User(messageData);
  if (userType == TAG_SLOGIN) {
    serverConn->server->chat_with_sender(serverConn->connection, serverConn->server, user);
  } else if (userType == TAG_RLOGIN) {
    serverConn->server->chat_with_receiver(serverConn->connection, serverConn->server, user);
  }
}


void handleClientLogin(ServerConnection *serverConn) {
  Message msg;
  if (!serverConn->connection->receive(msg)) {
    sendLoginErrorResponse(serverConn);
    return;
  }

  if (msg.tag == TAG_SLOGIN || msg.tag == TAG_RLOGIN) {
    sendLoginSuccessResponse(serverConn, msg.tag);
    handleClientCommunication(serverConn, msg.tag, msg.data);
  } else {
    sendLoginErrorResponse(serverConn);
  }
}



void *worker(void *arg) {
  pthread_detach(pthread_self());
  ServerConnection *serverConn = static_cast<ServerConnection*>(arg);
  handleClientLogin(serverConn);
  return nullptr;
}

} // anonymous namespace

////////////////////////////////////////////////////////////////////////
// Server member function implementation
////////////////////////////////////////////////////////////////////////

Server::Server(int port)
  : m_port(port), m_ssock(-1) {
  pthread_mutex_init(&m_lock, nullptr);
}

Server::~Server() {
  pthread_mutex_destroy(&m_lock);
}

bool Server::listen() {
  std::string portStr = std::to_string(m_port);
  m_ssock = open_listenfd(portStr.c_str());
  return m_ssock >= 0;
}

void Server::handle_client_requests() {
  while (true) {
    int client_fd = acceptClient();
    if (client_fd < 0) {
      std::cerr << "Error accepting client connection" << std::endl;
      continue;
    }
    startClientThread(client_fd);
  }
}

Room *Server::find_or_create_room(const std::string &room_name) {
  Guard g(m_lock);
  auto it = m_rooms.find(room_name);
  if (it == m_rooms.end()) {
    Room *new_room = new Room(room_name);
    m_rooms[room_name] = new_room;
    return new_room;
  }
  return it->second;
}

void Server::chat_with_sender(Connection *conn, Server *server, User* user) {
  Room *room = nullptr;
  while (conn->is_open()) {
    Message msg = Message();
    bool reception = conn->receive(msg);
    if (!reception) {
      Message response(TAG_ERR, "Receive error");
      conn->send(response);
      break;
    } else {
    if (msg.tag == TAG_QUIT) {
      Message response(TAG_OK, "Quit successful");
      bool quiting = conn->send(response);
      break;
    } else if (msg.tag == TAG_JOIN) {
      if (room != nullptr) {        
        room->remove_member(user);
      } 
      room = server->find_or_create_room(msg.data);
      room->add_member(user);
      Message response(TAG_OK, "Join successful" );
      conn->send(response);
    } else if(msg.tag == TAG_LEAVE) {
      if (room->member_exists(user)) {
        room->remove_member(user);
        Message response(TAG_OK, "Leave successful");
        conn->send(response);
      } else {
        Message response(TAG_ERR, "Leave unsuccessful, user isn't in the room");
        conn->send(response);
      } 
    } else {  //sendall
      if (room == nullptr || !room->member_exists(user)) {
        Message response(TAG_ERR, "Sendall unsuccessful, user isn't in a room");
        conn->send(response);
      } else {
        room->broadcast_message(user->username, msg.data);
        Message response(TAG_OK, "Sendall successful");
        conn->send(response);
      }
    }
    }
  }
  conn->close();
  return;
}

void Server::chat_with_receiver(Connection *conn, Server *server, User *user) {
  Room *room = nullptr;
  Message msg = Message();
  if (!conn->receive(msg)) {
    Message response(TAG_ERR, "Receive error");
    conn->send(response);
    return;
  } else if (msg.tag != TAG_JOIN) {
    Message response(TAG_ERR, "First message must be a join");
    conn->send(response);
    return;
  } else {
    room = server->find_or_create_room(msg.data);
    room->add_member(user);
    Message response(TAG_OK, "Join successful");
    conn->send(response);
  }
  while(conn->is_open()) {
    Message *uMsg = user->mqueue.dequeue();
    if (uMsg != nullptr && uMsg->tag != TAG_ERR) {
      conn->send(*uMsg);
    } else if (uMsg != nullptr && uMsg->tag == TAG_ERR) {
      Message response(TAG_ERR, "Receive error");
      conn->send(response);
      break;
    } else {
      continue;
    }
  }
  conn->close();
  return;
}

// Private helper methods
int Server::acceptClient() {
  return accept(m_ssock, nullptr, nullptr);
}

void Server::startClientThread(int client_fd) {
  Connection *conn = new Connection(client_fd);
  ServerConnection *serverConn = new ServerConnection{this, conn};
  pthread_t thread;
  if (pthread_create(&thread, nullptr, worker, serverConn) < 0) {
    std::cerr << "Error creating thread" << std::endl;
    delete conn;
    delete serverConn;
  }
}
