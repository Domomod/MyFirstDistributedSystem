#include "ReceiveInvitation.h"

void RecieveInvitationStrategy::changeState(State newState)
{
    {
        std::lock_guard<std::mutex> lock(state_mtx);
        state = newState;
        std::cout << "[from:" << node_id << "]" << " new state " << StateNames[state] << "\n";
    }
    in_team.notify_all();
}

void RecieveInvitationStrategy::changeStateUnguarded(State newState)
{
    state = newState;
    std::cout << "[from:" << node_id << "]" << " new state " << StateNames[state] << "\n";

}

void RecieveInvitationStrategy::ReplyToInvitation(Message& message)
{
    if(message.type == Invite)
    {
        teammate_id = message.sender;
        Communcatior::Send(Message(Agree ,node_id, message.tag), message.sender);
    }
}

bool RecieveInvitationStrategy::ReplyFromList()
{
    if(!invitations.empty())
    {
        Message invitation = (*invitations.begin()).second;
        invitations.erase(invitations.begin());
        ReplyToInvitation(invitation);
        return true;
    }
    return false;
}

void RecieveInvitationStrategy::SaveInvitation(Message& message)
{
    //Communication is Fifo, every two invitations (from the same node!) are separated by a rejection or agreement.
    invitations.emplace(message.sender, message);
}

void RecieveInvitationStrategy::RemoveInvitation(Message& message)
{
    invitations.erase(message.sender);
}


    void RecieveInvitationStrategy::HandleWhileIdle(Message& message)
    {
        if(message.type == Invite)
        {
            SaveInvitation(message);
        }
        else if(message.type == Reject)
        {
            RemoveInvitation(message);
        }
        else
        {
            std::cout << "[Responding] unexpected message while idle: " << message << "\n";
        }
    }
    void RecieveInvitationStrategy::HandleWhileCompeting(Message& message)
    {
        if(message.type == Invite)
        {
            changeState(WAITING);
            ReplyToInvitation(message);
        }
        else if(message.type == Reject)
        {
            RemoveInvitation(message);
        }
        else
        {
            std::cout << "[Responding] unexpected message while competing: " << message << "\n";
        }
    }

    void RecieveInvitationStrategy::HandleWhileWaiting(Message& message)
    {
        if(message.type == Invite)
        {
            SaveInvitation(message);
        }
        else if(message.type == Reject)
        {
            RemoveInvitation(message);
            if(teammate_id == message.sender)
                if(!ReplyFromList())
                    changeState(COMPETING);
        }
        else if(message.type == Agree)
        {
            changeState(IN_TEAM);
            in_team.notify_all();
        }
        else
        {
            std::cout << "[Responding] unexpected message while waiting: " << message << "\n";
        }
    }

    void RecieveInvitationStrategy::HandleWhileInTeam(Message& message)
    {
        if(message.type == Invite)
        {
            SaveInvitation(message);
        }
        else if(message.type == Reject)
        {
            RemoveInvitation(message);
        }
        else
        {
            std::cout << "[Responding] unexpected message while in team: " << message << "\n";
        }
    }

    RecieveInvitationStrategy::RecieveInvitationStrategy(int tid) : node_id(tid)
    {
        state = IDLE;
    }

    void RecieveInvitationStrategy::acquire()
    {
        std::unique_lock<std::mutex> lock(state_mtx);
        in_team.wait(lock, [=](){return state == IDLE;});
        if(ReplyFromList())
            changeStateUnguarded(WAITING);
        else
            changeStateUnguarded(COMPETING);
        in_team.wait(lock, [=](){return state == IN_TEAM;});
    }


    void RecieveInvitationStrategy::release()
    {
        {
            std::unique_lock<std::mutex> lock(state_mtx);
            in_team.wait(lock, [=](){return state == IN_TEAM;});
            changeStateUnguarded(IDLE);
            Communcatior::Send(Message(Disband, node_id, 0), teammate_id);
        }
        in_team.notify_all();
    }

    void RecieveInvitationStrategy::HandleMessage(Message& message)
    {
        if(state == IDLE)
            HandleWhileIdle(message);
        else if(state == COMPETING)
            HandleWhileCompeting(message);
        else if(state == WAITING)
            HandleWhileWaiting(message);
        else if(state == IN_TEAM)
            HandleWhileInTeam(message);
    }

    void RecieveInvitationStrategy::run()
    {
        while(true)
        {
            Message message = Communcatior::Recieve();
            HandleMessage(message);
        }
    };
