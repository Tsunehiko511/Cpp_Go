#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <iostream>

using namespace std;
#define BOARD_SIZE 9
#define W_SIZE 11


// 石を打ったときの処理
#define SUCCESS  0 		// 打てる
#define KILL 	 1 		// 自殺手
#define KO 		 2 		// 劫
#define ME 		 3 		// 眼
#define MISS 	 4 		// すでに石がある
#define PASS 	 5 		// パス

#define SPACE 0
#define BLACK 1
#define WHITE 2
#define WALL  3

#define FALSE 0
#define TRUE  1

typedef struct{
	int y;
	int x;
} point;




const char *visual[4] = {"・","🔴 ","⚪️ ", "◼️"};

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
	// 取り除く
	void remove(int y, int x){
		set(y, x, SPACE);
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

	vector<point> getSpace(){
		vector<point> space_array;
		for(int y = 1; y<10;y++){
			for(int x = 1; x<10;x++){
				if(get(y,x) == SPACE){
					space_array.push_back((point){y,x});
				}
			}
		}
		return space_array;
	}
};

class Player{
private:
	int color;
	int un_color;
public:
	int play(){
		return SUCCESS;
	}
};


void count_around(int checked[9][9], Board board, int y, int x, int color, int* joined, int* liberty){
	checked[y][x] = TRUE;
	*joined +=1;
	// 周辺を調べる
	point neighbors[4] = {(point){y-1,x}, (point){y+1,x}, (point){y,x-1}, (point){y,x+1}};
	for(int i = 0; i<4; i++){
		point neighbor = neighbors[i];
		if(checked[neighbor.y][neighbor.x]==TRUE){
			continue;
		}
		int data = board.get(neighbor.y, neighbor.x);
		if(data==SPACE){
			checked[neighbor.y][neighbor.x] = TRUE;
			*liberty += 1;
		}
		else if(data == color){
			count_around(checked, board, neighbor.y, neighbor.x, data, joined, liberty);
		}
	}
}
void count_joined_liberty(Board board, int y, int x, int color, int* joined, int* liberty){
	int checked[9][9] = {{FALSE}};
	count_around(checked, board, y, x, color, joined, liberty);
	return;
}

// 二次元配列を受け取り変更して返す
int(* double_array(int array[][9]))[9]{
	for(int i = 0; i<10; i++){
		for(int j = 0; j<10;j++){
			array[i][j] = 1;
		}
	}
	return array;
}


int main(void){
	srand((unsigned) time(NULL));
	// 碁盤の作成
	Board board;

	// プレイヤー
	int player = BLACK;

	// 先手
	int length=81;
	int passed = 0;

	// 対局開始
	while(1){
		// 空点の取得
		vector<point> v = board.getSpace();
		length = v.size();
		if (length == 0){
			break;
		}
		// 空点の中から手を選ぶ
		int r = rand()%length;
		int y = v[r].y;
		int x = v[r].x;
		printf("(%d,%d)\n", y, x);
		// 打つ
		board.set(y,x,player);
		// 打った結果を取得
		int result = SUCCESS;
		if(result==SUCCESS){
		}
		else{
			printf("エラー\n");
			break;
		}

		// パス判定
		if (result==PASS)
		{
			passed += 1;
		}
		else{
			passed = 0;
		}
		board.draw();
		player = 3 - player;
	}
	return 0;
}




