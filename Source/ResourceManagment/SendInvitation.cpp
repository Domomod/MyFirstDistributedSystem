#include "SendInvitation.h"

void SendInvitationStrategy::SendInvitations()
{
    Message invitation(
        Invite,
        node_id,
        current_invitation_id
        );
    for(auto target : invitationTargets)
    {
        Communcatior::Send(invitation, target);
    }

    current_invitation_id++;
}

void SendInvitationStrategy::changeState(State newState)
{
    {
        std::lock_guard<std::mutex> lock(state_mtx);
        state = newState;
        std::cout << "[from:" << node_id << "]" << " new state " << StateNames[state] << "\n";
    }
    in_team.notify_all();
}

void SendInvitationStrategy::changeStateUnguarded(State newState)
{
    state = newState;
    std::cout << "[from:" << node_id << "]" << " new state " << StateNames[state] << "\n";
}

bool SendInvitationStrategy::VerifyResponse(Message& message)
{
    return message.type == Agree && message.tag == current_invitation_id;
}

void SendInvitationStrategy::HandleWhileIdle(Message& message)
{
    std::cout << "[Inviting] unexpected message while idle: " << message << "\n";
}

void SendInvitationStrategy::HandleWhileCompeting(Message& message)
{
    if(message.type == Agree)
    {
        teammate_id = message.sender;
        for(auto& target : invitationTargets)
        {
            if(target != teammate_id)
                Communcatior::Send(Message(Reject, node_id, 0), target);
            else
                Communcatior::Send(Message(Agree, node_id, 0), target);
        }
        changeState(IN_TEAM);
        in_team.notify_all();
    }
    else
    {
        std::cout << "[Inviting] unexpected message while competing: " << message << "\n";
    }
}

void SendInvitationStrategy::HandleWhileInTeam(Message& message)
{
    if(message.type == Disband)
    {
        changeState(DISBANDED);
    }
    else
    {
        std::cout << "[Inviting] unexpected message while in_team: " << message << "\n";
    }
}

SendInvitationStrategy::SendInvitationStrategy(int tid, std::vector<int>& invitationTargets) : node_id(tid), invitationTargets(invitationTargets)
{
    current_invitation_id = 0;
    state = IDLE;
}

void SendInvitationStrategy::acquire()
{
    std::cout<<"[from:"<<node_id<<"] acquire()\n";
    { // Ensure IDLE state
        std::unique_lock<std::mutex> lock(state_mtx);
        std::cout<<"[from:"<<node_id<<"] wait == idle\n";
        in_team.wait(lock, [=](){return state == IDLE;});
        changeStateUnguarded(COMPETING);
        SendInvitations();
        std::cout<<"[from:"<<node_id<<"] wait == in_team\n";
        in_team.wait(lock, [=](){return state == IN_TEAM || state == DISBANDED;});
    }
        std::cout<<"[from:"<<node_id<<"] leaving acquire()\n";
}

void SendInvitationStrategy::release()
{
    std::cout<<"[from:"<<node_id<<"] release()\n";
    { // Ensure IN_TEAM state
        std::unique_lock<std::mutex> lock(state_mtx);
        std::cout<<"[from:"<<node_id<<"] wait == idle\n";
        in_team.wait(lock, [=](){return state == DISBANDED;});
        changeStateUnguarded(IDLE);
    }
}

void SendInvitationStrategy::HandleMessage(Message& message)
{
    if(state == IDLE)
        HandleWhileIdle(message);
    else if(state == COMPETING)
        HandleWhileCompeting(message);
    else if(state == IN_TEAM)
        HandleWhileInTeam(message);
}

void SendInvitationStrategy::run()
{
    while(true)
    {
        Message message = Communcatior::Recieve();
        HandleMessage(message);
    }
}
