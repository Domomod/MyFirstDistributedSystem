#include <mpi.h>
#include "Comunicator.h"

std::ostream& operator<<(std::ostream& os, const Message& msg)
{
    os << "[from:" << msg.sender <<"] " << TypeNames[msg.type] << "." << msg.tag;
    return os;
}


void Communcatior::Send(const Message& message, int destination)
{
    MPI_Send(&message, 3, MPI_INT, destination, MSG_TAG, MPI_COMM_WORLD);
    std::cout << message << " [to: " << destination << "]\n";
}

Message Communcatior::Recieve()
{
    Message message;
    MPI_Recv(&message, 3, MPI_INT, MPI_ANY_SOURCE, MSG_TAG, MPI_COMM_WORLD, nullptr);
    return message;
}
