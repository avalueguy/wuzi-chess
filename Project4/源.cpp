#include <bits/stdc++.h>
using namespace std;

#define N 19
#define EMPTY 0
#define BLACK 1
#define WHITE -1

int board[N][N];
int myColor;

int dx[4]={1,0,1,1};
int dy[4]={0,1,1,-1};

inline bool inMap(int x,int y){
    return x>=0&&x<N&&y>=0&&y<N;
}

bool ProcStep(int x1,int y1,int x2,int y2,int c,bool check){
    if(x2!=-1 && x1==x2 && y1==y2) return false;

    if(x2==-1){
        if(!inMap(x1,y1)||board[x1][y1]!=0) return false;
        if(!check) board[x1][y1]=c;
        return true;
    }

    if(!inMap(x1,y1)||!inMap(x2,y2)) return false;
    if(board[x1][y1]!=0||board[x2][y2]!=0) return false;

    if(!check){
        board[x1][y1]=c;
        board[x2][y2]=c;
    }
    return true;
}

// ========================
// ⭐ 核心：完全副本棋盘模拟
// ========================

bool isWin(int g[N][N],int x,int y,int c){
    for(int d=0;d<4;d++){
        int cnt=1;

        for(int k=1;k<6;k++){
            int nx=x+k*dx[d],ny=y+k*dy[d];
            if(!inMap(nx,ny)||g[nx][ny]!=c) break;
            cnt++;
        }
        for(int k=1;k<6;k++){
            int nx=x-k*dx[d],ny=y-k*dy[d];
            if(!inMap(nx,ny)||g[nx][ny]!=c) break;
            cnt++;
        }

        if(cnt>=5) return true;
    }
    return false;
}

// ========================
// 安全复制棋盘（关键）
// ========================
void copyBoard(int dst[N][N],int src[N][N]){
    for(int i=0;i<N;i++)
        for(int j=0;j<N;j++)
            dst[i][j]=src[i][j];
}

struct P{int x,y;};
struct M{int x1,y1,x2,y2;};

// ========================
// 评分（安全版）
// ========================
int evaluate(int g[N][N],int me,int op){
    int score=0;
    for(int x=0;x<N;x++){
        for(int y=0;y<N;y++){
            for(int d=0;d<4;d++){
                int nx=x+5*dx[d],ny=y+5*dy[d];
                if(!inMap(nx,ny)) continue;

                int c=0,o=0;
                for(int k=0;k<6;k++){
                    int p=g[x+k*dx[d]][y+k*dy[d]];
                    if(p==me) c++;
                    else if(p==op) o++;
                }

                if(o==0) score+=pow(10,c);
                if(c==0) score-=pow(10,o);
            }
        }
    }
    return score;
}

// ========================
// 生成候选点
// ========================
vector<P> genPoints(int g[N][N]){
    vector<P> pts;

    for(int x=0;x<N;x++){
        for(int y=0;y<N;y++){
            if(g[x][y]!=0) continue;

            bool near=false;
            for(int i=-2;i<=2;i++){
                for(int j=-2;j<=2;j++){
                    int nx=x+i,ny=y+j;
                    if(inMap(nx,ny)&&g[nx][ny]!=0){
                        near=true;
                        break;
                    }
                }
                if(near) break;
            }

            if(near) pts.push_back({x,y});
        }
    }

    if(pts.empty()) pts.push_back({9,9});
    return pts;
}

vector<M> genMoves(vector<P>& pts){
    vector<M> mv;
    for(int i=0;i<pts.size();i++){
        for(int j=i+1;j<pts.size();j++){
            mv.push_back({pts[i].x,pts[i].y,pts[j].x,pts[j].y});
        }
    }
    return mv;
}

// ========================
// ⭐ VCT-lite（纯副本）
// ========================
bool win2(int g[N][N],M m,int c){
    int tmp[N][N];
    copyBoard(tmp,g);

    if(tmp[m.x1][m.y1]!=0||tmp[m.x2][m.y2]!=0) return false;

    tmp[m.x1][m.y1]=c;
    tmp[m.x2][m.y2]=c;

    return isWin(tmp,m.x1,m.y1,c) || isWin(tmp,m.x2,m.y2,c);
}

