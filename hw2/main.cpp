#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <sys/wait.h> 
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <vector>
#include <errno.h>
#include "writeOutput.h"
#include "Miner.h"
#include <tuple>
#ifndef  FILE_H
#include "Producer.h"
#endif
#include "Smelter.h"
#include "Foundry.h"
using namespace std;

    vector<Miner *> vecMiners;
    vector<Transporter *> vecTransporters;
    vector<Smelter *> vecSmelters;
    vector<Foundry *> vecFoundries;
    MinersInfo* minersInfo;


    sem_t* minerEmpty;
    sem_t* minerFull;
    pthread_mutex_t* minerMutex;
    sem_t minersHasOre;
    pthread_mutex_t deadMinerLock;

    pthread_mutex_t* transporterCarriedOreMutex;
    pthread_mutex_t waitNextLoadMutex;

    pthread_mutex_t* smelterWaitingOreMutex;
    pthread_mutex_t* isSmelterAliveMutex;
    sem_t* waitUntilTwoOres;

    pthread_mutex_t* foundryWaitingOreMutex;
    pthread_mutex_t* isFoundryAliveMutex;
    sem_t* foundryWaitingIronSem;
    sem_t* foundryWaitingCoalSem;

    sem_t ironWait;
    sem_t copperWait;
    sem_t coalWait;



void* Miner::minerRoutine(void* miner)
{

    MinerInfo m_i;
    unsigned int time = ((Miner*)miner)->interval;
    
    FillMinerInfo(&m_i,((Miner*)miner)->ID, ((Miner*)miner)->oreType, ((Miner*)miner)->capacity,((Miner*)miner)->currentOreCount);
    WriteOutput(&m_i, NULL, NULL, NULL, MINER_CREATED);
    
    while(((Miner*)miner)->totalOre){
        
        sem_wait(&minerEmpty[((Miner*)miner)->ID-1]);
        

        FillMinerInfo(&m_i,((Miner*)miner)->ID, ((Miner*)miner)->oreType, ((Miner*)miner)->capacity,((Miner*)miner)->currentOreCount);
        WriteOutput(&m_i, NULL, NULL, NULL, MINER_STARTED);
        usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));
        
        pthread_mutex_lock(&minerMutex[((Miner*)miner)->ID-1]);
        (((Miner*)miner)->currentOreCount)++;
        ((Miner*)miner)->totalOre--;

        if (((Miner*)miner)->totalOre <= 0){
            pthread_mutex_lock(&deadMinerLock);
            ((Miner*)miner)->isActive = false;

            pthread_mutex_unlock(&deadMinerLock);
        }
        pthread_mutex_unlock(&minerMutex[((Miner*)miner)->ID-1]);

        sem_post(&minersHasOre);


        FillMinerInfo(&m_i,((Miner*)miner)->ID, ((Miner*)miner)->oreType, ((Miner*)miner)->capacity,((Miner*)miner)->currentOreCount);
        WriteOutput(&m_i, NULL, NULL, NULL, MINER_FINISHED);
        usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));



    }

    FillMinerInfo(&m_i,((Miner*)miner)->ID, ((Miner*)miner)->oreType, ((Miner*)miner)->capacity,((Miner*)miner)->currentOreCount);
    
    WriteOutput(&m_i, NULL, NULL, NULL, MINER_STOPPED);
   
    return NULL;
}
Miner* Miner::waitNextLoad(){
    
    pthread_mutex_lock(&waitNextLoadMutex);
    if(minersInfo->minerReminder == (minersInfo->minerCount-1)){
        minersInfo->minerReminder = 0;
    }
    else{
        minersInfo->minerReminder++;
    }
    for(int i = minersInfo->minerReminder;i<minersInfo->minerCount;i++){
        pthread_mutex_lock(&minerMutex[i]);
        if(vecMiners[i]->currentOreCount>0){
            minersInfo->minerReminder=i;
            pthread_mutex_unlock(&waitNextLoadMutex);
            return vecMiners[i];
        }
        pthread_mutex_unlock(&minerMutex[i]);
    }

    for(int i = 0;i<minersInfo->minerReminder;i++){
        pthread_mutex_lock(&minerMutex[i]);
        if(vecMiners[i]->currentOreCount>0){
            
            minersInfo->minerReminder=i;
            pthread_mutex_unlock(&waitNextLoadMutex);
            return vecMiners[i];
        }
        pthread_mutex_unlock(&minerMutex[i]);
    }
    for(int i = 0;i<minersInfo->minerCount;i++){
        pthread_mutex_lock(&minerMutex[i]);
        if(vecMiners[i]->currentOreCount>0){
            minersInfo->minerReminder=i;
            pthread_mutex_unlock(&waitNextLoadMutex);
            return vecMiners[i];
        }
        pthread_mutex_unlock(&minerMutex[i]);
    }

    pthread_mutex_unlock(&waitNextLoadMutex);
    return NULL;
}
bool Miner::checkEmpty(){
    for(Miner* miner:vecMiners){
        pthread_mutex_lock(&deadMinerLock);
        pthread_mutex_lock(&minerMutex[((Miner*)miner)->ID-1]);
        if(miner->isActive || miner->currentOreCount>0){

            pthread_mutex_unlock(&minerMutex[((Miner*)miner)->ID-1]);
            pthread_mutex_unlock(&deadMinerLock);
            return true;
        }
        pthread_mutex_unlock(&minerMutex[((Miner*)miner)->ID-1]);
        pthread_mutex_unlock(&deadMinerLock);
    }
    return false;
}

