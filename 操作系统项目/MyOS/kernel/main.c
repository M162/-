
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"


/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	struct task* p_task;
	struct proc* p_proc= proc_table;
	char* p_task_stack = task_stack + STACK_SIZE_TOTAL;
	u16   selector_ldt = SELECTOR_LDT_FIRST;
        u8    privilege;
        u8    rpl;
	int   eflags;
	int   i, j;
	int   prio;
	for (i = 0; i < NR_TASKS+NR_PROCS; i++) {
	        if (i < NR_TASKS) {     /* 任务 */
                        p_task    = task_table + i;
                        privilege = PRIVILEGE_TASK;
                        rpl       = RPL_TASK;
                        eflags    = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1 */
			prio      = 15;
                }
                else {                  /* 用户进程 */
                        p_task    = user_proc_table + (i - NR_TASKS);
                        privilege = PRIVILEGE_USER;
                        rpl       = RPL_USER;
                        eflags    = 0x202; /* IF=1, bit 2 is always 1 */
			prio      = 5;
                }

		strcpy(p_proc->name, p_task->name);	/* name of the process */
		p_proc->pid = i;			/* pid */

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(struct descriptor));
		p_proc->ldts[0].attr1 = DA_C | privilege << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(struct descriptor));
		p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;
		p_proc->regs.cs	= (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ds	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.es	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.fs	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ss	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = eflags;

		/* p_proc->nr_tty		= 0; */

		p_proc->p_flags = 0;
		p_proc->p_msg = 0;
		p_proc->p_recvfrom = NO_TASK;
		p_proc->p_sendto = NO_TASK;
		p_proc->has_int_msg = 0;
		p_proc->q_sending = 0;
		p_proc->next_sending = 0;

		for (j = 0; j < NR_FILES; j++)
			p_proc->filp[j] = 0;

		p_proc->ticks = p_proc->priority = prio;

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

        /* proc_table[NR_TASKS + 0].nr_tty = 0; */
        /* proc_table[NR_TASKS + 1].nr_tty = 1; */
        /* proc_table[NR_TASKS + 2].nr_tty = 1; */

	k_reenter = 0;
	ticks = 0;
	ESCAPE_STATE = 0;
	

	p_proc_ready	= proc_table;

	init_clock();
        init_keyboard();

	restart();

	while(1){}
}


/*****************************************************************************
 *                                get_ticks
 *****************************************************************************/
PUBLIC int get_ticks()
{
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = GET_TICKS;
	send_recv(BOTH, TASK_SYS, &msg);
	return msg.RETVAL;
}


