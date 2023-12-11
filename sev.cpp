#include <pthread.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <set>
#include <vector>
#include <cctype>
#include <cassert>
#include "message.h"
#include "connection.h"
#include "user.h"
#include "room.h"
#include "guard.h"
#include "server.h"

using std::stringstream;

////////////////////////////////////////////////////////////////////////
// Server implementation data types
////////////////////////////////////////////////////////////////////////

// add any additional data types that might be helpful
//       for implementing the Server member functions

struct Client {
  Connection *conn;
  Server *server;
};

////////////////////////////////////////////////////////////////////////
// Client thread functions
////////////////////////////////////////////////////////////////////////

namespace {

void *worker(void *arg) {
  pthread_detach(pthread_self());

  // use a static cast to convert arg from a void* to
  //       whatever pointer type describes the object(s) needed
  //       to communicate with a client (sender or receiver)
  Client* client = static_cast<Client*>(arg);


  // read login message (should be tagged either with
  //       TAG_SLOGIN or TAG_RLOGIN), send response

  Message loginMessage = Message();
  Message responseMessage = Message();
  bool log_rec = client->conn->receive(loginMessage);

  std::string tagtype = loginMessage.tag;


  if(tagtype == TAG_ERR) {
    responseMessage.tag = TAG_ERR;
    responseMessage.data = loginMessage.data;
    client->conn->send(responseMessage);
  } else if(tagtype == TAG_SLOGIN) {
    responseMessage.tag = TAG_OK;
    //TODO: look at this
    responseMessage.data = "logged in as sender";
    client->conn->send(responseMessage);
  } else if(tagtype == TAG_RLOGIN) {
    responseMessage.tag = TAG_OK;
    //TODO: look at this
    responseMessage.data = "logged in as receiver";
    client->conn->send(responseMessage);
  }


  //       depending on whether the client logged in as a sender or
  //       receiver, communicate with the client (implementing
  //       separate helper functions for each of these possibilities
  //       is a good idea)

    User *user = new User(loginMessage.data);

    if (loginMessage.tag == TAG_SLOGIN)
    {
      client->server->com_sender(client->conn, client->server, user);
    }
    else if (loginMessage.tag == TAG_RLOGIN)
    {
      client->server->com_receiver(client->conn, client->server, user);
    }

  return nullptr;
}
}

void Server::com_sender(Connection *conn, Server *server, User *user) {
  Room *room = nullptr;

  while(conn->is_open()) {
    Message message = Message();
    bool msg_rec = conn->receive(message);

    Message message_send = Message();
    

    //This is when the server fails to receive a message
    if(msg_rec == false) {
      message_send.tag = TAG_ERR;
      message_send.data = "failed to receive message";
      conn->send(message_send);
      break;
    } else { //this is when the server is able to receive a message
      if(message.tag == TAG_JOIN) {
        if(room == nullptr) {
          room = server->find_or_create_room(message.data);
          room->add_member(user);
          message_send.tag = TAG_OK;
          message_send.data = "succeeded in joining room";
          conn->send(message_send);
        } else {
          room->remove_member(user);
          room = server->find_or_create_room(message.data);
          room->add_member(user);
          message_send.tag = TAG_OK;
          message_send.data = "succeeded in joining new room";
          conn->send(message_send);
        }
      } else if(message.tag == TAG_QUIT) {
        message_send.tag = TAG_OK;
        message.data = "bye";
        bool quit_send = conn->send(message);
        break;
      } else if(message.tag == TAG_LEAVE) {
        if(room->member_exists(user)) {
          room->remove_member(user);
          message_send.tag = TAG_OK;
          message_send.data = "succeeded in leaving room";
          conn->send(message_send);
        } else {
          message_send.tag = TAG_ERR;
          message_send.data = "tried to leave room when not within one";
          conn->send(message_send);
        }
      } else {
        // Message msg_temp = Message();
        // msg_temp.tag = TAG_OK;
        // msg_temp.data = "blah blah blah";
        // conn->send(msg_temp);
        if(room == nullptr) {
          message_send.tag = TAG_ERR;
          message_send.data = "not within a room";
          conn->send(message_send);
        } else {
          
          room->broadcast_message(user->username, message.data);
          message_send.tag = TAG_OK;
          message_send.data = "message sent";
          conn->send(message_send);
        }
      }
    }  
  }
  conn->close();
  return;
}

void Server::com_receiver(Connection *conn, Server *server, User *user) {
  Room *room = nullptr;

  Message message = Message();
  bool msg_received = conn->receive(message);

  Message message_send = Message();

  //Failed to receive a message
  if(msg_received == false) {
    message_send.tag = TAG_ERR;
    message_send.data = "failed to receive message";
    conn->send(message_send);
    return;
  } else if(message.tag == TAG_JOIN) { //Message received had has tag_join
    room = server->find_or_create_room(message.data);
    //adding member to room
    room->add_member(user);
    //sending message to conn that the room joining was successful
    message_send.tag = TAG_OK;
    message_send.data = "succeeded in joining room";
    conn->send(message_send);
  } else {
    message_send.tag = TAG_ERR;
    message_send.data = "not a valid message";
    conn->send(message_send);
    return;
  }

  while(conn->is_open()) {
    Message message_send = Message();
    //if there are messages to be sent, then grab them from message queue and send them to connection
    Message *new_message_send = user->mqueue.dequeue();
    if(new_message_send == nullptr) {
      continue;
    }else if(new_message_send->tag == TAG_ERR) {
      message_send.tag = TAG_ERR;
      message_send.data = "received an error message";
      conn->send(message_send);
      break;
    } else {
      conn->send(*new_message_send);
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
  pthread_mutex_init(&m_lock, nullptr);
}

Server::~Server() {
  pthread_mutex_destroy(&m_lock);
}

bool Server::listen() {
  // use open_listenfd to create the server socket, return true
  //       if successful, false if not
  stringstream stream;
  stream << m_port;
  std::string port_s;
  stream >> port_s;
  const char* port_c = port_s.c_str();

  m_ssock = open_listenfd(port_c);

  if(isValidSock() == true) {
    return true;
  } else {
    std::cerr << "failed to open server";
    return false;
  }
}

bool Server::isValidSock() {
  if (m_ssock > 0) {
    return true;
  } else {
    return false;
  }
}

void Server::handle_client_requests() {
  //       infinite loop calling accept or Accept, starting a new
  //       pthread for each connected client
  while(1) {
    int deez = accept(m_ssock, nullptr, nullptr);

    if(deez < 0) {
      std::cerr << "failed to accept sock";
      //TODO: look at this later
      break;
    }

    Connection *conn_temp = new Connection(deez);

    Client *client = new Client;
    client->conn = conn_temp;
    client->server = this;

    pthread_t thread;

    int status = pthread_create(&thread, nullptr, worker, client);

    if(status < 0) {
      std::cerr << "failed to create thread";
      //TODO: look at this later
      break;
    } else {
      continue;
    }
  }
}

Room *Server::find_or_create_room(const std::string &room_name) {
  //       return a pointer to the unique Room object representing
  //       the named chat room, creating a new one if necessary
  Guard g(m_lock);

  auto currentKV = m_rooms.find(room_name);
  if(currentKV == m_rooms.end()) {
    Room  * newRoom = new Room(room_name);
    auto temp = std::make_pair(room_name, newRoom);
    m_rooms.insert(temp);
    return newRoom;
  } else {
    return currentKV->second;
  }
}
