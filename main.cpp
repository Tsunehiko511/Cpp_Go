#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <iostream>
#include <iomanip>
#include <map>
#include <unistd.h> // usleep関数
#include <time.h>   // for clock()

using namespace std;
#define BOARD_SIZE 9
#define W_SIZE 11
#define KOMI  6.5
// 石を打ったときの処理
#define SUCCESS  0 		// 打てる
#define KILL 	 1 		// 自殺手
#define KO 		 2 		// 劫
#define ME 		 3 		// 眼
#define MISS 	 4 		// すでに石がある
#define PASS 	 5 		// パス
// 盤上の種類
#define SPACE 0
#define BLACK 1
#define WHITE 2
#define WALL  3
// 戦略
#define RANDOM 1
#define MONTE_CARLO 2
// 真偽値
#define FALSE 0
#define TRUE  1
// 座標
typedef struct{
	int y;
	int x;
} point;

typedef struct{
	int black;
	int white;
} score_t;

const char *visual[4] = {"・","🔴 ","⚪️ "};

void getNeighbors(point center, point *neighbors){
//	printf("getNeighbors\n");
	neighbors[0] = (point){center.y-1,center.x};
	neighbors[1] = (point){center.y+1,center.x};
	neighbors[2] = (point){center.y,center.x-1};
	neighbors[3] = (point){center.y,center.x+1};
}

class Board{
private:
public:
	int data[W_SIZE][W_SIZE];
	point ko;
	Board(){
		for(int y = 0; y<W_SIZE; y++){
			for (int x = 0; x<W_SIZE; x++)
			{
				this->data[y][x] = SPACE;			
			}
		}
		for(int i=0; i<W_SIZE; i++){
			this->data[0][i] = this->data[W_SIZE-1][i] = this->data[i][0] = this->data[i][W_SIZE-1] = WALL;
		}
		this->ko = (point){1000,1000};
	}
	void copy(Board *board){
		memcpy(board->data, this->data, sizeof(this->data));
		board->ko = ko;
	}
	// 石の設置と取得
	void set(point position, int stone){
//		printf("set\n");
		this->data[position.y][position.x] = stone;
	}
	int get(point position){
		return this->data[position.y][position.x];
	}
	// 取り除く
	void remove(point position){
//		printf("remove\n");
		set(position, SPACE);
	}

	// 碁盤描画
	void draw(void){
		printf("  ");
		for (int x = 1; x<W_SIZE-1; x++) printf("%d ", x);
		printf("\n");
		for (int y = 1; y<W_SIZE-1; y++){
			printf("%d ", y);
			for (int x = 1; x<W_SIZE-1; x++){
				printf("%s",visual[this->data[y][x]]);
			}
			printf("\n");
		}
	}

	vector<point> getSpaces(){
//		printf("getSpaces\n");
		vector<point> space_array;
		for(int y = 1; y<10;y++){
			for(int x = 1; x<10;x++){
				point position = (point){y,x};
				if(get(position) == SPACE){
					space_array.push_back(position);
				}
			}
		}
		return space_array;
	}
};

void count_around(int checked[9][9], Board *board, point position, int color, int* joined, int* liberty);
void count_joined_liberty(Board *board, point position, int color, int* joined, int* liberty);

void getPoints(Board *board, double *count){
	int black_points = 0;
	int white_points = 0;
	for(int y=1; y<W_SIZE-1; y++){
		for(int x=1; x<W_SIZE-1; x++){
			int data = board->get((point){y,x});
			if(data == BLACK){
				black_points += 1;
			}
			else if(data == WHITE){
				white_points += 1;
			}
			else{
				int around[4] = {0,0,0,0}; // ４方向のSPACE,BLACK,WHITE,WALLの数
				point neighbors[4];
				getNeighbors((point){y,x}, neighbors);
				for(int i=0; i<4 ;i++){
					around[board->get(neighbors[i])] += 1;
				}
				// 黒だけに囲まれていれば黒地
				if(around[BLACK] > 0 && around[WHITE] == 0){
					black_points += 1;
				}
				// 白だけに囲まれていれば白地
				if(around[WHITE] > 0 && around[BLACK] == 0){
					white_points += 1;
				}
			}
		}
	}
	count[0] = (double)black_points; // 黒石＋黒地
	count[1] = (double)white_points; // 白石＋白地
}

void scoring(Board *board, double *score){
	getPoints(board, score);
	score[0] -= KOMI;
}
void judge(double *score){
	printf("%s：%3.1f ",visual[BLACK],score[0]);
	printf("%s：%3.0f\n",visual[WHITE],score[1]);
	if(score[0]>score[1]){
		printf("黒の勝ち\n");
	}else{
		printf("白の勝ち\n");		
	}
}


class Player{
private:
public:
	int color;
	int un_color;
	int tact;

