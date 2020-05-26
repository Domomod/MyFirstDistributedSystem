//
// Created by dominik on 25.05.2020.
//

#ifndef MYFIRSTDISTRIBUTEDSYSTEM_ABSTRACTSTRATEGY_H
#define MYFIRSTDISTRIBUTEDSYSTEM_ABSTRACTSTRATEGY_H

#include "Communication/Comunicator.h"

class AbstractStrategy {
protected:
    int resource_type;
    int node_id;
public:
    AbstractStrategy(int resourceType, int nodeId) : resource_type(resourceType), node_id(nodeId) {}

    virtual void acquire() = 0;

    virtual void release() = 0;

    virtual void HandleMessage(Message& message) = 0;

    virtual void run() = 0;
};


#endif //MYFIRSTDISTRIBUTEDSYSTEM_ABSTRACTSTRATEGY_H
