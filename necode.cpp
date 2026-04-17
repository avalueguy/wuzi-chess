#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include<time.h>
#include <array>
#include <algorithm>
#include <functional>
#include<iostream>
using namespace std;

#pragma warning(disable:4996);

#define BLACK 0
#define WHITE 1
#define EMPTY 2

#define zero 0
#define one 1
#define two 30
#define three 32
#define four 600
#define five 800
#define six  500000

#define Azero 0
#define Aone -1
#define Atwo -50
#define Athree -52
#define Afour -800
#define Afive -1000
#define Asix  -1000000


#define DT 2         //搜索广度
#define CG 400       //可行域大小
#define GG 20000     //树层上线

int KK;


struct Point { //点结构
	int x, y;
};
struct Step { //步结构
	Point first, second;
	int value;
};

typedef struct Tree { //博弈树

	Point first, second;         //当前的两个决策点
	Point ffirst, fsecond;		 //根的选择
	int value;                   //当前棋盘
}Tree;

typedef struct cango {
	int x;
	int y;
	int va;                     //用于启发式搜索排序
}cango;

struct {
	bool operator()(cango a, cango b)const {
		return a.va > b.va;
	}
}customLessA;

struct {
	bool operator()(cango a, cango b)const {
		return a.va < b.va;
	}
}customLessB;


int Board[19][19];//存储棋盘信息，其元素值为 BLACK, WHITE, EMPTY 之一
///////////////////////////////////
///////////////////////////////////
/////////////////////////////////
/////////////////////////////////
////////////////////////////////
//////////////////////////////////
///////////////////////////////////

/////////////////////////////////////////////////////////////////////////////复制棋盘  展示棋盘
void copy(int Board[19][19], int board[19][19])            //棋盘复刻为动态数组
{
	for (int i = 0; i < 19; i++) {
		for (int j = 0; j < 19; j++) {
			board[i][j] = Board[i][j];
		}
	}
}

/////////////////////////////////////////////////////////////////////////////评分模块
int power(int a[6], int tt)
{
	int j;
	int count1 = 0;   //我的棋子的个数
	int count2 = 0;   //对面的棋子的个数


	for (j = 0; j < 6; j++)     //数当前路中自己的棋子的个数   对面的棋子的个数   空白的个数
	{
		if (a[j] == tt)    count1++;
		if (a[j] == 1 - tt)count2++;
	}

	if (count2 == 0) {
		if (count1 == 0) { return zero; }
		if (count1 == 1) { return one; }
		if (count1 == 2) { return two; }
		if (count1 == 3) { return three; }
		if (count1 == 4) { return four; }
		if (count1 == 5) { return five; }
		if (count1 == 6) { return six; }
	}
	if (count1 == 0) {
		if (count2 == 0) { return Azero; }
		if (count2 == 1) { return Aone; }
		if (count2 == 2) { return Atwo; }
		if (count2 == 3) { return Athree; }
		if (count2 == 4) { return Afour; }
		if (count2 == 5) { return Afive; }
		if (count2 == 6) { return Asix; }
	}
	return 0;
}

void givepower(int tt, int chestmoble[3][3][3][3][3][3])    //tt是自己执棋的颜色
{
	int i[6];
	int a = 0;
	for (i[0] = 0; i[0] < 3; i[0]++) {
		for (i[1] = 0; i[1] < 3; i[1]++) {
			for (i[2] = 0; i[2] < 3; i[2]++) {
				for (i[3] = 0; i[3] < 3; i[3]++) {
					for (i[4] = 0; i[4] < 3; i[4]++) {
						for (i[5] = 0; i[5] < 3; i[5]++) {
							chestmoble[i[0]][i[1]][i[2]][i[3]][i[4]][i[5]] = power(i, tt);
						}
					}
				}
			}
		}
	}
}

