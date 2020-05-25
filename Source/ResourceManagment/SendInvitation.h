#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <iostream>

#include "Communication/Comunicator.h"

class SendInvitationStrategy
{
private:
    enum State
    {
        IDLE,
        COMPETING,
        IN_TEAM,
        DISBANDED,
        NUM_STATES
    } state;
    const std::string StateNames[NUM_STATES] = {"IDLE", "COMPETING", "IN_TEAM", "DISBANDED"};
    int node_id;
    int teammate_id;
    int current_invitation_id;
    std::vector<int> invitationTargets;

    std::mutex state_mtx;
    std::condition_variable in_team;

private:
    void SendInvitations();

    void changeState(State newState);

    void changeStateUnguarded(State newState);

    bool VerifyResponse(Message& message);

    void HandleWhileIdle(Message& message);

    void HandleWhileCompeting(Message& message);

    void HandleWhileInTeam(Message& message);

public:
    SendInvitationStrategy(int tid, std::vector<int>& invitationTargets);

    void acquire();

    void release();

    void HandleMessage(Message& message);

    void run();
};
