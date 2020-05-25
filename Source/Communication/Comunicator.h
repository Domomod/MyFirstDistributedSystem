#include <iostream>

#ifndef COMUNICATOR_H
#define COMUNICATOR_H

#define MSG_TAG 100

enum Type
{
    Invite,
    Agree,
    Reject,
    Disband
};
const std::string TypeNames[] = {"Invite", "Agree", "Reject", "Disband"};

struct Message
{
    int type;
    int sender;
    int tag;
    Message() = default;
    Message(int type, int sender, int tag) : type(type), sender(sender), tag(tag) {}
};

std::ostream& operator<<(std::ostream& os, const Message& msg);

class Communcatior
{
public:
    static void Send(const Message& message, int destination);
    static Message Recieve();
};

#endif