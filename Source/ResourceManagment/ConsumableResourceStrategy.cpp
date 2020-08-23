//
// Created by dominik on 27.05.2020.
//

#include "Communication//Comunicator.h"
#include "ConsumableResourceStrategy.h"

#include <utility>
#include <unistd.h>

ConsumableResourceStrategy::ConsumableResourceStrategy(int resourceType, int nodeId, Communicator *communicator,
                                                       int resourceCount, std::vector<int> nodes,
                                                       std::string nodeType, std::string resourceName)
        : AbstractStrategy(resourceType, nodeId, communicator, resourceName, nodeType), resource_count(resourceCount),
          state(IDLE), permits(0), request_priority(0), nodeType(nodeType) {
    for(int node : nodes)
    {
        if(node != nodeId)
            other_nodes.push_back(node);
    }
}

void ConsumableResourceStrategy::acquire() {
    std::unique_lock<std::mutex> lock(state_mtx);
    WriteLog("Tries to acquire the resource");
    changeState(COMPETING);
    permits = 0;
    request_priority = communicator->getLamportClock();
    Message msg(Request, node_id, 0, 0, resource_type, request_priority);
    communicator->Broadcast(msg, other_nodes);
    state_cv.wait(lock, [=]() { return state == ACQUIRED; });
    WriteLog("Acquired the resource");
}

void ConsumableResourceStrategy::release() {
    std::lock_guard<std::mutex> lock(state_mtx);
    WriteLog("Tries to release the resource");
    changeState(IDLE);
    resource_count--;

    for (auto &request : requests) {
        request.signature = -1;
        SendAgreement(request);
    }
    requests.clear();
    WriteLog("Released the resource");
}

void ConsumableResourceStrategy::run() {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    while (true) {
        Message msg = communicator->Recieve();
        HandleMessage(msg);
    }
#pragma clang diagnostic pop
}

void ConsumableResourceStrategy::HandleMessage(Message &message) {
    std::lock_guard<std::mutex> lock(state_mtx);
    if (state == IDLE) {
        HandleWhileIdle(message);
    } else if (state == COMPETING) {
        HandleWhileCompeting(message);
    } else if (state == ACQUIRED) {
        HandleWhileAcquired(message);
    }
}

void ConsumableResourceStrategy::HandleWhileIdle(Message &message) {
    if (message.type == Request) {
        SendAgreement(message);
    } else if (message.type == Increment) {
        resource_count += message.signature;
    }
}

void ConsumableResourceStrategy::HandleWhileCompeting(Message &message) {
    if (message.type == Request) {
        if (message.lamport_clock > request_priority ||
            (message.lamport_clock == request_priority && message.sender > node_id)) {
            std::cout << message.lamport_clock <<">"<< request_priority <<"||"<<message.lamport_clock <<"=="<< request_priority <<"&&"<< message.sender <<">"<< node_id<<"\n";
            SaveMessageForLater(message);
        } else {
            SendAgreement(message);
        }
    } else if (message.type == Agree) {
        if (message.signature == -1)
            resource_count--;
        permits++;
        CheckIfAcquired();
    } else if (message.type == Increment) {
        resource_count += message.signature;
        CheckIfAcquired();
    }
}

void ConsumableResourceStrategy::HandleWhileAcquired(Message &message) {
    if (message.type == Request) {
        SaveMessageForLater(message);
    } else if (message.type == Increment) {
        resource_count += message.signature;
    }
}

void ConsumableResourceStrategy::CheckIfAcquired() {
    if (permits == other_nodes.size() && resource_count > 0) {
        changeState(ACQUIRED);
        state_cv.notify_all();
    }
}

void ConsumableResourceStrategy::SaveMessageForLater(Message &message) {
    WriteLog("Saves request" + message.to_str());
    requests.push_back(message);
}

void ConsumableResourceStrategy::SendAgreement(Message &message) {
    communicator->Send(
            Message(Agree, node_id, message.sender, message.signature, resource_type));
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

[[noreturn]] void ConsumableResourceStrategy::produceResourceThread() {
    while(true)
    {
        {
            std::lock_guard<std::mutex> lock(state_mtx);
            WriteLog("Broadcasts new resources");
            Message msg(Increment, node_id, 0, std::rand()%4 + 1, resource_type, request_priority);
            for (auto node : other_nodes) {
                msg.destination = node;
                communicator->Send(msg);
            }

            resource_count += msg.signature;
            if (state == COMPETING && permits == other_nodes.size() && resource_count > 0) {
                state = ACQUIRED;
                std::cout << "[Node " << node_id << "] changed state to ACQUIRED\n";
                state_cv.notify_all();
            }
        }
        sleep(4);
    }

}

std::string ConsumableResourceStrategy::getStateName() {
    return StateNames[state];
}

void ConsumableResourceStrategy::changeState(State newState) {
    WriteLog("Changes state to " + StateNames[newState]);
    state = newState;
}


#pragma clang diagnostic pop

