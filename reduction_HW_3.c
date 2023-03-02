#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <mpi.h>
#include "clockcycle.h"

#define MASTER 0		/* taskid of first task */
#define FROM_MASTER 1		/* setting a message type */
#define FROM_WORKER 2		/* setting a message type */
void MPI_P2P_reduction(long long int * array_of_ints, long long int * recv_buffer, MPI_Datatype datatype, long long int array_partition,  int root)
{
		int mtype;
 		long long int local_sum;
 		int taskid;
 		int numtasks;
 		
 		//I need to get some things for the MPI message passing
		MPI_Status status;
    		MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
    		MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
		MPI_Request request = MPI_REQUEST_NULL;
    		
    		local_sum = 0;
    		
    		for(int i = 0; i < array_partition; i++)
    		{
    			local_sum += array_of_ints[i];
    		}

		mtype = FROM_WORKER;
		//This loop is the one that iterates down the groups of pairs (i.e. 16->8->4->2->1 pairs of ranks)
		for (int iteration = 0; iteration < log(numtasks)/log(2); iteration++)
		{
			int sender   = pow(2,iteration);//This guy will identify my senders this iteration.
			int receiver = pow(2,iteration+1);//This guy will identify my receivers this iteration.
			
			
			if(taskid % receiver == 0)//This guy says "if you are the 'even' rank" or "reciever rank"
			{

				long long int holder = local_sum;//hold my sum so I don't override it by accident
				MPI_Irecv(&local_sum, 1, datatype, taskid + sender, mtype, MPI_COMM_WORLD, &request);
				MPI_Wait(&request, &status);
				local_sum += holder;//combine together the two ranks
			}
			else if(taskid % sender == 0 )//This guy says "if you are the 'odd' rank" or "sender rank"
			{

				MPI_Isend(&local_sum, 1, datatype, taskid - sender , mtype, MPI_COMM_WORLD, &request);

			}
		 	
		 }

		if (taskid == root)
		{
			*recv_buffer = local_sum;//only on the root do I return the sum
		} 
}
int main(int argc, char **argv)
{	


	int numtasks,		/* number of tasks in partition */
    	taskid,			/* a task identifier */
    	numworkers,	  	/* number of worker tasks */
    	source,			/* task id of message source */
    	dest,			/* task id of message destination */

    	mtype,			/* message type */

    	i, j, k,		/* misc */
    	count;
    	
    	//some variables for timing 
    	unsigned long long  start_time = 0;
    	unsigned long long  end_time   = 0;
    	unsigned long long  cycles     = 0;
	double time        	       = 0;  
	int clockrate  		       = 512000000;  	

    	//need to initialize the world and get some MPI stuff
    	MPI_Status status;
    	MPI_Init(&argc, &argv);
  	MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
  	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    	
    	
	long long int SIZE 		= pow(2,30);
	long long int array_partition	=  SIZE/numtasks;//size of the array partition on each process 
	long long int * array_of_ints;//this is my array

	
	array_of_ints   = malloc(SIZE*sizeof(long long int));//I have to malloc the array because c is the worst

	long long int sum;
	
	long long int start = array_partition*taskid;
	//loop for filling the array
    	for (int q = 0; q < array_partition; q++)
	{
		array_of_ints[q] = q + start;
	}
	
	int root    = 0;
	
	start_time  = clock_now();//begins the timer
	
	MPI_P2P_reduction(array_of_ints, &sum, MPI_LONG_LONG_INT, array_partition, root);//this one is mine
	
	end_time    = clock_now();//ends the timer
	
	cycles      = end_time - start_time;
	time        = (double)cycles/clockrate;
	
	
	if (taskid == MASTER)
	{
		printf("The sum according to my algorithm is %llu.\n",sum);
		printf("This answer was produced in %f seconds.\n", time);
    		printf("The correct answer is %llu.\n",SIZE*(SIZE-1)/2);
	}
	
	long long int local_sum = 0;
	
	start_time  = clock_now();//begins the timer
	
    	for(int i = 0; i < array_partition; i++)
    	{
    		local_sum += array_of_ints[i];
    	}
    	sum = 0;
    	
    	MPI_Reduce(&local_sum, &sum, 1, MPI_LONG_LONG_INT, MPI_SUM, 0, MPI_COMM_WORLD);//this one is theirs
    	
    	end_time    = clock_now();//ends the timer
	
	cycles      = end_time - start_time;
	time        = (double)cycles/clockrate;
	
	if (taskid == MASTER)
	{
		printf("The sum accroding to MPI reduce is %llu.\n",sum);
		printf("This answer was produced in %f seconds.\n", time);
    		printf("The correct answer is %llu.\n",SIZE*(SIZE-1)/2);
	}
	
	free(array_of_ints);//free the arrays
	MPI_Finalize();//done with this world
}
