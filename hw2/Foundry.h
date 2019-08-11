#pragma once
#include "Producer.h"

class Foundry : Producer {
public:
    unsigned int WaitingIronCount = 0;
    unsigned int ProducedIngotCount = 0;
    unsigned int WaitingCoalCount = 0;
    bool isAlive = true;
    unsigned  int deadFoundryCount=0;

    Foundry(int ID, unsigned int capacity, unsigned int interval) : Producer(ID, capacity, interval) {

    }

public:

    void *transporterProducerRoutine(Transporter *transporter, Producer *foundry);

    static void *findHighPriorityFoundry(Transporter *transporter);

    static void *findLowPriorityFoundry(Transporter *transporter);

    static bool isFoundryAlive(Foundry* foundry);

    static void* foundryRoutine(void* foundry);
};