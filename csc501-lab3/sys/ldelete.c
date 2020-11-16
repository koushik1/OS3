#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * ldelete  --  delete a lock by releasing its table entry
 *------------------------------------------------------------------------
 */
int ldelete (int ld)
{

    STATWORD ps;    
	disable(ps);

    if (locks[ld].lstate==LFREE)
    {
        restore(ps);
		return(SYSERR);
    }

    if (ld < 0 || ld >= NLOCKS)
    {
        restore(ps);
		return(SYSERR);
    }

    struct	lentry	*lptr = &locks[ld];
    lptr->lstate = LFREE;
	lptr->ltype = DELETED;
	lptr->lprio = -1;

    int i = 0;
    for (i = 0; i < NPROC; i++)
    {
        if (lptr->lproc_list[i] == 1)
        {
            lptr->lproc_list[i] == 0;
            proctab[i].bm_locks[ld] = 0;
        }
    }

    int queue_head = nonempty(lptr->lqhead);
    if (queue_head)
    {
        int pid = getfirst(lptr->lqhead);
        while (pid != EMPTY)
        {
             proctab[pid].lock_id = -1;
             ready(pid,RESCHNO);

        }
        resched();
    }

    restore(ps);
	return(OK);



}