int givevalue(int board[19][19], int tt, int chestmoble[3][3][3][3][3][3])         //计算如果下在当前点的话当前棋盘的得分
{
	int i, j;
	int value = 0;
	int value1 = 0;                //横向路价值
	int value2 = 0;                //竖向路价值
	int value3 = 0;                //左上右下斜向路价值
	int value4 = 0;                //右上左下斜向路价值
	int bb[19][19];
	for (i = 0; i < 19; i++) {
		for (j = 0; j < 19; j++) {
			bb[i][j] = board[i][j];
		}
	}
	////////////横向路搜索价值
	for (i = 0; i < 19; i++) {
		for (j = 0; j < 14; j++) {
			value1 += chestmoble[bb[j][i]][bb[j + 1][i]][bb[j + 2][i]][bb[j + 3][i]][bb[j + 4][i]][bb[j + 5][i]];
		}
	}

	////////竖向路搜索价值
	for (i = 0; i < 19; i++) {
		for (j = 0; j < 14; j++) {
			value2 += chestmoble[bb[i][j]][bb[i][j + 1]][bb[i][j + 2]][bb[i][j + 3]][bb[i][j + 4]][bb[i][j + 5]];
		}
	}



	int p, q;    //斜向路搜索价值辅助变量
	////////右上左下  /   斜向路搜索价值
	//上半部分
	for (i = 5; i < 19; i++) {
		for (j = 0; j < i - 4; j++) {
			p = i - j;
			value3 += chestmoble[bb[p][j]][bb[p - 1][j + 1]][bb[p - 2][j + 2]][bb[p - 3][j + 3]][bb[p - 4][j + 4]][bb[p - 5][j + 5]];
		}
	}
	//下半部分
	for (i = 13; i > 0; i--) {
		for (j = 0; j < 14 - i; j++) {
			q = i + j;
			value3 += chestmoble[bb[q][18 - j]][bb[q + 1][17 - j]][bb[q + 2][16 - j]][bb[q + 3][15 - j]][bb[q + 4][14 - j]][bb[q + 5][13 - j]];
		}
	}

	///////左上右下  \   斜向路搜索价值
	//上半部分
	for (i = 13; i > -1; i--) {
		for (j = 0; j < 14 - i; j++) {
			p = i + j;
			value4 += chestmoble[bb[p][j]][bb[p + 1][j + 1]][bb[p + 2][j + 2]][bb[p + 3][j + 3]][bb[p + 4][j + 4]][bb[p + 5][j + 5]];
		}
	}
	//下半部分
	for (i = 5; i < 18; i++) {
		for (j = 0; j < i - 4; j++) {
			q = i - j;
			value4 += chestmoble[bb[q][18 - j]][bb[q - 1][17 - j]][bb[q - 2][16 - j]][bb[q - 3][15 - j]][bb[q - 4][14 - j]][bb[q - 5][13 - j]];
		}
	}
	value = value1 + value2 + value3 + value4;

	return value;

}

////////////////////////////////////////////////////////////////////////////局部优先模块
void judgecango(int bigboard[25][25], int yet[25][25], int x, int y, int& cc) {

	int a = x - 3;
	int b = y - 3;
	int i, j;
	for (i = x - DT; i < x + DT + 1; i++) {
		for (j = y - DT; j < y + DT + 1; j++) {
			if (bigboard[i][j] != -1 && bigboard[i][j] != 2 && yet[i][j] != 1) {     //当这个点不越界，不是棋子的时候,未被考虑过的时候
				bigboard[i][j] = 1;
				cc++;
				yet[i][j] = 1;
			}
		}
	}

}