void* Foundry::foundryRoutine(void* foundry)
{

    unsigned int time = ((Foundry*)foundry)->interval;
    timespec ts;


    FoundryInfo f_i;

    FillFoundryInfo( &f_i, ((Foundry*)foundry)->ID , ((Foundry*)foundry)->capacity, ((Foundry*)foundry)->WaitingIronCount, ((Foundry*)foundry)->WaitingCoalCount, ((Foundry*)foundry)->ProducedIngotCount );

    WriteOutput(NULL, NULL, NULL, &f_i, FOUNDRY_CREATED);

    while(1){
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 5;

        sem_timedwait(&foundryWaitingIronSem[((Foundry*)foundry)->ID -1], &ts);
        sem_timedwait(&foundryWaitingCoalSem[((Foundry*)foundry)->ID -1], &ts);

        if(errno == ETIMEDOUT){
            break;
        }
        pthread_mutex_lock(&foundryWaitingOreMutex[((Foundry*)foundry)->ID -1]);
        ((Foundry*)foundry)->WaitingIronCount--;
        ((Foundry*)foundry)->WaitingCoalCount--;
        pthread_mutex_unlock(&foundryWaitingOreMutex[((Foundry*)foundry)->ID -1]);

        FillFoundryInfo( &f_i, ((Foundry*)foundry)->ID , ((Foundry*)foundry)->capacity, ((Foundry*)foundry)->WaitingIronCount, ((Foundry*)foundry)->WaitingCoalCount, ((Foundry*)foundry)->ProducedIngotCount );
        WriteOutput(NULL, NULL, NULL, &f_i, FOUNDRY_STARTED);
        usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));

        ((Foundry*)foundry)->ProducedIngotCount++;


        sem_post(&coalWait);
        sem_post(&coalWait);
        sem_post(&ironWait);
        sem_post(&ironWait);

        FillFoundryInfo( &f_i, ((Foundry*)foundry)->ID , ((Foundry*)foundry)->capacity, ((Foundry*)foundry)->WaitingIronCount, ((Foundry*)foundry)->WaitingCoalCount, ((Foundry*)foundry)->ProducedIngotCount );
        WriteOutput(NULL, NULL, NULL, &f_i, FOUNDRY_FINISHED);
    }
    pthread_mutex_lock(&isFoundryAliveMutex[((Foundry*)foundry)->ID-1]);
    (((Foundry*)foundry)->isAlive)=false;
    pthread_mutex_unlock(&isFoundryAliveMutex[((Foundry*)foundry)->ID-1]);

    FillFoundryInfo( &f_i, ((Foundry*)foundry)->ID , ((Foundry*)foundry)->capacity, ((Foundry*)foundry)->WaitingIronCount, ((Foundry*)foundry)->WaitingCoalCount,((Foundry*)foundry)->ProducedIngotCount );
    WriteOutput(NULL, NULL, NULL, &f_i, FOUNDRY_STOPPED);


    return NULL;
}

