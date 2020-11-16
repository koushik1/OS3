/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>


/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;
	int ld;
	struct lentry *lptr;
	int reschflag = 0;

	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);

	for (ld = 0; ld < NLOCKS; ld++)
	{
		/* release all acquired locks */
		if (pptr->bm_locks[ld] == 1) 
		{
			releaseLDForProc(pid,ld);
			reschflag = 1;
		}
	}

	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;
					ld = pptr->lock_id;
					if (ld >= 0 || ld < NLOCKS)
					{
						pptr->pinh = 0;
						releaseLDForWaitProc(pid,ld);
					}

	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}
	if (reschflag == 1)
	{
		resched();
	}	
	
	restore(ps);
	return(OK);
}

void releaseLDForWaitProc(pid, ld)
{
	struct lentry *lptr;
	struct pentry *pptr;
	
	lptr = &locks[ld];
	pptr = &proctab[pid];

	dequeue(pid);
	pptr->lock_id = -1;
	pptr->wait_ltype = -1;
	pptr->wait_time = 0;

	lptr->lprio = get_max_process_prio(ld);	
	increaseProcPriority(ld,-1);
}
