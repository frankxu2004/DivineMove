#include "divinemove.h"
#include "board.h"
#include "uct.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <process.h>
static time_t start;
static HANDLE h[MAX_THREAD];
static board_status *gbs;
static intersection gcolor;
static board_status hboard[MAX_THREAD];
static uct_node *groot;
static int f[8] = {0, 1, 2, 3, 4, 5, 6, 7};
static int simulate_cnt;
CRITICAL_SECTION CriticalSection;
// static int hi;
int shapei[20] = {2, 2, 0, 0,-1,1,-1,1,2,1,-2,-1,2,-2,-1,1,0,0,1,-1};//Ìø£¬¼â£¬Ð¡·É
int shapej[20] = {0, 0, -2, 2,-1,1,1,-1,1,2,-1,-2,-1,1,2,-2,1,-1,0,0};

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
	if (num_stone>=130) return 2;
	if (bs->legal_moves_num<=60) return 2;
	
	return 1;
}

/*
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
					//printf("%f %f\n",black_influence,white_influence);

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
		//printf("Black area: %d\tWhile area: %d\n",black_sphere,white_sphere);
		return black_sphere - white_sphere;
}
*/



double simulate_game(board_status *bs, intersection color)
{
	
    int pass[3];
    intersection color_now;
    int i, j, pos, step;
    double score;
	
    step = 0;
    pass[OTHER_COLOR(color)] = (bs->last_move_pos == -14);
    pass[color] = 0;
    color_now = color;
    while (!(pass[BLACK] && pass[WHITE]) && step <= 200) {
        pos = generate_random_move(bs, color_now);
        pass[color_now] = (pos == -14);
        play_move(bs, I(pos), J(pos), color_now);
        color_now = OTHER_COLOR(color_now);
    }
    // debug_log_board_status(bs);
    //score = get_score(bs);
	score=mygetscore(bs);
    if (score >=-6.5 && color == WHITE)
        //return score+6.5;
		return 1;
    if (score < -6.5 && color == BLACK)
        //return -(score+6.5);
		return 1;
	if (score <-6.5 && color == WHITE)
		return -1;
	if (score >=-6.5 && color == BLACK)
		return -1;
    //return -abs(score+6.5);
	return 0;
}


int mygetscore(board_status *bs){
    int qhead,qtail,stringlength;
    int board[169],checked[169],color[169],strnow[169],que[169];
    int lib,tmp,colornow,i,position,white,black;

	for(position=0;position<169;position++)board[position]=bs->board[position];
    memset(checked,0,sizeof(checked));
    memset(color,0,sizeof(color));
    for(position=0;position<169;position++)
        if(board[position]!=0&&checked[position]==0&&color[position]==0)
        {
            colornow=board[position];
            memset(que,0,sizeof(que));
            memset(strnow,0,sizeof(strnow));
            qhead=qtail=stringlength=lib=0;
            que[qtail++]=position;
            checked[position]=1;
            while(qhead!=qtail){
                tmp=que[qhead++];
                strnow[stringlength++]=tmp;
                if(tmp%13>0){
                    if(checked[tmp-1]==0&&(board[tmp-1]==colornow)){
                       que[qtail++]=tmp-1;
                       checked[tmp-1]=1;
                    }
                    else if(checked[tmp-1]==0&&(board[tmp-1]==0)){
                        que[qtail++]=tmp-1;
                        checked[tmp-1]=1;
                        lib++;
                    }
                }
                if(tmp%13<12){
                    if(checked[tmp+1]==0&&(board[tmp+1]==colornow)){
                       que[qtail++]=tmp+1;
                       checked[tmp+1]=1;
                    }
                    else if(checked[tmp+1]==0&&(board[tmp+1]==0)){
                        que[qtail++]=tmp+1;
                        checked[tmp+1]=1;
                        lib++;
                    }
                }
                if(tmp/13>0){
                    if(checked[tmp-13]==0&&(board[tmp-13]==colornow)){
                       que[qtail++]=tmp-13;
                       checked[tmp-13]=1;
                    }
                    else if(checked[tmp-13]==0&&(board[tmp-13]==0)){
                        que[qtail++]=tmp-13;
                        checked[tmp-13]=1;
                        lib++;
                    }
                }
                if(tmp/13<12){
                    if(checked[tmp+13]==0&&(board[tmp+13]==colornow)){
                       que[qtail++]=tmp+13;
                       checked[tmp+13]=1;
                    }
                    else if(checked[tmp+13]==0&&(board[tmp+13]==0)){
                        que[qtail++]=tmp+13;
                        checked[tmp+13]=1;
                        lib++;
                    }
                }
            }
            if(lib>1)for(i=0;i<stringlength;i++)color[strnow[i]]=colornow;
            else for(i=0;i<stringlength;i++)color[strnow[i]]=(colornow==1?2:1);
        }
    white = black =0;
    for(i=0;i<169;i++){
        if(color[i]==1)white++;
        else if(color[i]==2)black++;
    }
    return (white-black);

}



