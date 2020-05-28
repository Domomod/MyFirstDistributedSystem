#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <iostream>

#include "Communication/Comunicator.h"
#include "ResourceManagment/AbstractStrategy.h"

class SendInvitationStrategy : public  AbstractStrategy
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
    int teammate_id;
    int current_invitation_id;
    std::vector<int> invitationTargets;

    std::mutex state_mtx;
    std::condition_variable in_team;

private:
    void SendInvitations();

    void changeStateUnguarded(State newState);

    bool VerifyResponse(Message& message);

    void HandleWhileIdle(Message& message);

    void HandleWhileCompeting(Message& message);

    void HandleWhileInTeam(Message& message);

public:
    SendInvitationStrategy(int resourceType, int nodeId, Communicator *communicator, const std::vector<int> &invitationTargets);

    void acquire();

    void release();

    void HandleMessage(Message& message);

    void run();
};
