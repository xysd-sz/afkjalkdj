#include<linux/sched.h>
#include<asm/segment.h>
#include <asm/system.h>
#include<all.h>
int volatile jumpp;
void post_message(int type){
	cli();
	if(jumpp<=10)
	jumpp++;
	sti();
	return;
}
int sys_get_message(struct message *msg) {
   	if(jumpp>0) --jumpp;
	return jumpp;
}
	
	