static void init_uct_node(int wins, int visits, uct_node *un)
{
    un->wins = wins;
    un->visits = visits;
    un->pos = POS(-1, -1);
    un->child = NULL;
    un->sibling = NULL;
}

static void free_uct_tree(uct_node *un)
{
	int k;
    if (un != NULL) {
        free_uct_tree(un->child);
        free_uct_tree(un->sibling);
        k = un->pos;
        free(un);
    }
}

static int create_uct_children(board_status *bs, intersection color,
    uct_node *un)
{
    int mi,stage;
    uct_node *now, *new_node;

    get_legal_moves2(bs, color);

    if (bs->legal_moves_num == 0)
        return 0;
	new_node = (uct_node *)malloc(sizeof(uct_node));
    init_uct_node(0, 0, new_node);
	new_node->pos = bs->legal_moves[0];
    un->child = new_node;
    now = un->child;

	for (mi = 1; mi <= bs->legal_moves_num-1; mi++) {
        new_node = (uct_node *)malloc(sizeof(uct_node));
        init_uct_node(0, 0, new_node);
        new_node->pos = bs->legal_moves[mi];
        now->sibling = new_node;
        now = now->sibling;
    }

	return bs->legal_moves_num;
}


static int root_create_uct_children(board_status *bs, intersection color,
    uct_node *un)
{
    int mi,stage;
    uct_node *now, *new_node;

    get_legal_moves2(bs, color);

    if (bs->legal_moves_num == 0)
        return 0;
	stage=stage_judge(bs);

    if (stage==0)
	{
		int caneat[169],numeat=0,jj;
		int mvchecked[169];
		int good_moves1[MAX_BOARD*MAX_BOARD],i,num_good_moves=0,cnt=0,j,k,ai,aj;numeat=Toeat(bs,caneat,color);
		memset(mvchecked,0,sizeof(mvchecked));
		for (i=0;i<=bs->legal_moves_num-1;++i)
		{
			int a=0;
			if (getlib(bs,bs->legal_moves[i], color)<2) continue;
			a=bs->legal_moves[i];
			if ((a>=28 && a<=30)||(a>=41 && a<=43)||(a>=54 && a<=56)||(a>=34 && a<=36)||(a>=47 && a<=49)||(a>=60 && a<=62)||(a>=106 && a<=108)||(a>=119 && a<=121)||(a>=132 && a<=134)||(a>=112 && a<=114)||(a>=125 && a<=127)||(a>=138 && a<=141)) if(mvchecked[bs->legal_moves[i]]==0){good_moves1[num_good_moves++]=bs->legal_moves[i];mvchecked[bs->legal_moves[i]]=1;continue;}
			for (jj=0;jj<=numeat-1;++jj) if (bs->legal_moves[i]==caneat[jj])if(mvchecked[bs->legal_moves[i]]==0){good_moves1[num_good_moves++]=bs->legal_moves[i];mvchecked[bs->legal_moves[i]]=1;break;}
			
			
		}
	if (num_good_moves == 0)
		for (i=0;i<=bs->legal_moves_num-1;++i) good_moves1[num_good_moves++]=bs->legal_moves[i];
		

	new_node = (uct_node *)malloc(sizeof(uct_node));
    init_uct_node(0, 0, new_node);
    new_node->pos = good_moves1[0];
    un->child = new_node;
    now = un->child;

    for (mi = 1; mi <= num_good_moves-1; mi++) {
        new_node = (uct_node *)malloc(sizeof(uct_node));
        init_uct_node(0, 0, new_node);
        new_node->pos = good_moves1[mi];
        now->sibling = new_node;
        now = now->sibling;
    }
	return num_good_moves;
	}

	if ( stage==2)
	{
			int cnt=0;
			intersection colorr=OTHER_COLOR(color);
			int i,j,k,flag,ai,aj,bi,bj;
			int good_moves1[MAX_BOARD * MAX_BOARD],num_good_moves=0;
		for (i=0;i<=bs->legal_moves_num-1;++i)
		{
			if (getlib(bs,bs->legal_moves[i], color)<2) continue;
			good_moves1[num_good_moves++]=bs->legal_moves[i];
			
		}
	if (num_good_moves == 0)
		for (i=0;i<=bs->legal_moves_num-1;++i) {
			if (getlib(bs,bs->legal_moves[i], color)<2) continue;
			good_moves1[num_good_moves++]=bs->legal_moves[i];
			}
	if (num_good_moves == 0)
		for (i=0;i<=bs->legal_moves_num-1;++i) {good_moves1[num_good_moves++]=bs->legal_moves[i];}
		
	new_node = (uct_node *)malloc(sizeof(uct_node));
    init_uct_node(0, 0, new_node);
    new_node->pos = good_moves1[0];
    un->child = new_node;
    now = un->child;

    for (mi = 1; mi <= num_good_moves-1; mi++) {
        new_node = (uct_node *)malloc(sizeof(uct_node));
        init_uct_node(0, 0, new_node);
        new_node->pos = good_moves1[mi];
        now->sibling = new_node;
        now = now->sibling;
    }

	return num_good_moves;
	}

	

	//ÖÐ¾Ö
	if (stage==1)
	{
		int caneat[169],numeat=0,jj;

		
			int cnt=0;
			intersection colorr=OTHER_COLOR(color);
			int i,j,k,flag,ai,aj,bi,bj;
			int mvchecked[169];
			int candidates[MAX_BOARD * MAX_BOARD];
			int max_rate=0,index_rate=-1,max_sphere=0,index_sphere=-1;
			
			int good_moves[MAX_BOARD * MAX_BOARD],good_moves1[MAX_BOARD * MAX_BOARD],num_good_moves=0;
			
			memset(mvchecked,0,sizeof(mvchecked));			
			flag=0;
			numeat=Toeat(bs,caneat,color);
		for (i=0;i<=bs->legal_moves_num-1;++i)
		{
			int times;
			if (getlib(bs,bs->legal_moves[i], color)<2) continue;
			if (good_moves[i]==1) if(mvchecked[bs->legal_moves[i]]==0){good_moves1[num_good_moves++]=bs->legal_moves[i];mvchecked[bs->legal_moves[i]]=1;}
			else 
			{
				times=0;
				for (k=0;k<=19;++k)
				{
				ai=I(bs->legal_moves[i])+shapei[k];aj=J(bs->legal_moves[i])+shapej[k];
				if (ON_BOARD(ai,aj)) {if (bs->board[POS(ai,aj)]==color ) if(mvchecked[bs->legal_moves[i]]==0){good_moves1[num_good_moves++]=bs->legal_moves[i];mvchecked[bs->legal_moves[i]]=1;break;}}
				}
				for (jj=0;jj<=numeat-1;++jj) if (bs->legal_moves[i]==caneat[jj])if(mvchecked[bs->legal_moves[i]]==0){good_moves1[num_good_moves++]=bs->legal_moves[i];mvchecked[bs->legal_moves[i]]=1;break;}
			}
		}

	if (num_good_moves == 0)
		for (i=0;i<=bs->legal_moves_num-1;++i) good_moves1[num_good_moves++]=bs->legal_moves[i];

	new_node = (uct_node *)malloc(sizeof(uct_node));
    init_uct_node(0, 0, new_node);
    new_node->pos = good_moves1[0];
    un->child = new_node;
    now = un->child;

    for (mi = 1; mi <= num_good_moves-1; mi++) {
        new_node = (uct_node *)malloc(sizeof(uct_node));
        init_uct_node(0, 0, new_node);
        new_node->pos = good_moves1[mi];
        now->sibling = new_node;
        now = now->sibling;
    }

	return num_good_moves;
	}



	
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
    //if(dbeatposition!=-1)return -169-dbeatposition;
    return cnt;
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



static uct_node* uct_select(uct_node *un)
{
    uct_node *now, *tmp;
    double uct_value, max_uct_value, win_rate;

    now = un->child;
    max_uct_value = -1000.0;
    tmp = NULL;
	//debug_log_enter();
	//debug_log("-------------------select by uvtvalue------------------");
	//debug_log_enter();
    while (now != NULL) {
        if (now->visits > 0) {
            win_rate = now->wins * 1.0 / now->visits;
            uct_value = win_rate+UCTK*sqrt(log(un->visits)*1.0/now->visits);
        } else
            uct_value = 10000;
        if (uct_value > max_uct_value) {
            max_uct_value = uct_value;
            tmp = now;
        }
		/*debug_log("x: ");
		debug_log_int(I(now->pos));
		debug_log("y: ");
		debug_log_int(J(now->pos));
		debug_log("uctvalue: ");
		debug_log_fl(uct_value);
        */
		now = now->sibling;
    }
    return tmp;
	/*debug_log("chile that chosen:");
	debug_log_int(I(tmp->pos));
	debug_log_int(J(tmp->pos));
	*/
	}

static void update_node(double res, uct_node *un) {
    un->wins += res;
    un->visits += 1;
}

static double simulate(board_status *bs, intersection color, uct_node *un)
{
    double res;
    uct_node *next;
    int i, j,k;

    if (un->child == NULL && un->visits < MAX_VISITS) //res = simulate_game(bs, color);
        {res=0;

			res=simulate_game(bs, color);
		EnterCriticalSection(&CriticalSection);
	    simulate_cnt++;
		LeaveCriticalSection(&CriticalSection);
	} 
    else {
        if (un->child == NULL)
			{if (color==gcolor) root_create_uct_children(bs, color, un);
			else create_uct_children(bs, color, un);}
        next = uct_select(un);
        if (next == NULL) {
            next = (uct_node *)malloc(sizeof(uct_node));
            init_uct_node(0, 0, next);
        }
        play_move(bs, I(next->pos), J(next->pos), color);
        color = OTHER_COLOR(color);
        res = simulate(bs, color, next);
        res = -1 * res;
    }
	EnterCriticalSection(&CriticalSection);
    update_node(-1 * res, un);
	LeaveCriticalSection(&CriticalSection);
    return res;
}

static uct_node* get_best_child(uct_node *uct_root)
{
    int max_visits = -1;
	int cnt = 0;
    uct_node *un, *bn;
    un = uct_root->child;
    bn = NULL;
    while (un != NULL) 
	{
		cnt += un->visits;
        if (un->visits > max_visits) {
            max_visits = un->visits;
            bn = un;
        }
        un = un->sibling;
    }
	stage_flag=0;
	if ((bn->wins)/(bn->visits)>0.5) stage_flag=1;
	if ((bn->wins)/(bn->visits)<-0.5) stage_flag=-1;
	
    return bn;
}

/* Generate a random move. */
int generate_random_move(board_status *bs, intersection color)
{
    int move, pos,ii,new_legal_moves[MAX_BOARD*MAX_BOARD];
    if (bs->ko_pos != POS(-1, -1)) {
        bs->legal[color-1][bs->ko_pos] = 1-is_legal_move(bs, color, bs->ko_pos);
        bs->legal[OTHER_COLOR(color)-1][bs->ko_pos] = 1-is_legal_move(bs, OTHER_COLOR(color), bs->ko_pos);
    }
    get_legal_moves2(bs, color);

    if (bs->legal_moves_num > 0) {
        move = bs->legal_moves[rand() % bs->legal_moves_num];
		

        return move;
    } else {
        /* But pass if no move was considered. */
        return -14;
    }
}

unsigned _stdcall sim_thread(void* xi) {
    time_t now = clock();
    int i, j = 0;
    int *hi = (int *)xi;
    //debug_log_int(*hi);
    while (now - start < 9000 && j < 400) {
        for (i = 0; i <= 100; i++) { 
        	memcpy(&hboard[*hi], gbs, sizeof(hboard[*hi]));
            simulate(&hboard[*hi], gcolor, groot);
        }
        j++;
        now = clock();
    }
    //debug_log_int(100*j);
}

int uct_search(board_status *bs, intersection ucolor)
{
    uct_node *uct_root;
    uct_node *best_child;
    board_status uct_board;
    intersection color;
    int i, node_num, pos;
    start = clock();
	simulate_cnt=0;
    memcpy(&uct_board, bs, sizeof(uct_board));
    color = ucolor;
    uct_root = (uct_node *)malloc(sizeof(uct_node));
    init_uct_node(0, 0, uct_root);
    node_num = root_create_uct_children(&uct_board, color, uct_root);

    groot = uct_root;
    gcolor = color;
    gbs = bs;
	InitializeCriticalSection(&CriticalSection);
    for (i = 0; i < 4; i++)
        h[i] = (HANDLE)_beginthreadex(NULL, 0, sim_thread, &f[i], 0, NULL);
    for (i = 0; i < 4; i++)
        WaitForSingleObject(h[i], INFINITE);
	DeleteCriticalSection(&CriticalSection);
    best_child = get_best_child(uct_root);
    if (best_child == NULL)
        {pos = POS(-1,-1);}
    else
        pos = best_child->pos;

    free_uct_tree(uct_root);
    return pos;
}




static
double simulate_game1(board_status *bs, intersection color)
{
	
    int pass[3];
    intersection color_now;
    int i, j, pos, step;
    double score;
	
    step = 0;
    pass[OTHER_COLOR(color)] = (bs->last_move_pos == -14);
    pass[color] = 0;
    color_now = color;
    while (!(pass[BLACK] && pass[WHITE]) && step <= 200) {
        pos = generate_random_move(bs, color_now);
        pass[color_now] = (pos == -14);
        play_move(bs, I(pos), J(pos), color_now);
        color_now = OTHER_COLOR(color_now);
    }
	score=mygetscore(bs);
    if (score >=-6.5 && color == WHITE)
        return score+6.5;
		//return 1;
    if (score < -6.5 && color == BLACK)
        return -(score+6.5);
		//return 1;
	if (score <-6.5 && color == WHITE)
		//return -1;
		return score+6.5;
	if (score >=-6.5 && color == BLACK)
		//return -1;
		return -(score+6.5);
	return 0;
}

static double simulate1(board_status *bs, intersection color, uct_node *un)
{
    double res;
    uct_node *next;
    int i, j,k;

    if (un->child == NULL && un->visits < MAX_VISITS) //res = simulate_game(bs, color);
        {res=0;

			res=simulate_game1(bs, color);
		EnterCriticalSection(&CriticalSection);
	    simulate_cnt++;
		LeaveCriticalSection(&CriticalSection);
	} 
    else {
        if (un->child == NULL)
			{if (color==gcolor) root_create_uct_children(bs, color, un);
			else create_uct_children(bs, color, un);}
        next = uct_select(un);
        if (next == NULL) {
            next = (uct_node *)malloc(sizeof(uct_node));
            init_uct_node(0, 0, next);
        }
        play_move(bs, I(next->pos), J(next->pos), color);
        color = OTHER_COLOR(color);
        res = simulate1(bs, color, next);
        res = -1 * res;
    }
	EnterCriticalSection(&CriticalSection);
    update_node(-1 * res, un);
	LeaveCriticalSection(&CriticalSection);
    return res;
}
unsigned _stdcall sim_thread1(void* xi) {
    time_t now = clock();
    int i, j = 0;
    int *hi = (int *)xi;
    //debug_log_int(*hi);
    while (now - start < 9000 && j < 400) {
        for (i = 0; i <= 100; i++) { 
        	memcpy(&hboard[*hi], gbs, sizeof(hboard[*hi]));
            simulate1(&hboard[*hi], gcolor, groot);
        }
        j++;
        now = clock();
    }
    //debug_log_int(100*j);
}

int uct_search1(board_status *bs, intersection ucolor)
{
    uct_node *uct_root;
    uct_node *best_child;
    board_status uct_board;
    intersection color;
    int i, node_num, pos;
    start = clock();
	simulate_cnt=0;
    memcpy(&uct_board, bs, sizeof(uct_board));
    color = ucolor;
    uct_root = (uct_node *)malloc(sizeof(uct_node));
    init_uct_node(0, 0, uct_root);
    node_num = root_create_uct_children(&uct_board, color, uct_root);
	if (node_num==0) return POS(-1,-1);
    groot = uct_root;
    gcolor = color;
    gbs = bs;
	InitializeCriticalSection(&CriticalSection);
    for (i = 0; i < 4; i++)
        h[i] = (HANDLE)_beginthreadex(NULL, 0, sim_thread1, &f[i], 0, NULL);
    for (i = 0; i < 4; i++)
        WaitForSingleObject(h[i], INFINITE);
	DeleteCriticalSection(&CriticalSection);
    best_child = get_best_child(uct_root);
    if (best_child == NULL)
        {pos = POS(-1,-1);
	}
    else
        pos = best_child->pos;
    free_uct_tree(uct_root);
    return pos;
}




