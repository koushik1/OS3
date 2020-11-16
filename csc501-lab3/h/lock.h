/* lock.h - isbadlock */

#ifndef _LOCK_H_
#define _LOCK_H_

#ifndef NLOCKS
#define	NLOCKS		50	
#endif

#define	LFREE 0		
#define	LUSED 1	

#ifndef DELETED
#define DELETED -6
#endif

#define READ 0
#define WRITE 1

struct	lentry	{		
	char lstate;		
	int	lqhead;		
	int	lqtail;		
	int	ltype;		
	int	lprio;		
	int lproc_list[NPROC]; 
};
extern	struct	lentry	lockTable[];
extern	int	nextlock;
extern unsigned long ctr1000;
#define	isbadlock(s)	(s<0 || s>=NLOCKS)

#endif