/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{
	int fd;
	int i, n;

	char tty_name[] = "/dev_tty0";

	char rdbuf[128];


	int fd_stdin  = open(tty_name, O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);

//	char filename[MAX_FILENAME_LEN+1] = "zsp01";
	const char bufw[80] = {0};
//	const int rd_bytes = 3;
//	char bufr[rd_bytes];
	
	clear();
	welcome();
	while (1) {
		printl("[root@localhost /] ");
		int r = read(fd_stdin, rdbuf, 70);
		rdbuf[r] = 0;
		//show();
        if (strcmp(rdbuf, "process") == 0)
        {
			ProcessManage();
        }
		else if (strcmp(rdbuf, "filemng") == 0)
		{
			printf("File Manager is already running on CONSOLE-1 ! \n");
			continue;

		}else if (strcmp(rdbuf, "help") == 0)
		{
			help();
		}
		else if (strcmp(rdbuf, "runttt") == 0)
		{

			TTT(fd_stdin, fd_stdout);
		}
		
		else if (strcmp(rdbuf, "clear") == 0)
		{
			clear();
			welcome();
		}
		
		else if (strcmp(rdbuf, "escape") == 0)
		{
			clear();
			escape();
			welcome();
		}
		else
			printf("Command not found, please check!\n");
	}

	
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	char tty_name[] = "/dev_tty1";

	int fd_stdin  = open(tty_name, O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);

	char rdbuf[128];
	char cmd[8];
	char filename[120];
	char buf[1024];
	int m,n;
	printf("                        ==================================\n");
	printf("                                    File Manager           \n");
	printf("                                 Kernel on Orange's \n\n");
	printf("                        ==================================\n");
	while (1) {
		printf("$ ");
		int r = read(fd_stdin, rdbuf, 70);
		rdbuf[r] = 0;
		
		

		if (strcmp(rdbuf, "help") == 0)
		{
			printf("=============================================================================\n");
			printf("Command List     :\n");
			printf("1. create [filename]       : Create a new file \n");
			printf("2. read [filename]         : Read the file\n");
			printf("3. write [filename]        : Write at the end of the file\n");
			printf("4. delete [filename]       : Delete the file\n");
			printf("5. ls                      : Display the file list\n");
			printf("6. help                    : Display the help message\n");
			printf("==============================================================================\n");		
		}
		else if (strcmp(rdbuf, "ls") == 0)
		{		
			buf[0] = 0;
			FILEOUT = buf;	
			lseek();
			printf("%s\n", buf);
		}
		else
		{
			int fd;
			int i = 0;
			int j = 0;
			char temp = -1;
			while(rdbuf[i]!=' ')
			{
				cmd[i] = rdbuf[i];
				i++;
			}
			cmd[i++] = 0;
			while(rdbuf[i] != 0)
			{
				filename[j] = rdbuf[i];
				i++;
				j++;
			}
			filename[j] = 0;

			if (strcmp(cmd, "create") == 0)
			{
				fd = open(filename, O_CREAT | O_RDWR);
				if (fd == -1)
				{
					printf("Failed to create file! Please check the fileaname!\n");
					continue ;
				}
				buf[0] = 0;
				write(fd, buf, 1);
				printf("File created: %s (fd %d)\n", filename, fd);
				close(fd);
			}
			else if (strcmp(cmd, "read") == 0)
			{
				fd = open(filename, O_RDWR);
				if (fd == -1)
				{
					printf("Failed to open file! Please check the fileaname!\n");
					continue ;
				}
				
				n = read(fd, buf, 1024);
				
				printf("%s\n", buf);
				close(fd);

			}
			else if (strcmp(cmd, "write") == 0)
			{
				fd = open(filename, O_RDWR);
				if (fd == -1)
				{
					printf("Failed to open file! Please check the fileaname!\n");
					continue ;
				}

				m = read(fd_stdin, rdbuf,80);
				rdbuf[m] = 0;
				
				n = write(fd, rdbuf, m+1);
				close(fd);
			}
			else if (strcmp(cmd, "delete") == 0)
			{
				m=unlink(filename);
				if (m == 0)
				{
					printf("File deleted!\n");
					continue;
				}
				else
				{
					printf("Failed to delete file! Please check the fileaname!\n");
					continue;
				}

			}
			else 
			{
				printf("Command not found, Please check!\n");
				continue;
			}

			
			
		}
		
			
	}

	assert(0); /* never arrive here */
}



/*======================================================================*/

/* global variable of game Tictactoe */
int tmpQP[3][3]; //表示棋盘数据的临时数组，其中的元素0表示该格为空，
//1表示计算机放下的子，-1表示人放下的子。

#define MAX_NUM 1000//扩展生成状态节点的最大数目
const int NO_BLANK=-1001; //表示没有空格
const int TREE_DEPTH=3; //搜索树的最大深度，如果增加此值可以提高计算机的“智力”，
//但同时也需要增加MAX_NUM的值。
const int NIL=1001;    //表示空
static int s_count;     //用来表示当前分析的节点的下标

struct State//该结构表示棋盘的某个状态，也可看做搜索树中的一个节点
{
	int QP[3][3]; //棋盘格局
	int e_fun; //当前状态的评估函数值
	int child[9]; //儿女节点的下标
	int parent; //双亲节点的下标
	int bestChild; //最优节点（评估函数值最大）的儿女节点下标
}States[MAX_NUM]; //用来保存搜索树中状态节点的数组




