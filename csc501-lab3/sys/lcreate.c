#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

LOCAL int newlock();


int lcreate()
{
	STATWORD ps;    
	int	ld;

	disable(ps);
	if ((ld=newlock())==SYSERR) {
		restore(ps);
		return(SYSERR);
	}
	
	restore(ps);
	return(ld);
}


LOCAL int newlock()
{
	int	ld;
	int	i;

	for (i=0 ; i<NLOCKS ; i++) {
		ld=nextlock--;
		if (ld < 0)
			nextlock = NLOCKS-1;
		if (lockTable[ld].lstate==LFREE) {
			lockTable[ld].lstate = LUSED;
			return(ld);
		}
	}
	return(SYSERR);
}
