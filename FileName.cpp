#include <windows.h>
#include <graphics.h>
#include <math.h>
#include <algorithm>
#include <stdio.h>  

#define NOMINMAX
#define DEG2RAD(deg) ((deg) * 3.14159265358979323846 / 180.0)

bool isTransparent(COLORREF color) {
    return color == BLACK;
}

bool isYellowArea(COLORREF color) {
    return GetRValue(color) > 200 && GetGValue(color) > 200 && GetBValue(color) < 100;
}

void showGameRules() {
    const wchar_t* rules = L"欢迎进入科目二驾考模拟游戏\n\n"
        L"考核项目包括：\n"
        L"1. 侧方停车\n"
        L"2. 倒车入库\n"
        L"3. 弯道行驶\n\n"
        L"游戏规则：\n"
        L"- 必须依次通过三项考试\n"
        L"- 最后驶入红色区域完成考试挑战成功\n"
        L"- 触碰黑色区域挑战失败\n"
        L"- 共有三次挑战机会，用完则游戏结束\n\n"
        L"操作说明：\n"
        L"W - 前进\n"
        L"S - 倒车\n"
        L"A - 左转\n"
        L"D - 右转\n"
        L"Q - 退出游戏";

    MessageBox(NULL, rules, L"游戏规则", MB_OK | MB_ICONINFORMATION);
}

void drawGameRules() {
    settextstyle(18, 0, L"微软雅黑");
    settextcolor(WHITE);

    setfillcolor(RGB(0, 0, 0));
    fillrectangle(0, 700, 1200, 780);

    outtextxy(50, 710, L"游戏规则");

    const wchar_t* rules[] = {
        L"- 必须依次通过三项考试",
        L"- 最后驶入红色区域完成考试",
        L"- 触碰黑色区考试不通过",
        L"- 共有三次挑战机会"
    };

    for (int i = 0; i < 4; i++) {
        outtextxy(50, 740 + i * 20, rules[i]);
    }

    outtextxy(700, 710, L"操作说明");

    const wchar_t* controls[] = {
        L"W - 前进",
        L"S - 倒车",
        L"A - 左转",
        L"D - 右转",
        L"Q - 退出游戏"
    };

    for (int i = 0; i < 5; i++) {
        outtextxy(700, 740 + i * 20, controls[i]);
    }
}

class Map {
private:
    IMAGE bg;
    bool bgLoaded;
    bool checkYellowLine(int x, int y) {
        if (!bgLoaded || x < 0 || x >= 1200 || y < 0 || y >= 900)
            return false;

        COLORREF pixel = getpixel(x, y);
        return GetRValue(pixel) > 220 && GetGValue(pixel) > 220 && 
               GetBValue(pixel) < 80 && GetBValue(pixel) > 20;
    }

public:
    const wchar_t* MAP_PATH = L"map.png";

    Map() : bgLoaded(false) {
        loadimage(&bg, MAP_PATH);

        if (bg.getwidth() <= 0 || bg.getheight() <= 0) {
            wchar_t msg[200];
            swprintf_s(msg, L"地图文件加载失败：%s\n将使用默认灰色地图，黑色边界为碰撞区域", MAP_PATH);
            MessageBox(NULL, msg, L"警告", MB_OK | MB_ICONWARNING);
            bgLoaded = false;
        }
        else {
            bgLoaded = true;
        }
    }

    void draw() {
        if (bgLoaded) putimage(0, 0, &bg);
        else {
            setfillcolor(LIGHTGRAY);
            fillrectangle(0, 0, 1200, 900);
            setlinecolor(BLACK);
            setlinestyle(PS_SOLID, 5);
            rectangle(50, 50, 1150, 850);
        }
    }

    bool checkCollision(int x, int y) {
        if (x < 0 || x >= 1200 || y < 0 || y >= 900) {
            return true;
        }

        if (bgLoaded) {
            COLORREF pixel = getpixel(x, y);
            return pixel == BLACK;
        }

        return (x < 50 || x >= 1150 || y < 50 || y >= 850);
    }

    bool checkSuccess(int x, int y) {
        if (!bgLoaded || x < 0 || x >= 1200 || y < 0 || y >= 900)
            return false;

        COLORREF pixel = getpixel(x, y);
        return GetRValue(pixel) > 200 && GetGValue(pixel) < 100 && GetBValue(pixel) < 100;
    }

    bool checkYellowArea(int x, int y) {
        if (!bgLoaded || x < 0 || x >= 1200 || y < 0 || y >= 900)
            return false;

        COLORREF pixel = getpixel(x, y);
        return isYellowArea(pixel);
    }
};

class Vehicle {
private:
    int x, y;
    double rotation;
    IMAGE img, scaledImg, rotatedImg;
    bool imgLoaded;
    const int scale = 4;
    const double ROTATION_STEP = DEG2RAD(5);
    const int MOVE_STEP = 10;

