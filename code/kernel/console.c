
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PUBLIC void scroll_screen(CONSOLE* p_con, int direction);
PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE* p_con);
PUBLIC void clear(CONSOLE* p_con);
PUBLIC void init_screen(TTY* p_tty);
PUBLIC void deepCopyCons(CONSOLE* dest,CONSOLE* src);
PUBLIC void changeColor(u8* start,u8* end,int color);
PUBLIC int equals(char* a,char* b,int len);
/*======================================================================*
			   init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY* p_tty)
{
	int v_mem_size = V_MEM_SIZE >> 1;	/* 显存总大小 (in WORD) */

	int con_v_mem_size                   = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr      = 0 * con_v_mem_size;
	p_tty->p_console->v_mem_limit        = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

	DEFAULT_CHAR_COLOR = 0x07;

	clear(p_tty->p_console);
}


// /*======================================================================*
// 			   is_current_console
// *======================================================================*/
// PUBLIC int is_current_console(CONSOLE* p_con)
// {
// 	return (p_con == &console_table[nr_current_console]);
// }


/*======================================================================*
			   out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE* p_con, char ch)
{
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
	switch(ch) {
		case '\n':
			if(mode == 2){
				//search
				u8* forEnd = input;  //直接让input+（input_num*2-str_num*2）因为类型问题会出错
				for(int i=0;i<input_num*2-str_num*2;i++) forEnd++; 
				for(u8* i=input;i<=forEnd;i+=2){	
					if(equals(i,str,str_num*2)){
						u8* start = (u8*)(V_MEM_BASE+i-input[0]);
						char sp[1][2] = {{' ',0x07}};
						char ta[4][2] = {{' ',0x0},{' ',0x0},{' ',0x0},{' ',0x0}};
						if(equals(sp,str,2)||(equals(ta,str,8))){     //带空格或tab的查找，红底绿色
							changeColor(start,(u8*)(start+str_num*2-2),0x42); 
						}else{
							changeColor(start,(u8*)(start+str_num*2-2),0x04); 
						}
					}
				}			
			}else{
				if (p_con->cursor < p_con->original_addr +
				p_con->v_mem_limit - SCREEN_WIDTH) {
					for(int i=p_con->cursor;i<p_con->original_addr + SCREEN_WIDTH * ((p_con->cursor - p_con->original_addr) /SCREEN_WIDTH + 1);i++){
						*p_vmem++ = ' ';
						*p_vmem++ = 0x1;
						if(mode == 1){
							str[str_num][0] = ' ';
							str[str_num][1] = 0x1;
							str_num++;
						}else if(mode == 0){
							input[input_num][0] = ' ';
							input[input_num][1] = 0x1;
							input_num++;
						}
					}
					p_con->cursor = p_con->original_addr + SCREEN_WIDTH * 
						((p_con->cursor - p_con->original_addr) /
						SCREEN_WIDTH + 1);
				}
			}
			
			break;
		case '\b':
			if (p_con->cursor > p_con->original_addr) {
				if(*(p_vmem-2)==' '&&*(p_vmem-1)==0x0){
					p_con->cursor -= 4;
					for(int i=0;i<4;i++){
						*(p_vmem-1) = DEFAULT_CHAR_COLOR;
						p_vmem -= 2;
						if(mode == 0){
							input[input_num-1][0] = '\0';
							input[input_num-1][1] = '\0';
							input_num--;
						}else if(mode == 1){
							str[str_num-1][0] = '\0';
							str[str_num-1][1] = '\0';
							str_num--;
						}
						
					}
				}else if(*(p_vmem-1)==0x1){
					while(*(p_vmem-1)==0x1){
						p_con->cursor--;
						*(p_vmem-2) = ' ';
						*(p_vmem-1) = DEFAULT_CHAR_COLOR;
						p_vmem -= 2;
						if(mode == 0){
							input[input_num-1][0] = '\0';
							input[input_num-1][1] = '\0';
							input_num--;
						}else if(mode == 1){
							str[str_num-1][0] = '\0';
							str[str_num-1][1] = '\0';
							str_num--;
						}
					}
				}else{
					p_con->cursor--;
					*(p_vmem-2) = ' ';
					*(p_vmem-1) = DEFAULT_CHAR_COLOR;
					p_vmem -= 2;
					if(mode == 0){
						input[input_num-1][0] = '\0';
						input[input_num-1][1] = '\0';
						input_num--;
					}else if(mode == 1){
						str[str_num-1][0] = '\0';
						str[str_num-1][1] = '\0';
						str_num--;
					}
				}
			}
			break;
		case '\t':
			if (p_con->cursor <
				p_con->original_addr + p_con->v_mem_limit - 4) {
				p_con->cursor += 4;
				for(int i=0;i<4;i++){
					*p_vmem++ = ' ';
					*p_vmem++ = 0x0;
					if(mode == 1){
						str[str_num][0] = ' ';
						str[str_num][1] = 0x0;
						str_num++;
					}else if(mode == 0){
						input[input_num][0] = ' ';
						input[input_num][1] = 0x0;
						input_num++;
					}
				}
			}
			break;
		case '#':    
			/* 奇数次按下esc，mode从0变为1，输入关键字，按下回车，mode从1变为2，
			屏蔽除esc外所有输入，再按esc，mode从2变为0*/
			if(mode == 1){
				deepCopyCons(console_tmp,p_con);
				DEFAULT_CHAR_COLOR = 0x04;
			}else if(mode==0){   
				DEFAULT_CHAR_COLOR = 0x07;
				for(;p_vmem>(u8*)(V_MEM_BASE + console_tmp->cursor * 2);p_vmem-=2){
					*(p_vmem-2) = ' ';
					*(p_vmem-1) = DEFAULT_CHAR_COLOR;
				}
				u8* forEnd = input;  //直接让input+（input_num*2-str_num*2）因为类型问题会出错
				for(int i=0;i<input_num*2-str_num*2;i++) forEnd++; 
				for(u8* i=input;i<=forEnd;i+=2){	
					if(equals(i,str,str_num*2)){
						u8* start = (u8*)(V_MEM_BASE+i-input[0]);
						char sp[1][2] = {{' ',0x07}};
						char ta[4][2] = {{' ',0x0},{' ',0x0},{' ',0x0},{' ',0x0}};
						if(equals(ta,str,8)){
							changeColor(start,(u8*)(start+str_num*2-2),0x0);    //查找过必须恢复tab颜色
						}else{
							changeColor(start,(u8*)(start+str_num*2-2),DEFAULT_CHAR_COLOR);
						}
						
					}
					
					
					// if(equals(sp,str,2)||(equals(ta,str,8))){     //带空格或tab的查找，红底绿色
						
					// }else{
					// 	changeColor(start,(u8*)(start+str_num*2-2),0x04); 
					// }
				}
				deepCopyCons(p_con,console_tmp);
				memset(str,'\0',STR_SIZE*2*sizeof(char));
				str_num = 0;
			}
			break;
		default:
			if (p_con->cursor <
				p_con->original_addr + p_con->v_mem_limit - 1) {
				*p_vmem++ = ch;
				*p_vmem++ = DEFAULT_CHAR_COLOR;
				if(mode == 1){
					str[str_num][0] = ch;
					str[str_num][1] = 0x07;
					str_num++;
				}else if(mode == 0){
					input[input_num][0] = ch;
					input[input_num][1] = DEFAULT_CHAR_COLOR;
					input_num++;
				}
				p_con->cursor++;
			}
			break;
	}

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
		scroll_screen(p_con, SCR_DN);
	}
	
	flush(p_con);
}

