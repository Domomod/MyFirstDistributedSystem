//
// Created by dominik on 27.05.2020.
//

#ifndef MYFIRSTDISTRIBUTEDSYSTEM_CONSUMABLERESOURCESTRATEGY_H
#define MYFIRSTDISTRIBUTEDSYSTEM_CONSUMABLERESOURCESTRATEGY_H

#include <vector>
#include <list>
#include <mutex>
#include <condition_variable>

#include "Communication/LamportClock.h"
#include "AbstractStrategy.h"

class ConsumableResourceStrategy : public AbstractStrategy{
private:
    enum State{
        IDLE,
        COMPETING,
        ACQUIRED,
        STATE_COUNT
    } state;

    std::string StateNames[STATE_COUNT] = {"IDLE","COMPETING", "ACQUIRED"};

    std::mutex state_mtx;
    std::condition_variable state_cv;
    std::vector<int> other_nodes;
    std::vector<Message> requests;
    std::string nodeType;
    int permits;
    int resource_count;
    int request_priority;
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
    ConsumableResourceStrategy(int resourceType, int nodeId, Communicator *communicator,
                               int resourceCount, std::vector<int> nodes,
                               std::string nodeType, std::string resourceName);

    void acquire() override;

    void release() override;

    void HandleMessage(Message &message) override;

    void run() override;

    [[noreturn]] void produceResourceThread();

    void SaveMessageForLater(Message &message);

    void CheckIfAcquired();
};


#endif //MYFIRSTDISTRIBUTEDSYSTEM_CONSUMABLERESOURCESTRATEGY_H
