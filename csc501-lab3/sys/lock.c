#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

int lock (int ldes1, int type, int priority)
{
	STATWORD ps;
	struct lentry *lptr;
	struct pentry *pptr;
	disable(ps);
	
	if ((ldes1 < 0 || ldes1 >= NLOCKS) || (lptr= &locks[ldes1])->lstate==LFREE) {
		restore(ps);
		return(SYSERR);
	}
	
	pptr = &proctab[currpid];

        /* ltype = DELETED means it has not been acquired by any process yet for Read/Write but is created as lstate = LUSED  */
	/* thus any process could get the lock */
	if (lptr->ltype == DELETED)
	{
		lptr->ltype = type;
		lptr->lprio = -1;
		lptr->lproc_list[currpid] = 1;

		pptr->bm_locks[ldes1] = 1; /* set bit mask corresponding to lock desc to 1 for the current process */
		pptr->lock_id = -1; /* as it acquires lock it will take value -1  */
		pptr->wait_ltype = -1; /* as the current process is not being blocked */	 	
	}
	
	/* already some process has acquired READ lock on the descriptor  */
	else if (lptr->ltype == READ)
	{
		/* current process wants to acquire WRITE lock */
		if (type == WRITE)
		{
			/* block the current process as WRITE lock is exclusive */
            restore(ps);
            return block_process(pptr,ldes1,priority,type,currpid);

		}		
				
		/* current process wants to acquire READ lock */
		else if (type == READ)
		{
			/* check whether any higher write priority process is there in lock's wait queue */
			int writerProcExist = 0;
			int next = lptr->lqhead;
			struct pentry *wptr;
			
			while (q[next].qnext != lptr->lqtail)
			{
				next = q[next].qnext;
				wptr = &proctab[next];
				if (wptr->wait_ltype == WRITE && q[next].qkey > priority)
				{
					writerProcExist = 1;
					break;	
				}	
			}
			
			/* there is no writer process having priority more than current process so assign READ lock */
			if (writerProcExist == 0)
			{
				lptr->ltype = type;
				lptr->lprio = get_max_process_prio(ldes1); /* set lprio field to max priority process in wait queue of the lock */
				lptr->lproc_list[currpid] = 1;

				pptr->bm_locks[ldes1] = 1; /* set bit mask corresponding to lock desc to 1 for the current process */
				pptr->lock_id = -1; /* as it acquires lock it will take value -1  */
				pptr->wait_ltype = -1; /* as the current process is not being blocked */
				increaseProcPriority (ldes1,-1);	
			}
			
			/* there is high wait priority writer process in queue so block current process */
			if (writerProcExist == 1)
			{
				/* block the current process*/
                restore(ps);
                return block_process(pptr,ldes1,priority,type,currpid);

			}
		}			
	}

	/* already some process has acquired WRITE lock on the descriptor */
	else if (lptr->ltype == WRITE)
	{
        restore(ps);
        return block_process(pptr,ldes1,priority,type,currpid);
		
	}
	
	restore(ps);
	return(OK); 
}

int get_max_process_prio(int lock_d)
{

    struct lentry *lptr;
    lptr = &locks[lock_d];
    struct pentry *pptr ;

    int curr ;
    int max_priority = -1;
    for(curr = q[lptr->lqhead].qnext; curr!=lptr->lqtail; curr = q[curr].qnext)
    {
        pptr = &proctab[curr];
        int current_prio = -1;
        if (pptr->pinh == 0)
        {
            current_prio =  pptr->pprio;
        }
        else
        {
            current_prio =  pptr->pinh;
        }

        if (current_prio > max_priority)
            max_priority = current_prio;
    }
    return max_priority;
}

int block_process(struct pentry *pptr,int lock_d,int priority,int type,int pid)
{
    struct lentry *lptr;
    lptr= &locks[lock_d];
    pptr->pstate = PRWAIT;
    pptr->lock_id = lock_d;  
    pptr->wait_time = ctr1000;   
    pptr->wait_ltype = type; 

    insert(pid, lptr->lqhead, priority); 
    lptr->lprio = get_max_process_prio(lock_d); 
    int current_prio = -1;

    if (pptr->pinh == 0)
    {
        current_prio =  pptr->pprio;
    }
    else
    {
        current_prio =  pptr->pinh;
    }

    increaseProcPriority(lock_d,current_prio); 		 
    resched();
    return OK;	
}

void increaseProcPriority (int lock_d, int priority)
{
	struct lentry *lptr;
	struct pentry *pptr;
	int i;
	int tmpld;
	int gprio = -1;
	int maxprio = -1;				
	lptr = &locks[lock_d];

	for (i=0;i<NPROC;i++)
	{
		if (lptr->lproc_list[i] == 1)
		{
			pptr = &proctab[i];

            if (pptr->pinh == 0)
            {
                gprio =  pptr->pprio;
            }
            else
            {
                gprio =  pptr->pinh;
            }			
		 	
			if (priority == -1)
			{
				maxprio = getMaxWaitProcPrioForPI(i);
				
				
				if (maxprio > pptr->pprio)
				{
					pptr->pinh = maxprio;
				}
				else
				{
					pptr->pinh = 0;
				}
				
				tmpld = checkProcessTransitivityForPI(i);
				if (tmpld != -1)
				{
					increaseProcPriority (tmpld,-1);
				}			 
			}

			else if (gprio < priority)
			{
				pptr->pinh = priority;
				tmpld = checkProcessTransitivityForPI(i);
				if (tmpld != -1)
				{
					increaseProcPriority (tmpld,-1);
				}			 
			}	
		}
	} 		

}

int checkProcessTransitivityForPI (int pid)
{
	int ld;
	struct pentry *pptr;

	pptr = &proctab[pid];
	ld = pptr->lock_id;
	if (ld < 0 || ld >= NLOCKS)
	{
		return -1;
	}
	else
	{
		return ld;
	}			
}	

int getMaxWaitProcPrioForPI (int pid)
{
	int i;
	int maxprio = -1;
	struct pentry *pptr;
	struct lentry *lptr;

	pptr = &proctab[pid];
	
	for (i=0;i<NLOCKS;i++)
	{
		if (pptr->bm_locks[i] == 1)
		{
			lptr = &locks[i];
			if (maxprio < lptr->lprio)
			{
				maxprio = lptr->lprio;
			}
		}		
	}
		
	return maxprio;
}
