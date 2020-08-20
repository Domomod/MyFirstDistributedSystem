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
#include "ResourceManagment/RenewableResourceStrategy.h"
#include "ResourceManagment/ConsumableResourceStrategy.h"

#define InitialTasksCount 10
#define DragonCarcassCount 2
#define WorkbenchCount 1

#define HEAD 0
#define TORSO 1
#define TAIL 2

enum ResourceTypes {
    HeadTorsoConnection,
    TorsoTailConnection,
    Task,
    Workbench,
    DragonCarcass
};

class Head {
private:
    int node_id;
    Communicator communicator;
    RecieveInvitationStrategy recieveTorsoInvitation;
    RenewableResourceStrategy workbenches;
private:
    [[noreturn]] void communication_thread() {
        while (true) {
            Message msg = communicator.Recieve();
            recieveTorsoInvitation.HandleMessage(msg);
            switch (msg.resource_type) {
                case HeadTorsoConnection:
                    recieveTorsoInvitation.HandleMessage(msg);
                    break;
                case Workbench:
                    workbenches.HandleMessage(msg);
                    break;
            }
        }
    }

    [[noreturn]] void processing_thread() {
        while (true) {
            recieveTorsoInvitation.acquire();
            std::cout << "Head " << node_id << " accepted team invite.\n";
            //workbenches.acquire();

            recieveTorsoInvitation.release();
            //workbenches.release();

        }
    }


public:
    void run() {
        std::thread th(&Head::communication_thread, this);
        processing_thread();
        th.join();
    }

    explicit Head(int node_id, std::vector<int> heads, int workbench_count)
            : node_id(node_id),
              communicator(node_id, "Head "),
              recieveTorsoInvitation(HeadTorsoConnection,
                                     node_id,
                                     &communicator,
                                     "HeadTorsoLink(" + std::to_string(HeadTorsoConnection) + ")",
                                     "Head"),
              workbenches(Workbench,
                          node_id,
                          &communicator,
                          workbench_count,
                          heads,
                          "Workbench("+ std::to_string(Workbench) + ")",
                          "Head") {
    }
};

class Torso {
private:
    int node_id;
    Communicator communicator;
    SendInvitationStrategy sendInvitationToHead;
    RecieveInvitationStrategy recieveInvitationFromTail;
    RenewableResourceStrategy tasks;
private:
    [[noreturn]] void communication_thread() {
        while (true) {
            Message msg = communicator.Recieve();
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

    [[noreturn]] void processing_thread() {
        while (true) {
            sendInvitationToHead.acquire();
            std::cout << "Torso " << node_id << " teamed with a Head.\n";
            recieveInvitationFromTail.acquire();
            std::cout << "Torso " << node_id << " teamed with a Tail.\n";

            //tasks.acquire();

            recieveInvitationFromTail.release();

            //tasks.release();

            sendInvitationToHead.release();
        }
    }

public:
    void run() {
        std::thread th(&Torso::communication_thread, this);
        std::thread th2;
        //if(node_id == TORSO){ /*One torso recieves an additional job of broadcasting new tasks*/
        //    th2 = std::thread(&ConsumableResourceStrategy::produceResourceThread, &tasks);
        //}
        processing_thread();
        th.join();
        th2.join();
    }

    explicit Torso(int node_id, const std::vector<int> &heads, const std::vector<int> torsoes, int taskCount)
            : node_id(node_id),
              communicator(node_id, "Torso"),
              sendInvitationToHead(
                      HeadTorsoConnection,
                      node_id,
                      &communicator,
                      heads, "Torso", "HeadTorsoLink("+ std::to_string(HeadTorsoConnection) + ")"),
              recieveInvitationFromTail(
                      TorsoTailConnection,
                      node_id,
                      &communicator, "TailTorsoLink(" + std::to_string(TorsoTailConnection) + ")", "Torso"),
              tasks(Task,
                    node_id,
                    &communicator,
                    taskCount,
                    torsoes, "Task(" + std::to_string(Task) + ")", "Torso") {}
};

class Tail {
private:
    int node_id;
    Communicator communicator;
    SendInvitationStrategy sendInvitationToTorso;
    RenewableResourceStrategy dragonCarcass;
private:
    [[noreturn]] void communication_thread() {
        while (true) {
            Message msg = communicator.Recieve();
            switch (msg.resource_type) {
                case TorsoTailConnection:
                    sendInvitationToTorso.HandleMessage(msg);
                    break;
                case DragonCarcass:
                    dragonCarcass.HandleMessage(msg);
                    break;
            }

        }
    }

    [[noreturn]] void processing_thread() {
        while (true) {
            sendInvitationToTorso.acquire();
            std::cout << "Tail " << node_id << " found a Torso.\n";
            dragonCarcass.acquire();

            //Wskrzeszanie

            dragonCarcass.release();

            sendInvitationToTorso.release();
        }
    }

public:
    void run() {
        std::thread th(&Tail::communication_thread, this);
        processing_thread();
        th.join();
    }

    explicit Tail(int node_id, const std::vector<int> &torsos, std::vector<int> tails, int resourceCount)
            : node_id(node_id),
              communicator(node_id, "Tail "),
              sendInvitationToTorso(TorsoTailConnection,
                                    node_id, &communicator,
                                    torsos, "Tail", "TailTorsoLink("+ std::to_string(TorsoTailConnection) + ")"),
              dragonCarcass(DragonCarcass,
                            node_id,
                            &communicator,
                            resourceCount,
                            tails, "DragonCarcass("+ std::to_string(DragonCarcass) + ")", "Tail") {}
};



int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int size, tid;
    long double inside = 0, total = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &tid);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    std::cout << "Hello cruel world " << tid << "\n";

    std::vector<int> tails;
    std::vector<int> torsos;
    std::vector<int> heads;
    for (int i = 0; i < size; i++) {
        if (i % 3 == HEAD)
            heads.push_back(i);
        if (i % 3 == TORSO)
            torsos.push_back(i);
        if (i % 3 == TAIL)
            tails.push_back(i);
    }

    switch (tid % 3) {
        case HEAD:
            Head(tid, heads, WorkbenchCount).run();
            break;
        case TORSO:
            Torso(tid, heads, torsos, InitialTasksCount).run();
            break;
        case TAIL:
            Tail(tid, torsos, tails, DragonCarcassCount).run();
            break;
    }

    MPI_Finalize();
}