/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE* p_con)
{
	set_cursor(p_con->cursor);
	set_video_start_addr(p_con->current_start_addr);
}

PUBLIC void clear(CONSOLE* p_con){
	disp_pos = 0;
	/* 光标位置在最开始处 */
	p_con->cursor = p_con->original_addr;
	set_cursor(p_con->cursor);

	for(int i=0;i<80*25;i++){
		disp_str(" ");
	}
	disp_pos = 0;
}

PUBLIC void deepCopyCons(CONSOLE* dest,CONSOLE* src){
	dest->current_start_addr = src->current_start_addr;
	dest->cursor = src->cursor;
	dest->original_addr = src->original_addr;
	dest->v_mem_limit = src->v_mem_limit;
}

PUBLIC void changeColor(u8* start,u8* end,int color){  //start、end都指字符的低地址，即字符位
	u8* i = start;
	for(;i<=end;i++){
		*(++i) = color;
	}
}

PUBLIC int equals(char* a,char* b,int len){
	int res = 1;
	for(int i=0;i<len;i++){
		if(*(a++) != *(b++)){
			res = 0;
			break;
		}
	}
	return res;
}

/*======================================================================*
			    set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}




/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE* p_con, int direction)
{
	// if (direction == SCR_UP) {
	// 	if (p_con->current_start_addr > p_con->original_addr) {
	// 		p_con->current_start_addr -= SCREEN_WIDTH;
	// 	}
	// }
	if (direction == SCR_DN) {
		if (p_con->current_start_addr + SCREEN_SIZE <
		    p_con->original_addr + p_con->v_mem_limit) {
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}

	set_video_start_addr(p_con->current_start_addr);
	set_cursor(p_con->cursor);
}

