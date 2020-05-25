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

void inviting(int tid, int size)
{
	std::vector<int> accepting_nodes;
	std::shuffle(accepting_nodes.begin(), accepting_nodes.end(), std::random_device());
	for(int i = 0; i < size/2; i++)
		accepting_nodes.push_back(i);

	SendInvitationStrategy strategy(tid, accepting_nodes);
	std::thread th(&SendInvitationStrategy::run, &strategy);

	while(true)
	{
		strategy.acquire();
		std::cout << " Node " << tid << " found a teammate.\n";
		sleep(4);
		strategy.release();
	}
}

void accepting(int tid, int size)
{
	RecieveInvitationStrategy strategy(tid);
	std::thread th(&RecieveInvitationStrategy::run, &strategy);
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

	if(tid < (size/2))
	{
		accepting(tid, size);
	}
	else
	{
		inviting(tid, size);
	}

	MPI_Finalize();
}
