#include <mpi.h>
#include <thread>

#include <vector>
#include <functional>
#include <algorithm>
#include <random>
#include <iostream>
#include <unistd.h>

#include "ResourceManagment/SendInvitation.h"
#include "ResourceManagment/ReceiveInvitation.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
enum ResourceTypes {
    HeadTorsoConnection,
    TorsoTailConnection,
    ResourceTypesNumber
};

class Node {
protected:
    int node_id;

    virtual void communication_thread() = 0;

    virtual void processing_thread() = 0;

public:
    explicit Node(int nodeId) : node_id(nodeId) {}

    void run() {
        std::thread th(&Node::communication_thread, this);
        processing_thread();
        th.join();
    }
};

class Head : public Node {
private:
    RecieveInvitationStrategy teamupWithTorso;
private:
    void communication_thread() override {
        while (true) {
            Message msg = Communcatior::Recieve();
            teamupWithTorso.HandleMessage(msg);
        }
    }

    void processing_thread() override {
        while (true) {
            teamupWithTorso.acquire();
            std::cout << " Node " << node_id << " accepted team invite.\n";
            teamupWithTorso.release();
        }
    }

public:
    explicit Head(int node_id) : Node(node_id), teamupWithTorso(HeadTorsoConnection, node_id) {

    }
};

class Torso : public Node {
private:
    SendInvitationStrategy sendInvitationToHead;
    RecieveInvitationStrategy recieveInvitationFromTail;
private:
    void communication_thread() override {
        while (true) {
            Message msg = Communcatior::Recieve();
            switch (msg.resource_type) {
                case HeadTorsoConnection:
                    sendInvitationToHead.HandleMessage(msg);
                    break;
                case TorsoTailConnection:
                    recieveInvitationFromTail.HandleMessage(msg);
                    break;
            }
        }
    }

    void processing_thread() override {
        while (true) {
            sendInvitationToHead.acquire();
            std::cout << " Torso " << node_id << " teamed with a Head.\n";

            recieveInvitationFromTail.acquire();
            std::cout << " Torso " << node_id << " teamed with a Tail.\n";


            sendInvitationToHead.release();
            recieveInvitationFromTail.release();
        }
    }

public:
    explicit Torso(int node_id, const std::vector<int> &heads) : Node(node_id),
                                                                 sendInvitationToHead(HeadTorsoConnection,
                                                                                      node_id, heads),
                                                                 recieveInvitationFromTail(
                                                                         TorsoTailConnection, node_id) {}
};

class Tail : public Node {
private:
    SendInvitationStrategy sendInvitationToTorso;
private:
    void communication_thread() override {
        while (true) {
            Message msg = Communcatior::Recieve();
            sendInvitationToTorso.HandleMessage(msg);
        }
    }

    void processing_thread() override {
        while (true) {
            sendInvitationToTorso.acquire();
            std::cout << " Tail " << node_id << " found a Torso.\n";
            sendInvitationToTorso.release();
        }
    }

public:
    explicit Tail(int node_id, const std::vector<int> &torsos) : Node(node_id),
                                                                sendInvitationToTorso(TorsoTailConnection,
                                                                                      node_id, torsos) {}
};

#define HEAD 0
#define TORSO 1
#define TAIL 2

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int size, tid;
    long double inside = 0, total = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &tid);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    std::cout << "Hello cruel world " << tid << "\n";

    std::vector<int> torsos;
    std::vector<int> heads;
    for(int i = 0; i < size; i ++)
    {
        if (i % 3 == HEAD)
            heads.push_back(i);
        if (i % 3 == TORSO)
            torsos.push_back(i);
    }

    switch(tid % 3)
    {
        case HEAD:
            Head(tid).run();
            break;
        case TORSO:
            Torso(tid, heads).run();
            break;
        case TAIL:
            Tail(tid, torsos).run();
            break;
    }

    MPI_Finalize();
}

#pragma clang diagnostic pop