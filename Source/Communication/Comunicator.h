#include <iostream>
#include <mutex>
#include <mpi.h>
#include <sstream>
#include <vector>

#ifndef COMUNICATOR_H
#define COMUNICATOR_H

#define MSG_TAG 100

enum Type {
    Request,
    Agree,
    Reject,
    Disband,
    Increment
};
const std::string TypeNames[] = {"Request", "Agree", "Reject", "Disband", "Increment"};

struct Message;

class Communicator {
    friend class Message;

private:
    MPI_Status status{};
    int node_id;
    int lamportClock;
    std::mutex clock_mtx;
    std::string nodeType;
private:
    struct _Message {
        _Message() = default;
        _Message(int type, int signature, int lamportClock, int resourceType);

        explicit _Message(Message &message);

        int type;
        int signature;
        int lamport_clock;
        int resource_type;
    };

public:
    Communicator(int nodeId, std::string nodeType = "Node");

    void Send(Message message);

    Message Recieve();

    int getLamportClock();

    void Broadcast(Message message, std::vector<int> destination_list);
};

struct Message {
    int type;
    int sender;
    int destination;
    int signature;
    int lamport_clock;
    int resource_type;

    Message() = default;

    Message(int type, int sender, int destination, int signature, int resource_type, int lamport_clock = 0)
            : type(type), sender(sender), destination(destination), signature(signature), resource_type(resource_type),
              lamport_clock(lamport_clock) {}

    Message(Communicator::_Message &message, int sender, int destination)
            : type(message.type), sender(sender), destination(destination), signature(message.signature), resource_type(message.resource_type),
              lamport_clock(message.lamport_clock) {}

    std::string to_str(){
        std::stringstream ss;
        ss << "[from:" << sender << "] " << TypeNames[type] << "." << signature << "." << lamport_clock << " [to:" << destination << "]";
        return ss.str();
    }
};

std::ostream &operator<<(std::ostream &os, const Message &msg);

#endif