    const int INIT_X = 100;
    const int INIT_Y = 450;
    const double INIT_ROTATION = 0;

    int imgWidth, imgHeight;

public:
    const wchar_t* CAR_PATH = L"car.png";

    Vehicle() :
        x(INIT_X), y(INIT_Y), rotation(INIT_ROTATION),
        imgLoaded(false), imgWidth(0), imgHeight(0) {

        loadimage(&img, CAR_PATH);

        if (img.getwidth() <= 0 || img.getheight() <= 0) {
            imgLoaded = false;
            wchar_t msg[200];
            swprintf_s(msg, L"车辆图像加载失败：%s\n将使用红色矩形代替", CAR_PATH);
            MessageBox(NULL, msg, L"警告", MB_OK | MB_ICONWARNING);
            imgWidth = 50;
            imgHeight = 30;
        }
        else {
            imgLoaded = true;
            imgWidth = img.getwidth() / scale;
            imgHeight = img.getheight() / scale;
            loadimage(&scaledImg, CAR_PATH, imgWidth, imgHeight, true);
            rotatedImg = scaledImg;
        }

        printf("车辆初始位置: x=%d, y=%d, 图像尺寸: %dx%d\n", x, y, imgWidth, imgHeight);
        rotateImage();
    }

    bool moveForward(Map& map) {
        int newX = x + static_cast<int>(MOVE_STEP * cos(rotation));
        int newY = y + static_cast<int>(MOVE_STEP * sin(rotation));

        if (checkCollision(map, newX, newY))
            return false;

        x = newX;
        y = newY;
        applyBoundaryConstraints();
        return true;
    }

    bool moveBackward(Map& map) {
        int newX = x - static_cast<int>(MOVE_STEP * cos(rotation));
        int newY = y - static_cast<int>(MOVE_STEP * sin(rotation));

        if (checkCollision(map, newX, newY))
            return false;

        x = newX;
        y = newY;
        applyBoundaryConstraints();
        return true;
    }

    void turnLeft() {
        rotation -= ROTATION_STEP;
        rotateImage();
    }

    void turnRight() {
        rotation += ROTATION_STEP;
        rotateImage();
    }

    void draw() {
        if (imgLoaded) {
            int drawX = x - rotatedImg.getwidth() / 2;
            int drawY = y - rotatedImg.getheight() / 2;

            SetWorkingImage(&rotatedImg);
            for (int i = 0; i < rotatedImg.getwidth(); i++) {
                for (int j = 0; j < rotatedImg.getheight(); j++) {
                    COLORREF color = getpixel(i, j);
                    if (!isTransparent(color)) {
                        SetWorkingImage(NULL);
                        putpixel(drawX + i, drawY + j, color);
                        SetWorkingImage(&rotatedImg);
                    }
                }
            }
            SetWorkingImage(NULL);
        }
        else {
            POINT points[4];
            int width = imgWidth;
            int height = imgHeight;

            points[0].x = x + static_cast<int>(-width / 2 * cos(rotation) - height / 2 * sin(rotation));
            points[0].y = y + static_cast<int>(-width / 2 * sin(rotation) + height / 2 * cos(rotation));
            points[1].x = x + static_cast<int>(width / 2 * cos(rotation) - height / 2 * sin(rotation));
            points[1].y = y + static_cast<int>(width / 2 * sin(rotation) + height / 2 * cos(rotation));
            points[2].x = x + static_cast<int>(width / 2 * cos(rotation) + height / 2 * sin(rotation));
            points[2].y = y + static_cast<int>(width / 2 * sin(rotation) - height / 2 * cos(rotation));
            points[3].x = x + static_cast<int>(-width / 2 * cos(rotation) + height / 2 * sin(rotation));
            points[3].y = y + static_cast<int>(-width / 2 * sin(rotation) - height / 2 * cos(rotation));

            setfillcolor(RED);
            fillpolygon(points, 4);
        }
    }

    void reset() {
        x = INIT_X;
        y = INIT_Y;
        rotation = INIT_ROTATION;
        rotateImage();
        printf("车辆已复位: x=%d, y=%d\n", x, y);
    }

    bool checkSuccess(Map& map) {
        int points[5][2] = {
            {x, y},
            {x - imgWidth / 2, y - imgHeight / 2},
            {x + imgWidth / 2, y - imgHeight / 2},
            {x - imgWidth / 2, y + imgHeight / 2},
            {x + imgWidth / 2, y + imgHeight / 2}
        };

        for (int i = 0; i < 5; i++) {
            if (map.checkSuccess(points[i][0], points[i][1]))
                return true;
        }
        return false;
    }

