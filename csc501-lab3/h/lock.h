#ifndef _LOCK_H_
#define _LOCK_H_

#define READ	1
#define WRITE	2
#define DELETED -6
#define NLOCKS 50

#define LFREE  0
#define LUSED  1

struct	lentry	{		
	char lstate;		
	int	lqhead;		
	int	lqtail;		
	int	ltype;	
	int	lprio;		
	int lproc_list[NPROC]; 
};

extern struct lentry locks[];
extern	int	nextlock;
extern unsigned long ctr1000;
#endif
