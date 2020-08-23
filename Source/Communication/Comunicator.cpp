#include <mpi.h>
#include <mutex>
#include <vector>
#include "Comunicator.h"

std::ostream& operator<<(std::ostream& os, const Message& msg)
{
    os << "[from:" << msg.sender << "] " << TypeNames[msg.type] << "." << msg.resource_type << "." << msg.signature << "." << msg.lamport_clock << " [to:" << msg.destination << "]";
    return os;
}


void Communicator::Broadcast(Message message, std::vector<int> destination_list){
    std::lock_guard<std::mutex> lock(clock_mtx);
    message.lamport_clock = lamportClock;
    lamportClock = lamportClock + 1;


    for(auto destination : destination_list){
        _Message smaller_msg(message);
        MPI_Send(&smaller_msg, 4, MPI_INT, destination, MSG_TAG, MPI_COMM_WORLD);
        message.destination = destination; // For proper log printing
        std::cout << "["<< nodeType <<" " << message.sender << "] sending: "<< message << "\n";
    }
}

void Communicator::Send(Message message)
{
    std::lock_guard<std::mutex> lock(clock_mtx);
    message.lamport_clock = lamportClock;
    lamportClock = lamportClock + 1;

    _Message smaller_msg(message);
    MPI_Send(&smaller_msg, 4, MPI_INT, message.destination, MSG_TAG, MPI_COMM_WORLD);
    std::cout << "["<< nodeType <<" " << message.sender << "] sending: "<< message << "\n";
}

Message Communicator::Recieve()
{
    _Message _message{};
    MPI_Status status{};
    MPI_Recv(&_message, 4, MPI_INT, MPI_ANY_SOURCE, MSG_TAG, MPI_COMM_WORLD, &status);

    std::lock_guard<std::mutex> lock(clock_mtx);
    lamportClock = std::max(_message.lamport_clock, lamportClock) + 1;

    Message message(_message, status.MPI_SOURCE, node_id);
    std::cout << "["<< nodeType <<" " << node_id << "] recieved: "<< message << "\n";
    return message;
}

int Communicator::getLamportClock() {
    std::lock_guard<std::mutex> lock(clock_mtx);
    return lamportClock;
}

Communicator::Communicator(int nodeId, std::string nodeType) : node_id(nodeId), lamportClock(0), nodeType(nodeType) {}

Communicator::_Message::_Message(int type, int signature, int lamportClock, int resourceType) : type(type),
                                                                                                signature(signature),
                                                                                                lamport_clock(
                                                                                                        lamportClock),
                                                                                                resource_type(
                                                                                                        resourceType) {}

Communicator::_Message::_Message(Message &message) : type(message.type),
                                                     signature(message.signature),
                                                     lamport_clock(
                                                             message.lamport_clock),
                                                     resource_type(
                                                             message.resource_type) {

}