    bool isFullyInYellowArea(Map& map) {
        int points[5][2] = {
            {x, y},
            {x - imgWidth / 2, y - imgHeight / 2},
            {x + imgWidth / 2, y - imgHeight / 2},
            {x - imgWidth / 2, y + imgHeight / 2},
            {x + imgWidth / 2, y + imgHeight / 2}
        };

        for (int i = 0; i < 5; i++) {
            if (!map.checkYellowArea(points[i][0], points[i][1]))
                return false;
        }
        return true;
    }

    bool isPartiallyInYellowArea(Map& map) {
        int points[5][2] = {
            {x, y},
            {x - imgWidth / 2, y - imgHeight / 2},
            {x + imgWidth / 2, y - imgHeight / 2},
            {x - imgWidth / 2, y + imgHeight / 2},
            {x + imgWidth / 2, y + imgHeight / 2}
        };

        for (int i = 0; i < 5; i++) {
            if (map.checkYellowArea(points[i][0], points[i][1]))
                return true;
        }
        return false;
    }

    int getWidth() const { return imgWidth; }
    int getHeight() const { return imgHeight; }
    int getX() const { return x; }
    int getY() const { return y; }

private:
    void rotateImage() {
        if (!imgLoaded) return;

        int w = scaledImg.getwidth();
        int h = scaledImg.getheight();

        int newWidth = static_cast<int>(w * fabs(cos(rotation)) + h * fabs(sin(rotation)));
        int newHeight = static_cast<int>(w * fabs(sin(rotation)) + h * fabs(cos(rotation)));

        imgWidth = newWidth;
        imgHeight = newHeight;

        rotatedImg = IMAGE(newWidth, newHeight);
        SetWorkingImage(&rotatedImg);
        setbkcolor(BLACK);
        cleardevice();

        int cx = newWidth / 2;
        int cy = newHeight / 2;

        SetWorkingImage(&scaledImg);
        for (int i = 0; i < w; i++) {
            for (int j = 0; j < h; j++) {
                double dx = i - w / 2;
                double dy = j - h / 2;

                double rx = dx * cos(rotation) - dy * sin(rotation);
                double ry = dx * sin(rotation) + dy * cos(rotation);

                int nx = static_cast<int>(rx + cx);
                int ny = static_cast<int>(ry + cy);

                if (nx >= 0 && nx < newWidth && ny >= 0 && ny < newHeight) {
                    COLORREF color = getpixel(i, j);
                    if (!isTransparent(color)) {
                        SetWorkingImage(&rotatedImg);
                        putpixel(nx, ny, color);
                        SetWorkingImage(&scaledImg);
                    }
                }
            }
        }
        SetWorkingImage(NULL);
    }

    void applyBoundaryConstraints() {
        if (x < imgWidth / 2) x = imgWidth / 2;
        if (x > 1200 - imgWidth / 2) x = 1200 - imgWidth / 2;
        if (y < imgHeight / 2) y = imgHeight / 2;
        if (y > 900 - imgHeight / 2) y = 900 - imgHeight / 2;
    }

    bool checkCollision(Map& map, int testX, int testY) {
        int corners[4][2] = {
            {testX - imgWidth / 2, testY - imgHeight / 2},
            {testX + imgWidth / 2, testY - imgHeight / 2},
            {testX - imgWidth / 2, testY + imgHeight / 2},
            {testX + imgWidth / 2, testY + imgHeight / 2}
        };

        for (int i = 0; i < 4; i++) {
            if (map.checkCollision(corners[i][0], corners[i][1]))
                return true;
        }
        return map.checkCollision(testX, testY);
    }
};

enum GameState {
    RUNNING,
    SUCCESS,
    FAILURE,
    GAME_OVER
};

