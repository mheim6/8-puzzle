/*8puzzle.c
	CSCE A405 PA1: 8 Puzzle search
		Emily Sekona (esekona@alaska.edu)
		Monica Heim (mheim6@alaska.edu)
		Chandra Boyle (cdboyle@alaska.edu)
*/

//for C++ compiler
//extern "C" {

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

#define PTSIZE	16383
#define HASHPRIME	11

typedef struct ptentry ptentry;
typedef struct fringelink fringelink;
typedef struct movelink movelink;
typedef struct solution solution;

struct ptentry{
	char puz[9];
	char prev[9];
	int cost;
	ptentry *next;
};
struct fringelink{
	char puz[9];
	int f;
	fringelink *next;
};
struct movelink{
	char puz[9];
	movelink *next;
};
struct solution{
	uint32_t length;
	uint32_t expanded;
	uint32_t fringesize;
	movelink *moves;
};

char to_char(char i);
void print_block(char* block);
void pcopy(char*,char*);
int pequal(char*,char*);
void invert(char*);
int parity(char*);

int h_none(char*,char*); //bfs
int h_misplaced(char*,char*);
int h_manhattan(char*,char*);
int h_gaschnig(char*,char*);

int pthash(char*);
uint32_t ptfree(ptentry*);
uint32_t pt_free(ptentry**);
ptentry **pt_alloc();
void pt_add(ptentry**,char*,char*,int);
ptentry *pt_find(ptentry**,char*);

uint32_t ffree(fringelink*);
fringelink *fpop(fringelink*,char*);
fringelink *fpush(fringelink*,char*,int);

void freemoves(movelink*);
movelink *move(char*);

solution solve(char*,char*,int(*h)(char*,char*));

//general puzzle manipulation

char to_char(char i)
{
	if (i>=0 &&i<=7) return i+'1';
	else if (i==8) return 'x';
	else { printf("ERROR in Program!"); return -1; }

}

void print_block(char* block)
{
	printf("\n");
	printf ("\n-------");
	printf ("\n|%c|%c|%c|", to_char(block[0]), to_char(block[1]), to_char(block[2]));
	printf ("\n-------");
	printf ("\n|%c|%c|%c|", to_char(block[3]), to_char(block[4]), to_char(block[5]));
	printf ("\n-------");
	printf ("\n|%c|%c|%c|", to_char(block[6]), to_char(block[7]), to_char(block[8]));
	printf ("\n-------");
}

void pcopy(char *src, char *dest)
{
	((uint64_t*)dest)[0]=((uint64_t*)src)[0];
	dest[8]=src[8];
}

int pequal(char *a, char *b)
{
	return ((uint64_t*)a)[0]==((uint64_t*)b)[0] && a[8]==b[8];
}

void invert(char *puz)
{
	char map[9];
	int i;
	
	for(i=0;i<9;i++) map[(int)puz[i]]=i;
	for(i=0;i<9;i++) puz[i]=map[i];
}

int parity(char *puz)
{
	int i,j,p=0;
	char map[9]={0,1,2,3,4,5,6,7};
	
	for(i=0;i<9;i++) if(puz[i]!=8){
		p+=map[(int)puz[i]];
		for(j=(int)puz[i];j<8;j++) map[j]--;
	}return p%2;
}

//heuristic functions

int h_none(char *puz, char *goal) 
{
	return 0;
}

int h_misplaced(char *puz, char *goal)
{
	int i,h=0;
	
	for(i=0;i<8;i++) h+=(puz[i]!=goal[i]);
	return h;
}

int h_manhattan(char *puz, char *goal)
{
	int i,h=0;
	
	for(i=0;i<8;i++) h+=abs(goal[i]%3-puz[i]%3)+abs(goal[i]/3-puz[i]/3);
	return h;
}

int h_gaschnig(char *puz, char *goal)
{
	int i=0,h=0;
	char map[9];
	
	pcopy(puz,map);
	invert(goal);
	do{	char s,p;
		while(i<8 && goal[(int)map[i]]==i) i++;
		if(i==8) break;
		if(goal[(int)map[8]]==8) s=i;
		else s=goal[(int)map[8]];
		p=map[8];
		map[8]=map[(int)s];
		map[(int)s]=p;
		h++;
	}while(1);
	invert(goal);
	return h;
}

//encounter table operations

int pthash(char *puz)
{
	uint32_t x=0;
	int i;
	
	for(i=0;i<9;i++){
		x*=HASHPRIME;
		x+=(int)puz[i];
	}return x%PTSIZE;
}

uint32_t ptfree(ptentry *pte)
{
	uint32_t ct;
	
	if(pte==NULL) return 0;
	ct=ptfree(pte->next);
	free(pte);
	return ct+1;
}

