/* Includes
 **************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

#include <ncurses.h>


/* Macros / Defnitions
 ************************/
#define DEFAULT_INIT_LENGTH 8
#define DEFAULT_DRAW_CHAR  'S' 
#define DEFAULT_ERASE_CHAR  ' ' 
#define DEFAULT_TRACE_CHAR '.'
#define DEFAULT_FOOD_CHAR '@' 

#define ASCII_ESC	  27
#define ASCII_ARROW_UP    259
#define ASCII_ARROW_DN    258
#define ASCII_ARROW_LEFT  260
#define ASCII_ARROW_RIGHT 261

#define DEFAULT_DELAY (ONE_MILLI_SECOND * 100)
#define MIN_THRESHOLD_DELAY ( ONE_MILLI_SECOND * 20 )
#define ONE_MILLI_SECOND (1000)

#define SPEED_INC (-10) 
#define SPEED_DEC (+10) 

/* enums
 ***********/
 typedef enum  { 
		DIR_LEFT  = 0xA000,
 		DIR_RIGHT = 0xA002,
 		DIR_UP    = 0xA004,
 		DIR_DOWN  = 0xA006
	} direction_t;

/* Structures
 *******************/
typedef struct coord {
        NCURSES_SIZE_T x;	
        NCURSES_SIZE_T y;	
} COORD, *P_COORD;

typedef struct snake_segment {
	struct snake_segment *next;
	struct snake_segment *previous;
	direction_t dir;
	int length;
	COORD coord_start;
	COORD coord_end;
} SSEG, *P_SSEG;

typedef struct snake {
	int seg_count;
	P_SSEG seg_head;
	P_SSEG seg_tail; 
} SNAKE , *P_SNAKE;

typedef struct settings {
	int delay;
	bool pause;
	bool portal;
	bool cheat;
	chtype ch_draw;
	chtype ch_erase;
	chtype ch_food;
	bool b_show_segcount;
	bool b_show_length;
} SETTINGS, *P_SETTINGS;

typedef struct food {
	bool b_eaten;
	COORD coord;
} FOOD, *P_FOOD; 

/* Prototypes
 ****************/
WINDOW* ncurses_init();
void ncurses_uninit();

void init_settings(P_SETTINGS pset);
P_SNAKE snake_init();
void free_snake(P_SNAKE psnake);
void snake_draw_init(WINDOW *w, P_SETTINGS pset, P_SNAKE psnake);

bool snake_move(WINDOW *w, P_SETTINGS pset, P_SNAKE psnake, P_FOOD pfood);
bool snake_steer(WINDOW *w, P_SNAKE psnake, direction_t dir);
bool place_food(WINDOW *w , P_SETTINGS pset, P_FOOD pfood, P_SNAKE psnake);
bool eat_food(P_SNAKE psnake, P_FOOD pfood);
bool is_self_collision(P_SETTINGS pset, P_SNAKE psnake);
P_SSEG generate_new_head(direction_t newdir, P_COORD newcoord);
void insert_new_head(P_SNAKE psnake, P_SSEG pnewhead);

bool process_char(int ch, WINDOW *w, P_SETTINGS psettings, P_SNAKE psnake);

bool is_coord_on_snake(P_COORD pc_inq, P_SNAKE psnake);
void rank_coord(P_SSEG pseg, P_COORD *ppc_small, P_COORD *ppc_large);
bool is_coord_on_seg(P_COORD pc_inq, P_SSEG pseg);

void get_border_portal_coord(WINDOW *w, P_SNAKE psnake,P_COORD pc);

void reverse_snake(P_SNAKE psnake);
direction_t reverse_direction(direction_t dir);

/* Routines
 *************/
