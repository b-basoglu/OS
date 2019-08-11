#include "Transporter.h"

#pragma once
class Producer {
protected:
    unsigned int ID;
    unsigned int capacity;
    unsigned int interval;

    Producer(int ID,unsigned int capacity,unsigned int interval){
        this->ID = ID;
        this->capacity =capacity;
        this->interval = interval;
    }

public:
    virtual void* transporterProducerRoutine(Transporter* transporter,Producer* producer)=0;

};