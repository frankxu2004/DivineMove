#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "uct.h"
#include "divinemove.h"
#include "board.h"

/* The GTP specification leaves the initial board size and komi to the
* discretion of the engine. We make the uncommon choices of 6x6 board
* and komi -3.14.
*/
#include <windows.h>

//序盘 0， 中盘1， 收官2




/* Board represented by a 1D array. The first board_size*board_size
* elements are used. Vertices are indexed row by row, starting with 0
* in the upper left corner.
*/
static int board[MAX_BOARD * MAX_BOARD];

/* Stones are linked together in a circular list for each string. */
static int next_stone[MAX_BOARD * MAX_BOARD];

/* Storage for final status computations. */
static int final_status[MAX_BOARD * MAX_BOARD];

/* Point which would be an illegal ko recapture. */
static int ko_i, ko_j;

/* Offsets for the four directly adjacent neighbors. Used for looping. */

static int deltai[8] = {-1, 1, 0, 0,-1,1,-1,1};
static int deltaj[8] = {0, 0, -1, 1,-1,1,1,-1};

static time_t start;


/* Macros to convert between 1D and 2D coordinates. The 2D coordinate
* (i, j) points to row i and column j, starting with (0,0) in the
* upper left corner.
*/
#define POS(i, j) ((i) * board_size + (j))
#define I(pos) ((pos) / board_size)
#define J(pos) ((pos) % board_size)
#define EMPTY 0
#define BLACK 1
#define WHITE 2
/* Macro to find the opposite color. */
#define OTHER_COLOR(color) (WHITE + BLACK - (color))



struct ThreadParams
{
	int* moves;
	int size;
};
DWORD WINAPI ThreadRoutine(LPVOID lpArg)
{
	struct ThreadParams para;

	int* moves = para.moves;
	int size = para.size;
	para = *(struct ThreadParams *)lpArg;
	return NULL;
}



void
	init_divinemove()
{
	int k;
	int i, j;

	stage_flag=0;
	
	clear_board(&main_board);

	/*  开局
	*/

}



static int
calculate_sphere(board_status *bs,int ai, int aj,intersection color){
		int black_sphere = 0;
		int white_sphere = 0;
		float THRESHOLD=0;
		int pos;
		int sphere[MAX_BOARD * MAX_BOARD];
		int tmpboard[MAX_BOARD * MAX_BOARD];
		for(pos = 0;pos < board_size*board_size;pos++){
			tmpboard[pos]=bs->board[pos];}
		tmpboard[POS(ai,aj)]=color;

		for(pos = 0;pos < board_size*board_size;pos++){
			if(tmpboard[pos] == BLACK){
				++black_sphere;
				sphere[pos] = BLACK;
			}
			else{
				if(tmpboard[pos] == WHITE){
					++white_sphere;
					sphere[pos] = WHITE;
				}
				else{
					float black_influence = 0;
					float white_influence = 0;
					int i;
					for(i = 0;i < board_size*board_size;i++){
						if(i == pos) continue;
						if(tmpboard[i] == BLACK){
							int dist = abs(I(i) - I(pos)) + abs(J(i) - J(pos));
							black_influence += 1/dist;
						}
						if(tmpboard[i] == WHITE){
							int dist = abs(I(i) - I(pos)) + abs(J(i) - J(pos));
							white_influence += 1/dist;
						}
					}

					if(black_influence - white_influence > THRESHOLD){
						++black_sphere;
						sphere[pos] = BLACK;
					}
					if(white_influence - black_influence > THRESHOLD){
						++white_sphere;
						sphere[pos] = WHITE;
					}
				}
			}
		}
		return black_sphere - white_sphere;
}

static
int stage_judge(board_status *bs)
{
	int num_stone=0,i,j;
	for (i=0;i<=MAX_BOARD*MAX_BOARD-1;++i)
	{
		if (bs->board[i]!=EMPTY) num_stone++;
	}
	get_legal_moves(bs, BLACK);
	if (num_stone<=20 ) return 0;
	if (bs->legal_moves_num>=140) return 0;
	if (num_stone>=100) return 2;
	if (bs->legal_moves_num<=50) return 2;
	
	return 1;
}

