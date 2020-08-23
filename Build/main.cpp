#include <mpi.h>
#include <thread>

#include <vector>
#include <algorithm>
#include <iostream>

#include "cxxopts.hpp"
#include "ResourceManagment/SendInvitation.h"
#include "ResourceManagment/ReceiveInvitation.h"
#include "ResourceManagment/RenewableResourceStrategy.h"
#include "ResourceManagment/ConsumableResourceStrategy.h"

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
            workbenches.acquire();

            recieveTorsoInvitation.release();
            workbenches.release();

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
    bool produces_new_tasks;
    Communicator communicator;
    SendInvitationStrategy sendInvitationToHead;
    RecieveInvitationStrategy recieveInvitationFromTail;
    ConsumableResourceStrategy tasks;
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
                case Task:
                    tasks.HandleMessage(msg);
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

            tasks.acquire();
            tasks.release();

            recieveInvitationFromTail.release();

            sendInvitationToHead.release();
        }
    }

public:
    void run() {
        std::thread th(&Torso::communication_thread, this);
        std::thread th2;
        if(produces_new_tasks){
            th2 = std::thread(&ConsumableResourceStrategy::produceResourceThread, &tasks);
        }
        processing_thread();
        th.join();
        th2.join();
    }

    explicit Torso(int node_id, const std::vector<int> &heads, const std::vector<int> torsoes, int taskCount, bool produces_new_tasks)
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
                    torsoes, "Torso", "Task(" + std::to_string(Task) + ")"),
              produces_new_tasks(produces_new_tasks) {}
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
            //std::cout << "Tail " << node_id << " found a Torso.\n";
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

    try
    {
        cxxopts::Options options(argv[0], " - example command line options");
        options
                .positional_help("[optional args]")
                .show_positional_help();

        int TailsCount, TorsosCount, HeadsCount, TasksCount, DragonsCount, WorkbenchesCount;

        options
                .allow_unrecognised_options()
                .add_options()
                        ("tail", "Amount of tail type nodes", cxxopts::value<int>(TailsCount)->default_value("3"))
                        ("torso", "Amount of torso type nodes", cxxopts::value<int>(TorsosCount)->default_value("3"))
                        ("head", "Amount of head type nodes", cxxopts::value<int>(HeadsCount)->default_value("3"))
                        ("task", "Initial amount of tasks", cxxopts::value<int>(TasksCount)->default_value("3"))
                        ("dragon", "Amount of dragon type resources", cxxopts::value<int>(DragonsCount)->default_value("2"))
                        ("workbench", "Amount of workbench type resources", cxxopts::value<int>(WorkbenchesCount)->default_value("2"))
                ;


        auto result = options.parse(argc, argv);

        std::vector<int> tails;
        std::vector<int> torsos;
        std::vector<int> heads;
        int id = 0;
        int SpecialTorso;
        int thisNode;
        while(HeadsCount>0){
            if(tid == id) thisNode = HEAD;
            heads.push_back(id);
            HeadsCount--;
            id++;
        }
        SpecialTorso = id;
        while(TorsosCount>0){
            if(tid == id) thisNode = TORSO;
            torsos.push_back(id);
            TorsosCount--;
            id++;
        }
        while(TailsCount>0){
            if(tid == id) thisNode = TAIL;
            tails.push_back(id);
            TailsCount--;
            id++;
        }

        switch (thisNode) {
            case HEAD:
                Head(tid, heads, WorkbenchesCount).run();
                break;
            case TORSO:
                Torso(tid, heads, torsos, TasksCount, tid == SpecialTorso).run();
                break;
            case TAIL:
                Tail(tid, torsos, tails, DragonsCount).run();
                break;
        }
    }
    catch (const cxxopts::OptionException& e)
    {
        std::cout << "error parsing options: " << e.what() << std::endl;
        exit(1);
    }

    MPI_Finalize();
}