int main()
{
	WINDOW *w = NULL;
	P_SNAKE psnake = NULL;
	FOOD food;
	int ch = 0;
	SETTINGS settings;

	/* Initialize settings */
	init_settings(&settings);

	w = ncurses_init();
	
	psnake = snake_init(w);
	if( ! psnake ) {
		ncurses_uninit();
		return 1;
	}

	/* Draw the initial snake */
	snake_draw_init(w, &settings, psnake);

	food.b_eaten = true;
	while ( !(ch == 'x' || 
                  ch == 'q' ||
                  ch == ASCII_ESC 
                 )) {

		if(food.b_eaten) {
			place_food(w, &settings, &food ,psnake);
		}

		usleep(settings.delay);

		if(!settings.pause) {
			if(!snake_move(w, &settings, psnake, &food)) {
				break;
			}
		}

		ch = tolower(getch());
		process_char(ch, w, &settings, psnake);
	}
	
	ncurses_uninit();
	free_snake(psnake);
	return 0;
}

bool place_food(WINDOW *w , P_SETTINGS pset, P_FOOD pfood, P_SNAKE psnake)
{
	long int rnd = 0;
	P_COORD pcoord = &pfood->coord;

	pfood->b_eaten = false;

	srandom(time(NULL));		

	//pset->ch_food = '0';
	do {
		pcoord->y = random() % w->_maxy;  
		//pcoord->y = psnake->seg_head->coord_start.y;  
		pcoord->x = random() % w->_maxx;  
		//pset->ch_food++;
	} while(is_coord_on_snake(pcoord, psnake));

	mvwaddch(w, pcoord->y, pcoord->x, pset->ch_food);
}

bool process_char(int ch, WINDOW *w, P_SETTINGS pset, P_SNAKE psnake)
{
	switch(ch) 
	{
		case '-':	
			pset->delay += SPEED_DEC;
			break;	
		case '+':
			if ( (pset->delay + SPEED_INC) >= MIN_THRESHOLD_DELAY) {
				pset->delay += SPEED_INC;
			}
			break;	
		case 't':
			pset->ch_erase = (pset->ch_erase == DEFAULT_ERASE_CHAR) ? 
                                           DEFAULT_TRACE_CHAR :
				           DEFAULT_ERASE_CHAR;
			break;
		case 's':
			pset->b_show_segcount = pset->b_show_segcount ? false : true; 
			if(pset->b_show_segcount) 
				pset->b_show_length = false;
			break;
		case 'l':
			pset->b_show_length = pset->b_show_length ? false : true; 
			if(pset->b_show_length) 
				pset->b_show_segcount = false;
			break;
		case 'p':
			pset->pause = pset->pause ? false : true;
			break;
		case 'o':
			pset->portal = pset->portal ? false: true;
			break;
		case 'c':
			pset->cheat = pset->cheat ? false: true;
			break;
		case ASCII_ARROW_LEFT:
			snake_steer(w, psnake, DIR_LEFT);
			break;
		case ASCII_ARROW_RIGHT:
			snake_steer(w, psnake, DIR_RIGHT);
			break;
		case ASCII_ARROW_UP:
			snake_steer(w, psnake, DIR_UP);
			break;
		case ASCII_ARROW_DN:
			snake_steer(w, psnake, DIR_DOWN);
			break;
	}
}

bool seg_update_coord(direction_t dir, P_COORD pcoord)
{
	switch(dir) {
		case DIR_LEFT:
			pcoord->x--;
			break;
		case DIR_RIGHT:
			pcoord->x++;
			break;
		case DIR_UP:
			pcoord->y--;
			break;
		case DIR_DOWN:
			pcoord->y++;
			break;
	}
}

bool seg_update_headxy(P_SSEG seg)
{
	seg_update_coord(seg->dir, &seg->coord_start);
}

bool seg_update_tailxy(P_SSEG seg)
{
	seg_update_coord(seg->dir, &seg->coord_end);
}

bool is_border(WINDOW *w, P_SNAKE psnake)
{
	P_SSEG head = psnake->seg_head;
	P_COORD pcoord = &head->coord_start;

	if(pcoord->x >= w->_begx && 
 	   pcoord->x <= w->_maxx &&
	   pcoord->y >= w->_begy &&
	   pcoord->y <= w->_maxy) { 
		return false;
	}
	
	return true;
}

