#include <linux/kernel.h>
#include <asm/io.h>
#include <all.h>
#include <asm/segment.h>
#define vga_graph_memstart 0xA0000
#define vga_graph_memsize 64000
#define cursor_side 6
#define vga_width 320
#define vga_heignt 200 
int volatile jumpp;
int sys_init_graphics()
{
    outb(0x05,0x3CE);
    outb(0x40,0x3CF);/* shift256=1*/
    outb(0x06,0x3CE);
    outb(0x05,0x3CF);/*0101 0xA0000*/
    outb(0x04,0x3C4);
    outb(0x08,0x3C5);/*0000 jilian*/


    outb(0x01,0x3D4);
    outb(0x4F,0x3D5);/* end horizontal display=79 ??*/
    outb(0x03,0x3D4);
    outb(0x82,0x3D5);/*display enable skew=0*/

    outb(0x07,0x3D4);
    outb(0x1F,0x3D5);/*vertical display end No8,9 bit=1,0*/
    outb(0x12,0x3D4);
    outb(0x8F,0x3D5);/*vertical display end low 7b =0x8F*/
    outb(0x17,0x3D4);
    outb(0xA3,0x3D5);/*SLDIV=1 ,scanline clock/2*/



    outb(0x14,0x3D4);
    outb(0x40,0x3D5);/*DW=1*/
    outb(0x13,0x3D4);
    outb(0x28,0x3D5);/*Offset=40, 20:become long*/

    outb(0x0C,0x3D4);/**/
    outb(0x00,0x3D5);/**/
    outb(0x0D,0x3D4);/**/
    outb(0x00,0x3D5);/*Start Address=0xA0000*/
    
    

    int i;
    char * p;
    p = (char *)vga_graph_memstart;
    for (i = 0; i < vga_graph_memsize; ++i)
        *p++ = 7;

    return 0;
}

int sys_repaint(struct pho * pho)
{
	int i, j;
    char * p;
    long color = get_fs_long(&pho->color);
    long x = get_fs_long(&pho->x);
    long y = get_fs_long(&pho->y);
    long dx = get_fs_long(&pho->dx);
    long dy = get_fs_long(&pho->dy);
    
    for (i = x; i < x+dx; ++i) if (0 <= i && i < vga_width)
        for (j = y; j < y+dy; ++j) if (0 <= j && j < vga_heignt){
            p = (char *)vga_graph_memstart + vga_width*j + i;
            *p = 10;
        }
        

    return 0; 
}