void Init()   //初始化函数，当前的棋盘格局总是保存在States[0]中
{
	int i,j;
	s_count=0; 
	for(i=0;i<3;i++)
		for(j=0;j<3;j++)
			States[0].QP[i][j]=0; //将棋盘清空
	States[0].parent=NIL;   //初始节点没有双亲节点
}

void PrintQP() //打印当棋盘格局的函数
{
	int i,j;
	for(i=0;i<3;i++)
	{
		for(j=0;j<3;j++)
			if (States[0].QP[i][j] == -1)
			{
				printf("%c       ",1);
			}
			else if (States[0].QP[i][j] == 1)
			{
				printf("%c       ",2);
			}
			else
			{
				printf("%d       ",0);
			}

			printf("\n");
	}
}

int IsWin(struct State s) //有人赢了吗？返回0表示没有人赢，返回-1表示人赢了，返回1表示计算机赢了
{
	int i,j;
	for(i=0;i<3;i++)
	{
		if(s.QP[i][0]==1&&s.QP[i][1]==1&&s.QP[i][2]==1)return 1;
		if(s.QP[i][0]==-1&&s.QP[i][1]==-1&&s.QP[i][2]==-1)return -1;
	}
	for(i=0;i<3;i++)
	{
		if(s.QP[0][i]==1&&s.QP[1][i]==1&&s.QP[2][i]==1)return 1;
		if(s.QP[0][i]==-1&&s.QP[1][i]==-1&&s.QP[2][i]==-1)return -1;
	}
	if((s.QP[0][0]==1&&s.QP[1][1]==1&&s.QP[2][2]==1)||(s.QP[2][0]==1&&s.QP[1][1]==1&&s.QP[0][2]==1))return 1;
	if((s.QP[0][0]==-1&&s.QP[1][1]==-1&&s.QP[2][2]==-1)||(s.QP[2][0]==-1&&s.QP[1][1]==-1&&s.QP[0][2]==-1))return -1;
	return 0;
}

int e_fun(struct State s)//评估函数
{
	int flag=1;
	int i,j;
	for(i=0;i<3;i++)
		for(j=0;j<3;j++)
			if(s.QP[i][j]==0)flag= FALSE;
	if(flag)return NO_BLANK;

	if(IsWin(s)==-1)return -MAX_NUM;//如果计算机输了，返回最小值
	if(IsWin(s)==1)return MAX_NUM;//如果计算机赢了，返回最大值
	int count=0;//该变量用来表示评估函数的值

	//将棋盘中的空格填满自己的棋子，既将棋盘数组中的0变为1
	for(i=0;i<3;i++)
		for(j=0;j<3;j++)
			if(s.QP[i][j]==0)tmpQP[i][j]=1;
			else tmpQP[i][j]=s.QP[i][j];

			//电脑一方
			//计算每一行中有多少行的棋子连成3个的
			for(i=0;i<3;i++)
				count+=(tmpQP[i][0]+tmpQP[i][1]+tmpQP[i][2])/3;
			//计算每一列中有多少列的棋子连成3个的
			for(i=0;i<3;i++)
				count+=(tmpQP[0][i]+tmpQP[1][i]+tmpQP[2][i])/3;
			//斜行有没有连成3个的？
			count+=(tmpQP[0][0]+tmpQP[1][1]+tmpQP[2][2])/3;
			count+=(tmpQP[2][0]+tmpQP[1][1]+tmpQP[0][2])/3;

			//将棋盘中的空格填满对方的棋子，既将棋盘数组中的0变为-1
			for(i=0;i<3;i++)
				for(j=0;j<3;j++)
					if(s.QP[i][j]==0)tmpQP[i][j]=-1;
					else tmpQP[i][j]=s.QP[i][j];

					//对方
					//计算每一行中有多少行的棋子连成3个的
					for(i=0;i<3;i++)
						count+=(tmpQP[i][0]+tmpQP[i][1]+tmpQP[i][2])/3;
					//计算每一列中有多少列的棋子连成3个的
					for(i=0;i<3;i++)
						count+=(tmpQP[0][i]+tmpQP[1][i]+tmpQP[2][i])/3;
					//斜行有没有连成3个的？
					count+=(tmpQP[0][0]+tmpQP[1][1]+tmpQP[2][2])/3;
					count+=(tmpQP[2][0]+tmpQP[1][1]+tmpQP[0][2])/3;

					return count;
}

