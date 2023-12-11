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

// TODO: add any additional data types that might be helpful
//       for implementing the Server member functions
struct ServerConnection {
  Server * server;
  Connection * connection;
};
////////////////////////////////////////////////////////////////////////
// Client thread functions
////////////////////////////////////////////////////////////////////////

namespace {
void *worker(void *arg) {
  pthread_detach(pthread_self());

  // TODO: use a static cast to convert arg from a void* to
  //       whatever pointer type describes the object(s) needed
  //       to communicate with a client (sender or receiver)
  ServerConnection *serverConn = static_cast<ServerConnection*>(arg);
  Message msg;
  serverConn->connection->receive(msg);
  std::string usertype;
  // TODO: read login message (should be tagged either with
  //       TAG_SLOGIN or TAG_RLOGIN), send response
  if (msg.tag == TAG_SLOGIN) {
    Message response(TAG_SLOGIN, "Sender logged in");
    serverConn->connection->send(response); 
    usertype = "sender";
  } else if (msg.tag == TAG_RLOGIN) {
    Message response(TAG_RLOGIN, "Receiver logged in");
    serverConn->connection->send(response); 
    usertype = "receiver";
  } else if (msg.tag == TAG_ERR){
    Message response(TAG_ERR, "Login unsuccessful");
    serverConn->connection->send(response);
  }
  // TODO: depending on whether the client logged in as a sender or
  //       receiver, communicate with the client (implementing
  //       separate helper functions for each of these possibilities
  //       is a good idea)
  //Message responder;
  User *user = new User(msg.data);
  if (usertype == "sender") {
    serverConn->server->chat_with_sender(serverConn->connection, serverConn->server, user);
  } else if (usertype == "receiver") {
    serverConn->server->chat_with_receiver(serverConn->connection, serverConn->server, user);
  }
  return nullptr;
}

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
void Server::chat_with_sender(Connection *conn, Server *server, User* user) {
  Room *room = nullptr;
  Message msg = Message();
  while (conn->is_open()) {
    
    if (!conn->receive(msg)) {
      Message response(TAG_ERR, "Receive error");
      conn->send(response);
      break;
    } else {
    if (msg.tag == TAG_QUIT) {
      Message response(TAG_OK, "Quit successful");
      conn->send(response);
      break;
    } else if (msg.tag == TAG_JOIN) {
      if (room != nullptr) {        
        room->remove_member(user);
      } 
      room = server->find_or_create_room(msg.data);
      room->add_member(user);
      Message response(TAG_OK, room->get_room_name() + ":Join successful" );
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

////////////////////////////////////////////////////////////////////////
// Server member function implementation
////////////////////////////////////////////////////////////////////////

Server::Server(int port)
  : m_port(port)
  , m_ssock(-1) {
  pthread_mutex_init(&m_lock, NULL);

}

Server::~Server() {
  pthread_mutex_destroy(&m_lock);
  close(m_ssock);
}

bool Server::listen() {
  std::string toStr = std::to_string(m_port);
  m_ssock = open_listenfd(toStr.c_str());
  return m_ssock != -1;

  // TODO: use open_listenfd to create the server socket, return true
  //       if successful, false if not
}

void Server::handle_client_requests() {
  // TODO: infinite loop calling accept or Accept, starting a new
  //       pthread for each connected client
    while (true) {
        int client = accept(m_ssock, NULL, NULL);
        if (client < 0) {
            // Error handling
            std::cerr << "Error accepting client connection" << std::endl;
            break;
        }
        Connection *conn = new Connection(client);
        ServerConnection *serverConn = new ServerConnection;
        serverConn->server = this;
        serverConn->connection = conn;
        pthread_t thread;
        if (pthread_create(&thread, nullptr, worker, serverConn) < 0) {
            // Error handling
            std::cerr << "Error creating thread" << std::endl;
            break;
        } else {
          continue;
        }
    }

}

Room *Server::find_or_create_room(const std::string &room_name) {
  // TODO: return a pointer to the unique Room object representing
  //       the named chat room, creating a new one if necessary
    Guard g(m_lock);
    auto it = m_rooms.find(room_name);
    if (it == m_rooms.end()) {
        Room *new_room = new Room(room_name);
        auto temp = std::make_pair(room_name, new_room);
        m_rooms.insert(temp);
       // pthread_mutex_unlock(&m_lock);
        return new_room;
    }
   // pthread_mutex_unlock(&m_lock);
    return it->second;

}
