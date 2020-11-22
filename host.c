#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>

#define wrong(){printf("Usage ./host [host-id] [key] [depth]\n"); exit(1);}
#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)
#define L0 "\033[m"
#define L1 "\033[0;32;32m"
#define L2 "\033[0;35m"

typedef struct {
	int id;
	int score;
	int rank;
} Player;

int cur_max(Player *ply){
	int max=0;
	for(int i=0; i<8; i++){
		if (ply[i].rank!=0)		continue;
		if (ply[i].score>max)	max = ply[i].score;
	}
	return max;
}

int main(int argc, char** argv)// ./host [host-id] [key] [depth]
{	
	if (argc != 4) wrong();
	if (atoi(argv[3]) == 0){ //root host
		//------------------------------do only once before exit--------------------------

		/*********************************************************************************
		*	varable initial
		*********************************************************************************/
		pid_t c1_pid,c2_pid;	// pid of l1 child, life cycle:until -1
		int pipe1_fdr[2], pipe1_fdw[2]; //for child1
		int pipe2_fdr[2], pipe2_fdw[2];	//for child2
		

		/*********************************************************************************
		*	FIFO and pipe 
		*********************************************************************************/

		// create pipe for child commnuication
		if (pipe(pipe1_fdr)<0)	ERR_EXIT("pipe error");
		if (pipe(pipe1_fdw)<0)	ERR_EXIT("pipe error");
		
		if ((c1_pid=fork())<0)	ERR_EXIT("fork error");
		if (c1_pid==0){	// l1 child1

			if (pipe1_fdw[0] != STDIN_FILENO){
				dup2(pipe1_fdw[0], STDIN_FILENO); //child redirect to stdin				
				close(pipe1_fdw[0]);
			}
			if (pipe1_fdr[1] != STDOUT_FILENO){
				dup2(pipe1_fdr[1], STDOUT_FILENO); //child redirect to stdout				
				close(pipe1_fdr[1]);
			}
			close(pipe1_fdw[1]);
			close(pipe1_fdr[0]);

			execlp("./host", "./host", argv[1], argv[2], "1",(char *)NULL);	//child go to l1 host
		}

		//parent port of child 1 pipe: pipe1_fdw[1] for write and pipe1_fdr[0] for read
		close(pipe1_fdw[0]);
		close(pipe1_fdr[1]);
		// still root host
		if (pipe(pipe2_fdr)<0)	ERR_EXIT("pipe error");
		if (pipe(pipe2_fdw)<0)	ERR_EXIT("pipe error");

		if ((c2_pid=fork())<0)	ERR_EXIT("fork error");
		if (c2_pid==0){	// l1 child2
			//close all fd of child 1

			close(pipe1_fdr[0]);
			close(pipe1_fdw[1]);
			
			if (pipe2_fdw[0] != STDIN_FILENO){
				dup2(pipe2_fdw[0], STDIN_FILENO); //child redirect to stdin				
				close(pipe2_fdw[0]);
			}
			if (pipe2_fdr[1] != STDOUT_FILENO){
				dup2(pipe2_fdr[1], STDOUT_FILENO); //child redirect to stdout				
				close(pipe2_fdr[1]);
			}
			close(pipe2_fdw[1]);
			close(pipe2_fdr[0]);
			execlp("./host", "./host", argv[1], argv[2], "1", (char *)NULL);//child go to l1 host
		}
		//parent port of child 2 pipe: pipe2_fdw[1] for write and pipe2_fdr[1] for read	
		close(pipe2_fdw[0]);
		close(pipe2_fdr[1]);
		//finish create child
		FILE *ch1_r=fdopen(pipe1_fdr[0], "r"), *ch1_w=fdopen(pipe1_fdw[1], "w");
		FILE *ch2_r=fdopen(pipe2_fdr[0], "r"), *ch2_w=fdopen(pipe2_fdw[1], "w");
	
		char r_filename[20];
		sprintf(r_filename, "fifo_%s.tmp", argv[1]);
		char *w_filename="fifo_0.tmp";

		FILE *r_fifo=fopen(r_filename,"r");
		//------------------------------do every auction auction--------------------------

		/*********************************************************************************
		*	variable in while loop
		*********************************************************************************/

		Player players[8];		// life cycle: per host
		int op1[2];
		int op2[2];
		int ranking,cur_add,max;
		while(1){
			/*********************************************************************************
			*	get player id
			*********************************************************************************/
			//read player id, life cycle: per host
			//init_player(&players[0]);	// life cycle: per host
			for(int i=0; i<8; i++){
					players[i].id=0;
					players[i].score=0;
					players[i].rank=0;
				}
			fscanf(r_fifo, "%d %d %d %d %d %d %d %d", 
					&players[0].id, &players[1].id, 
					&players[2].id, &players[3].id, 
					&players[4].id, &players[5].id, 
					&players[6].id, &players[7].id);
			
			/*********************************************************************************
			*	write to child
			*********************************************************************************/		

			//send player to child
		
			fprintf(ch1_w, "%d %d %d %d\n", players[0].id, players[1].id, players[2].id, players[3].id);fflush(ch1_w);
			fprintf(ch2_w, "%d %d %d %d\n", players[4].id, players[5].id, players[6].id, players[7].id);fflush(ch2_w);
			if(players[0].id == -1)	break;
			/*********************************************************************************
			*	start 10 rounds
			*********************************************************************************/	

			//read from child
			//printf(L0"start 10 rounds\n");
			for (int rounds=0; rounds<10; rounds++){
				
				fscanf(ch1_r, "%d %d", &op1[0], &op1[1]);
				fscanf(ch2_r, "%d %d", &op2[0], &op2[1]);
				
				int winner = op1[1]>op2[1]?op1[0]:op2[0];
				int target = 0;
				while (players[target].id != winner){
					target++;				
				}
				players[target].score++;							
			}
			//printf(L0"done 10 rounds\n");
			
			/*********************************************************************************
			*	start ranking
			*********************************************************************************/	

			ranking = 1;
			cur_add = 0;
			max = 0;
			while(ranking!=9){
				max = cur_max(players);
				for(int i=0; i<8; i++){
					if (players[i].score==max){
						players[i].rank=ranking;
						cur_add++;
					}
				}
				ranking+=cur_add;
				cur_add=0;
				//printf("rank%d\n",ranking);
			}
			//printf("opening FIFO start\n");
			FILE *w_fifo=fopen(w_filename,"w");
			//printf("opening FIFO done\n");
			//printf("writing FIFO start\n");
			fprintf(w_fifo,"%s\n%d %d\n%d %d\n%d %d\n%d %d\n%d %d\n%d %d\n%d %d\n%d %d\n", argv[2],
							players[0].id, players[0].rank, players[1].id, players[1].rank, 
							players[2].id, players[2].rank, players[3].id, players[3].rank, 
							players[4].id, players[4].rank, players[5].id, players[5].rank, 
							players[6].id, players[6].rank, players[7].id, players[7].rank);fflush(w_fifo);
			//printf("writing FIFO done\n");
			
		}
		/*********************************************************************************
		*	end routine
		*********************************************************************************/

		fclose(ch1_w);fclose(ch1_r);
		fclose(ch2_w);fclose(ch2_r);
		close(pipe1_fdr[0]);close(pipe1_fdw[1]);
		close(pipe2_fdr[0]);close(pipe2_fdw[1]);
		// int status_c1, status_c2;
		// waitpid(c1_pid,&status_c1);
		waitpid(c1_pid,NULL,0);
		waitpid(c2_pid,NULL,0);
	}
	else if (atoi(argv[3]) == 1){
		/*********************************************************************************
		*	varable initial
		*********************************************************************************/
		pid_t c1_pid,c2_pid;	// pid of l1 child, life cycle:until -1
		int pipe1_fdr[2], pipe1_fdw[2]; //for child1
		int pipe2_fdr[2], pipe2_fdw[2];	//for child2
		
		/*********************************************************************************
		*	FIFO and pipe 
		*********************************************************************************/

		// create pipe for child commnuication
		if (pipe(pipe1_fdr)<0)	ERR_EXIT("pipe error");
		if (pipe(pipe1_fdw)<0)	ERR_EXIT("pipe error");
		
		if ((c1_pid=fork())<0)	ERR_EXIT("fork error");
		if (c1_pid==0){	// l2 child1
			if (pipe1_fdw[0] != STDIN_FILENO){
				dup2(pipe1_fdw[0], STDIN_FILENO); //child redirect to stdin				
				close(pipe1_fdw[0]);
			}
			if (pipe1_fdr[1] != STDOUT_FILENO){
				dup2(pipe1_fdr[1], STDOUT_FILENO); //child redirect to stdout				
				close(pipe1_fdr[1]);
			}
			close(pipe1_fdw[1]);
			close(pipe1_fdr[0]);

			execlp("./host", "./host", argv[1], argv[2], "2", NULL);	//child go to l1 host
		}
		//parent port of child 1 pipe: pipe1_fdw[1] for write and pipe1_fdr[1] for read
		close(pipe1_fdw[0]);
		close(pipe1_fdr[1]);

		// still root host
		if (pipe(pipe2_fdr)<0)	ERR_EXIT("pipe error");
		if (pipe(pipe2_fdw)<0)	ERR_EXIT("pipe error");

		if ((c2_pid=fork())<0)	ERR_EXIT("fork error");
		if (c2_pid==0){	// l2 child2
			//close all fd of child 1
			close(pipe1_fdr[0]);
			close(pipe1_fdw[1]);
			
			if (pipe2_fdw[0] != STDIN_FILENO){
				dup2(pipe2_fdw[0], STDIN_FILENO); //child redirect to stdin				
				close(pipe2_fdw[0]);
			}
			if (pipe2_fdr[1] != STDOUT_FILENO){
				dup2(pipe2_fdr[1], STDOUT_FILENO); //child redirect to stdout				
				close(pipe2_fdr[1]);
			}
			close(pipe2_fdw[1]);
			close(pipe2_fdr[0]);
			execlp("./host", "./host", argv[1], argv[2], "2", NULL);//child go to l1 host
		}
		//parent port of child 2 pipe: pipe2_fdw[1] for write and pipe2_fdr[1] for read	
		close(pipe2_fdw[0]);
		close(pipe2_fdr[1]);
		//finish create child
		FILE *ch1_r=fdopen(pipe1_fdr[0], "r"), *ch1_w=fdopen(pipe1_fdw[1], "w");
		FILE *ch2_r=fdopen(pipe2_fdr[0], "r"), *ch2_w=fdopen(pipe2_fdw[1], "w");
		//------------------------------do every auction auction--------------------------

		/*********************************************************************************
		*	variable in while loop
		*********************************************************************************/

		int players[4];
		int op1[2];
		int op2[2];
		while(1){
			/*********************************************************************************	
			*	read players from parent
			*********************************************************************************/
			//fprintf(stderr,L1"l1 from l0 start\n");
			scanf("%d %d %d %d", &players[0], &players[1], &players[2], &players[3]);
			//fprintf(stderr,L1"l1 from l0 done\n");

			/*********************************************************************************
			*	write to child
			*********************************************************************************/		

			//send player to child
			//fprintf(stderr,L1"l1 to l2 start\n");
			fprintf(ch1_w, "%d %d\n", players[0], players[1]);fflush(ch1_w);
			fprintf(ch2_w, "%d %d\n", players[2], players[3]);fflush(ch2_w);
			//fprintf(stderr,L1"l1 to l2 done\n");
			if(players[0] == -1)	break;	

			/*********************************************************************************
			*	start 10 rounds
			*********************************************************************************/	

			//read from child and write bigger to parent(stdout)
			for (int rounds=0; rounds<10; rounds++){
				//fprintf(stderr,L1"reply from l2 start\n");
				fscanf(ch1_r, "%d %d", &op1[0], &op1[1]);
				fscanf(ch2_r, "%d %d", &op2[0], &op2[1]);
				//fprintf(stderr,L1"reply from l2 end\n");
				//fprintf(stderr,L1"l1 to l0 start\n");
				if(op1[1]>op2[1]){
					printf("%d %d\n", op1[0], op1[1]);fflush(stdout);
					}
				else{
					printf("%d %d\n", op2[0], op2[1]);fflush(stdout);
					}	
				//fprintf(stderr,L1"l1 to l0 done\n");						
			}		
		}
		fclose(ch1_w);fclose(ch1_r);
		fclose(ch2_w);fclose(ch2_r);
		close(pipe1_fdr[0]);close(pipe1_fdw[1]);
		close(pipe2_fdr[0]);close(pipe2_fdw[1]);
		waitpid(c1_pid,NULL,0);
		waitpid(c2_pid,NULL,0);
	}
	else if (atoi(argv[3]) == 2){
		int players[2];
		int op1[2], op2[2];
		
		while(1){
			//fprintf(stderr,L2"l2 from l1 start\n");
			scanf("%d %d", &players[0], &players[1]);
			//fprintf(stderr,L2"l2 from l1 done\n");
			if (players[0] == -1)	break;
			char ply1[4], ply2[4];
			sprintf(ply1,"%d",players[0]);
			sprintf(ply2,"%d",players[1]);
			//fprintf(stderr,L2"%s\n",ply1);
			/*********************************************************************************
			*	varable initial
			*********************************************************************************/
			pid_t ply1_pid, ply2_pid;	// pid of player
			int pipe1_fdr[2];	//for player1
			int pipe2_fdr[2];	//for player2
			//fprintf(stderr,L2"fork players start\n");
			/*********************************************************************************
			*	create pipe
			*********************************************************************************/
			if (pipe(pipe1_fdr)<0)	ERR_EXIT("pipe error");
			
			if ((ply1_pid=fork())<0)	ERR_EXIT("fork error");
			if (ply1_pid==0){	// player1
				if (pipe1_fdr[1] != STDOUT_FILENO){
					dup2(pipe1_fdr[1], STDOUT_FILENO); //child redirect to stdout				
					close(pipe1_fdr[1]);
				}
				close(pipe1_fdr[0]);
				execlp("./player", "./player", ply1, NULL);	//child go to l1 host
			}
			close(pipe1_fdr[1]);

			if (pipe(pipe2_fdr)<0)	ERR_EXIT("pipe error");
			
			if ((ply2_pid=fork())<0)	ERR_EXIT("fork error");
			if (ply2_pid==0){	// player1
				close(pipe1_fdr[0]);
				if (pipe2_fdr[1] != STDOUT_FILENO){
					dup2(pipe2_fdr[1], STDOUT_FILENO); //child redirect to stdout				
					close(pipe2_fdr[1]);
				}
				close(pipe2_fdr[0]);
				execlp("./player", "./player", ply2, NULL);	//child go to l1 host
			}
			close(pipe2_fdr[1]);
			FILE *ch1_r=fdopen(pipe1_fdr[0], "r");
			//if(ch1_r==NULL)	fprintf(stderr,L2"error:%s\n",strerror(errno));
			FILE *ch2_r=fdopen(pipe2_fdr[0], "r");
			//fprintf(stderr,L2"fork players done\n");
			/*********************************************************************************
			*	start 10 rounds
			*********************************************************************************/	

			//read from child and write bigger to parent(stdout)
			for (int rounds=0; rounds<10; rounds++){
				// fprintf(stderr,L2"get player status start\n");
				fscanf(ch1_r, "%d %d", &op1[0], &op1[1]);
				fscanf(ch2_r, "%d %d", &op2[0], &op2[1]);
				// fprintf(stderr,L2"player %d:%d!\n",op1[0],op1[1]);
				// fprintf(stderr,L2"L2 to L1 start\n");
				if(op1[1]>op2[1]){					
					printf("%d %d\n", op1[0], op1[1]);fflush(stdout);
					}
				else{
					printf("%d %d\n", op2[0], op2[1]);fflush(stdout);
					}
				// fprintf(stderr,L2"L2 to L1 end\n");							
			}
			fclose(ch1_r);
			fclose(ch2_r);
			close(pipe1_fdr[0]);
			close(pipe2_fdr[0]);
			while ((wait(NULL)) > 0);
			waitpid(ply1_pid,NULL,0);
			waitpid(ply2_pid,NULL,0);
			// fprintf(stderr,L2"wait done\n");
		}
		exit(0);
	}
	return 0;
}