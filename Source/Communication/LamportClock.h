//
// Created by dominik on 27.05.2020.
//

#ifndef MYFIRSTDISTRIBUTEDSYSTEM_LAMPORTCLOCK_H
#define MYFIRSTDISTRIBUTEDSYSTEM_LAMPORTCLOCK_H


#include "Comunicator.h"

class LamportClock {
private:
    int timeStamp;
public:
    LamportClock() : timeStamp(0) {
    }

    int getTimeStamp()
    {
        return timeStamp;
    }

    void updateTimeStamp(Message& msg)
    {
        timeStamp = std::max(msg.signature, timeStamp) + 1;
    }

    void increaseTimeStamp()
    {
        timeStamp++;
    }
};


#endif //MYFIRSTDISTRIBUTEDSYSTEM_LAMPORTCLOCK_H