void* Smelter::smelterRoutine(void* smelter)
{


    timespec ts;
    unsigned int time = ((Smelter*)smelter)->interval;


    SmelterInfo s_i;
    FillSmelterInfo(&s_i,((Smelter*)smelter)->ID, ((Smelter*)smelter)->oreType, ((Smelter*)smelter)->capacity,((Smelter*)smelter)->WaitingOreCount,((Smelter*)smelter)->ProducedIngotCount);
    WriteOutput(NULL, NULL, &s_i, NULL, SMELTER_CREATED);



    while(1){
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 5;
        sem_timedwait(&waitUntilTwoOres[((Smelter*)smelter)->ID-1],&ts);
        sem_timedwait(&waitUntilTwoOres[((Smelter*)smelter)->ID-1],&ts);
        if(errno == ETIMEDOUT){
            break;
        }
        pthread_mutex_lock(&smelterWaitingOreMutex[((Smelter*)smelter)->ID -1]);
        ((Smelter*)smelter)->WaitingOreCount=((Smelter*)smelter)->WaitingOreCount-2;
        pthread_mutex_unlock(&smelterWaitingOreMutex[((Smelter*)smelter)->ID -1]);


        FillSmelterInfo(&s_i,((Smelter*)smelter)->ID, ((Smelter*)smelter)->oreType, ((Smelter*)smelter)->capacity,((Smelter*)smelter)->WaitingOreCount,((Smelter*)smelter)->ProducedIngotCount);
        WriteOutput(NULL, NULL, &s_i, NULL, SMELTER_STARTED);
        usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));



        ((Smelter*)smelter)->ProducedIngotCount ++;


        if(((Smelter*)smelter)->oreType == IRON){
            sem_post(&ironWait);
            sem_post(&ironWait);
        } else {
            sem_post(&copperWait);
            sem_post(&copperWait);
        }


        FillSmelterInfo(&s_i,((Smelter*)smelter)->ID, ((Smelter*)smelter)->oreType, ((Smelter*)smelter)->capacity,((Smelter*)smelter)->WaitingOreCount,((Smelter*)smelter)->ProducedIngotCount);
        WriteOutput(NULL, NULL, &s_i, NULL, SMELTER_FINISHED);


    }
    pthread_mutex_lock(&isSmelterAliveMutex[((Smelter*)smelter)->ID-1]);
    (((Smelter*)smelter)->isAlive) = false;
    pthread_mutex_unlock(&isSmelterAliveMutex[((Smelter*)smelter)->ID-1]);

    FillSmelterInfo(&s_i,((Smelter*)smelter)->ID, ((Smelter*)smelter)->oreType, ((Smelter*)smelter)->capacity,((Smelter*)smelter)->WaitingOreCount,((Smelter*)smelter)->ProducedIngotCount);
    WriteOutput(NULL, NULL, &s_i, NULL, SMELTER_STOPPED);

    return NULL;

}

