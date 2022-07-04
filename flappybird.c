#define __LIBRARY__
#include<stdio.h>
#include<unistd.h>
#include<all.h>

#define BIRD_X 120
#define BIRD_Y 100
#define BIRD_WIDTH 10
#define BIRD_HEIGHT 8

#define MAX_BARRIER 20

#define CLOCK_TRIGGER 20
#define DROP_PER_TRIGGER 1
#define UP_PER_CLICK 10
#define LEFT_PER_TRIGGER 1

#define BIRD_COLOR 12
#define BARRIER_COLOR 12
#define BACKGROUND_COLOR 3
#define GAME_OVER_COLOR 12

#define VAG_WIDTH 320
#define VGA_HEIGHT 200

#define BARRIER_WIDTH 10
#define BARRIER_INTERVAL 20
#define BARRIER_HEIGHT (rand()%(VGA_HEIGHT*3/4))


_syscall0(int,init_graphics)
_syscall1(int,get_message,struct message * ,msg)
_syscall1(int,repaint,struct pho *,pho)
_syscall2(int,timercreate,long,ms,int,type)

int fron, rear;
struct pho barrier[MAX_BARRIER];
int i;
struct message msg;
struct pho obj;
struct pho background0,gameover;
struct pho bird;


int init_all()
{
	bird.x = BIRD_X;
    bird.y = BIRD_Y;
    bird.dx = BIRD_WIDTH;
    bird.dy = BIRD_HEIGHT;
	bird.color = BIRD_COLOR;

	fron = rear = 0;

	background0.color = BACKGROUND_COLOR;
    background0.x = 0;
    background0.y = 0;
    background0.dx = VAG_WIDTH;
    background0.dy = VGA_HEIGHT;

	gameover.color = GAME_OVER_COLOR;
    gameover.x = 0;
    gameover.y = 0;
    gameover.dx = VAG_WIDTH;
    gameover.dy = VGA_HEIGHT;
	return 0;
}


/*paint*/
int paint_barrier(void) /*paint barrier*/
{
    int i;
    struct pho rect;
    for (i = fron; i != rear; i = (i+1)%MAX_BARRIER) {
        rect.color = BARRIER_COLOR;
        rect.x = barrier[i].x;
        rect.y = barrier[i].y;
        rect.dx = barrier[i].dx;
        rect.dy = barrier[i].dy;
        if (repaint(&rect) < 0)
            return -1;
    }
    return 0;
}
int paint_all()/*all*/
{
	if (repaint(&background0) < 0)
        return -1;
    if (repaint(&bird) < 0)
        return -1;
    if (paint_barrier() < 0)
        return -1;
    return 0;
}

int push_obj(struct pho * obj) {
    if (rear != (fron + MAX_BARRIER - 1) % MAX_BARRIER) {
        barrier[rear].x = obj->x;
        barrier[rear].y = obj->y;
        barrier[rear].dx = obj->dx;
        barrier[rear].dy = obj->dy;
        rear = (rear + 1) % MAX_BARRIER;
        return 0;
    }
    return -1;
}

int pop_obj(struct pho * obj) {
    if (rear == fron)
        return -1;
    if (obj != NULL) {
        obj->x = barrier[fron].x;
        obj->y = barrier[fron].y;
        obj->dx = barrier[fron].dx;
        obj->dy = barrier[fron].dy;
    }
    fron = (fron + 1) % MAX_BARRIER;
    return 0;
}



int main()
{
	timercreate(CLOCK_TRIGGER, TYPE_USER_TIMER_INFTY); /* create time */
    init_graphics(); /*Graphical interface*/
	
	if(init_all()!=0)/*initialize*/
	{/*printf("bird error 1\n");*/return -1;}
	while(1)
	{	/*sleep(1);*/
		get_message(&msg);
		if(msg.mid<0)
			continue;
		if(msg.mid==MSG_MOUSE_LEFT_DOWN)
		{bird.y -= UP_PER_CLICK;}
		else
		{bird.y += DROP_PER_TRIGGER;}
		if(paint_all() != 0)
		{/*printf("bird error 2\n");*/return -1;}
		for (i = fron; i != rear; i = (i+1)%MAX_BARRIER) /*Determine if the game has failed*/
                if (barrier[i].x < bird.x+bird.dx && bird.x < barrier[i].x+barrier[i].dx)
                    if (barrier[i].y < bird.y+bird.dy && bird.y < barrier[i].y+barrier[i].dy)
                        {
							repaint(&gameover);
							return 0;
						}
            for (i = fron; i != rear; i = (i+1)%MAX_BARRIER) /* bird is stationary ,need barriers move*/
                barrier[i].x -= LEFT_PER_TRIGGER;
            if (fron == rear) /* there is no barriers,add barriers */
			{ 
                obj.dx = BARRIER_WIDTH;
                obj.dy = BARRIER_HEIGHT;
                obj.x = VAG_WIDTH;
                obj.y = 0;
                push_obj(&obj);
            }
            else /* add new barriers,delete old barriers */
			{ 
                if (barrier[(rear+MAX_BARRIER-1)%MAX_BARRIER].x+barrier[(rear+MAX_BARRIER-1)%MAX_BARRIER].dx+BARRIER_INTERVAL <= VAG_WIDTH) {
                    obj.dx = BARRIER_WIDTH;
                    obj.dy = BARRIER_HEIGHT;
                    obj.x = VAG_WIDTH;
                    if (barrier[(rear+MAX_BARRIER-1)%MAX_BARRIER].y)
                        obj.y = 0;
                    else
                        obj.y = VGA_HEIGHT - obj.dy;
                    push_obj(&obj);
                }
                if (barrier[fron].x+barrier[fron].dx <= 0)
                    pop_obj(NULL);
            }
	}
repaint(&gameover);
return 0;
}