bool snake_steer(WINDOW *w, P_SNAKE psnake, direction_t dir)
{
	P_SSEG pnewhead = NULL;
	P_SSEG head = psnake->seg_head;
	bool dir_reversed = false;

	if( psnake->seg_head->dir == dir ) {
		return true;
	}
	
	
	//mvprintw(0,0,"dir=%x head-dir=%x CHECKING", dir, psnake->seg_head->dir);
	switch(dir) 
	{
	case DIR_UP:
		if(psnake->seg_head->dir == DIR_DOWN) {
			reverse_snake(psnake);	
			dir_reversed = true;
		}
		break;
	case DIR_DOWN:
		if(psnake->seg_head->dir == DIR_UP) {
			reverse_snake(psnake);	
			dir_reversed = true;
		}
		break;
	case DIR_LEFT:
		if(psnake->seg_head->dir == DIR_RIGHT) {
			reverse_snake(psnake);	
			dir_reversed = true;
		}
		break;
	case DIR_RIGHT:
		if(psnake->seg_head->dir == DIR_LEFT) {
			reverse_snake(psnake);	
			dir_reversed = true;
		}
		break;
	}
	//mvprintw(0,0,"dir=%x head-dir=%x %s", dir, psnake->seg_head->dir, dir_ok ? "PASSED" : "FAILED");
	if(dir_reversed) {
		return true;
	}
	

	pnewhead = generate_new_head(dir, &head->coord_start);
	if( ! pnewhead) {
		return false;
	}
	seg_update_tailxy(pnewhead);
	
	/* Make new segment the head of the snake */
	insert_new_head(psnake, pnewhead);

	return true;
}

P_SSEG generate_new_head(direction_t newdir, P_COORD newcoord)
{
	P_SSEG pnewhead = NULL;

	pnewhead = calloc(sizeof(SSEG), 1);
	if( ! pnewhead) {
		return false;
	}

	/* Setup the new segment in the expected direction */
	pnewhead->length = 0;	
	pnewhead->dir = newdir;
	pnewhead->coord_start.x = newcoord->x;
	pnewhead->coord_start.y = newcoord->y; 
	pnewhead->coord_end.x = newcoord->x;
	pnewhead->coord_end.y = newcoord->y;

	return pnewhead;

}

void insert_new_head(P_SNAKE psnake, P_SSEG pnewhead)
{
	/* First perform pointer surgeries on
           all involved nodes/segments
        */
	pnewhead->previous = NULL;
	pnewhead->next = psnake->seg_head;
	psnake->seg_head->previous = pnewhead;
	
	/* Now designate the new head and
           increment segment count
        */
	psnake->seg_head = pnewhead;
	psnake->seg_count++;
}

bool eat_food(P_SNAKE psnake, P_FOOD pfood)
{
	P_COORD coord_head = &psnake->seg_head->coord_start;
	P_COORD coord_food = &pfood->coord;

	if(coord_head->y == coord_food->y &&
	   coord_head->x == coord_food->x) {
		pfood->b_eaten = true;
		return true;
	}

	return false;
}

bool is_coord_on_snake(P_COORD pc_inq, P_SNAKE psnake)
{
	P_SSEG head = psnake->seg_head;
	P_SSEG eachseg = NULL;

	eachseg = head;
	while(eachseg) {
		if(is_coord_on_seg(pc_inq, eachseg))
			return true;
		eachseg = eachseg->next;
	}

	return false;	
}

bool is_self_collision(P_SETTINGS pset, P_SNAKE psnake)
{
	P_SSEG head = psnake->seg_head;
	P_COORD pc_inq = &head->coord_start;
	P_SSEG eachseg = NULL;

	if(pset->cheat) {
		return false;
	}

	if(psnake->seg_count == 1) {
		return false;
	}

	eachseg = head->next;
	while(eachseg) {
		if(is_coord_on_seg(pc_inq, eachseg))
			return true;
		eachseg = eachseg->next;
	}

	return false;	
}

