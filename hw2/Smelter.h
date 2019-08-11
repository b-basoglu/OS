#pragma once
#include "Producer.h"

class Smelter : Producer{
    public:
        OreType oreType;
        unsigned int ProducedIngotCount = 0;
        unsigned int WaitingOreCount = 0;
        bool isAlive = true;
        unsigned  int deadSmelterCount=0;

        Smelter(int ID,OreType oreType,unsigned int capacity,unsigned int interval):Producer(ID,capacity,interval){
            this->oreType=oreType;
        }

public:
    void* transporterProducerRoutine(Transporter* transporter,Producer* smelter);
    static void * findHighPrioritySmelter(Transporter* transporter);
    static void * findLowPrioritySmelter(Transporter* transporter);
    static bool isSmelterAlive(Smelter* smelter);
    static void* smelterRoutine(void* smelter);

};