bool Transporter::isTransporterCarry(Transporter* transporter,OreType oreType){
    pthread_mutex_lock(&transporterCarriedOreMutex[((Transporter*)transporter)->ID-1]);
    if(transporter->carriedOre == oreType){
        pthread_mutex_unlock(&transporterCarriedOreMutex[((Transporter*)transporter)->ID-1]);
        return true;
    }
    pthread_mutex_unlock(&transporterCarriedOreMutex[((Transporter*)transporter)->ID-1]);
    return false;
}
bool Smelter::isSmelterAlive(Smelter* smelter){
    pthread_mutex_lock(&isSmelterAliveMutex[smelter->ID-1]);
    if(smelter->isAlive){
        pthread_mutex_unlock(&isSmelterAliveMutex[smelter->ID-1]);
        return true;
    }
    pthread_mutex_unlock(&isSmelterAliveMutex[smelter->ID-1]);
    return false;

}
bool Foundry::isFoundryAlive(Foundry* foundry){
    pthread_mutex_lock(&isFoundryAliveMutex[foundry->ID-1]);
    if(foundry->isAlive){
        pthread_mutex_unlock(&isFoundryAliveMutex[foundry->ID-1]);
        return true;
    }
    pthread_mutex_unlock(&isFoundryAliveMutex[foundry->ID-1]);
    return false;

}
void * Smelter::findHighPrioritySmelter(Transporter* transporter){
    for(Smelter* smelter:vecSmelters){
        if(!isSmelterAlive(smelter)){
            continue;
        }
        if(smelter->oreType == IRON){
            if(transporter->isTransporterCarry(transporter,IRON)){
                pthread_mutex_lock(&smelterWaitingOreMutex[smelter->ID-1]);
                if(smelter->WaitingOreCount == 1){
                    return smelter;
                }
                pthread_mutex_unlock(&smelterWaitingOreMutex[smelter->ID-1]);

            }
        }else if(smelter->oreType == COPPER){
            if(transporter->isTransporterCarry(transporter,COPPER)){
                pthread_mutex_lock(&smelterWaitingOreMutex[smelter->ID-1]);
                if(smelter->WaitingOreCount == 1){
                    return smelter;
                }
                pthread_mutex_unlock(&smelterWaitingOreMutex[smelter->ID-1]);


            }
        }
    }
    return NULL;
}
void * Smelter::findLowPrioritySmelter(Transporter* transporter){

    for(Smelter* smelter:vecSmelters){
        if(!isSmelterAlive(smelter)){
            continue;
        }
        if(smelter->oreType == IRON){
            if(transporter->isTransporterCarry(transporter,IRON)){

                pthread_mutex_lock(&smelterWaitingOreMutex[smelter->ID-1]);
                if(smelter->capacity > smelter->WaitingOreCount){
                    return smelter;
                }
                pthread_mutex_unlock(&smelterWaitingOreMutex[smelter->ID-1]);

            }
        }else if(smelter->oreType == COPPER){
            if(transporter->isTransporterCarry(transporter,COPPER)){
                pthread_mutex_lock(&smelterWaitingOreMutex[smelter->ID-1]);
                if(smelter->capacity > smelter->WaitingOreCount){
                    return smelter;
                }
                pthread_mutex_unlock(&smelterWaitingOreMutex[smelter->ID-1]);

            }
        }
    }
    return NULL;
}
void * Foundry::findHighPriorityFoundry(Transporter* transporter){
    for(Foundry* foundry:vecFoundries){
        if(!isFoundryAlive(foundry)){
            continue;
        }
        if(transporter->isTransporterCarry(transporter,IRON)){
                pthread_mutex_lock(&foundryWaitingOreMutex[foundry->ID-1]);
                if(foundry->WaitingCoalCount >0 && foundry->WaitingIronCount == 0) {
                    sem_wait(&coalWait);
                    return foundry;
                }
                pthread_mutex_unlock(&foundryWaitingOreMutex[foundry->ID-1]);

        }
        else if(transporter->isTransporterCarry(transporter,COAL)){
                pthread_mutex_lock(&foundryWaitingOreMutex[foundry->ID-1]);
                if(foundry->WaitingCoalCount ==0  && foundry->WaitingIronCount >0){

                    sem_wait(&ironWait);
                    return foundry;
                }
                pthread_mutex_unlock(&foundryWaitingOreMutex[foundry->ID-1]);
        }
    }
    return NULL;
}
void * Foundry::findLowPriorityFoundry(Transporter* transporter){
    for(Foundry* foundry:vecFoundries){
        if(!isFoundryAlive(foundry)){
            continue;
        }
        if(transporter->isTransporterCarry(transporter,IRON)){
            pthread_mutex_lock(&foundryWaitingOreMutex[foundry->ID-1]);
            if(foundry->capacity > foundry->WaitingIronCount + foundry->WaitingCoalCount ){
                return foundry;
            }
            pthread_mutex_unlock(&foundryWaitingOreMutex[foundry->ID-1]);


        }
        else if(transporter->isTransporterCarry(transporter,COAL)){

            pthread_mutex_lock(&foundryWaitingOreMutex[foundry->ID-1]);
            if(foundry->capacity > foundry->WaitingIronCount + foundry->WaitingCoalCount ){
                return foundry;
            }
            pthread_mutex_unlock(&foundryWaitingOreMutex[foundry->ID-1]);

        }
    }
    return NULL;
}
pair<void *, bool> Transporter::WaitProducer(Transporter* transporter){
    void* producer;
    if (transporter->carriedOre == IRON){
        sem_wait(&ironWait);
        producer = Smelter::findHighPrioritySmelter(transporter);
        if(producer!= NULL){
            return make_pair(producer,true);
        }
        producer = Foundry::findHighPriorityFoundry(transporter);
        if(producer!= NULL){
            return make_pair(producer, false);
        }
        producer = Smelter::findLowPrioritySmelter(transporter);
        if(producer!= NULL){
            return make_pair(producer,true);
        }
        producer = Foundry::findLowPriorityFoundry(transporter);
        if(producer!= NULL){
            return make_pair(producer,false);
        }

    }
    else if (transporter->carriedOre == COPPER){

        sem_wait(&copperWait);
        producer = Smelter::findHighPrioritySmelter(transporter);
        if(producer!= NULL){
            return make_pair(producer,true);

        }
        producer = Smelter::findLowPrioritySmelter(transporter);
        if(producer!=NULL){
            return  make_pair(producer,true);
        }

    }

    else{
        sem_wait(&coalWait);
        producer = Foundry::findHighPriorityFoundry(transporter);
        if(producer!=NULL){
            return make_pair(producer,false);
        }
        producer = Foundry::findLowPriorityFoundry(transporter);
        if(producer!= NULL){
            return make_pair(producer,false);
        }

    }

    return make_pair((void* )NULL,true);


}
void* Foundry::transporterProducerRoutine(Transporter* transporter,Producer* foundry)
{
    FoundryInfo f_i;
    TransporterInfo t_i;
    unsigned int time = ((Transporter*)transporter)->interval;
    FillFoundryInfo( &f_i, ((Foundry*)foundry)->ID , 0, 0, 0, 0 );
    FillTransporterInfo(&t_i, ((Transporter*)transporter)->ID, &((Transporter*)transporter)->carriedOre);
    WriteOutput(NULL,&t_i,NULL,&f_i,TRANSPORTER_TRAVEL);
    usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));

    if(((Transporter*)transporter)->carriedOre == IRON){
        ((Foundry*)foundry)->WaitingIronCount++;
    }else{
        ((Foundry*)foundry)->WaitingCoalCount++;
    }

    pthread_mutex_unlock(&foundryWaitingOreMutex[((Foundry*)foundry)->ID-1]);
    if(((Transporter*)transporter)->carriedOre ==IRON){
        sem_post(&foundryWaitingIronSem[((Foundry*)foundry)->ID-1]);
    }else{
        sem_post(&foundryWaitingCoalSem[((Foundry*)foundry)->ID-1]);
    }


    FillFoundryInfo( &f_i, ((Foundry*)foundry)->ID , ((Foundry*)foundry)->capacity, ((Foundry*)foundry)->WaitingIronCount, ((Foundry*)foundry)->WaitingCoalCount, ((Foundry*)foundry)->ProducedIngotCount );
    FillTransporterInfo(&t_i, ((Transporter*)transporter)->ID, &((Transporter*)transporter)->carriedOre);
    WriteOutput(NULL,&t_i,NULL,&f_i,TRANSPORTER_DROP_ORE);
    usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));

    return NULL;
}



