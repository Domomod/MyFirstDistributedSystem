#include "ReceiveInvitation.h"

RecieveInvitationStrategy::RecieveInvitationStrategy(int resourceType, int nodeId, Communicator *communicator) : AbstractStrategy(resourceType,
                                                                                                      nodeId, communicator) {
    state = IDLE;
}


void RecieveInvitationStrategy::acquire() {
    std::unique_lock<std::mutex> lock(state_mtx);
    in_team.wait(lock, [=]() { return state == IDLE; });
    if (ReplyFromList())
        changeStateUnguarded(WAITING);
    else
        changeStateUnguarded(COMPETING);
    in_team.wait(lock, [=]() { return state == IN_TEAM; });
}


void RecieveInvitationStrategy::release() {
    {
        std::unique_lock<std::mutex> lock(state_mtx);
        in_team.wait(lock, [=]() { return state == IN_TEAM; });
        changeStateUnguarded(IDLE);
        communicator->Send(Message(Disband, node_id, teammate_id, accepted_invitation_id, resource_type));
    }
    in_team.notify_all();
}

void RecieveInvitationStrategy::HandleMessage(Message &message) {
    std::lock_guard<std::mutex> lock(state_mtx);

    if (state == IDLE)
        HandleWhileIdle(message);
    else if (state == COMPETING)
        HandleWhileCompeting(message);
    else if (state == WAITING)
        HandleWhileWaiting(message);
    else if (state == IN_TEAM)
        HandleWhileInTeam(message);
}

void RecieveInvitationStrategy::run() {
    while (true) {
        Message message = communicator->Recieve();
        HandleMessage(message);
    }
};

void RecieveInvitationStrategy::HandleWhileIdle(Message &message) {
    if (message.type == Request) {
        SaveInvitation(message);
    } else if (message.type == Reject) {
        RemoveInvitation(message);
    } else {
        std::cout << "[Node " << node_id << "] unexpected message while idle: " << message << "\n";
    }
}

void RecieveInvitationStrategy::HandleWhileCompeting(Message &message) {
    if (message.type == Request) {
        changeStateUnguarded(WAITING);
        ReplyToInvitation(message);
    } else if (message.type == Reject) {
        RemoveInvitation(message);
    } else {
        std::cout << "[Node " << node_id << "] unexpected message while competing: " << message << "\n";
    }
}

void RecieveInvitationStrategy::HandleWhileWaiting(Message &message) {
    if (message.type == Request) {
        SaveInvitation(message);
    } else if (message.type == Reject) {
        RemoveInvitation(message);
        if (teammate_id == message.sender) {
            if (!ReplyFromList())
                changeStateUnguarded(COMPETING);
        }
    } else if (message.type == Agree) {
        changeStateUnguarded(IN_TEAM);
        in_team.notify_all();
    } else {
        std::cout << "[Node " << node_id << "] unexpected message while waiting: " << message << "\n";
    }
}

void RecieveInvitationStrategy::HandleWhileInTeam(Message &message) {
    if (message.type == Request) {
        SaveInvitation(message);
    } else if (message.type == Reject) {
        RemoveInvitation(message);
    } else {
        std::cout << "[Node " << node_id << "] unexpected message while in team: " << message << "\n";
    }
}

void RecieveInvitationStrategy::changeStateUnguarded(State newState) {
    state = newState;
    std::cout << "[Node:" << node_id << "]" << " new state " << StateNames[state] << "\n";

}

void RecieveInvitationStrategy::ReplyToInvitation(Message &message) {
    if (message.type == Request) {
        teammate_id = message.sender;
        accepted_invitation_id = message.signature;
        communicator->Send(Message(Agree, node_id, teammate_id, accepted_invitation_id, resource_type));
    }
}

bool RecieveInvitationStrategy::ReplyFromList() {

    if (!invitations.empty()) {
        Message invitation = (*invitations.begin()).second;
        invitations.erase(invitations.begin());
        ReplyToInvitation(invitation);
        return true;
    }
    return false;
}

void RecieveInvitationStrategy::SaveInvitation(Message &message) {
    //Communication is Fifo, every two invitations (from the same node!) are separated by a rejection or agreement.
    invitations.emplace(message.sender, message);
}

void RecieveInvitationStrategy::RemoveInvitation(Message &message) {
    invitations.erase(message.sender);
}