#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <iostream>
#include <map>

#include "Communication/Comunicator.h"


#ifndef RECIEVE_INVITATION_STRATEGY_H
#define RECIEVE_INVITATION_STRATEGY_H

class RecieveInvitationStrategy
{
private:
 enum State
    {
        IDLE,
        COMPETING,
        WAITING,
        IN_TEAM,
        NUM_STATES
    } state;
    const std::string StateNames[NUM_STATES]  = {"IDLE", "COMPETING", "WAITING", "IN_TEAM"};
    int node_id;
    int teammate_id;
    std::map<int, Message> invitations;
    std::mutex state_mtx;
    std::condition_variable in_team;
private:
    void changeState(State newState);

    void changeStateUnguarded(State newState);

    void ReplyToInvitation(Message& message);

    bool ReplyFromList();

    void SaveInvitation(Message& message);

    void RemoveInvitation(Message& message);

    void HandleWhileIdle(Message& message);

    void HandleWhileCompeting(Message& message);

    void HandleWhileWaiting(Message& message);

    void HandleWhileInTeam(Message& message);

public:
    RecieveInvitationStrategy(int tid);

    void acquire();

    void release();

    void HandleMessage(Message& message);

    void run();
};

#endif