int gambit(board_status *bs, intersection color)
{
	int flag=0,i,j,k;
	for (i=0;i<=4;i++){
		{for (j=0;j<=4;j++)
		if (bs->board[POS(i,j)]==OTHER_COLOR(color)) {flag=1;break;}}
	if (flag==1) break;}
	if (flag==0) {if (bs->board[42]==EMPTY)return 42; else {if (bs->board[30]==EMPTY) return 30;}}

	flag=0;
	for (i=MAX_BOARD-5;i<=MAX_BOARD-1;i++){
		{for (j=MAX_BOARD-5;j<=MAX_BOARD-1;j++)
		if (bs->board[POS(i,j)]==OTHER_COLOR(color)) {flag=1;break;}}
	if (flag==1) break;}
	if (flag==0) {if (bs->board[126]==EMPTY)return 126; else {if (bs->board[114]==EMPTY) return 114;}}

	flag=0;
	for (i=MAX_BOARD-5;i<=MAX_BOARD-1;i++){
		{for (j=0;j<=4;j++)
		if (bs->board[POS(i,j)]==OTHER_COLOR(color)) {flag=1;break;}}
	if (flag==1) break;}
	if (flag==0) {if (bs->board[120]==EMPTY) return 120; else {if (bs->board[106]==EMPTY) return 106;}}

	flag=0;
	for (j=MAX_BOARD-5;j<=MAX_BOARD-1;j++){
		{for (i=0;i<=4;i++)
		if (bs->board[POS(i,j)]==OTHER_COLOR(color)) {flag=1;break;}}
	if (flag==1) break;}
	if (flag==0) {if (bs->board[48]==EMPTY)return 48; else {if (bs->board[34]==EMPTY) return 34;}}

	return 47;
}


int capture(board_status *bs, intersection color)
{
	int i,j,k,ai,aj,flag,candi,flag1;
	for (i=0;i<=MAX_BOARD*MAX_BOARD-1;++i)
	{
		if (bs->board[i]==OTHER_COLOR(color) )
		{
			flag=0;
			for (k=0;k<=3;++k)
			{
				ai=I(i)+deltai[k];aj=J(i)+deltaj[k];
				if (ON_BOARD(ai,aj)) {if (bs->board[POS(ai,aj)]==color) flag++;else candi=POS(ai,aj);} else{flag++;}
			}
			flag1=0;
			if (flag==3 && bs->board[candi]==EMPTY) 
			{
				for (k=0;k<=3;++k)
				{
				ai=I(candi)+deltai[k];aj=J(candi)+deltaj[k];
				if (ON_BOARD(ai,aj)) if (bs->board[POS(ai,aj)]!=OTHER_COLOR(color)) return candi;
				}
			} 
		}
	}
	return -1;
}


