#include "ReceiveInvitation.h"

RecieveInvitationStrategy::RecieveInvitationStrategy(int resourceType, int nodeId, Communicator *communicator,
                                                     std::string resourceName, std::string nodeName) : AbstractStrategy(resourceType,
                                                                                                      nodeId, communicator, resourceName, nodeName) {
    state = IDLE;
}


void RecieveInvitationStrategy::acquire() {
    WriteLog("Tries to acquire the resource");
    std::unique_lock<std::mutex> lock(state_mtx);
    in_team.wait(lock, [=]() { return state == IDLE; });
    if (ReplyFromList())
        changeState(WAITING);
    else
        changeState(COMPETING);
    in_team.wait(lock, [=]() { return state == IN_TEAM; });
    WriteLog("Acquired the resource");
}


void RecieveInvitationStrategy::release() {
    {
        WriteLog("Is ordered to release the resource");
        std::unique_lock<std::mutex> lock(state_mtx);
        in_team.wait(lock, [=]() { return state == IN_TEAM; });
        changeState(IDLE);
        communicator->Send(Message(Disband, node_id, teammate_id, accepted_invitation_id, resource_type));
    }
    in_team.notify_all();
    WriteLog("Released the resource");
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
        UnexpectedMessage(message);
    }
}

void RecieveInvitationStrategy::HandleWhileCompeting(Message &message) {
    if (message.type == Request) {
        changeState(WAITING);
        ReplyToInvitation(message);
    } else if (message.type == Reject) {
        RemoveInvitation(message);
    } else {
        UnexpectedMessage(message);
    }
}

void RecieveInvitationStrategy::HandleWhileWaiting(Message &message) {
    if (message.type == Request) {
        SaveInvitation(message);
    } else if (message.type == Reject) {
        RemoveInvitation(message);
        if (teammate_id == message.sender) {
            if (!ReplyFromList())
                changeState(COMPETING);
        }
    } else if (message.type == Agree) {
        changeState(IN_TEAM);
        in_team.notify_all();
    } else {
        UnexpectedMessage(message);
    }
}

void RecieveInvitationStrategy::HandleWhileInTeam(Message &message) {
    if (message.type == Request) {
        SaveInvitation(message);
    } else if (message.type == Reject) {
        RemoveInvitation(message);
    } else {
        UnexpectedMessage(message);
    }
}

void RecieveInvitationStrategy::UnexpectedMessage(Message &message) { WriteLog("Recieved unexpected msg: " + message.to_str()); }

void RecieveInvitationStrategy::changeState(State newState) {
    WriteLog("Changes state to " + StateNames[newState]);
    state = newState;
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

std::string RecieveInvitationStrategy::getStateName() {
    return StateNames[state];
}
