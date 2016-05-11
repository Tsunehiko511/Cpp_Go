#include <stdio.h>

#define SPACE 0
#define BLACK 1
#define WHITE 2
#define WALL  3

#define BOARD_SIZE 9
#define W_SIZE BOARD_SIZE+2

const char *visual[3] = {"・","🔴 ","⚪️ "};

class Board{
private:
	int data[W_SIZE][W_SIZE];
public:
	Board(){
		for(int y = 0; y<W_SIZE; y++){
			for (int x = 0; x<W_SIZE; x++)
			{
				data[y][x] = SPACE;
			}
		}
		for(int i=0; i<W_SIZE; i++){
			data[0][i] = data[W_SIZE-1][i] = data[i][0] = data[i][W_SIZE-1] = WALL;
		}
	}

	// 石の設置と取得
	void set(int y, int x, int stone){
		data[y][x] = stone;
	}
	int get(int y, int x){
		return data[y][x];
	}

	// 碁盤描画
	void draw(void){
		printf("  ");
		for (int x = 1; x<W_SIZE-1; x++) printf("%d ", x);
		printf("\n");
		for (int y = 1; y<W_SIZE-1; y++){
			printf("%d ", y);
			for (int x = 1; x<W_SIZE-1; x++){
				printf("%s",visual[data[y][x]]);
			}
			printf("\n");
		}
	}
};

int main(void){
	// 碁盤の作成
	Board board;
	board.draw();
	board.set(2, 5, BLACK);
	board.set(3, 5, WHITE);
	board.draw();

	// プレイヤー

	// 先手

	// 対局開始

	return 0;
}