//计算机通过该函数决定走哪一步，并对当前的棋局做出判断。
int AutoDone()
{

	int 
		MAX_F=NO_BLANK, //保存对自己最有利的棋局（最大）的评估函数值
		parent=-1, //以当前棋局为根生成搜索树，所以当前棋局节点无双亲节点
		count,   //用来计算当前生成的最后一个扩展节点的下标

		tag;   //标示每一层搜索树中最后一个节点的下标
	int 
		max_min=TREE_DEPTH%2, //标识取下一层评估函数的最大值还是最小值？
		//max_min=1取下一层中的最大值，max_min=0取最小值
		IsOK=FALSE;    //有没有找到下一步落子的位置？
	s_count=0;   //扩展生成的节点数初始值为0

	if(IsWin(States[0])==-1)//如果人赢了
	{
		printf("Conguatulations! You Win! GAME OVER.\n");
		return TRUE;
	}

	int i,j,t,k,i1,j1;
	for(t=0;t<TREE_DEPTH;t++)//依次生成各层节点
	{
		count=s_count;//保存上一层节点生成的最大下标
		for(k=parent+1;k<=count;k++)//生成一层节点
		{
			int n_child=0;//该层节点的孩子节点数初始化为0
			for(i=0;i<3;i++)
				for(j=0;j<3;j++)
					if(States[k].QP[i][j]==0)//如果在位置(i,j)可以放置一个棋子
					{       //则
						s_count++;    //生成一个节点，节点数（最大下标）数加1
						for(i1=0;i1<3;i1++) //该3×3循环将当前棋局复制到新节点对应的棋局结构中
							for(j1=0;j1<3;j1++)
								States[s_count].QP[i1][j1]=States[k].QP[i1][j1];
						States[s_count].QP[i][j]=t%2==0?1:-1;//根据是人下还是计算机下，在空位上落子
						States[s_count].parent=k;   //将父母节点的下标k赋给新生成的节点
						States[k].child[n_child++]=s_count; //下标为k的父母节点有多了个子女

						//如果下一步有一步期能让电脑赢，则停止扩展节点，转向结局打印语句
						if(t==0&&e_fun(States[s_count])==MAX_NUM)
						{
							States[k].e_fun=MAX_NUM;
							States[k].bestChild=s_count;//最好的下一步棋所在的节点的下标为s_count
							goto L2;
						}
					}
		}
		parent=count;   //将双亲节点设置为当前双亲节点的下一层节点
//		printf("%d\n",s_count); //打印生成节点的最大下标
	}

	tag=States[s_count].parent;//设置最底层标志，以便从下到上计算最大最小值以寻找最佳解路径。
	int pos_x,pos_y;//保存计算机落子的位置
	for(i=0;i<=s_count;i++)
	{
		if(i>tag) //保留叶节点的评估函数值
		{
			States[i].e_fun=e_fun(States[i]);
		}
		else //抹去非叶节点的评估函数值
			States[i].e_fun=NIL;
	}
	while(!IsOK)//寻找最佳落子的循环
	{
		for(i=s_count;i>tag;i--)
		{
			if(max_min)//取子女节点的最大值
			{
				if(States[States[i].parent].e_fun<States[i].e_fun||States[States[i].parent].e_fun==NIL)
				{
					States[States[i].parent].e_fun=States[i].e_fun; //设置父母节点的最大最小值
					States[States[i].parent].bestChild=i;   //设置父母节点的最佳子女的下标
				}
			}
			else//取子女节点的最小值
			{
				if(States[States[i].parent].e_fun>States[i].e_fun||States[States[i].parent].e_fun==NIL)
				{
					States[States[i].parent].e_fun=States[i].e_fun; //设置父母节点的最大最小值
					States[States[i].parent].bestChild=i;   //设置父母节点的最佳子女的下标
				}
			}
		}
		s_count=tag; //将遍历的节点上移一层
		max_min=!max_min; //如果该层都是MAX节点，则它的上一层都是MIN节点，反之亦然。
		if(States[s_count].parent!=NIL)//如果当前遍历的层中不包含根节点，则tag标志设为上一层的最后一个节点的下标
			tag=States[s_count].parent;
		else
			IsOK=TRUE; //否则结束搜索
	}
	int x,y;
L2: //取落子的位置，将x,y坐标保存在变量pos_x和pos_y中，并将根（当前）节点中的棋局设为最佳儿子节点的棋局

	for(x=0;x<3;x++)
	{
		for(y=0;y<3;y++)
		{
			if(States[States[0].bestChild].QP[x][y]!=States[0].QP[x][y])
			{
				pos_x=x;
				pos_y=y;    
			}
			States[0].QP[x][y]=States[States[0].bestChild].QP[x][y]; 
		}
	}


	MAX_F=States[0].e_fun;
	//cout<<MAX_F<<endl;

	printf("The computer put a Chessman at: %d,%d\nThe QP now is:\n",pos_x+1,pos_y+1);
	PrintQP();
	if(MAX_F==MAX_NUM) //如果当前节点的评估函数为最大值，则计算机赢了
	{
		printf("The computer WIN! You LOSE! GAME OVER.\n");
		return TRUE;
	}
	if(MAX_F==NO_BLANK) //否则，如果棋盘时候没空可放了，则平局。
	{
		printf("DRAW GAME!\n");
		return TRUE;
	}
	return FALSE;
}

