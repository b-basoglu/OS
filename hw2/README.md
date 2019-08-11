#
	Objective

	This assignment aims to familiarize you with the development of multi-threaded applications
	and synchronization using C. Your task is to implement a simulator for mining (Turkish; maden
	çıkarma) by simulating the different agents within the scenario using different threads. Towards
	this end, you will be using mutexes, semaphores and condition variables for synchronizing the
	different threads.
	Keywords— Thread, Semaphore, Mutex, Condition Variable

	Problem Definition
		We want to simulate the processing of ores (Turkish; cevher) into ingots (Turkish: Külçe).
		Specifically, iron, copper and coal ores are mined and processed to produce copper and steel
		ingots
		
		The processing is handled by four types of agents whose functions are described as below:
		
		• Miners, produce ores at a certain rate and have three types: iron, copper and coal
		miners. A miner has a limited capacity to store the ores it produced. It sleeps when its
		storage gets full and wakes up, when ores are taken. There exists a maximum number
		of ores that a miner can produce. Once a miner reaches this number, it quits.
		• Smelters produce copper or iron ingots at a certain rate. A smelter uses 2 ores to
		produce 1 ingot. Each smelter has a limited storage capacity for incoming ores. A
		smelter quits, if it cannot not produce ingots (due to the lack of incoming ores) for a
		certain duration.
		• Foundries: produce steel ingots at a certain rate. A foundry uses 1 iron and 1 coal ore
		to produce 1 steel ingot. Each foundry has a limited storage capacity for incoming iron
		and coal ores. The storage capacities for both ore types are the same. A foundry quits
		if it can not produce ingots (due to the lack of incoming ores) for a certain duration.
		• Transporters: carry ores from miners to smelters or foundries. A transporter can
		carry one ore at a time. The transporter iterates over the miners, and can loads ores
		from them if the miner has ores in its storage.
		
		Once an ore is loaded, the transporter carries it to producer (a smelter or a foundry)
		based on the type of the ore. Copper ores are carried to smelters, Iron ore can be carried
		either to smelters or foundries and, coal ores are carried to foundries. The transporter
		iterates over smelters or foundries, and can drop ores to them if the smelter or foundry
		has room in its storage. The transporters can drop ores to smelters or foundries while
		they are producing ingots. The transporter should only iterate over available producers
		with empty storage space or wait until one’s storage becomes available.
		Transporters works until all the miners stop producing ores and have no ore left in their
		storage. Transports should work miners in order and return to the first miner after
		reaching the end.


		Remaining description is described in THE2.pdf.