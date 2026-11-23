#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include <conio.h>

#define WIDTH 20
#define HEIGHT 20
#define MAX_ATTACKS 30
#define MAX_BULLETS 200
#define MAX_ITEMS 5
#define MAX_LIVES 3

#define COLOR_DEFAULT 7     
#define COLOR_WARNING 4     
#define COLOR_ORANGE 6      
#define COLOR_ATTACK 12     
#define COLOR_PLAYER 11     
#define COLOR_INVINCIBLE 8  
#define COLOR_LIFE 10       
#define COLOR_SLOW 13       
#define COLOR_SHIELD 9      
#define COLOR_SCORE 14      

typedef enum { LASER_ROW, LASER_COL, LASER_DIAG1, LASER_DIAG2, BOMB } AttackType;
typedef enum { ITEM_LIFE, ITEM_SLOW, ITEM_SHIELD, ITEM_SCORE } ItemType;

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

typedef struct {
    ItemType type;
    int x, y;
    int timer;
    int active;
} Item;

void gotoxy(int x, int y) {
    COORD pos = { (short)x, (short)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

void setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

int getItemColor(ItemType type) {
    if (type == ITEM_LIFE) return COLOR_LIFE;
    if (type == ITEM_SLOW) return COLOR_SLOW;
    if (type == ITEM_SHIELD) return COLOR_SHIELD;
    return COLOR_SCORE;
}

ItemType rollItemType() {
    int r = rand() % 100;
    if (r < 45) return ITEM_SCORE;   // 45%: 점수 +200
    if (r < 75) return ITEM_SLOW;    // 30%: 투사체 감속
    if (r < 90) return ITEM_LIFE;    // 15%: 목숨 회복 (최대 3)
    return ITEM_SHIELD;              // 10%: 보호막
}

int main() {
    char boardChar[HEIGHT][WIDTH];
    int boardColor[HEIGHT][WIDTH];

    Attack attacks[MAX_ATTACKS] = { 0 };
    Bullet bullets[MAX_BULLETS] = { 0 };
    Item items[MAX_ITEMS] = { 0 };

    int playerX = WIDTH / 2;
    int playerY = HEIGHT / 2;
    int score = 0;
    int lives = 3;
    int hasShield = 0;
    int invincibleTimer = 0; // 피격 후 남은 무적 틱
    int slowTimer = 0;       // 투사체 감속 남은 틱
    int tickCount = 0;       // 감속 주기 계산용 카운터
    int isGameOver = 0;

    const int WARNING_TICKS = 15;
    const int ACTIVE_TICKS = 5;
    const int INVINCIBLE_DURATION = 10; // 약 0.8초 (80ms * 10)
    const int SLOW_DURATION = 60;       // 약 4.8초 (80ms * 60)
    const int ITEM_LIFETIME = 100;      // 맵에 아이템 유지 시간
    const int SCORE_ITEM_BONUS = 200;   // 점수 아이템 추가 점수

    srand((unsigned int)time(NULL));

    CONSOLE_CURSOR_INFO cursorInfo = { 0, };
    cursorInfo.dwSize = 1;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    system("cls");

    // 0. 게임 시작 전 3초 준비 카운트다운
    for (int k = 3; k >= 1; k--) {
        gotoxy(0, 0);
        setColor(COLOR_DEFAULT);
        printf("Score: 00000  Lives: ");
        setColor(COLOR_LIFE);
        for (int l = 0; l < lives; l++) printf("♥ ");
        setColor(COLOR_DEFAULT);
        printf(" Shield: OFF\n");
        printf("------------------------------------------\n");
        for (int i = 0; i < HEIGHT; i++) {
            printf("|");
            for (int j = 0; j < WIDTH; j++) {
                if (playerX == j && playerY == i) {
                    setColor(COLOR_PLAYER); printf("@ "); setColor(COLOR_DEFAULT);
                }
                else {
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
    while (_kbhit()) _getch();

    // 메인 게임 루프 시작
    while (!isGameOver) {
        tickCount++;
        if (invincibleTimer > 0) invincibleTimer--;
        if (slowTimer > 0) slowTimer--;

        // 1. 키보드 입력 처리
        if (GetAsyncKeyState(VK_UP) & 0x8000) { if (playerY > 0) playerY--; }
        else if (GetAsyncKeyState(VK_DOWN) & 0x8000) { if (playerY < HEIGHT - 1) playerY++; }

        if (GetAsyncKeyState(VK_LEFT) & 0x8000) { if (playerX > 0) playerX--; }
        else if (GetAsyncKeyState(VK_RIGHT) & 0x8000) { if (playerX < WIDTH - 1) playerX++; }

        if (GetAsyncKeyState('Q') & 0x8000) { break; }

        // 2. 시간에 따른 난이도 동적 증가
        int maxBombs = 1 + (score / 800);
        int maxLasers = 2 + (score / 400);
        if (maxBombs > 3) maxBombs = 3;
        if (maxLasers > 6) maxLasers = 6;

        int attackProb = 10 + (score / 100);
        if (attackProb > 30) attackProb = 30;

        int bulletProb = 20 + (score / 80);
        if (bulletProb > 50) bulletProb = 50;

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
            AttackType newType = (AttackType)(rand() % 5);
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
                    }
                    else if (dir == 1) {
                        bullets[i].x = rand() % WIDTH; bullets[i].y = HEIGHT - 1;
                        bullets[i].dx = 0; bullets[i].dy = -1;
                    }
                    else if (dir == 2) {
                        bullets[i].x = 0; bullets[i].y = rand() % HEIGHT;
                        bullets[i].dx = 1; bullets[i].dy = 0;
                    }
                    else if (dir == 3) {
                        bullets[i].x = WIDTH - 1; bullets[i].y = rand() % HEIGHT;
                        bullets[i].dx = -1; bullets[i].dy = 0;
                    }
                    break;
                }
            }
        }

        // 3-3. 아이템 생성 (2% 확률)
        if (rand() % 100 < 2) {
            for (int i = 0; i < MAX_ITEMS; i++) {
                if (!items[i].active) {
                    items[i].active = 1;
                    items[i].type = rollItemType();
                    items[i].x = rand() % WIDTH;
                    items[i].y = rand() % HEIGHT;
                    items[i].timer = ITEM_LIFETIME;
                    break;
                }
            }
        }

        // 4. 총알 이동 처리 (감속 아이템 활성 시 2틱마다 1칸 이동)
        int moveBullets = (slowTimer > 0) ? (tickCount % 2 == 0) : 1;
        if (moveBullets) {
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (bullets[i].active) {
                    bullets[i].x += bullets[i].dx;
                    bullets[i].y += bullets[i].dy;

                    if (bullets[i].x < 0 || bullets[i].x >= WIDTH || bullets[i].y < 0 || bullets[i].y >= HEIGHT) {
                        bullets[i].active = 0;
                    }
                }
            }
        }

        // 5. 예고형 공격 업데이트 및 폭탄 파편 발사
        for (int i = 0; i < MAX_ATTACKS; i++) {
            if (attacks[i].active) {
                attacks[i].timer--;

                if (attacks[i].timer == ACTIVE_TICKS && attacks[i].type == BOMB) {
                    int dirs[8][2] = { { -1,-1 },{ 0,-1 },{ 1,-1 },{ -1,0 },{ 1,0 },{ -1,1 },{ 0,1 },{ 1,1 } };
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

        // 5-2. 아이템 수명 감소
        for (int i = 0; i < MAX_ITEMS; i++) {
            if (items[i].active) {
                items[i].timer--;
                if (items[i].timer <= 0) items[i].active = 0;
            }
        }

        // 6. 맵 렌더링 초기화
        for (int i = 0; i < HEIGHT; i++) {
            for (int j = 0; j < WIDTH; j++) {
                boardChar[i][j] = ' ';
                boardColor[i][j] = COLOR_DEFAULT;
            }
        }

        // 6-0. 아이템 그리기
        for (int i = 0; i < MAX_ITEMS; i++) {
            if (items[i].active) {
                boardChar[items[i].y][items[i].x] = '+';
                boardColor[items[i].y][items[i].x] = getItemColor(items[i].type);
            }
        }

        // 6-1. 경고(Warning) 그리기
        for (int i = 0; i < MAX_ATTACKS; i++) {
            if (!attacks[i].active || attacks[i].timer <= ACTIVE_TICKS) continue;

            if (attacks[i].type == LASER_ROW) {
                for (int x = 0; x < WIDTH; x++) { boardChar[attacks[i].y][x] = '!'; boardColor[attacks[i].y][x] = COLOR_WARNING; }
            }
            else if (attacks[i].type == LASER_COL) {
                for (int y = 0; y < HEIGHT; y++) { boardChar[y][attacks[i].x] = '!'; boardColor[y][attacks[i].x] = COLOR_WARNING; }
            }
            else if (attacks[i].type == LASER_DIAG1) {
                for (int d = -WIDTH; d <= WIDTH; d++) {
                    int nx = attacks[i].x + d, ny = attacks[i].y + d;
                    if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
                        boardChar[ny][nx] = '!'; boardColor[ny][nx] = COLOR_WARNING;
                    }
                }
            }
            else if (attacks[i].type == LASER_DIAG2) {
                for (int d = -WIDTH; d <= WIDTH; d++) {
                    int nx = attacks[i].x + d, ny = attacks[i].y - d;
                    if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
                        boardChar[ny][nx] = '!'; boardColor[ny][nx] = COLOR_WARNING;
                    }
                }
            }
            else if (attacks[i].type == BOMB) {
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
            }
            else if (attacks[i].type == LASER_COL) {
                for (int y = 0; y < HEIGHT; y++) { boardChar[y][attacks[i].x] = '#'; boardColor[y][attacks[i].x] = COLOR_ATTACK; }
            }
            else if (attacks[i].type == LASER_DIAG1) {
                for (int d = -WIDTH; d <= WIDTH; d++) {
                    int nx = attacks[i].x + d, ny = attacks[i].y + d;
                    if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
                        boardChar[ny][nx] = '#'; boardColor[ny][nx] = COLOR_ATTACK;
                    }
                }
            }
            else if (attacks[i].type == LASER_DIAG2) {
                for (int d = -WIDTH; d <= WIDTH; d++) {
                    int nx = attacks[i].x + d, ny = attacks[i].y - d;
                    if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
                        boardChar[ny][nx] = '#'; boardColor[ny][nx] = COLOR_ATTACK;
                    }
                }
            }
            else if (attacks[i].type == BOMB) {
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

        // 7-1. 아이템 획득 판정
        for (int i = 0; i < MAX_ITEMS; i++) {
            if (items[i].active && items[i].x == playerX && items[i].y == playerY) {
                if (items[i].type == ITEM_LIFE) {
                    if (lives < MAX_LIVES) lives++;
                    score += 50;
                }
                else if (items[i].type == ITEM_SLOW) {
                    slowTimer = SLOW_DURATION;
                    score += 50;
                }
                else if (items[i].type == ITEM_SHIELD) {
                    hasShield = 1;
                    score += 50;
                }
                else if (items[i].type == ITEM_SCORE) {
                    score += SCORE_ITEM_BONUS;
                }
                items[i].active = 0;
            }
        }

        // 7-2. 피격 및 무적/보호막 판정
        if (boardChar[playerY][playerX] == '#' || boardChar[playerY][playerX] == '*') {
            if (hasShield) {
                hasShield = 0;
                invincibleTimer = INVINCIBLE_DURATION;
            }
            else if (invincibleTimer <= 0) {
                lives--;
                invincibleTimer = INVINCIBLE_DURATION;
                if (lives <= 0) {
                    lives = 0;
                    isGameOver = 1;
                }
            }
            else {
                score++;
            }
        }
        else {
            score++;
        }

        // 8. 화면 출력
        gotoxy(0, 0);
        setColor(COLOR_DEFAULT);
        printf("Score: %05d  Lives: ", score);
        setColor(COLOR_LIFE);
        for (int l = 0; l < lives; l++) printf("♥ ");
        for (int l = lives; l < MAX_LIVES; l++) printf("  ");

        setColor(COLOR_DEFAULT);
        printf("Shield: ");
        if (hasShield) {
            setColor(COLOR_SHIELD); printf("ON ");
        }
        else {
            setColor(COLOR_DEFAULT); printf("OFF");
        }

        setColor(COLOR_DEFAULT);
        printf("  Slow: ");
        if (slowTimer > 0) {
            setColor(COLOR_SLOW); printf("ON ");
        }
        else {
            setColor(COLOR_DEFAULT); printf("OFF");
        }
        printf("\n");

        setColor(COLOR_DEFAULT);
        printf("------------------------------------------\n");

        for (int i = 0; i < HEIGHT; i++) {
            setColor(COLOR_DEFAULT);
            printf("|");

            for (int j = 0; j < WIDTH; j++) {
                if (playerX == j && playerY == i) {
                    if (invincibleTimer > 0) {
                        setColor((tickCount % 2 == 0) ? COLOR_PLAYER : COLOR_INVINCIBLE);
                    }
                    else if (hasShield) {
                        setColor(COLOR_SHIELD);
                    }
                    else {
                        setColor(COLOR_PLAYER);
                    }
                    printf("@ ");
                }
                else {
                    setColor(boardColor[i][j]);
                    printf("%c ", boardChar[i][j]);
                }
            }
            setColor(COLOR_DEFAULT);
            printf("|\n");
        }
        setColor(COLOR_DEFAULT);
        printf("------------------------------------------\n");
        printf("+: 초록=목숨회복(최대3) 보라=감속 파랑=쉴드 노랑=+200점\n");

        Sleep(80);
    }

    // 게임 오버
    setColor(COLOR_DEFAULT);
    gotoxy(0, HEIGHT + 4);
    printf("\n=== GAME OVER ===\n");
    printf("최종 버틴 점수: %d\n\n", score);

    return 0;
}