int getbestmove(board_status *bs, intersection color){
    int oppcolor;
    int qhead,qtail,libpos,EOSnow,tmp,flag,maxm,maxp;
    int checked0[169],checkedlib[169],que0[169],cpyboard[169],sq[169],opplib[169];
    int EOS[30][2];
    
    int i,j;
	int eat[169],eatnum,save[169],savenum;
	memset(checked0,0,sizeof(checked0));

	memset(EOS,0,sizeof(EOS));
    memset(save,0,sizeof(save));
    savenum=0;
    oppcolor=color;
    for(i=0;i<169;i++){
        if(bs->board[i]==oppcolor&&checked0[i]==0){
            qhead=qtail=EOSnow=flag=0;
            libpos=-1;
            memset(que0,0,sizeof(que0));
			memset(checkedlib,0,sizeof(checkedlib));
            que0[qtail++]=i;
			checked0[i]=1;
            while(qhead!=qtail){
                tmp=que0[qhead++];
                EOSnow++;
                if(tmp%13>0){
                    if(bs->board[tmp-1]==oppcolor&&checked0[tmp-1]==0){
                        que0[qtail++]=tmp-1;
						checked0[tmp-1]=1;
                    }
                    else if(bs->board[tmp-1]==0&&checkedlib[tmp-1]==0){
                        if(libpos!=-1)flag=1;
						checkedlib[tmp-1]=1;
                        libpos=tmp-1;
                    }
                }
                if(tmp%13<12){
                    if(bs->board[tmp+1]==oppcolor&&checked0[tmp+1]==0){
                        que0[qtail++]=tmp+1;
						checked0[tmp+1]=1;
                    }
                    else if(bs->board[tmp+1]==0&&checkedlib[tmp+1]==0){
                        if(libpos!=-1)flag=1;
						checkedlib[tmp+1]=1;
                        libpos=tmp+1;
                    }
                }
                if(tmp>12){
                    if(bs->board[tmp-13]==oppcolor&&checked0[tmp-13]==0){
                        que0[qtail++]=tmp-13;
						checked0[tmp-13]=1;
                    }
                    else if(bs->board[tmp-13]==0&&checkedlib[tmp-13]==0){
                        if(libpos!=-1)flag=1;
						checkedlib[tmp-13]=1;
                        libpos=tmp-13;
                    }
                }
                if(tmp<156){
                    if(bs->board[tmp+13]==oppcolor&&checked0[tmp+13]==0){
                        que0[qtail++]=tmp+13;
						checked0[tmp+13]=1;
                    }
                    else if(bs->board[tmp+13]==0&&checkedlib[tmp+13]==0){
                        if(libpos!=-1)flag=1;
						checkedlib[tmp+13]=1;
                        libpos=tmp+13;
                    }
                }
            }
            if(flag==0&&EOSnow>0){
                if(getlib(bs,libpos,color)>2){
                        EOS[savenum][0]=libpos;
						sq[savenum]=que0[0];
                        EOS[savenum++][1]=EOSnow;
                }
            }
        }
    }
    for(i=0;i<savenum;i++){
        maxm=0;
        maxp=-1;
        for(j=0;j<savenum;j++){
            if(EOS[j][1]>maxm){
                maxm=EOS[j][1];
                maxp=EOS[j][0];
            }
        }
        save[i]=maxp;
    }

    //eat the most
    memset(EOS,0,sizeof(EOS));
    memset(eat,0,sizeof(eat));
	oppcolor=(color==1?2:1);
	memset(opplib,0,sizeof(opplib));
    eatnum=0;
    for(i=0;i<169;i++){
		if(bs->board[i]==oppcolor&&checked0[i]==0){
            qhead=qtail=EOSnow=flag=0;
            libpos=-1;
            memset(que0,0,sizeof(que0));
			memset(checkedlib,0,sizeof(checkedlib));
            que0[qtail++]=i;
			checked0[i]=1;
            while(qhead!=qtail){
                tmp=que0[qhead++];
                EOSnow++;
                if(tmp%13>0){
                    if(bs->board[tmp-1]==oppcolor&&checked0[tmp-1]==0){
                        que0[qtail++]=tmp-1;
						checked0[tmp-1]=1;
                    }
                    else if(bs->board[tmp-1]==0&&checkedlib[tmp-1]==0){
                        if(libpos!=-1)flag=1;
						checkedlib[tmp-1]=1;
                        libpos=tmp-1;
                    }
                }
                if(tmp%13<12){
                    if(bs->board[tmp+1]==oppcolor&&checked0[tmp+1]==0){
                        que0[qtail++]=tmp+1;
						checked0[tmp+1]=1;
                    }
                    else if(bs->board[tmp+1]==0&&checkedlib[tmp+1]==0){
                        if(libpos!=-1)flag=1;
						checkedlib[tmp+1]=1;
                        libpos=tmp+1;
                    }
                }
                if(tmp/13>0){
                    if(bs->board[tmp-13]==oppcolor&&checked0[tmp-13]==0){
                        que0[qtail++]=tmp-13;
						checked0[tmp-13]=1;
                    }
                    else if(bs->board[tmp-13]==0&&checkedlib[tmp-13]==0){
                        if(libpos!=-1)flag=1;
						checkedlib[tmp-13]=1;
                        libpos=tmp-13;
                    }
                }
                if(tmp/13<12){
                    if(bs->board[tmp+13]==oppcolor&&checked0[tmp+13]==0){
                        que0[qtail++]=tmp+13;
						checked0[tmp+13]=1;
                    }
                    else if(bs->board[tmp+13]==0&&checkedlib[tmp+13]==0){
                        if(libpos!=-1)flag=1;
						checkedlib[tmp+13]=1;
                        libpos=tmp+13;
                    }
                }
            }
            if(flag==0&&EOSnow>0){
					for(qhead=0;qhead<qtail;qhead++)opplib[que0[qhead]]=1;
					tmp=(libpos%13>0?opplib[libpos-1]:0)+(libpos%13<12?opplib[libpos+1]:0)+(libpos/13>0?opplib[libpos-13]:0)+(libpos/13<12?opplib[libpos+13]:0);
                    if((libpos%13>0&&bs->board[libpos-1]!=oppcolor)||(libpos%13<12&&bs->board[libpos+1]!=oppcolor)||
                        (libpos/13>0&&bs->board[libpos-13]!=oppcolor)||(libpos/13<12&&bs->board[libpos+13]!=oppcolor)||tmp>1){
                        EOS[eatnum][0]=libpos;
                        EOS[eatnum++][1]=EOSnow;
						for(j=0;j<169;j++)cpyboard[j]=bs->board[j];
						cpyboard[libpos]=color;
						for(qhead=0;qhead<qtail;qhead++)cpyboard[que0[qhead]]=0;
						for(j=0;j<savenum;j++)if(getlib(bs,sq[j],color)>1)return libpos;

                    }
            }
        }
    }
    for(i=0;i<eatnum;i++){
        maxm=0;
        maxp=-1;
        for(j=0;j<eatnum;j++){
            if(EOS[j][1]>maxm){
                maxm=EOS[j][1];
                maxp=EOS[j][0];
            }
        }
        eat[i]=maxp;
    }


   	if (savenum>=1) return save[0];
	if (eatnum>=1) return eat[0];
	return -1;
}
static
int getlib(board_status *bs,int pos, intersection color){
    int lib=0;
    int qhead,qtail,tmp;
    int que[169],checked[169];
	
    memset(checked,0,sizeof(checked));
    qhead=qtail=0;
    que[qtail++]=pos;
    checked[pos]=1;
    while(qhead!=qtail){
        tmp=que[qhead++];
        if(tmp%13>0){
            if(checked[tmp-1]==0&&bs->board[tmp-1]==color){que[qtail++]=tmp-1;checked[tmp-1]=1;}
            else if(bs->board[tmp-1]==0&&checked[tmp-1]==0){
                lib++;
                checked[tmp-1]=1;
            }
        }
        if(tmp%13<12){
            if(checked[tmp+1]==0&&bs->board[tmp+1]==color){que[qtail++]=tmp+1;checked[tmp+1]=1;}
            else if(bs->board[tmp+1]==0&&checked[tmp+1]==0){
                lib++;
                checked[tmp+1]=1;
            }
        }
        if(tmp/13>0){
            if(checked[tmp-13]==0&&bs->board[tmp-13]==color){que[qtail++]=tmp-13;checked[tmp-13]=1;}
            else if(bs->board[tmp-13]==0&&checked[tmp-13]==0){
                lib++;
                checked[tmp-13]=1;
            }
        }
        if(tmp/13<12){
            if(checked[tmp+13]==0&&bs->board[tmp+13]==color){que[qtail++]=tmp+13;checked[tmp+13]=1;}
            else if(bs->board[tmp+13]==0&&checked[tmp+13]==0){
                lib++;
                checked[tmp+13]=1;
            }
        }
    }
    return lib;
}
static
int Toeat(board_status *bs,int *caneat,  intersection mycolor){
    int checked[169],checkedlib[169],lib[169],que[169],strid[169],w[169];
    int i,tmp,qhead,qtail,libb,flag,cnt,id,eatnumber,ww,maxnow,dbeatposition,tmpw;
    int oppcolor=(mycolor==1?2:1);
    memset(checked,0,sizeof(checked));
    memset(lib,0,sizeof(lib));
    memset(strid,0,sizeof(strid));
    id=0;
    for(i=0;i<169;i++){
        if(checked[i]==0&&bs->board[i]==oppcolor){
            id++;
            ww=0;
            memset(que,0,sizeof(que));
            qhead=qtail=libb=0;
            memset(checkedlib,0,sizeof(checkedlib));
            que[qtail++]=i;
            checked[i]=1;
            while(qhead!=qtail){
                tmp=que[qhead++];
                ww++;
                if(tmp%13>0){
                    if(bs->board[tmp-1]==0&&checkedlib[tmp-1]==0){
                        libb++;
                        checkedlib[tmp-1]=1;
                    }
                    else if(bs->board[tmp-1]==oppcolor&&checked[tmp-1]==0){
                        que[qtail++]=tmp-1;
                        checked[tmp-1]=1;
                    }
                }
                if(tmp%13<12){
                    if(bs->board[tmp+1]==0&&checkedlib[tmp+1]==0){
                        libb++;
                        checkedlib[tmp+1]=1;
                    }
                    else if(bs->board[tmp+1]==oppcolor&&checked[tmp+1]==0){
                        que[qtail++]=tmp+1;
                        checked[tmp+1]=1;
                    }
                }
                if(tmp/13>0){
                    if(bs->board[tmp-13]==0&&checkedlib[tmp-13]==0){
                        libb++;
                        checkedlib[tmp-13]=1;
                    }
                    else if(bs->board[tmp-13]==oppcolor&&checked[tmp-13]==0){
                        que[qtail++]=tmp-13;
                        checked[tmp-13]=1;
                    }
                }
                if(tmp/13<12){
                    if(bs->board[tmp+13]==0&&checkedlib[tmp+13]==0){
                        libb++;
                        checkedlib[tmp+13]=1;
                    }
                    else if(bs->board[tmp+13]==oppcolor&&checked[tmp+13]==0){
                        que[qtail++]=tmp+13;
                        checked[tmp+13]=1;
                    }
                }
            }
            for(qhead=0;qhead<qtail;qhead++){lib[que[qhead]]=libb;strid[que[qhead]]=id;w[que[qhead]]=ww;}
        }
    }


    cnt=maxnow=0;
    dbeatposition=-1;
    for(i=0;i<169;i++){
        if(bs->board[i]==0){
            flag=libb=eatnumber=tmpw=0;
            if(i%13>0){
                if(bs->board[i-1]==oppcolor&&lib[i-1]==2){flag=1;eatnumber=(eatnumber==0?strid[i-1]:-1);tmpw+=w[i-1];}
            }
            if(i%13<12){
                if(bs->board[i+1]==oppcolor&&lib[i+1]==2){flag=1;eatnumber=(eatnumber==0?strid[i+1]:-1);tmpw+=w[i+1];}
            }
            if(i/13>0){
                if(bs->board[i-13]==oppcolor&&lib[i-13]==2){flag=1;eatnumber=(eatnumber==0?strid[i-13]:-1);tmpw+=w[i-13];}
            }
            if(i/13<12){
                if(bs->board[i+13]==oppcolor&&lib[i+13]==2){flag=1;eatnumber=(eatnumber==0?strid[i+13]:-1);tmpw+=w[i+13];}
            }
            if(eatnumber==-1&&tmpw>maxnow)if(getlib(bs,i,mycolor)>1){
                maxnow=tmpw,
                dbeatposition=i;
            }
            if(flag==1&&getlib(bs,i,mycolor)>1) caneat[cnt++]=i;
        }

    }
    return dbeatposition;
    //return cnt;
}