void get_border_portal_coord(WINDOW *w, P_SNAKE psnake,P_COORD pc)
{
	P_SSEG head = psnake->seg_head;
	direction_t dir = head->dir;
	P_COORD phead_coord = &head->coord_start;

	switch(dir) {
		case DIR_UP:
			pc->x = phead_coord->x;
			pc->y = w->_maxy;
			break;	
		case DIR_DOWN:
			pc->x = phead_coord->x;
			pc->y = w->_begy;
			break;	
		case DIR_LEFT:
			pc->y = phead_coord->y;
			pc->x = w->_maxx;
			break;	
		case DIR_RIGHT:
			pc->y = phead_coord->y;
			pc->x = w->_begx;
			break;	
	}
}

bool snake_move(WINDOW *w, P_SETTINGS pset, P_SNAKE psnake, P_FOOD pfood)
{
	P_SSEG head = psnake->seg_head;
	P_SSEG tail = psnake->seg_tail;
        chtype ch = pset->b_show_segcount ?  (psnake->seg_count + '0') :
		    	pset->b_show_length ? head->length + '0'  : pset->ch_draw;	
	P_SSEG pnewhead = NULL;
	COORD newcoord = {0,0};

	seg_update_headxy(head);

	if( is_border(w, psnake)) {
		if(!pset->portal) {
	        	sleep(2);	
			beep();
			return false;
		}
		//In portal-mode snake appear on the other side
		get_border_portal_coord(w, psnake, &newcoord);
		pnewhead = generate_new_head(head->dir, &newcoord); 
		if( ! pnewhead) {
	        	sleep(2);	
			beep();
			return false;
		}
		insert_new_head(psnake, pnewhead);
		head = pnewhead;
	}

	if( is_self_collision(pset, psnake)) {
	        sleep(2);	
		beep();
		return false;
	}

	mvwaddch(w, 
               	head->coord_start.y, 
                head->coord_start.x, 
		ch);
	head->length++;

	if(eat_food(psnake, pfood)) {
		return true;
	}

	mvwaddch(w, tail->coord_end.y, tail->coord_end.x, pset->ch_erase);
	tail->length--;
	if ( tail->length == 0 ) {
		psnake->seg_tail = tail->previous;	
		psnake->seg_tail->next = NULL;
		psnake->seg_count--;
		free(tail);
	} 
	else {
		seg_update_tailxy(tail);
	}

	return true;
}

void init_settings(P_SETTINGS pset) 
{
	pset->delay = DEFAULT_DELAY;
	pset->pause = false;
	pset->portal = true;
	pset->cheat = false;
	pset->ch_draw = DEFAULT_DRAW_CHAR;
	pset->ch_erase = DEFAULT_ERASE_CHAR;
	pset->ch_food = DEFAULT_FOOD_CHAR;
	pset->b_show_segcount = false;
        pset->b_show_length = false;
} 

P_SNAKE snake_init(WINDOW *w)
{
	P_SNAKE psnake = NULL;
	P_SSEG p_initseg = NULL;

	psnake = calloc(sizeof(SNAKE),1);
	if( ! psnake) {
		return NULL;
	}
	
	p_initseg = calloc(sizeof(SSEG), 1);
	if( ! p_initseg) {
		free(psnake);
		return NULL;
	}
	
	/* TODO: Handle case where maxx/maxy of ncurses window
                 is not large enough to hold the initial size of
                 the snake
	*/
	
	/* Initialize the very first segment */
	p_initseg->previous = NULL;
	p_initseg->next = NULL;
	p_initseg->length = DEFAULT_INIT_LENGTH;	
	p_initseg-> dir = DIR_UP;
	p_initseg->coord_start.x = w->_maxx;
	p_initseg->coord_start.y = w->_maxy - DEFAULT_INIT_LENGTH + 1;
	p_initseg->coord_end.x = w->_maxx;
	p_initseg->coord_end.y = w->_maxy;

	/* Initialize snake */
	psnake->seg_count = 1;

	/* Insert initial segment into the snake */
	psnake->seg_head = p_initseg;
	psnake->seg_tail = p_initseg;

	return psnake;
}

void snake_draw_init(WINDOW *w, P_SETTINGS pset, P_SNAKE psnake)
{
	int i;
	P_SSEG pseg = psnake->seg_head; 
	NCURSES_SIZE_T y = pseg->coord_start.y;
	NCURSES_SIZE_T x = pseg->coord_start.x;

	//mvwprintw(w, 1,1, "y=%d x=%d maxy=%d maxx=%d", y, x, w->_maxy, w->_maxx);

	for(i=0; i < pseg->length; i++, y++) {
		mvwaddch(w, y, x, pset->ch_draw);
	}
}