void* Smelter::transporterProducerRoutine(Transporter* transporter,Producer* smelter)
{
    SmelterInfo s_i;
    TransporterInfo t_i;

    unsigned int time = ((Transporter*)transporter)->interval;
    FillSmelterInfo(&s_i,((Smelter*)smelter)->ID, (OreType)NULL, 0,0,0);
    FillTransporterInfo(&t_i, ((Transporter*)transporter)->ID, &((Transporter*)transporter)->carriedOre);
    WriteOutput(NULL,&t_i,&s_i,NULL,TRANSPORTER_TRAVEL);
    usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));



    ((Smelter*)smelter)->WaitingOreCount ++;


    pthread_mutex_unlock(&smelterWaitingOreMutex[((Smelter*)smelter)->ID-1]);

    FillSmelterInfo(&s_i,((Smelter*)smelter)->ID, ((Smelter*)smelter)->oreType, ((Smelter*)smelter)->capacity,((Smelter*)smelter)->WaitingOreCount,((Smelter*)smelter)->ProducedIngotCount);

    FillTransporterInfo(&t_i, ((Transporter*)transporter)->ID, &((Transporter*)transporter)->carriedOre);
    WriteOutput(NULL,&t_i,&s_i,NULL,TRANSPORTER_DROP_ORE);


    sem_post(&waitUntilTwoOres[((Smelter*)smelter)->ID-1]);

    usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));


    return NULL;
}




