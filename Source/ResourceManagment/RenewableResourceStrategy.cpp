//
// Created by dominik on 27.05.2020.
//

#include "Communication//Comunicator.h"
#include "RenewableResourceStrategy.h"

#include <utility>

RenewableResourceStrategy::RenewableResourceStrategy(int resourceType, int nodeId, Communicator *communicator,
                                                     int resourceCount,
                                                     std::vector<int> otherNodes)
        : AbstractStrategy(resourceType, nodeId, communicator), resource_count(resourceCount),
          other_nodes(std::move(otherNodes)), state(IDLE), permits(0), request_number(0), request_priority(0) {
}

void RenewableResourceStrategy::acquire() {
    std::unique_lock<std::mutex> lock(state_mtx);
    state = COMPETING;
    permits = 0;
    std::cout << "[Node " << node_id << "] changed state to COMPETING \n";

    request_priority = communicator->getLamportClock();
    request_number++;
    Message msg(Request, node_id, 0, request_number, resource_type, request_priority);
    for (auto node : other_nodes) {
        msg.destination = node;
        communicator->Send(msg);
    }
    state_cv.wait(lock, [=]() { return state == ACQUIRED; });
}

void RenewableResourceStrategy::release() {
    std::lock_guard<std::mutex> lock(state_mtx);
    state = IDLE;
    std::cout << "[Node " << node_id << "] changed state to IDLE \n";
    for (auto &request : requests) {
        SendAgreement(request);
    }
    requests.clear();
    std::cout << "[Node " << node_id << "] release() - step out\n";
}

void RenewableResourceStrategy::run() {
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
            SendAgreement(message);
        } else {
            std::cout << "[Node " << node_id << "] keeps request: " << message << "\n";
            requests.push_back(message);
        }
    } else if (message.type == Agree && message.signature == request_number) {
        permits++;
        std::cout << "[Node " << node_id << "] Agreement has matching request number\n";
        if (permits + resource_count > other_nodes.size()) {
            state = ACQUIRED;
            std::cout << "[Node " << node_id << "] changed state to ACQUIRED\n";
            state_cv.notify_all();
        }
    }
}

void RenewableResourceStrategy::HandleWhileAcquired(Message &message) {
    if (message.type == Request) {
        std::cout << "[Node " << node_id << "] keeps request: " << message << "\n";
        requests.push_back(message);
    }
}

void RenewableResourceStrategy::SendAgreement(Message &message) {
    communicator->Send(
            Message(Agree, node_id, message.sender, message.signature, resource_type));
}

