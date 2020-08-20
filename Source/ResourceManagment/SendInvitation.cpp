#include "SendInvitation.h"

SendInvitationStrategy::SendInvitationStrategy(int resourceType, int nodeId, Communicator *communicator,
                                               const std::vector<int> &invitationTargets, std::string nodeName,
                                               std::string resourceName)
        : AbstractStrategy(resourceType, nodeId, communicator, resourceName, nodeName),
        teammate_id(0),
        current_invitation_id(0),
        state(IDLE),
        invitationTargets(invitationTargets) {
}

void SendInvitationStrategy::acquire() {
    WriteLog("Tries to acquire the resource");
    { // Ensure IDLE state
        std::unique_lock<std::mutex> lock(state_mtx);
        WriteLog("Awaits IDLE state");
        in_team.wait(lock, [=]() { return state == IDLE; });
        changeStateUnguarded(COMPETING);
        SendInvitations();
        WriteLog("Awaits IN_TEAM state");
        in_team.wait(lock, [=]() { return state == IN_TEAM; });
    }
    WriteLog("Acquired the resource");
}

void SendInvitationStrategy::release() {
    WriteLog("Tries to release the resource");
    { // Ensure IN_TEAM state
        communicator->Send(Message(Agree, node_id, teammate_id, current_invitation_id, resource_type));
        std::unique_lock<std::mutex> lock(state_mtx);
        WriteLog("Awaits IDLE state");
        in_team.wait(lock, [=]() { return state == DISBANDED; });
        changeStateUnguarded(IDLE);
    }
    WriteLog("Released the resource");
}

[[noreturn]] void SendInvitationStrategy::run() {
    while (true) {
        Message message = communicator->Recieve();
        HandleMessage(message);
    }
}

void SendInvitationStrategy::HandleMessage(Message &message) {
    std::lock_guard<std::mutex> lock(state_mtx);

    if (state == IDLE)
        HandleWhileIdle(message);
    else if (state == COMPETING)
        HandleWhileCompeting(message);
    else if (state == IN_TEAM)
        HandleWhileInTeam(message);
}

void SendInvitationStrategy::HandleWhileIdle(Message &message) {
    UnexpectedMessage(message);
}

void SendInvitationStrategy::UnexpectedMessage(Message &message){
    WriteLog("Recieved unexpected message " + message.to_str());
}

void SendInvitationStrategy::HandleWhileCompeting(Message &message) {
    if (message.type == Agree) {
        teammate_id = message.sender;
        for (auto &target : invitationTargets) {
            if (target != teammate_id)
                communicator->Send(Message(Reject, node_id, target, current_invitation_id, resource_type));
        }
        changeStateUnguarded(IN_TEAM);
        in_team.notify_all();
    } else {
        UnexpectedMessage(message);
    }
}

void SendInvitationStrategy::HandleWhileInTeam(Message &message) {
    if (message.type == Disband) {
        changeStateUnguarded(DISBANDED);
        in_team.notify_all();
    } else {
        UnexpectedMessage(message);
    }
}

void SendInvitationStrategy::SendInvitations() {
    Message invitation(
            Request,
            node_id,
            0,
            current_invitation_id,
            resource_type
    );
    for (auto target : invitationTargets) {
        invitation.destination = target;
        communicator->Send(invitation);
    }

    current_invitation_id++;
}

void SendInvitationStrategy::changeStateUnguarded(State newState) {
    WriteLog("Changes state to " + StateNames[newState]);
    state = newState;
}

bool SendInvitationStrategy::VerifyResponse(Message &message) {
    return message.type == Agree && message.signature == current_invitation_id;
}

std::string SendInvitationStrategy::getStateName() {
    return StateNames[state];
}
