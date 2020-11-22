#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

#define wrong(){printf("Usage ./player [playerid]\n"); exit(1);}
#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)

int main(int argc, char** argv){
    if (argc != 2) wrong();
    int bid_list[21] = {20, 18, 5, 21, 8, 7, 2, 19, 14, 13, 9, 1, 6, 10, 16, 11, 4, 12, 15, 17, 3};
    int bid;
    int player_id=atoi(argv[1]);
	for (int rounds = 1; rounds <= 10; rounds++){
		bid = bid_list[player_id + rounds - 2] * 100;
		printf("%d %d\n", player_id, bid);
	}
	return 0;
} 