// ========================
// 搜索（完全副本版）
// ========================
int dfs(int g[N][N],int dep,int alpha,int beta,bool meTurn,int me,int op,clock_t st){

    if(clock()-st>0.85*CLOCKS_PER_SEC)
        return meTurn?-1e8:1e8;

    int val=evaluate(g,me,op);
    if(dep==0) return val;

    auto pts=genPoints(g);
    if(pts.size()>10) pts.resize(10);

    auto mv=genMoves(pts);

    if(meTurn){
        int best=-1e9;

        for(auto &m:mv){
            int tmp[N][N];
            copyBoard(tmp,g);

            if(tmp[m.x1][m.y1]!=0||tmp[m.x2][m.y2]!=0) continue;

            tmp[m.x1][m.y1]=me;
            tmp[m.x2][m.y2]=me;

            int v=dfs(tmp,dep-1,alpha,beta,false,me,op,st);

            best=max(best,v);
            alpha=max(alpha,v);
            if(beta<=alpha) break;
        }
        return best;
    }else{
        int best=1e9;

        for(auto &m:mv){
            int tmp[N][N];
            copyBoard(tmp,g);

            if(tmp[m.x1][m.y1]!=0||tmp[m.x2][m.y2]!=0) continue;

            tmp[m.x1][m.y1]=op;
            tmp[m.x2][m.y2]=op;

            int v=dfs(tmp,dep-1,alpha,beta,true,me,op,st);

            best=min(best,v);
            beta=min(beta,v);
            if(beta<=alpha) break;
        }
        return best;
    }
}

// ========================
// 主函数
// ========================
int main(){
    int turn,x1,y1,x2,y2;
    cin>>turn;

    myColor=WHITE;

    for(int i=0;i<turn;i++){
        cin>>x1>>y1>>x2>>y2;
        if(x1==-1) myColor=BLACK;
        if(x1>=0) ProcStep(x1,y1,x2,y2,-myColor,false);

        if(i<turn-1){
            cin>>x1>>y1>>x2>>y2;
            if(x1>=0) ProcStep(x1,y1,x2,y2,myColor,false);
        }
    }

    int sx=-1,sy=-1,rx=-1,ry=-1;

    // 黑先
    if(turn==1 && myColor==BLACK){
        cout<<"9 9 -1 -1\n";
        return 0;
    }

    auto pts=genPoints(board);
    auto mv=genMoves(pts);

    // ===== 必赢 =====
    for(auto &m:mv){
        if(win2(board,m,myColor)){
            sx=m.x1;sy=m.y1;
            rx=m.x2;ry=m.y2;
            goto END;
        }
    }

    // ===== 必防 =====
    for(auto &m:mv){
        if(win2(board,m,-myColor)){
            sx=m.x1;sy=m.y1;
            rx=m.x2;ry=m.y2;
            goto END;
        }
    }

    // ===== 搜索 =====
    {
        int best=-1e9;
        clock_t st=clock();

        for(auto &m:mv){
            int tmp[N][N];
            copyBoard(tmp,board);

            if(tmp[m.x1][m.y1]!=0||tmp[m.x2][m.y2]!=0) continue;

            tmp[m.x1][m.y1]=myColor;
            tmp[m.x2][m.y2]=myColor;

            int v=dfs(tmp,1,-1e9,1e9,false,myColor,-myColor,st);

            if(v>best){
                best=v;
                sx=m.x1;sy=m.y1;
                rx=m.x2;ry=m.y2;
            }
        }
    }

END:

    // ===== 最终保险 =====
    if(!ProcStep(sx,sy,rx,ry,myColor,true)){
        vector<P> empty;
        for(int i=0;i<N;i++){
            for(int j=0;j<N;j++){
                if(board[i][j]==0)
                    empty.push_back({i,j});
            }
        }

        if(empty.size()>=2){
            sx=empty[0].x;sy=empty[0].y;
            rx=empty[1].x;ry=empty[1].y;
        }else if(empty.size()==1){
            sx=empty[0].x;sy=empty[0].y;
            rx=-1;ry=-1;
        }
    }

    cout<<sx<<" "<<sy<<" "<<rx<<" "<<ry<<"\n";
}