void* Transporter::transporterRoutine(void* transporter)
{
    Miner* miner;

    TransporterInfo t_i;
    MinerInfo m_i;
    FillTransporterInfo(&t_i, ((Transporter*)transporter)->ID, &((Transporter*)transporter)->carriedOre);
    WriteOutput(NULL, &t_i, NULL, NULL, TRANSPORTER_CREATED);
    unsigned int time = ((Transporter*)transporter)->interval;
    
    while(1){

        if(!Miner::checkEmpty()){
            break;
        }

        sem_wait(&minersHasOre);

        miner= Miner::waitNextLoad();

        FillMinerInfo(&m_i,((Miner*)miner)->ID, (OreType)NULL, 0,0);
        FillTransporterInfo(&t_i, ((Transporter*)transporter)->ID, &((Transporter*)transporter)->carriedOre);
        WriteOutput(&m_i,&t_i,NULL,NULL,TRANSPORTER_TRAVEL);
        usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));

        (((Miner*)miner)->currentOreCount)--;
        pthread_mutex_unlock(&minerMutex[((Miner*)miner)->ID-1]);

        pthread_mutex_lock(&transporterCarriedOreMutex[((Transporter*)transporter)->ID-1]);
        (((Transporter*)transporter)->carriedOre) = ((Miner*)miner)->oreType;
        pthread_mutex_unlock(&transporterCarriedOreMutex[((Transporter*)transporter)->ID-1]);

        FillMinerInfo(&m_i,((Miner*)miner)->ID, ((Miner*)miner)->oreType, ((Miner*)miner)->capacity,((Miner*)miner)->currentOreCount);
        FillTransporterInfo(&t_i, ((Transporter*)transporter)->ID, &((Transporter*)transporter)->carriedOre);
        WriteOutput(&m_i,&t_i,NULL,NULL,TRANSPORTER_TAKE_ORE);
        usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));
        sem_post(&minerEmpty[miner->ID-1]);

        pair<void*,bool> producer = Transporter::WaitProducer(((Transporter*)transporter));
        if(producer.first == NULL){

            break;
        }
        if(producer.second){
            ((Smelter*)producer.first)->transporterProducerRoutine((Transporter*)transporter,(Producer*)(producer.first));
        }else{
            ((Foundry*)producer.first)->transporterProducerRoutine((Transporter*)transporter,(Producer*)(producer.first));
        }
    }

    sem_post(&minersHasOre);
    FillTransporterInfo(&t_i, ((Transporter*)transporter)->ID, &((Transporter*)transporter)->carriedOre);
    
    WriteOutput(NULL, &t_i, NULL, NULL, TRANSPORTER_STOPPED);
    
    return NULL;
}


