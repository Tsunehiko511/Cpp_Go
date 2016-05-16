#include <stdio.h>
#include <math.h>
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

#define MAX_CHILD W_SIZE*W_SIZE+1 //局面での手の数　+1はPASS


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
#define RANDOM 			1
#define MONTE_CARLO 2
#define UCB 				3
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

// １手の情報を保持する構造体
typedef struct {
	point position; // 手の座標
	int win; 				// この手の勝数
	int games; 			// この手を選んだ回数
	double rate; 		// この手の勝率
} PRE_MOVE;


const char *visual[4] = {"・","🔴 ","⚪️️ "};

void getNeighbors(point center, point *neighbors){
//	printf("getNeighbors\n");
	neighbors[0] = (point){center.y-1,center.x};
	neighbors[1] = (point){center.y+1,center.x};
	neighbors[2] = (point){center.y,center.x-1};
	neighbors[3] = (point){center.y,center.x+1};
}

int isERROR(point position){
	if(position.y == 0 && position.x == 0){
		return TRUE;
	}else{
		return FALSE;
	}
}

int isPASS(point position){
	if(position.y == 0 && position.x == 0){
		return TRUE;
	}else{
		return FALSE;
	}
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
		point position;
		for(int y = 1; y<10;y++){
			for(int x = 1; x<10;x++){
				position = (point){y,x};
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
	int data;
	int around[4];
	point neighbors[4];

	for(int y=1; y<W_SIZE-1; y++){
		for(int x=1; x<W_SIZE-1; x++){
			data = board->get((point){y,x});
			if(data == BLACK){
				black_points += 1;
			}
			else if(data == WHITE){
				white_points += 1;
			}
			else{
				memset(around, 0, sizeof(around)); // ４方向のSPACE,BLACK,WHITE,WALLの数
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
		point neighbor;
		getNeighbors(position,neighbors);
		for(int i=0; i<4; i++){
			neighbor = neighbors[i];
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
		point neighbors[4];
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
		point neighbors2[4];
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

	// 盤面boardのときに相手playerから始まったときのplayout結果(score)
	void playout(Board *board, int color, double *score){
		Player player1 = Player(3-color, RANDOM);
		Player player2 = Player(color, RANDOM);
		Player player = player1;
		int passed = 0;

		while(passed<2){
			int result = player.play(board);
			if(result == SUCCESS){
				passed = 0;
			}
			else{
				passed += 1;
			}
			if(player.color==player1.color){
				player = player2;
			}
			else{
				player = player1;
			}
		}
		scoring(board, score);
	}

	int random_choice(Board *board){
//		printf("random_choice\n");
		vector<point> spaces = board->getSpaces();
		int l = spaces.size();
		int n;
		int result;
		while(l>0){
			n = rand()%l;
			point position = spaces[n];
			result = move(board, position);
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

	int monte_carlo(Board *board){
		clock_t start = clock();

		const int TRY_GAMES = 30;
		int try_total = 0;
		int best_winner = -1;
		point best_position = {0,0};
		double score[2];
		// すべての手対して１手打つ（盤面は崩れるのでコピー）
		Board thinking_board;
		Board thinking_board_next;
		vector<point> spaces = board->getSpaces();
		int l = spaces.size();
		int result;
		int win_count;

		for(int i=0; i<l; i++){
			point position = spaces[i];
			board->copy(&thinking_board);
			result = this->move(&thinking_board, position);
			if(result != SUCCESS){
				continue;
			}
			win_count = 0;
			for (int n=0; n<TRY_GAMES; n++){
				thinking_board.copy(&thinking_board_next);
				memset(score, 0.0, sizeof(score));
				// 相手プレーヤーからのplayout
				playout(&thinking_board_next, this->un_color, score);
				if((score[0] > score[1] && this->color == BLACK)||(score[0] < score[1] && this->color == WHITE)){
					win_count += 1;
				}
			}
			try_total += TRY_GAMES;

			if(win_count > best_winner){
				best_winner = win_count;
				best_position = position;
			}
		}
		printf("playout：%d 回, ", try_total);
		clock_t end = clock();
		double elap = (double)(end-start)/CLOCKS_PER_SEC;
		std::cout << "time：" << elap << "sec. " << (double)try_total/elap << "playout/sec. " << std::endl;
		// printf("%s (%d,%d)\n",visual[this->color], best_position.y,best_position.x);
		if(best_position.y==0 && best_position.x==0){
			return PASS;
		}
		posi = best_position;
		return this->move(board, best_position);
	}
	// UCBで候補から打つ手を選ぶ
	PRE_MOVE* select_with_ucb(PRE_MOVE *pre_moves, int num_pre_moves, int sum_playout){
		PRE_MOVE* selected;
		double max_ucb = -999;
		const double C = 0.31;
		for(int i=0; i<num_pre_moves; i++){
			// printf("(%d,%d)\n", pre_moves[i].position.y,pre_moves[i].position.x);
			double ucb;
			if(pre_moves[i].games == 0){
				ucb = 10000 + rand();
			}
			else{
				ucb = pre_moves[i].rate +  sqrt(log(sum_playout)/pre_moves[i].games);
			}
			if(ucb > max_ucb){
				max_ucb = ucb;
				selected = &pre_moves[i];
			}
		}
		return selected;
	}



	int ucb_choice(Board* board){
		clock_t start = clock();

		const int PLAYOUT_MAX = 2700;
		int sum_playout = 0;

		point best_position = {0,0};
		double score[2];
		// すべての手対して１手打つ（盤面は崩れるのでコピー）
		Board thinking_board;
		Board thinking_board_next;
		vector<point> spaces = board->getSpaces();
		int l = spaces.size();
		PRE_MOVE pre_moves[l];
		int result;
		const double C = 0.31;
		int num_pre_moves = 0;

		// 合法手に対して1playoutを行い勝率を取得する
		for(int i=0; i<l; i++){
			point position = spaces[i];
			board->copy(&thinking_board);
			// １手打ってみる（合法手か調べる）
			result = this->move(&thinking_board, position);
			if(result != SUCCESS){
				continue;
			}
			pre_moves[num_pre_moves].position = spaces[i];
			pre_moves[num_pre_moves].position = spaces[i];
			pre_moves[num_pre_moves].win = 0;
			pre_moves[num_pre_moves].games = 0;
			pre_moves[num_pre_moves].rate = 0.0;
			num_pre_moves += 1;
		}

		while(num_pre_moves>0){
			// 候補から手を選ぶ
			PRE_MOVE* selected = select_with_ucb( pre_moves, num_pre_moves, sum_playout);
			if(sum_playout>=PLAYOUT_MAX){
				// printf("(%d,%d)\n", selected->position.y, selected->position.x);
				best_position.y = selected->position.y;
				best_position.x = selected->position.x;
				break;
			}
			board->copy(&thinking_board);
			// １手打ってみる（合法手か調べる）
			this->move(&thinking_board, selected->position);
			// playoutする
			memset(score, 0.0, sizeof(score));
			// 相手プレーヤーからのplayout
			playout(&thinking_board, this->un_color, score);
			// playout回数をカウントする
			sum_playout += 1;
			selected->games += 1;
			// 勝率の計算をする
			if((score[0] > score[1] && this->color == BLACK)||(score[0] < score[1] && this->color == WHITE)){
				selected->win += 1;
			}
			selected->rate = selected->win/selected->games;
		}

  	printf("playout：%d 回, ", sum_playout);
		clock_t end = clock();
		double elap = (double)(end-start)/CLOCKS_PER_SEC;
		std::cout << "time：" << elap << "sec. " << (double)sum_playout/elap << "playout/sec. " << std::endl;
		if(isERROR(best_position)){
			return PASS;
		}
		posi = best_position;
		return this->move(board, best_position);
	}

	int tactics(Board *board){
		if(this->tact == MONTE_CARLO){
			return monte_carlo(board);
		}
		else if(this->tact == UCB){
			return ucb_choice(board);
		}
		else{
			return random_choice(board);
		}
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
	point neighbor;
	int data;
	for(int i = 0; i<4; i++){
		neighbor = neighbors[i];
		if(checked[neighbor.y][neighbor.x]==TRUE){
			continue;
		}
		data = board->get(neighbor);
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
	Player black = Player(BLACK, UCB);
	Player white = Player(WHITE, RANDOM);
	Player player = black;
	// 先手
	int passed = 0;

	int result;
	// 対局開始
	while(passed < 2){
		result = player.play(&board);
		if(result==SUCCESS){
			printf("%s (%d,%d)\n",visual[player.color], player.posi.y,player.posi.x);
			board.draw();
		}
		// パス判定
		if (result==PASS){
			passed += 1;
			//printf("%s　パス\n", visual[player.color]);
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
	return 0;
}
