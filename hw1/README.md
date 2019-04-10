	Problem Definition
	In this THE, you are going to implement a modified version of the MapReduce model. MapReduce was
designed to process large data sets on clusters using parallel algorithms. It can be described as a system
managing the data flow between each computing element of an algorithm. In other words, it manages
communication between various nodes of a parallel algorithm designed to comply with the MapReduce
programming model. MapReduce does not manage what an algorithm does, it manages how it does it.
	A MapReduce program is composed of two main procedures, Map() and Reduce(). Each processing
element executing Map() is called mapper and executing Reduce() is called reducer. Generally, MapReduce
programs are executed with more than one mapper and reducer. In our case, each mapper and each reducer
is executed as a separate process. Number of mappers and reducers in the system will always be equal to
each other, which may not be the case for a real world MapReduce instance. In a MapReduce program,
input data are initially divided into bunches based on a key K1 and each bunch is served to a separate
mapper as input. Outputs generated by the mappers are combined and divided again using another key
K2. Then, each bunch is served to a reducer as input. Final outputs are generated by reducers for each
key in K2. In our case, there is no separate key for Reduce(). Output of each mapper is forwarded directly
to the reducer with the same id. Moreover, reducers forward their outputs to the next reducer instead of
writing them to the output