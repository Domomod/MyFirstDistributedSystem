#include <iostream>

#ifndef COMUNICATOR_H
#define COMUNICATOR_H

#define MSG_TAG 100

enum Type {
    Invite,
    Agree,
    Reject,
    Disband
};
const std::string TypeNames[] = {"Invite", "Agree", "Reject", "Disband"};

struct Message {
    int type;
    int sender;
    int destination;
    int tag;
    int resource_type;

    Message() = default;

    Message(int type, int sender, int destination, int tag, int resource_type) : type(type), sender(sender),
                                                                                 destination(destination), tag(tag),
                                                                                 resource_type(resource_type) {}
};

std::ostream &operator<<(std::ostream &os, const Message &msg);

class Communcatior {
public:
    static void Send(const Message &message);

    static Message Recieve();
};

#endif