//用户通过此函数来输入落子的位置，
//比如，用户输入31，则表示用户在第3行第1列落子。
void UserInput(int fd_stdin,int fd_stdout)
{

	int n;
	int pos = -1,x,y;
	char szCmd[80]={0};
L1: printf("Please Input The Line Position where you put your Chessman (x): "); 
	n = read(fd_stdin,szCmd,80);
	szCmd[1] = 0;
	atoi(szCmd,&x);
	printf("Please Input The Column Position where you put your Chessman (y): "); 
	n = read(fd_stdin,szCmd,80);
	szCmd[1] = 0;
	atoi(szCmd,&y);
	if(x>0&&x<4&&y>0&&y<4&&States[0].QP[x-1][y-1]==0)
		States[0].QP[x-1][y-1]=-1;
	else
	{
		printf("Input Error!");
		goto L1;
	}

}

void TestC()
{
	spin("TestC");
}

void TTT(int fd_stdin,int fd_stdout)
{
	/*char tty_name[] = "/dev_tty2";

	char rdbuf[128];


	int fd_stdin  = open(tty_name, O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);

	printf("begin!");*/

	/*printl("!!!!!!!!!!!!!");*/
	/*printf("                        ==================================\n");
	printf("                                    File Manager           \n");
	printf("                                 Kernel on Orange's \n\n");
	printf("                        ==================================\n");*/
	char buf[80]={0};
	///*	int i = 0;
	//while(1){
	//printf("C");
	//milli_delay(200);
	//}*/
	char IsFirst = 0;
	int IsFinish = FALSE;
	while(!IsFinish)
	{

		Init();
		printf("The QiPan (QP) is: \n");

		PrintQP();
		
		printf("Do you want do first?(y/n):");
		read(fd_stdin,buf,2);
		IsFirst = buf[0];
		do{

			if(IsFirst=='y')
			{
				UserInput(fd_stdin, fd_stdout);
				IsFinish=AutoDone();
			}else{
				IsFinish=AutoDone();
				if(!IsFinish)UserInput(fd_stdin, fd_stdout);
			}
			
		}while(!IsFinish);
		if (IsFinish)
		{
			printf("Play Again?(y/n)");
			char cResult;
			read(fd_stdin,buf,2);
			cResult = buf[0];
			printf("%c",cResult);
			if (cResult == 'y')
			{
				clear();
				IsFinish = FALSE;

			}
			else
			{
				clear();
			}

		}
	}
	
}