int main() {
    showGameRules();

    HWND hwnd = initgraph(1200, 900);
    if (!hwnd) {
        MessageBox(NULL, L"图形窗口初始化失败！", L"错误", MB_OK);
        return 1;
    }
    SetForegroundWindow(hwnd);

    Map map;
    Vehicle car;
    BeginBatchDraw();

    bool keyWDown = false, keySDown = false, keyADown = false, keyDDown = false, keyQDown = false;
    const int MOVE_INTERVAL = 50;
    ULONGLONG lastMoveTime = GetTickCount64();

    GameState gameState = RUNNING;
    int attemptsLeft = 3;

    bool lateralParkingStarted = false;    
    bool reverseParkingStarted = false;   

 
    bool yellowArea1Passed = false;
    bool yellowArea2Passed = false;
    bool wasInYellowArea = false;
    bool isInYellowArea = false;

    while (true) {
        bool currentW = (GetAsyncKeyState('W') & 0x8000) != 0;
        bool currentS = (GetAsyncKeyState('S') & 0x8000) != 0;
        bool currentA = (GetAsyncKeyState('A') & 0x8000) != 0;
        bool currentD = (GetAsyncKeyState('D') & 0x8000) != 0;
        bool currentQ = (GetAsyncKeyState('Q') & 0x8000) != 0;

      
        if (currentQ && !keyQDown) break;

       
        keyWDown = currentW;
        keySDown = currentS;
        keyADown = currentA;
        keyDDown = currentD;
        keyQDown = currentQ;

        if (gameState == RUNNING) {
            
            ULONGLONG currentTime = GetTickCount64();

            bool collisionOccurred = false;
            bool successOccurred = false;

        
            if (currentTime - lastMoveTime > MOVE_INTERVAL) {
           
                if (keyWDown) {
                    if (!car.moveForward(map))
                        collisionOccurred = true;
                }
                if (keySDown) {
                    if (!car.moveBackward(map))
                        collisionOccurred = true;
                }
                if (keyADown) car.turnLeft();
                if (keyDDown) car.turnRight();

                lastMoveTime = currentTime;
            }

         
            isInYellowArea = car.isPartiallyInYellowArea(map);

           
            if (isInYellowArea && !wasInYellowArea) {
                int centerX = car.getX();
                int centerY = car.getY();

                if (centerX < 600 && !yellowArea1Passed) {
                    yellowArea1Passed = true;
                    lateralParkingStarted = true;
                    MessageBox(NULL, L"已通过侧方停车考试", L"考试项目", MB_OK | MB_ICONINFORMATION);
                }
                
                else if (centerX >= 600 && !yellowArea2Passed && yellowArea1Passed) {
                    yellowArea2Passed = true;
                    reverseParkingStarted = true;
                    MessageBox(NULL, L"已通过倒车入库考试", L"考试项目", MB_OK | MB_ICONINFORMATION);
                }
            }
            wasInYellowArea = isInYellowArea;

            if (car.checkSuccess(map) && yellowArea1Passed && yellowArea2Passed) {
                successOccurred = true;
                gameState = SUCCESS;
            }
            else if (car.checkSuccess(map) && (!yellowArea1Passed || !yellowArea2Passed)) {
           
                MessageBox(NULL, L"请先完成前面的考试项目！", L"考试未完成", MB_OK | MB_ICONWARNING);
            }

            if (collisionOccurred) {
                attemptsLeft--;

                wchar_t message[100];
                swprintf_s(message, L"挑战失败！\n剩余尝试次数: %d", attemptsLeft);
                MessageBox(NULL, message, L"碰撞检测", MB_OK | MB_ICONWARNING);

                car.reset();
                lateralParkingStarted = false;
                reverseParkingStarted = false;

                if (attemptsLeft <= 0) {
                    gameState = GAME_OVER;
                    yellowArea1Passed = false;
                    yellowArea2Passed = false;
                }
            }

            if (successOccurred) {
                MessageBox(NULL, L"考试通过！恭喜完成科目二考试！", L"游戏成功", MB_OK | MB_ICONINFORMATION);
               
                yellowArea1Passed = false;
                yellowArea2Passed = false;
                lateralParkingStarted = false;
                reverseParkingStarted = false;
                car.reset();
                gameState = RUNNING;
            }

            if (gameState == GAME_OVER) {
                MessageBox(NULL, L"考试失败！\n所有尝试次数已用完。", L"游戏结束", MB_OK | MB_ICONINFORMATION);
             
                attemptsLeft = 3;
                car.reset();
                yellowArea1Passed = false;
                yellowArea2Passed = false;
                lateralParkingStarted = false;
                reverseParkingStarted = false;
                gameState = RUNNING;
            }
        }

        cleardevice();
        map.draw();
        car.draw();

        drawGameRules();


        settextstyle(16, 0, L"微软雅黑");
        settextcolor(YELLOW);
        wchar_t attemptsText[20];
        swprintf_s(attemptsText, L"剩余尝试次数: %d", attemptsLeft);
        outtextxy(10, 10, attemptsText);

        settextcolor(WHITE);
        outtextxy(10, 40, L"考试项目进度:");
        outtextxy(10, 70, lateralParkingStarted ? L"✓ 侧方停车" : L"○ 侧方停车");
        outtextxy(10, 100, reverseParkingStarted ? L"✓ 倒车入库" : L"○ 倒车入库");
        outtextxy(10, 130, (yellowArea1Passed && yellowArea2Passed) ? L"✓ 弯道行驶" : L"○ 弯道行驶");

      
        if (gameState == SUCCESS) {
            settextcolor(GREEN);
            outtextxy(500, 10, L"考试通过！按Q退出");
        }
        else if (gameState == GAME_OVER) {
            settextcolor(RED);
            outtextxy(500, 10, L"游戏结束！按任意键重新开始");
        }

        FlushBatchDraw();
        Sleep(10);
    }

    EndBatchDraw();
    closegraph();
    return 0;
}