/* Generate a move. */
int generate_move(board_status *bs, intersection color)
{

	int pos,stage,i,caneat[169];
	stage=stage_judge(bs);
	if (stage==2 &&stage_flag==1)
	{
	pos=getbestmove(bs,color);
	if (pos==-1)   
	{
	pos=Toeat(bs,caneat,color);
	if (pos==-1) pos=uct_search(bs,color);
	}
	}
	else
	{
		pos=getbestmove(bs,color);
		if (pos==-1)   
		{
		pos=Toeat(bs,caneat,color);
		if (pos==-1) pos=uct_search1(bs,color);
		}
	}
	return pos;


}


void
	place_free_handicap(board_status *bs, int handicap)
{
	int low = board_size >= 13 ? 3 : 2;
	int mid = board_size / 2;
	int high = board_size - 1 - low;

	if (handicap >= 2) {
		play_move(bs,high, low, BLACK);   /* bottom left corner */
		play_move(bs,low, high, BLACK);   /* top right corner */
	}

	if (handicap >= 3)
		play_move(bs,low, low, BLACK);    /* top left corner */

	if (handicap >= 4)
		play_move(bs,high, high, BLACK);  /* bottom right corner */

	if (handicap >= 5 && handicap % 2 == 1)
		play_move(bs,mid, mid, BLACK);    /* tengen */

	if (handicap >= 6) {
		play_move(bs,mid, low, BLACK);    /* left edge */
		play_move(bs,mid, high, BLACK);   /* right edge */
	}

	if (handicap >= 8) {
		play_move(bs,low, mid, BLACK);    /* top edge */
		play_move(bs,high, mid, BLACK);   /* bottom edge */
	}
}




/*
* Local Variables:
* tab-width: 8
* c-basic-offset: 2
* End:
*/
