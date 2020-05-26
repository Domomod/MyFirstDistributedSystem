#include <mpi.h>
#include "Comunicator.h"

std::ostream& operator<<(std::ostream& os, const Message& msg)
{
    os << "[from:" << msg.sender <<"] " << TypeNames[msg.type] << "." << msg.tag << " [to:" << msg.destination << "]";
    return os;
}


void Communcatior::Send(const Message& message)
{
    MPI_Send(&message, 5, MPI_INT, message.destination, MSG_TAG, MPI_COMM_WORLD);
    std::cout << "[Node " << message.sender << "] sending: "<< message << "\n";
}

Message Communcatior::Recieve()
{
    Message message;
    MPI_Recv(&message, 5, MPI_INT, MPI_ANY_SOURCE, MSG_TAG, MPI_COMM_WORLD, nullptr);
    std::cout << "[Node " << message.destination << "] recieved: "<< message << "\n";

    return message;
}
