#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include <conio.h>

#define WIDTH 20
#define HEIGHT 20
#define MAX_ATTACKS 30
#define MAX_BULLETS 200

#define COLOR_DEFAULT 7     
#define COLOR_WARNING 4     
#define COLOR_ORANGE 6      
#define COLOR_ATTACK 12     
#define COLOR_PLAYER 11     

// 공격 타입에 대각선 2종류 추가 (총 5개)
typedef enum { LASER_ROW, LASER_COL, LASER_DIAG1, LASER_DIAG2, BOMB } AttackType;

typedef struct {
    AttackType type;
    int x, y;       
    int timer;      
    int active;     
} Attack;

typedef struct {
    int x, y;
    int dx, dy;
    int active;
} Bullet;

void gotoxy(int x, int y) {
    COORD pos = { (short)x, (short)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

void setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

int main() {
    char boardChar[HEIGHT][WIDTH];
    int boardColor[HEIGHT][WIDTH];
    
    Attack attacks[MAX_ATTACKS] = { 0 };
    Bullet bullets[MAX_BULLETS] = { 0 };
    
    int playerX = WIDTH / 2;
    int playerY = HEIGHT / 2;
    int score = 0;
    int isGameOver = 0;
    
    const int WARNING_TICKS = 15; 
    const int ACTIVE_TICKS = 5;   

    srand((unsigned int)time(NULL));

    // 커서 숨기기 및 화면 초기화
    CONSOLE_CURSOR_INFO cursorInfo = { 0, };
    cursorInfo.dwSize = 1;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    system("cls");

    // 0. 게임 시작 전 3초 준비 카운트다운
    for (int k = 3; k >= 1; k--) {
        gotoxy(0, 0);
        setColor(COLOR_DEFAULT);
        printf("Score: 00000\n");
        printf("------------------------------------------\n"); 
        for (int i = 0; i < HEIGHT; i++) {
            printf("|");
            for (int j = 0; j < WIDTH; j++) {
                if (playerX == j && playerY == i) {
                    setColor(COLOR_PLAYER); printf("@ "); setColor(COLOR_DEFAULT);
                } else {
                    printf("  ");
                }
            }
            printf("|\n");
        }
        printf("------------------------------------------\n");
        setColor(COLOR_PLAYER);
        printf(">>> 게임 시작 %d초 전! 준비하세요! <<<\n", k);
        Sleep(1000);
    }
    // 카운트다운 도중 미리 눌러둔 키보드 입력(버퍼) 비우기
    while(_kbhit()) _getch();

    // 메인 게임 루프 시작
    while (!isGameOver) {
        // 1. 키보드 입력 처리
        if (GetAsyncKeyState(VK_UP) & 0x8000) { if (playerY > 0) playerY--; }
        else if (GetAsyncKeyState(VK_DOWN) & 0x8000) { if (playerY < HEIGHT - 1) playerY++; }
        
        if (GetAsyncKeyState(VK_LEFT) & 0x8000) { if (playerX > 0) playerX--; }
        else if (GetAsyncKeyState(VK_RIGHT) & 0x8000) { if (playerX < WIDTH - 1) playerX++; }
        
        if (GetAsyncKeyState('Q') & 0x8000) { break; }

        // 2. 시간에 따른 난이도(확률 및 한도) 동적 증가 시스템
        int maxBombs = 1 + (score / 800);   // 약 60초마다 폭탄 동시 허용 개수 1개씩 증가
        int maxLasers = 2 + (score / 400);  // 약 30초마다 레이저 동시 허용 개수 1개씩 증가
        if (maxBombs > 3) maxBombs = 3;     // 최대 3개로 제한
        if (maxLasers > 6) maxLasers = 6;   // 최대 6개로 제한

        int attackProb = 10 + (score / 100); // 공격 등장 확률 지속 상승
        if (attackProb > 30) attackProb = 30;

        int bulletProb = 20 + (score / 80);  // 일반 총알 등장 확률 지속 상승
        if (bulletProb > 50) bulletProb = 50;

        // 활성화된 공격 개수 카운트
        int activeBombs = 0;
        int activeLasers = 0;
        for (int i = 0; i < MAX_ATTACKS; i++) {
            if (attacks[i].active) {
                if (attacks[i].type == BOMB) activeBombs++;
                else activeLasers++;
            }
        }

        // 3-1. 예고형 공격 생성
        if (rand() % 100 < attackProb) { 
            AttackType newType = (AttackType)(rand() % 5); // 0~4번 타입 (대각선 포함)
            int canSpawn = 0;
            
            if (newType == BOMB && activeBombs < maxBombs) canSpawn = 1;
            else if (newType != BOMB && activeLasers < maxLasers) canSpawn = 1;

            if (canSpawn) {
                for (int i = 0; i < MAX_ATTACKS; i++) {
                    if (!attacks[i].active) {
                        attacks[i].active = 1;
                        attacks[i].type = newType;
                        attacks[i].x = rand() % WIDTH;
                        attacks[i].y = rand() % HEIGHT;
                        attacks[i].timer = WARNING_TICKS + ACTIVE_TICKS;
                        break; 
                    }
                }
            }
        }

        // 3-2. 일반 총알 생성
        if (rand() % 100 < bulletProb) { 
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!bullets[i].active) {
                    bullets[i].active = 1;
                    int dir = rand() % 4; 
                    
                    if (dir == 0) {
                        bullets[i].x = rand() % WIDTH; bullets[i].y = 0;
                        bullets[i].dx = 0; bullets[i].dy = 1;
                    } else if (dir == 1) {
                        bullets[i].x = rand() % WIDTH; bullets[i].y = HEIGHT - 1;
                        bullets[i].dx = 0; bullets[i].dy = -1;
                    } else if (dir == 2) {
                        bullets[i].x = 0; bullets[i].y = rand() % HEIGHT;
                        bullets[i].dx = 1; bullets[i].dy = 0;
                    } else if (dir == 3) {
                        bullets[i].x = WIDTH - 1; bullets[i].y = rand() % HEIGHT;
                        bullets[i].dx = -1; bullets[i].dy = 0;
                    }
                    break;
                }
            }
        }

        // 4. 총알 이동 처리
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                bullets[i].x += bullets[i].dx;
                bullets[i].y += bullets[i].dy;
                
                if (bullets[i].x < 0 || bullets[i].x >= WIDTH || bullets[i].y < 0 || bullets[i].y >= HEIGHT) {
                    bullets[i].active = 0;
                }
            }
        }

        // 5. 예고형 공격 업데이트 및 폭탄 파편 발사
        for (int i = 0; i < MAX_ATTACKS; i++) {
            if (attacks[i].active) {
                attacks[i].timer--;
                
                if (attacks[i].timer == ACTIVE_TICKS && attacks[i].type == BOMB) {
                    int dirs[8][2] = {{-1,-1}, {0,-1}, {1,-1}, {-1,0}, {1,0}, {-1,1}, {0,1}, {1,1}};
                    for (int d = 0; d < 8; d++) {
                        for (int b = 0; b < MAX_BULLETS; b++) {
                            if (!bullets[b].active) {
                                bullets[b].active = 1;
                                bullets[b].x = attacks[i].x + dirs[d][0];
                                bullets[b].y = attacks[i].y + dirs[d][1];
                                bullets[b].dx = dirs[d][0];
                                bullets[b].dy = dirs[d][1];
                                break;
                            }
                        }
                    }
                }
                if (attacks[i].timer <= 0) attacks[i].active = 0; 
            }
        }

        // 6. 맵 렌더링 초기화
        for (int i = 0; i < HEIGHT; i++) {
            for (int j = 0; j < WIDTH; j++) {
                boardChar[i][j] = ' '; 
                boardColor[i][j] = COLOR_DEFAULT;
            }
        }

        // 6-1. 경고(Warning) 그리기
        for (int i = 0; i < MAX_ATTACKS; i++) {
            if (!attacks[i].active || attacks[i].timer <= ACTIVE_TICKS) continue;
            
            if (attacks[i].type == LASER_ROW) {
                for (int x = 0; x < WIDTH; x++) { boardChar[attacks[i].y][x] = '!'; boardColor[attacks[i].y][x] = COLOR_WARNING; }
            } else if (attacks[i].type == LASER_COL) {
                for (int y = 0; y < HEIGHT; y++) { boardChar[y][attacks[i].x] = '!'; boardColor[y][attacks[i].x] = COLOR_WARNING; }
            } else if (attacks[i].type == LASER_DIAG1) { // '\' 방향 대각선
                for (int d = -WIDTH; d <= WIDTH; d++) {
                    int nx = attacks[i].x + d, ny = attacks[i].y + d;
                    if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
                        boardChar[ny][nx] = '!'; boardColor[ny][nx] = COLOR_WARNING;
                    }
                }
            } else if (attacks[i].type == LASER_DIAG2) { // '/' 방향 대각선
                for (int d = -WIDTH; d <= WIDTH; d++) {
                    int nx = attacks[i].x + d, ny = attacks[i].y - d;
                    if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
                        boardChar[ny][nx] = '!'; boardColor[ny][nx] = COLOR_WARNING;
                    }
                }
            } else if (attacks[i].type == BOMB) {
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int nx = attacks[i].x + dx, ny = attacks[i].y + dy;
                        if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
                            boardChar[ny][nx] = '!'; boardColor[ny][nx] = COLOR_ORANGE; 
                        }
                    }
                }
            }
        }

        // 6-2. 실제 타격(Active) 그리기
        for (int i = 0; i < MAX_ATTACKS; i++) {
            if (!attacks[i].active || attacks[i].timer > ACTIVE_TICKS) continue;
            
            if (attacks[i].type == LASER_ROW) {
                for (int x = 0; x < WIDTH; x++) { boardChar[attacks[i].y][x] = '#'; boardColor[attacks[i].y][x] = COLOR_ATTACK; }
            } else if (attacks[i].type == LASER_COL) {
                for (int y = 0; y < HEIGHT; y++) { boardChar[y][attacks[i].x] = '#'; boardColor[y][attacks[i].x] = COLOR_ATTACK; }
            } else if (attacks[i].type == LASER_DIAG1) {
                for (int d = -WIDTH; d <= WIDTH; d++) {
                    int nx = attacks[i].x + d, ny = attacks[i].y + d;
                    if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
                        boardChar[ny][nx] = '#'; boardColor[ny][nx] = COLOR_ATTACK;
                    }
                }
            } else if (attacks[i].type == LASER_DIAG2) {
                for (int d = -WIDTH; d <= WIDTH; d++) {
                    int nx = attacks[i].x + d, ny = attacks[i].y - d;
                    if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
                        boardChar[ny][nx] = '#'; boardColor[ny][nx] = COLOR_ATTACK;
                    }
                }
            } else if (attacks[i].type == BOMB) {
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int nx = attacks[i].x + dx, ny = attacks[i].y + dy;
                        if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
                            boardChar[ny][nx] = '#'; boardColor[ny][nx] = COLOR_ATTACK; 
                        }
                    }
                }
            }
        }

        // 6-3. 총알 그리기
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                if (bullets[i].x >= 0 && bullets[i].x < WIDTH && bullets[i].y >= 0 && bullets[i].y < HEIGHT) {
                    boardChar[bullets[i].y][bullets[i].x] = '*';
                    boardColor[bullets[i].y][bullets[i].x] = COLOR_ATTACK;
                }
            }
        }

        // 7. 충돌 판정
        if (boardChar[playerY][playerX] == '#' || boardChar[playerY][playerX] == '*') {
            isGameOver = 1;
        } else {
            score++;
        }

        // 8. 화면 출력 (정사각형 뷰 유지)
        gotoxy(0, 0);
        setColor(COLOR_DEFAULT);
        printf("Score: %05d\n", score);
        printf("------------------------------------------\n"); 

        for (int i = 0; i < HEIGHT; i++) {
            setColor(COLOR_DEFAULT);
            printf("|");
            
            for (int j = 0; j < WIDTH; j++) {
                if (playerX == j && playerY == i) {
                    setColor(COLOR_PLAYER);
                    printf("@ ");
                } else {
                    setColor(boardColor[i][j]);
                    printf("%c ", boardChar[i][j]);
                }
            }
            setColor(COLOR_DEFAULT);
            printf("|\n");
        }
        printf("------------------------------------------\n");
        // 준비 완료 문구 덮어쓰기 위해 공백 출력
        printf("                                          \n");

        Sleep(80); 
    }

    // 게임 오버
    setColor(COLOR_DEFAULT);
    gotoxy(0, HEIGHT + 4);
    printf("\n=== GAME OVER ===\n");
    printf("최종 버틴 점수: %d\n\n", score);

    return 0;
}
