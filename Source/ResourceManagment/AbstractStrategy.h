//
// Created by dominik on 25.05.2020.
//

#ifndef MYFIRSTDISTRIBUTEDSYSTEM_ABSTRACTSTRATEGY_H
#define MYFIRSTDISTRIBUTEDSYSTEM_ABSTRACTSTRATEGY_H

#include "Communication/Comunicator.h"

class AbstractStrategy {
protected:
    Communicator *communicator;
    int resource_type;
    int node_id;
    std::string nodeType;
    std::string resourceName;

    virtual std::string getStateName() = 0;

public:
    AbstractStrategy(int resourceType, int nodeId, Communicator *communicator, std::string resourceName, std::string nodeType)
            : resource_type(resourceType), node_id(nodeId), communicator(communicator), resourceName(resourceName), nodeType(nodeType) {}

    virtual void acquire() = 0;

    virtual void release() = 0;

    virtual void HandleMessage(Message &message) = 0;

    virtual void run() = 0;

    void WriteLog(std::string msg) {
        std::cout << nodeType << " " << node_id << " [" << resourceName << ", "  << getStateName() << "]:\t" << msg << "\n";
    }
};


#endif //MYFIRSTDISTRIBUTEDSYSTEM_ABSTRACTSTRATEGY_H