void savecanpoint(int board[19][19], cango can[200], int& cc)
{
	cc = 0;
	int i, j;
	int bigboard[25][25];
	int yet[25][25];
	for (i = 0; i < 25; i++) {                       //初始化
		for (j = 0; j < 25; j++) {
			yet[i][j] = 0;                           //未被考虑  记未0  考虑后带入函数   记未1
			if (i < 3 || j < 3 || i>21 || j>21) {
				bigboard[i][j] = -1;                     //棋局外的点  -1
			}
			else {
				bigboard[i][j] = 0;                      //棋局内的点存为0
				if (board[i - 3][j - 3] != 2) {            //棋局内的点不是空白时
					bigboard[i][j] = 2;                  //棋局内是棋子的点 保存为 2 
				}
			}
		}
	}

	for (i = 0; i < 25; i++) {                           //将棋子附近的点标记为可行
		for (j = 0; j < 25; j++) {
			if (bigboard[i][j] == 2) {
				judgecango(bigboard, yet, i, j, cc);
			}
		}
	}

	int k = 0;                                         //保存适合行走数组的辅助变量
	for (i = 0; i < 25; i++) {
		for (j = 0; j < 25; j++) {
			if (bigboard[i][j] == 1) {
				can[k].x = i - 3;
				can[k].y = j - 3;
				k++;
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////启发式搜索模块

void sortA(cango can[CG], int cc, int chestmoble[3][3][3][3][3][3], int board[19][19],int tt,int& HH) {
	int SS;
	SS = givevalue(board, tt, chestmoble);
	for (int i = 0; i < cc; i++) {
		board[can[i].x][can[i].y] = tt;
		can[i].va = givevalue(board, tt, chestmoble);
		if (SS - can[i].va == 0) {
			HH++;
		}
		board[can[i].x][can[i].y] = EMPTY;
	}
	std::sort(can, can + cc, customLessA);
}

void sortB(cango can[CG], int cc, int chestmoble[3][3][3][3][3][3], int board[19][19], int tt,int& HH) {

	int SS;
	SS= givevalue(board, tt, chestmoble);
	for (int i = 0; i < cc; i++) {
		board[can[i].x][can[i].y] = 1-tt;
		can[i].va = givevalue(board, tt, chestmoble);
		if (SS - can[i].va == 0) {
			HH++;
		}
		board[can[i].x][can[i].y] = EMPTY;
	}
	std::sort(can, can + cc, customLessB);
}

/////////////////////////////////////////////////////////////////////////////和局判断模块
bool peace(int board[19][19], int tt,int chestmoble[3][3][3][3][3][3]) {
	int VV;
	if (KK > 55) {
		VV= givevalue(board, tt, chestmoble);
		if (VV == 0) {
			return true;
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////博弈树模块

void searchtwo(int board[19][19], int tt, int chestmoble[3][3][3][3][3][3], Tree& FF, int CT) {
	int cc;
	cango can[CG];
	savecanpoint(board, can, cc);

	int VV;
	int ZZ = 100000;
	int i, j, k;
	int HH=0;
	sortB(can, cc, chestmoble, board,tt,HH);       //对可行步进行影响力排序
	if (peace(board, tt, chestmoble) != true) {
		cc = cc - HH;
	}
	//if (CT > 1 && cc > 100) {
		//cc = 100;
	//}
	//cout << cc << endl;
	for (i = 0; i < cc; i++) {
		for (j = i + 1; j < cc; j++) {
			board[can[i].x][can[i].y] = 1 - tt;
			board[can[j].x][can[j].y] = 1 - tt;
			//show(board);
			VV = givevalue(board, tt, chestmoble);
			//cout << "当前选择点为" << can[i].x <<"|"<< can[i].y << "|" << can[j].x << "|" << can[j].y << endl;
			// << "棋盘价值为" << VV << endl;
			if (CT == 1 && VV < FF.value) {
				FF.value = VV;
				//showFF(FF);
				FF.ffirst.x = FF.first.x;
				FF.ffirst.y = FF.first.y;
				FF.fsecond.x = FF.second.x;
				FF.fsecond.y = FF.second.y;
			}
			if (CT > 1) {
				//cout << "in";
				if (VV < FF.value) {
					board[can[i].x][can[i].y] = EMPTY;
					board[can[j].x][can[j].y] = EMPTY;
					return;
				}
				if (ZZ > VV) {
					ZZ = VV;
				}
			}
			board[can[i].x][can[i].y] = EMPTY;
			board[can[j].x][can[j].y] = EMPTY;
		}
	}
	if (ZZ > FF.value && CT > 1) {
		FF.value = ZZ;
		FF.ffirst.x = FF.first.x;
		FF.ffirst.y = FF.first.y;
		FF.fsecond.x = FF.second.x;
		FF.fsecond.y = FF.second.y;
	}
	return;
}

Step searchone(int board[19][19], int tt, int chestmoble[3][3][3][3][3][3])
{
	Step step;    //返回
	int cc;       //可行点计数
	cango can[CG];
	savecanpoint(board, can, cc);   //保存可行数组和可行点数
	Tree FF;                        //辅助用的树结构
	FF.first.x = 0;
	FF.first.y = 0;
	FF.ffirst.x = 0;
	FF.ffirst.y = 0;
	FF.second.x = 0;
	FF.second.y = 0;
	FF.fsecond.x = 0;
	FF.fsecond.y = 0;
	FF.value = 100000;             //树节点价值初始化
	int i, j, k;
	int CT = 1;                     //计数器 记录第N个枝杈
	int NN;                         //第一步能赢的判断
	int HH=0;


	sortA(can, cc, chestmoble, board,tt,HH);       //对可行步进行影响力排序
	//cout << HH << endl;
	//cc = cc - HH;
	//if (cc > 50) {
		//cc = 50;
	//}

	for (i = 0; i < cc; i++) {
		for (j = i + 1; j < cc; j++) {

			FF.first.x = can[i].x;
			FF.first.y = can[i].y;
			FF.second.x = can[j].x;
			FF.second.y = can[j].y;
			board[can[i].x][can[i].y] = tt;
			board[can[j].x][can[j].y] = tt;
			NN = givevalue(board, tt, chestmoble);
			if (NN > 400000) {
				step.first.x = can[i].x;
				step.first.y = can[i].y;
				step.second.x = can[j].x;
				step.second.y = can[j].y;
				step.value = NN;
				return step;
			}
			searchtwo(board, tt, chestmoble, FF, CT);
			board[can[i].x][can[i].y] = EMPTY;
			board[can[j].x][can[j].y] = EMPTY;
			CT++;
		}
	}
	step.first.x = FF.ffirst.x;
	step.first.y = FF.ffirst.y;
	step.second.x = FF.fsecond.x;
	step.second.y = FF.fsecond.y;
	step.value = FF.value;
	return step;
}

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

int main()
{
	// 优化流，防止Botzone读写超时
	ios_base::sync_with_stdio(false);
	cin.tie(NULL);

	// 初始化棋盘
	for (int i = 0; i < 19; ++i)
		for (int j = 0; j < 19; ++j)
			Board[i][j] = EMPTY;

	int turnID;
	if (!(cin >> turnID)) return 0; // 读取回合数

	int x0, y0, x1, y1;
	int computerSide = WHITE; 
	KK = turnID; // 将你的原代码全局变量KK同步为当前的回合数

	// 历史复盘循环，替代原来的while(1)
	for (int i = 0; i < turnID; i++)
	{
		cin >> x0 >> y0 >> x1 >> y1;
		if (cin.fail()) break;

		// 如果第一回合传入的是-1，说明我们是黑方
		if (i == 0 && x0 == -1 && y0 == -1) {
			computerSide = BLACK; 
		}

		// 记录对方落子
		if (x0 >= 0 && x0 < 19 && y0 >= 0 && y0 < 19) Board[x0][y0] = 1 - computerSide;
		if (x1 >= 0 && x1 < 19 && y1 >= 0 && y1 < 19) Board[x1][y1] = 1 - computerSide;

		// 记录我方落子
		if (i < turnID - 1) {
			cin >> x0 >> y0 >> x1 >> y1;
			if (cin.fail()) break;
			
			if (x0 >= 0 && x0 < 19 && y0 >= 0 && y0 < 19) Board[x0][y0] = computerSide;
			if (x1 >= 0 && x1 < 19 && y1 >= 0 && y1 < 19) Board[x1][y1] = computerSide;
		}
	}

	int startX = -1, startY = -1, resultX = -1, resultY = -1;
	bool selfFirstBlack = (turnID == 1 && computerSide == BLACK);

	// 进行决策判定
	if (selfFirstBlack) {
		// 黑棋先手只走一步，放在正中天元
		startX = 9; startY = 9; resultX = -1; resultY = -1;
	} else {
		// 调用你原有的决策代码模块
		int chestmoble[3][3][3][3][3][3];            
		int tt = computerSide;                       

		memset(chestmoble, 0, sizeof(chestmoble));   
		givepower(tt, chestmoble);                   

		int board[19][19];
		copy(Board, board);

		Step step;
		if (peace(board, tt, chestmoble) == true) {
			int cc;                                 
			cango can[CG];
			savecanpoint(board, can, cc);   
			step.first.x = can[0].x;
			step.first.y = can[0].y;
			step.second.x = can[1].x;
			step.second.y = can[1].y;
		}
		else {
			step = searchone(board, tt, chestmoble);
		}

		startX = step.first.x;
		startY = step.first.y;
		resultX = step.second.x;
		resultY = step.second.y;
	}

	// 最终输出决策坐标
	cout << startX << ' ' << startY << ' ' << resultX << ' ' << resultY << endl;
	return 0;
}