int main(){
    int ironCapacity=0;
    int copperCapacity=0;
    int coalCapacity=0;
    unsigned int I,C,T,R;
    int num_Miners,num_Transporters, num_Smelters,num_Foundries;
    

    cin>>num_Miners;                               //Miner
    for(int i=0; i<num_Miners; i++)
    {
        cin>>I>>C>>T>>R;
        
        vecMiners.push_back(new Miner(i+1,(OreType)T,C,I,R));

        
    }

                                                         
    cin>>num_Transporters;                               //Transporter
    for(int i=0; i<num_Transporters; i++)
    {
        cin>>I;
        vecTransporters.push_back(new Transporter(i+1,I));
    }
                              
    cin>>num_Smelters;                               //Smelter
    for(int i=0; i<num_Smelters; i++)
    {
        cin>>I>>C>>T;
        vecSmelters.push_back(new Smelter(i+1,(OreType)T,C,I));
        if((OreType)T == IRON){
            ironCapacity +=C;
        }else{
            copperCapacity +=C;
        }



    }
                                  
    cin>>num_Foundries;                               //  Foundry
    for(int i=0; i<num_Foundries; i++)
    {
        cin>>I>>C;
        vecFoundries.push_back(new Foundry(i+1,C,I));
        ironCapacity+=C;
        coalCapacity+=C;
    }

    minersInfo = new MinersInfo();
    minersInfo->minerCount = num_Miners;
    minerEmpty = new sem_t[num_Miners];
    minerMutex = new pthread_mutex_t[num_Miners];
    minerFull = new sem_t[num_Miners];

    waitUntilTwoOres = new sem_t[num_Smelters];
    transporterCarriedOreMutex = new pthread_mutex_t[num_Transporters];
    smelterWaitingOreMutex = new pthread_mutex_t[num_Smelters];
    isSmelterAliveMutex = new pthread_mutex_t[num_Smelters];


    foundryWaitingOreMutex = new pthread_mutex_t[num_Foundries];
    isFoundryAliveMutex= new pthread_mutex_t[num_Foundries];
    foundryWaitingIronSem= new sem_t[num_Foundries];
    foundryWaitingCoalSem=new sem_t[num_Foundries];
    new sem_t[num_Foundries];

    for(int i=0; i<num_Miners; i++)
    {
        sem_init(&minerFull[i],0,0);
        sem_init(&minerEmpty[i],0,vecMiners[i]->capacity);
        pthread_mutex_init(&minerMutex[i],NULL);
    }
    sem_init(&minersHasOre, 0,0);
    pthread_mutex_init(&deadMinerLock, NULL);
    sem_init(&ironWait,0,ironCapacity);
    sem_init(&copperWait,0,copperCapacity);
    sem_init(&coalWait,0,coalCapacity);

    for(int i=0;i < num_Smelters; i++){
        pthread_mutex_init(&smelterWaitingOreMutex[i],NULL);
        pthread_mutex_init(&isSmelterAliveMutex[i],NULL);
        sem_init(&waitUntilTwoOres[i],0,0);

    }
    for(int i=0;i < num_Foundries; i++){
        pthread_mutex_init(&foundryWaitingOreMutex[i],NULL);
        pthread_mutex_init(&isFoundryAliveMutex[i],NULL);
        sem_init(&foundryWaitingIronSem[i],0,0);
        sem_init(&foundryWaitingCoalSem[i],0,0);


    }



    for(int i=0; i<num_Transporters; i++)
    {
        pthread_mutex_init(&transporterCarriedOreMutex[i],NULL);
    }
    pthread_mutex_init(&waitNextLoadMutex, NULL);
    
    pthread_t minerThreads[num_Miners];
    pthread_t smelterThreads[num_Smelters];
    pthread_t foundryThreads[num_Foundries];
    pthread_t transporterThreads[num_Transporters];

    InitWriteOutput();

    for(int i=0; i<num_Miners; i++)                              //  Miner threads
    {
        pthread_create(&(minerThreads[i]),NULL,Miner::minerRoutine,(void *)vecMiners[i]);
    }

    for(int i=0; i<num_Smelters; i++)                              //  Smelter threads
    {   
        pthread_create(&(smelterThreads[i]),NULL,Smelter::smelterRoutine,(void *) vecSmelters[i]);
    }

    for(int i=0; i<num_Foundries; i++)                              //  Foundry threads
    {
        pthread_create(&(foundryThreads[i]),NULL,Foundry::foundryRoutine,(void *) vecFoundries[i]);
    }

    for(int i=0; i<num_Transporters; i++)                              //  Transporter threads
    {
        pthread_create(&(transporterThreads[i]),NULL,Transporter::transporterRoutine,(void *) vecTransporters[i]);
    }




    for(int i=0;i<num_Miners;i++){
        pthread_join(minerThreads[i],NULL);
    }

    for(int i=0;i<num_Smelters;i++){
        pthread_join(smelterThreads[i],NULL);
    }

    for(int i=0;i<num_Foundries;i++){
        pthread_join(foundryThreads[i],NULL);
    }
    for(int i=0;i<num_Transporters;i++){
        pthread_join(transporterThreads[i],NULL);
    }



    for(int i=0; i<num_Miners; i++)
    {
        sem_destroy(&minerFull[i]);
        sem_destroy(&minerEmpty[i]);
        pthread_mutex_destroy(&minerMutex[i]);
    }
    sem_destroy(&minersHasOre);
    pthread_mutex_destroy(&deadMinerLock);
    for(int i=0;i < num_Smelters; i++){
        pthread_mutex_destroy(&smelterWaitingOreMutex[i]);
        pthread_mutex_destroy(&isSmelterAliveMutex[i]);
        sem_destroy(&waitUntilTwoOres[i]);
    }

    for(int i=0;i < num_Foundries; i++){
        pthread_mutex_destroy(&foundryWaitingOreMutex[i]);
        pthread_mutex_destroy(&isFoundryAliveMutex[i]);
        sem_destroy(&foundryWaitingIronSem[i]);
        sem_destroy(&foundryWaitingCoalSem[i]);

    }
    sem_destroy(&ironWait);
    sem_destroy(&copperWait);
    sem_destroy(&coalWait);

    for(int i=0; i<num_Transporters; i++)
    {
        pthread_mutex_destroy(&transporterCarriedOreMutex[i]);
    }    
    pthread_mutex_destroy(&waitNextLoadMutex);





    for(int i=0; i<num_Miners; i++)
    {
        delete vecMiners[i];
    }
    for(int i=0; i<num_Smelters; i++)
    {
        delete vecSmelters[i];
    }
    for(int i=0; i<num_Foundries; i++)
    {
        delete vecFoundries[i];
    }
    for(int i=0; i<num_Transporters; i++)
    {
        delete vecTransporters[i];
    }
    delete[] smelterWaitingOreMutex;
    delete[] minersInfo;
    delete[] minerEmpty;
    delete[] minerFull;
    delete[] transporterCarriedOreMutex;
    delete[] minerMutex;
    delete[] waitUntilTwoOres;
    delete[] isSmelterAliveMutex;
    delete[] foundryWaitingOreMutex;
    delete[] isFoundryAliveMutex;
    delete[] foundryWaitingIronSem;
    delete[] foundryWaitingCoalSem;
    return 0;
}