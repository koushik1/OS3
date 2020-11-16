#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>


int lcreate()
{
	STATWORD ps;    
	disable(ps);

	int	lock_desc;
    int i;
    for (i = 0; i < NLOCKS; i++)
    {
        if (nextlock < 0)
        {
            nextlock = NLOCKS - 1;
        }
        lock_desc = nextlock;
		if (locks[lock_desc].lstate==LFREE) {
			locks[lock_desc].lstate = LUSED;
            restore(ps);
			return(lock_desc);
		}
        nextlock = nextlock - 1;

    }
    restore(ps);
    return(SYSERR);


}