uint32_t pt_free(ptentry **tbl)
{
	uint32_t ct=0;
	int i;
	
	if(tbl==NULL) return 0;
	for(i=0;i<PTSIZE;i++) ct+=ptfree(tbl[i]);
	free(tbl);
	return ct;
}

ptentry **pt_alloc()
{
	ptentry **tbl;
	
	if((tbl=(ptentry**)calloc(PTSIZE,sizeof(ptentry*)))==NULL) return NULL;
	return tbl;
}

void pt_add(ptentry **tbl, char *puz, char *par, int cost)
{
	int x;
	ptentry *p,*e;
	
	x=pthash(puz);
	if((e=(ptentry*)malloc(sizeof(ptentry)))==NULL) return;
	pcopy(puz,e->puz);
	if(par==NULL) e->prev[0]=(char)0x7f;
	else pcopy(par,e->prev);
	e->cost=cost;
	e->next=NULL;
	if((p=tbl[x])==NULL) tbl[x]=e;
	else{
		while(p->next!=NULL) p=p->next;
		p->next=e;
	}
}

ptentry *pt_find(ptentry **tbl, char *puz)
{
	int x;
	ptentry *p;
	
	x=pthash(puz);
	p=tbl[x];
	while(p!=NULL){
		if(pequal(puz,p->puz)) return p;
		p=p->next;
	}return p;
}

//fringe operations

uint32_t ffree(fringelink *fringe)
{
	uint32_t ct;
	
	if(fringe==NULL) return 0;
	ct=ffree(fringe->next);
	free(fringe);
	return ct+1;
}

fringelink *fpop(fringelink *fringe, char *puz)
{
	fringelink *f;
	
	if(fringe==NULL) return NULL;
	pcopy(fringe->puz,puz);
	f=fringe->next;
	free(fringe);
	return f;
}

fringelink *fpush(fringelink *fringe, char *puz, int f)
{
	fringelink *fn,*fp,*fc;
	
	if((fn=(fringelink*)malloc(sizeof(fringelink)))==NULL) return fringe;
	pcopy(puz,fn->puz);
	fn->f=f;
	fn->next=NULL;
	if(fringe==NULL) return fn;
	fp=fringe;
	if(f<(fringe->f)){
		fn->next=fringe;
		fp=fringe=fn;
	}else while(fp!=NULL){
		if((fc=fp->next)==NULL || f<(fc->f)){
			fp->next=fn;
			fn->next=fc;
			fp=fn;
			break;
		}fp=fc;
	}while(fp!=NULL){
		if((fc=fp->next)!=NULL && pequal(puz,fc->puz)){
			fp->next=fc->next;
			free(fc);
			fc=fp->next;
		}else fp=fc;
	}return fringe;
}

//move generator

void freemoves(movelink *moves)
{
	if(moves==NULL) return;
	freemoves(moves->next);
	free(moves);
}

movelink *move(char *puz)
{
	movelink *mv,*list=NULL;
	int i;
	
	if(puz[8]%3<2){
		mv=(movelink*)malloc(sizeof(movelink));
		if(mv==NULL) return NULL;
		mv->puz[8]=puz[8]+1;
		for(i=0;i<8;i++){
			if(puz[i]==mv->puz[8]) mv->puz[i]=puz[8];
			else mv->puz[i]=puz[i];
		}mv->next=list;
		list=mv;
	}if(puz[8]%3>0){
		mv=(movelink*)malloc(sizeof(movelink));
		if(mv==NULL) return NULL;
		mv->puz[8]=puz[8]-1;
		for(i=0;i<8;i++){
			if(puz[i]==mv->puz[8]) mv->puz[i]=puz[8];
			else mv->puz[i]=puz[i];
		}mv->next=list;
		list=mv;
	}if(puz[8]/3<2){
		mv=(movelink*)malloc(sizeof(movelink));
		if(mv==NULL) return NULL;
		mv->puz[8]=puz[8]+3;
		for(i=0;i<8;i++){
			if(puz[i]==mv->puz[8]) mv->puz[i]=puz[8];
			else mv->puz[i]=puz[i];
		}mv->next=list;
		list=mv;
	}if(puz[8]/3>0){
		mv=(movelink*)malloc(sizeof(movelink));
		if(mv==NULL) return NULL;
		mv->puz[8]=puz[8]-3;
		for(i=0;i<8;i++){
			if(puz[i]==mv->puz[8]) mv->puz[i]=puz[8];
			else mv->puz[i]=puz[i];
		}mv->next=list;
		list=mv;
	}return list;
}

//A* search function