	point posi;
	Player(int color, int strategy){
		this->color = color;
		un_color = 3 - this->color;
		this->tact = strategy;
	}
	int play(Board *board){
		return tactics(board);
	}
	// 相手の石を取る
	void capture(Board *board, point position){
//		printf("capture\n");
		board->remove(position);
		point neighbors[4];
		getNeighbors(position,neighbors);
		for(int i=0; i<4; i++){
			point neighbor = neighbors[i];
			if(board->get(neighbor) == this->un_color){
				capture(board, neighbor);
			}
		}
	}
	int move(Board *board, point position){
//		printf("move\n");
		if (position.y == 0 && position.x == 0){
			return PASS;
		}
		// すでに石がある
		if(board->get(position) != SPACE){
//			printf("すでに石がある\n");
			return MISS;
		}
		// positionに対して四方向の [連石, 呼吸点, 色]を調べる
		int joineds[4] = {0,0,0,0};
		int libertys[4] = {0,0,0,0};
		int colors[4] = {0,0,0,0};

		int space = 0;
		int wall = 0;
		int mikata_safe = 0;
		int take_sum = 0;
		point ko = {0,0};
		point neighbors[4] = {0,0,0,0};
		getNeighbors(position,neighbors);
		// 打つ前の４方向をしらべる
		for(int i=0; i<4; i++){
			colors[i] = board->get(neighbors[i]);
			if (colors[i] == SPACE){
				space += 1;
				continue;
			}
			if (colors[i] == WALL){
				wall += 1;
				continue;
			}
			// 連石と呼吸点の数を数える
			count_joined_liberty(board, neighbors[i], colors[i], &joineds[i], &libertys[i]);
			if (colors[i] == this->un_color && libertys[i] == 1){
				take_sum += joineds[i];
				ko = neighbors[i];
			}
			if (colors[i] == this->color && libertys[i] >= 2){
				mikata_safe += 1;
			}
		}
		// ルール違反
		if (take_sum == 0 && space == 0 && mikata_safe == 0){
			return KILL;
		}
		if (position.y == board->ko.y && position.x == board->ko.x){
			return KO;
		}
		if(wall + mikata_safe == 4){
			return ME;
		}
		// 石を取る
		point neighbors2[4] = {0,0,0,0};
		getNeighbors(position,neighbors2);
		for (int i = 0; i < 4; ++i){
			if (colors[i] == this->un_color && libertys[i] == 1){
				capture(board, neighbors2[i]);
			}
		}
		// 石を打つ
		// printf("%s (%d,%d)\n", visual[this->color], position.y, position.x);
		board->set(position, this->color);
		int joined = 0;
		int liberty = 0;
		count_joined_liberty(board, position, this->color, &joined, &liberty);
//		printf("エラーチェック1\n");
		if (take_sum == 1 && joined == 1 && liberty == 1){
			board->ko = ko;
//			printf("エラーチェック2\n");
		}
		else{
//			printf("エラーチェック3\n");
			board->ko = (point){10000,10000};
		}
//		printf("エラーチェック4\n");
		return SUCCESS;
	}

	int random_choice(Board *board){
//		printf("random_choice\n");
		vector<point> spaces = board->getSpaces();
		int l = spaces.size();
		while(l>0){
			int n = rand()%l;
			point position = spaces[n];
			int result = move(board, position);
			//printf("random_choice_select\n");
			// board->draw();
			if(result == SUCCESS){
				posi = position;
				return SUCCESS;
			}
			// printf("l=%d\n", l);
			spaces[n] = spaces[l-1];
			l -=1;
		}
		return PASS;
	}
	int tactics(Board *board){
		return random_choice(board);
	/*
		if(this->tact==MONTE_CARLO){
			//return monte_carlo(board);
		}
		else{
			return random_choice(board);
		}
	*/
	}
};

void count_around(int checked[11][11], Board *board, point position, int color, int* joined, int* liberty){
	int y = position.y;
	int x = position.x;
	// printf("count (%d,%d)\n", y, x);	
	checked[y][x] = TRUE;
	*joined +=1;
	// 周辺を調べる
	point neighbors[4] = {(point){y-1,x}, (point){y+1,x}, (point){y,x-1}, (point){y,x+1}};
	for(int i = 0; i<4; i++){
		point neighbor = neighbors[i];
		if(checked[neighbor.y][neighbor.x]==TRUE){
			continue;
		}
		int data = board->get(neighbor);
		if(data==SPACE){
			checked[neighbor.y][neighbor.x] = TRUE;
			*liberty += 1;
		}
		else if(data == color){
			// printf("count繰り返し\n");
			count_around(checked, board, neighbor, data, joined, liberty);
		}
	}
}
void count_joined_liberty(Board *board, point position, int color, int* joined, int* liberty){
	int checked[11][11] = {{FALSE}};
	count_around(checked, board, position, color, joined, liberty);
	// printf("count_joined_liberty END\n");
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


double run(void){
	clock_t start = clock();
	// 碁盤の作成
	Board board;
	// プレイヤー
	Player black = Player(BLACK, RANDOM);
	Player white = Player(WHITE, RANDOM);
	Player player = black;
	//board.set((point){2,5},BLACK);
	// 先手
	int length = 0;
	int passed = 0;
	// 対局開始
	while(passed < 2){
		int result = player.play(&board);
		if(result==SUCCESS){
			//board.draw();
			// usleep(100000); // 1000000=1sec
		}
		// パス判定
		if (result==PASS){
			passed += 1;
		}
		else{
			passed = 0;
		}

		if(player.color == BLACK){
			player = white;
		}
		else{
			player = black;
		}
	}
	double score[2] = {0,0};
	scoring(&board, score);
	judge(score);
	clock_t end = clock();
	board.draw();

	double elap = (double)(end-start)/CLOCKS_PER_SEC;
	return elap;
}

int main(){
	double sum = 0.0;
	size_t loop_count = 1000;
	srand((unsigned) time(NULL));

	for(size_t i=0; i<loop_count; i++){
		sum += run();
	}
	std::cout << "total time " << sum << " sec. " << std::endl;
	sum /= static_cast<double>(loop_count);
	std::cout << "average time " << sum << " sec. " << std::endl;
	std::cout << "average playout " << std::setprecision(8) << (double)CLOCKS_PER_SEC / (sum * CLOCKS_PER_SEC) << std::endl;

	return 0;
}