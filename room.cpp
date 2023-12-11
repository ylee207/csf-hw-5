#include "guard.h"
#include "message.h"
#include "message_queue.h"
#include "user.h"
#include "room.h"

Room::Room(const std::string &room_name)
  : room_name(room_name) {
    pthread_mutex_init(&lock, nullptr);
  // TODO: initialize the mutex
}

Room::~Room() {
  // TODO: destroy the mutex
  pthread_mutex_destroy(&lock);
}

void Room::add_member(User *user) {
  // TODO: add User to the room
  Guard g(lock);
  members.insert(user);

}

void Room::remove_member(User *user) {
  // TODO: remove User from the room
  Guard g(lock);
  members.erase(user);
}
bool Room::member_exists(User *user) {
  return members.find(user) != members.end();
}
std::string remove_newline(std::string &str) {
  size_t pos = str.find("\n");
  if (pos != std::string::npos) {
    str.erase(pos);
  }
  return str;
}
void Room::broadcast_message(const std::string &sender_username, const std::string &message_text) {
  // TODO: send a message to every (receiver) User in the room
  Guard g(lock);
  for (auto user : members) {
    if (user->username != sender_username) {
      room_name = get_room_name();
      room_name = remove_newline(room_name);
      std::string username = sender_username;
      username = remove_newline(username);
      std::string message = message_text;
      message = remove_newline(message);
      std::string text = room_name + ":" + username + ":" + message;
      Message *msg = new Message(TAG_DELIVERY, text);
      user->mqueue.enqueue(msg);

    }
  }
}
