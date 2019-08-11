class Transporter {
    public:
        unsigned int ID;
        unsigned int interval;
        OreType carriedOre;
        Transporter(int ID,unsigned int interval){
            this->ID = ID;            
            this->interval = interval;
        }
        static void* transporterRoutine(void* transporter);
        static std::pair<void *, bool> WaitProducer(Transporter* transporter);
        bool isTransporterCarry(Transporter* transporter,OreType oreType);
};