WINDOW* ncurses_init()
{
	WINDOW *w = NULL;
	w = initscr();
	keypad(stdscr, TRUE);
	cbreak();
	noecho();
	nonl();
	nodelay(w, true);

	/* Reserve space for key help */
        //w->_begy = 0;	
	//mvprintw(0,0,"w->_begy=%d\n",w->_begy);

	//start_color();
	//init_pair(1, COLOR_RED, COLOR_BLACK);
	//attron(COLOR_PAIR(1));
	
	return w;
}

void rank_coord(P_SSEG pseg, P_COORD *ppc_small, P_COORD *ppc_large)
{
	if(pseg->coord_start.y > pseg->coord_end.y) {
		*ppc_large = &pseg->coord_start;	
		*ppc_small = &pseg->coord_end;	
		return;
	} 
	else if(pseg->coord_start.y < pseg->coord_end.y) {
		*ppc_large = &pseg->coord_end;	
		*ppc_small = &pseg->coord_start;	
		return;
	}
	else {
		if(pseg->coord_start.x > pseg->coord_end.x) {
			*ppc_large = &pseg->coord_start;	
			*ppc_small = &pseg->coord_end;	
			return;
		}
		else {
			//At this point start.x is either < or = end.x
			//If it is equal then, both - the start and the end cord 
			//are exactly the same	
			*ppc_large = &pseg->coord_end;	
			*ppc_small = &pseg->coord_start;	
			return;
		}
	}
}

bool is_coord_on_seg(P_COORD pc_inq, P_SSEG pseg)
{
	P_COORD pc_small=NULL, pc_large=NULL;
	if(pseg->dir == DIR_LEFT || pseg->dir == DIR_RIGHT) {
		if( pc_inq->y != pseg->coord_start.y) {
			return false;
		}
		rank_coord(pseg, &pc_small, &pc_large);
		if(pc_inq->x >= pc_small->x && pc_inq->x <= pc_large->x) {
			return true;
		} 
		else {
			return false;
		}
	} 	

	if(pseg->dir == DIR_UP || pseg->dir == DIR_DOWN) {
		if( pc_inq->x != pseg->coord_start.x) {
			return false;
		}
		rank_coord(pseg, &pc_small, &pc_large);
		if(pc_inq->y >= pc_small->y && pc_inq->y <= pc_large->y) {
			return true;
		} 
		else {
			return false;
		}
	} 	

	//should never reach here
	printf("is_cord_on_page: hit no logic's land");
	assert(false);
}


void ncurses_uninit()
{
	endwin();
}

void free_snake(P_SNAKE psnake)
{
	P_SSEG seg = psnake->seg_head;
	P_SSEG next = NULL;
	int count = 0;

	while(seg) {
		next = seg->next;
		free(seg);
		seg = next;
	}

	free(psnake);
}

direction_t reverse_direction(direction_t dir)
{
	switch(dir) {
		case DIR_LEFT:
			return DIR_RIGHT;
		case DIR_RIGHT:
			return DIR_LEFT;
		case DIR_UP:
			return DIR_DOWN;
		case DIR_DOWN:
			return DIR_UP;
	}

	//Should never reach here !
	return DIR_UP;
}

void reverse_snake(P_SNAKE psnake)
{
	P_SSEG seg = psnake->seg_head;
	P_SSEG next = NULL;
	COORD coord_temp;
	
	while(seg) 
	{	
		next = seg->next;
		seg->next = seg->previous;
		seg->previous = next;
		seg->dir = reverse_direction(seg->dir);
		coord_temp = seg->coord_start;
		seg->coord_start = seg->coord_end;
		seg->coord_end = coord_temp;
		seg = next;
	}

	
	seg = psnake->seg_head;
	psnake->seg_head = psnake->seg_tail;
	psnake->seg_tail = seg; 
}
