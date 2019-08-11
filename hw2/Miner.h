class Miner {
    public:
        unsigned int ID;
        OreType oreType;
        unsigned int capacity;
        unsigned int interval;
        unsigned int totalOre;
        unsigned int currentOreCount =0;
        bool isActive = true;



        Miner(int ID,OreType oreType,unsigned int capacity,unsigned int interval,unsigned int totalOre){
            this->ID = ID;
            this->oreType=oreType;
            this->capacity =capacity;
            this->interval = interval;
            this->totalOre = totalOre;

        }
        static void* minerRoutine(void* miner);
        static Miner* waitNextLoad();
        static bool checkEmpty();

};
class MinersInfo{
    public:
        int minerReminder = -1;
        int minerCount=0;

        MinersInfo(){
            minerReminder = -1;
            minerCount=0;
        }

};

