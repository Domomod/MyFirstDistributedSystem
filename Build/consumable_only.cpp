//
// Created by dominik on 27.05.2020.
//

#include <mpi.h>
#include <thread>

#include <vector>
#include <functional>
#include <algorithm>
#include <random>
#include <iostream>
#include <unistd.h>

#include "ResourceManagment/SendInvitation.h"
#include "ResourceManagment/ConsumableResourceStrategy.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

void node(int tid, int size)
{
    std::vector<int> nodes;
    for(int i = 0; i < size; i ++)
    {
        if(i != tid)
            nodes.push_back(i);
    }

    Communicator communicator(tid);
    ConsumableResourceStrategy strategy(0, tid, &communicator, 3, nodes, Node, std::__cxx11::string());
    std::thread th(&ConsumableResourceStrategy::run, &strategy);
    std::thread th2;
    if(tid == 0)
        th2 = std::thread(&ConsumableResourceStrategy::produceResourceThread, &strategy);

    while(true)
    {
        strategy.acquire();
        std::cout << " Node " << tid << " accepted team invite.\n";
        strategy.release();
    }
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int size, tid;
    long double inside = 0, total = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &tid);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    std::cout << "Hello cruel world " << tid << "\n";

    node(tid, size);

    MPI_Finalize();
}

#pragma clang diagnostic pop