/*****************************************************************************
 *                                panic
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...)
{
	int i;
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)&fmt + 4);

	i = vsprintf(buf, fmt, arg);

	printl("%c !!panic!! %s", MAG_CH_PANIC, buf);

	/* should never arrive here */
	__asm__ __volatile__("ud2");
}

void clear()
{
	clear_screen(0,console_table[current_console].cursor);
	console_table[current_console].crtc_start = 0;
	console_table[current_console].cursor = 0;
	
}

//void show()
//{
//	printf("%d  %d  %d  %d",console_table[current_console].con_size, console_table[current_console].crtc_start, console_table[current_console].cursor, console_table[current_console].orig);
//}

void help()
{
	printf("=============================================================================\n");
	printf("Command List     :\n");
	printf("1. process       : A process manage,show you all process-info here\n");
	printf("2. filemng       : Run the file manager\n");
	printf("3. clear         : Clear the screen\n");
	printf("4. help          : Show this help message\n");
//	printf("5. taskmanager   : Run a task manager,you can add or kill a process here\n");
	printf("5. runttt        : Run a small game on this OS\n");
	printf("6. escape        : Run another small game on this OS\n");
	printf("==============================================================================\n");		
}

void ProcessManage()
{
	int i;
	printf("=============================================================================\n");
	printf("      myID      |    name       | spriority    | running?\n");
	//进程号，进程名，优先级，是否是系统进程，是否在运行
	printf("-----------------------------------------------------------------------------\n");
	for ( i = 0 ; i < NR_TASKS + NR_PROCS ; ++i )//逐个遍历
	{
//		if ( proc_table[i].priority == 0) continue;//系统资源跳过
		printf("        %d           %s            %d                yes\n", proc_table[i].pid, proc_table[i].name, proc_table[i].priority);
	}
	printf("=============================================================================\n");



}

void welcome()
{
	printf("                        ==================================\n");
	printf("                                     MyOS v1.0.0  \n");
	printf("                               Kernel on MyTinix v1.0.2 \n\n");
	printf("                                       Welcome !\n");
	printf("                        ==================================\n");
}