solution solve(char *start, char *goal, int (*h)(char*,char*))
{
	ptentry **ptable;
	fringelink *fringe;
	solution s={0,0,0,NULL};
	char puz[9];
	
	if(parity(start)!=parity(goal)) return s;
	if((ptable=pt_alloc())==NULL) return s;
	invert(start); invert(goal);
	pt_add(ptable,start,NULL,0);
	fringe=fpush(NULL,start,0);
	
	while(1){
		ptentry *ptpar;
		movelink *mv;
		fringe=fpop(fringe,puz);
		if(pequal(puz,goal)) break;
		ptpar=pt_find(ptable,puz);
		mv=move(puz);
		while(mv!=NULL){
			movelink *m;
			ptentry *pte;
			pte=pt_find(ptable,mv->puz);
			if(pte==NULL){
				pt_add(ptable,mv->puz,puz,ptpar->cost+1);
				fringe=fpush(fringe,mv->puz,ptpar->cost+h(mv->puz,goal)+1);
			}else if((pte->cost)>(ptpar->cost+1)){
				pcopy(puz,pte->prev);
				pte->cost=ptpar->cost+1;
				fringe=fpush(fringe,mv->puz,ptpar->cost+h(mv->puz,goal)+1);
			}m=mv;
			mv=mv->next;
			free(m);
		}if(fringe==NULL){
			ffree(fringe);
			pt_free(ptable);
			invert(start);
			invert(goal);
			return s;
		}
	}
	
	while(1){
		movelink *m;
		ptentry *pte;
		if((m=(movelink*)malloc(sizeof(movelink)))==NULL) break;
		pte=pt_find(ptable,puz);
		if(pte==NULL) break;
		invert(puz);
		pcopy(puz,m->puz);
		m->next=s.moves;
		s.moves=m;
		if(pte->prev[0]==0x7f) break;
		pcopy(pte->prev,puz);
		s.length++;
	}s.fringesize=ffree(fringe);
	s.expanded=pt_free(ptable)-s.fringesize;
	invert(start); invert(goal);
	return s;
}

//entry point

int main()
{
	char block[9];
	char goal_block[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8}; //8 indicates no tile 

	//printf("\nThe Eight Puzzle!\n");
		printf("\n\n\n\t\t\t===================================\n");
	  printf("\t\t\t\t8 Puzzle Game\n");
	  printf("\t\t\t===================================\n\n\n");

		printf("\nPlease enter the START state of the game: \n"
					 " [Represent tiles with numbers 1 to 8, and the blank space as 'x'.]\n");

	int i = 0;
	while(i<9)
	{
		char chr;
		chr = fgetc(stdin);
		if (chr==32) continue;
		if (chr=='x') block[i] = 8;
		else if (chr >= '1' && chr <= '9') block[i] = chr - '1';
		else { printf("Error. Input not valid. (Ex. 2 1 3 4 7 5 6 8 x.) "); return 1; }
		i++;
	}

	fgetc(stdin); //flush out the end of line character

	printf("\n  Enter the Goal State. (EX. 1 2 3 4 5 6 7 8 x): ");

	i = 0;
	while(i<9)
	{
		char chr;
		chr = fgetc(stdin);
		if (chr==32) continue;
		if (chr=='x') goal_block[i] = 8;
		else if (chr >= '1' && chr <= '9') goal_block[i] = chr - '1';
		else { printf("chr=%d. Error. Wrong Input. (Ex.2 1 3 4 7 5 6 8 x) ",(int) chr); return 1; }
		i++;
	}

	printf("\nIn Progress...");

	solution s_bfs = solve(block, goal_block, &h_none);

	if (s_bfs.moves == NULL) {
			printf("done!\n");
			printf("There is no solution to this puzzle pair.\n");
	}
	else {
		solution s_mis = solve(block, goal_block, &h_misplaced);
		solution s_man = solve(block, goal_block, &h_manhattan);
		solution s_gas = solve(block, goal_block, &h_gaschnig);
		freemoves(s_mis.moves);
		freemoves(s_man.moves);
		freemoves(s_gas.moves);

		printf("done. \nFound the solution of least number of steps (%d).\n", s_bfs.length);
		printf("Nodes expanded (nodes in fringe):\n");
		printf("  Breadth-first search: %d (%d)\n", s_bfs.expanded, s_bfs.fringesize);
		printf("  Misplaced tiles: %d (%d)\n", s_mis.expanded, s_mis.fringesize);
		printf("  Manhattan distance: %d (%d)\n", s_man.expanded, s_man.fringesize);
		printf("  Gaschnig heuristic: %d (%d)\n", s_gas.expanded, s_gas.fringesize);
	
		char chr[15];
		printf("\nWant to print each step? (Y/N)?");
		scanf("%s", chr);
		if(chr[0] =='y' || chr[0]=='Y') {
			movelink *moves = s_bfs.moves;

			while (moves != NULL) {
				print_block(moves->puz);
				moves = moves->next;
			}
		}

		printf("\nDisplay Complete.\n");
	

		freemoves(s_bfs.moves);
	}

	return 0;
}

//}
