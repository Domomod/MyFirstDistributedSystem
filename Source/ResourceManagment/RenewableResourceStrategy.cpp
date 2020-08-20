//
// Created by dominik on 27.05.2020.
//

#include "Communication//Comunicator.h"
#include "RenewableResourceStrategy.h"

#include <utility>

RenewableResourceStrategy::RenewableResourceStrategy(int resourceType, int nodeId, Communicator *communicator,
                                                     int resourceCount, std::vector<int> nodes,
                                                     std::string resourceName, std::string nodeName)
        : AbstractStrategy(resourceType, nodeId, communicator, resourceName, nodeName), resource_count(resourceCount),
        state(IDLE), permits(0), request_number(0), request_priority(0) {

    for(int node : nodes)
    {
        if(node != nodeId)
            other_nodes.push_back(node);
    }
}

void RenewableResourceStrategy::acquire() {
    WriteLog("Tries to acquire the resource");
    std::unique_lock<std::mutex> lock(state_mtx);
    changeState(COMPETING);
    permits = 0;
    request_priority = communicator->getLamportClock();
    request_number++;
    Message msg(Request, node_id, 0, request_number, resource_type, request_priority);
    for (auto node : other_nodes) {
        msg.destination = node;
        communicator->Send(msg);
    }
    state_cv.wait(lock, [=]() { return state == ACQUIRED; });
    WriteLog("Acquired the resource");
}

void RenewableResourceStrategy::release() {
    WriteLog("Tries to release the resouce");
    std::lock_guard<std::mutex> lock(state_mtx);
    changeState(IDLE);
    for (auto &request : requests) {
        SendAgreement(request);
    }
    requests.clear();
    WriteLog("Released the resource");
}

[[noreturn]] void RenewableResourceStrategy::run() {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    while (true) {
        Message msg = communicator->Recieve();
        HandleMessage(msg);
    }
#pragma clang diagnostic pop
}

void RenewableResourceStrategy::HandleMessage(Message &message) {
    std::lock_guard<std::mutex> lock(state_mtx);
    if (state == IDLE) {
        HandleWhileIdle(message);
    } else if (state == COMPETING) {
        HandleWhileCompeting(message);
    } else if (state == ACQUIRED) {
        HandleWhileAcquired(message);
    }

}

void RenewableResourceStrategy::HandleWhileIdle(Message &message) {
    if (message.type == Request) {
        SendAgreement(message);
    }
}

void RenewableResourceStrategy::HandleWhileCompeting(Message &message) {
    if (message.type == Request) {
        if (message.lamport_clock > request_priority ||
            (message.lamport_clock == request_priority && message.sender > node_id)) {
            SaveMessageForLater(message);
        } else {

            SendAgreement(message);
        }
    } else if (message.type == Agree && message.signature == request_number) {
        permits++;
        if (permits + resource_count > other_nodes.size()) {
            changeState(ACQUIRED);
            state_cv.notify_all();
        }
    }
}

void RenewableResourceStrategy::HandleWhileAcquired(Message &message) {
    if (message.type == Request) {
        SaveMessageForLater(message);

    }
}

void RenewableResourceStrategy::SendAgreement(Message &message) {
    communicator->Send(
            Message(Agree, node_id, message.sender, message.signature, resource_type));
}

void RenewableResourceStrategy::changeState(State newState) {
    WriteLog("Changes state to " + StateNames[newState]);
    state = newState;
}

void RenewableResourceStrategy::SaveMessageForLater(Message &message) {
    WriteLog("Saves request " + message.to_str());
    requests.push_back(message);
}

std::string RenewableResourceStrategy::getStateName() {
    return StateNames[state];
}