void escape()
{        
    char p[21][81];
    int e[81][2],f[81],v[81],t;
    int herox,heroy,hp,score;
    int i1,j1;
    int input;
    int fd,highScore;
    int store[2];

restart:
    input=0;
    heroy=10;
    herox=40;
    hp=3;
    t=0;score=0;
    ESCAPE_STATE = 1;
    ESCAPE_INPUT_Y = 0;
    ESCAPE_INPUT_X = 0;
    for(i1 = 0; i1 < 81; ++i1)
    {
	e[i1][0]=0;
	e[i1][1]=80;
	f[i1]=0;
	v[i1]=0;
    }
    
    clear();
    while(1==1)
    {	
	//clear the memory
	for(i1=0;i1<=20;i1++)
	{
		for(j1=0;j1<=80;j1++)
		{
			p[i1][j1]=' ';
		}
	}

	//init an enemy
	f[t]=ran(1,2);	//give the enemy a radom direction
	v[t]=ran(1,3);	//give the enemy a radom velocity
	//give the enemy a radom start position
	if(f[t]==1)
	{
		e[t][0]=ran(1,19);
		e[t][1]=ran(1,2);
		if(e[t][1]==2)
		{
			e[t][1]=79;
			v[t]=-v[t];
		}
	}
	else
	{
		e[t][1]=ran(1,80);
		e[t][0]=ran(1,2);
		if(e[t][0]==2)
		{
			e[t][0]=20;
			v[t]=-v[t];
		}
	}

	//enemys move
	for(int i2=0;i2<=80;i2++)
	{
	    if(f[i2]==1)
	    {
		if((e[i2][1]>=0)&&(e[i2][1]<80)&&(e[i2][0]>0)&&(e[i2][0]<21))
		{
		    e[i2][1]=e[i2][1]+v[i2];
		    p[ e[i2][0] ][ e[i2][1] ]='-';
		}
	    }
	    else
	    {
		if((e[i2][1]>=0)&&(e[i2][1]<80)&&(e[i2][0]>0)&&(e[i2][0]<21))
		{
		    e[i2][0]=e[i2][0]+v[i2];
		    p[ e[i2][0] ][ e[i2][1] ]='|';
		}
	    }		
	}

	
	//hero moves
	/*switch(input)
	{
		case 1:
			if(--heroy<2)++heroy;
			break;
		case 2:
			if(++heroy>19)--heroy;
			break;
		case 3:
			if(--herox<1)++herox;
			break;
		case 4:
			if(++herox>78)--herox;
			break;
	}*/

	if(ESCAPE_INPUT_Y ==1)
		if(--heroy<2)++heroy;
	if(ESCAPE_INPUT_Y ==2)
		if(++heroy>19)--heroy;
	if(ESCAPE_INPUT_X ==1)
		if(--herox<1)++herox;
	if(ESCAPE_INPUT_X ==2)
		if(++herox>78)--herox;
	
	ESCAPE_INPUT_Y = 0;
	ESCAPE_INPUT_X = 0;
	
	j1=herox;
	i1=heroy;

	//check whether the hero is attacked by the enemy 
	if( (p[i1-1][j1] != ' ')||
	    (p[i1+1][j1] != ' ')||
	    (p[i1][j1] != ' ')  ||
	    (p[i1][j1+1] != ' ')||
	    (p[i1][j1-1] != ' ') )
	{
	    hp--;
	}

	//if the hero died , end the game
	if(hp<1)break;

	//print the hero
	j1=herox;
	i1=heroy;
	p[i1-1][j1]='+';
	p[i1+1][j1]='+';
	p[i1][j1]='+';
	p[i1][j1+1]='*';
	p[i1][j1-1]='*';


	clear();
	//print the current HP and score
	printf("HP:%d                Score:%d\n",hp,score);


	//print to screen
		
	for(int j=1;j<=20;j++)
	{
		write(0,p[j],80);
	}
	while(ESCAPE_STATE < 0);
	if(ESCAPE_STATE == 2)break;
	milli_delay(2500);
	if(++t > 80)
	    t=0;
	++score;
    }
    clear();
    ESCAPE_STATE = 2;

    fd = open("escape_score", O_RDWR);
    if (fd == -1)//if the file to store the score doesn't exist, create one.
    {
	fd = open("escape_score", O_CREAT | O_RDWR);
	if(fd == -1)	
	{
	    printf("Failed to create file to store the score!\n");
	    goto end;
	}
	else
	    highScore=0;
    }
    else	//if the file to store the score exists, read it.
    {
	read(fd, store, 2);
	highScore = store[0];
	close(fd);
	fd = open("escape_score", O_RDWR);
    }

    if(score > highScore)
    {
	printf("HIGH SCORE!!!\n\n\n");
	highScore = score;
	store[0] = score;
	store[1] = 0;
	write(fd, store , 2);
    }
				
    close(fd);
    printf("Game Over\n\n\nThe highest score is %d\n\nYour score is %d\n\n\ntype esc to exit, type enter to restart.\n",highScore,score);

    end:
    while(ESCAPE_STATE == 2);
    if(ESCAPE_STATE ==1)goto restart;
    clear();
}






