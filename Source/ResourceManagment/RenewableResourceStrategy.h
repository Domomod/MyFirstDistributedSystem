//
// Created by dominik on 27.05.2020.
//

#ifndef MYFIRSTDISTRIBUTEDSYSTEM_RENEWABLERESOURCESTRATEGY_H
#define MYFIRSTDISTRIBUTEDSYSTEM_RENEWABLERESOURCESTRATEGY_H

#include <vector>
#include <list>
#include <mutex>
#include <condition_variable>

#include "Communication/LamportClock.h"
#include "AbstractStrategy.h"

class RenewableResourceStrategy : public AbstractStrategy{
private:
    enum State{
        IDLE,
        COMPETING,
        ACQUIRED,
        STATE_COUNT
    } state;
    std::string StateNames[STATE_COUNT] = {"IDLE","COMPETING","ACQUIRED"};
    std::mutex state_mtx;
    std::condition_variable state_cv;
    const int resource_count;
    std::vector<int> other_nodes;
    std::vector<Message> requests;
    int permits;
    int request_priority;
    int request_number;
private:
protected:
    std::string getStateName() override;

private:

    void changeState(State newState);

    void HandleWhileIdle(Message &message);

    void HandleWhileCompeting(Message &message);

    void HandleWhileAcquired(Message &message);

    void SendAgreement(Message &message);
public:
    RenewableResourceStrategy(int resourceType, int nodeId, Communicator *communicator,
                              int resourceCount, std::vector<int> nodes,
                              std::string resourceName, std::string nodeName);

    void acquire() override;

    void release() override;

    void HandleMessage(Message &message) override;

    [[noreturn]] void run() override;

    void SaveMessageForLater(Message &message);
};


#endif //MYFIRSTDISTRIBUTEDSYSTEM_RENEWABLERESOURCESTRATEGY_H
