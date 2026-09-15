#include <SDL.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <iomanip>
#include <sstream>

const int WIDTH = 640;
const int HEIGHT = 480;
const int PIXEL_SCALE = 2;
const int SCREEN_W = WIDTH / PIXEL_SCALE;
const int SCREEN_H = HEIGHT / PIXEL_SCALE;

struct Insect {
    std::string name;
    float x, y;
    float vx, vy;
    int type; 
    bool isTarget;
    SDL_Color color;
    std::vector<float> movementHistory;
    bool wasInsideLastFrame;

    void reset(bool target, std::string n, SDL_Color col, int t) {
        name = n;
        isTarget = target;
        color = col;
        type = t;
        x = static_cast<float>(rand() % (SCREEN_W - 60) + 30);
        y = static_cast<float>(rand() % (SCREEN_H / 2 - 20) + 20);
        vx = (static_cast<float>(rand() % 20 - 10)) / 4.0f;
        vy = (static_cast<float>(rand() % 20 - 10)) / 4.0f;
        movementHistory.clear();
        wasInsideLastFrame = false;
    }

    void update(float speedFactor) {
        x += vx * speedFactor;
        y += vy * speedFactor;

        vx += (static_cast<float>(rand() % 10 - 5) / 15.0f);
        vy += (static_cast<float>(rand() % 10 - 5) / 15.0f);

        vx = std::clamp(vx, -2.5f, 2.5f);
        vy = std::clamp(vy, -2.5f, 2.5f);

        if (x < 20) { x = 20; vx *= -1; }
        if (x > SCREEN_W - 30) { x = SCREEN_W - 30; vx *= -1; }
        if (y < 20) { y = 20; vy *= -1; }
        if (y > (SCREEN_H * 3 / 5) - 30) { y = (SCREEN_H * 3 / 5) - 30; vy *= -1; }

        movementHistory.push_back(vy);
        if (movementHistory.size() > 50) {
            movementHistory.erase(movementHistory.begin());
        }
    }
};

// Universal background loader supporting both color and grayscale modes
SDL_Texture* loadBackground(SDL_Renderer* renderer, const std::string& filename, int targetW, int targetH, bool grayscale) {
    cv::Mat img = cv::imread(filename, cv::IMREAD_COLOR);
    if (img.empty()) {
        img = cv::imread("../" + filename, cv::IMREAD_COLOR);
    }
    if (img.empty()) {
        std::cout << "Warning: Could not load " << filename << ". Falling back to blank." << std::endl;
        return nullptr;
    }

    cv::Mat processed;
    if (grayscale) {
        cv::Mat gray;
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
        cv::cvtColor(gray, processed, cv::COLOR_GRAY2BGRA);
    } else {
        cv::cvtColor(img, processed, cv::COLOR_BGR2BGRA);
    }

    cv::Mat resized;
    cv::resize(processed, resized, cv::Size(targetW, targetH));

    SDL_Texture* texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, targetW, targetH
    );
    SDL_UpdateTexture(texture, nullptr, resized.data, resized.step[0]);
    return texture;
}

void drawPixelChar(SDL_Renderer* renderer, char ch, int x, int y) {
    ch = std::toupper(ch);
    const bool font[38][7][5] = {
        /* A */ {{0,1,1,1,0},{1,0,0,0,1},{1,0,0,0,1},{1,1,1,1,1},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1}},
        /* B */ {{1,1,1,1,0},{1,0,0,0,1},{1,0,0,0,1},{1,1,1,1,0},{1,0,0,0,1},{1,0,0,0,1},{1,1,1,1,0}},
        /* C */ {{0,1,1,1,1},{1,0,0,0,0},{1,0,0,0,0},{1,0,0,0,0},{1,0,0,0,0},{1,0,0,0,0},{0,1,1,1,1}},
        /* D */ {{1,1,1,1,0},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{1,1,1,1,0}},
        /* E */ {{1,1,1,1,1},{1,0,0,0,0},{1,0,0,0,0},{1,1,1,1,0},{1,0,0,0,0},{1,0,0,0,0},{1,1,1,1,1}},
        /* F */ {{1,1,1,1,1},{1,0,0,0,0},{1,0,0,0,0},{1,1,1,1,0},{1,0,0,0,0},{1,0,0,0,0},{1,0,0,0,0}},
        /* G */ {{0,1,1,1,1},{1,0,0,0,0},{1,0,0,0,0},{1,0,1,1,1},{1,0,0,0,1},{1,0,0,0,1},{0,1,1,1,1}},
        /* H */ {{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{1,1,1,1,1},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1}},
        /* I */ {{0,1,1,1,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,1,1,1,0}},
        /* J */ {{0,0,0,0,1},{0,0,0,0,1},{0,0,0,0,1},{0,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{0,1,1,1,0}},
        /* K */ {{1,0,0,0,1},{1,0,0,1,0},{1,0,1,0,0},{1,1,0,0,0},{1,0,1,0,0},{1,0,0,1,0},{1,0,0,0,1}},
        /* L */ {{1,0,0,0,0},{1,0,0,0,0},{1,0,0,0,0},{1,0,0,0,0},{1,0,0,0,0},{1,0,0,0,0},{1,1,1,1,1}},
        /* M */ {{1,0,0,0,1},{1,1,0,1,1},{1,0,1,0,1},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1}},
        /* N */ {{1,0,0,0,1},{1,1,0,0,1},{1,0,1,0,1},{1,0,0,1,1},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1}},
        /* O */ {{0,1,1,1,0},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{0,1,1,1,0}},
        /* P */ {{1,1,1,1,0},{1,0,0,0,1},{1,0,0,0,1},{1,1,1,1,0},{1,0,0,0,0},{1,0,0,0,0},{1,0,0,0,0}},
        /* Q */ {{0,1,1,1,0},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{1,0,1,0,1},{1,0,0,1,0},{0,1,1,0,1}},
        /* R */ {{1,1,1,1,0},{1,0,0,0,1},{1,0,0,0,1},{1,1,1,1,0},{1,0,1,0,0},{1,0,0,1,0},{1,0,0,0,1}},
        /* S */ {{0,1,1,1,1},{1,0,0,0,0},{1,0,0,0,0},{0,1,1,1,0},{0,0,0,0,1},{0,0,0,0,1},{1,1,1,1,0}},
        /* T */ {{1,1,1,1,1},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0}},
        /* U */ {{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{0,1,1,1,0}},
        /* V */ {{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{0,1,0,1,0},{0,1,0,1,0},{0,0,1,0,0}},
        /* W */ {{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{1,0,1,0,1},{1,0,1,0,1},{1,1,0,1,1},{1,0,0,0,1}},
        /* X */ {{1,0,0,0,1},{1,0,0,0,1},{0,1,0,1,0},{0,0,1,0,0},{0,1,0,1,0},{1,0,0,0,1},{1,0,0,0,1}},
        /* Y */ {{1,0,0,0,1},{1,0,0,0,1},{0,1,0,1,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0}},
        /* Z */ {{1,1,1,1,1},{0,0,0,0,1},{0,0,0,1,0},{0,0,1,0,0},{0,1,0,0,0},{1,0,0,0,0},{1,1,1,1,1}},
        /* 0 */ {{1,1,1,1,1},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{1,1,1,1,1}},
        /* 1 */ {{0,0,1,0,0},{0,1,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,1,1,1,0}},
        /* 2 */ {{1,1,1,1,1},{0,0,0,0,1},{0,0,0,0,1},{1,1,1,1,1},{1,0,0,0,0},{1,0,0,0,0},{1,1,1,1,1}},
        /* 3 */ {{1,1,1,1,1},{0,0,0,0,1},{0,0,0,0,1},{1,1,1,1,1},{0,0,0,0,1},{0,0,0,0,1},{1,1,1,1,1}},
        /* 4 */ {{1,0,0,0,1},{1,0,0,0,1},{1,0,0,0,1},{1,1,1,1,1},{0,0,0,0,1},{0,0,0,0,1},{0,0,0,0,1}},
        /* 5 */ {{1,1,1,1,1},{1,0,0,0,0},{1,0,0,0,0},{1,1,1,1,1},{0,0,0,0,1},{0,0,0,0,1},{1,1,1,1,1}},
        /* 6 */ {{1,1,1,1,1},{1,0,0,0,0},{1,0,0,0,0},{1,1,1,1,1},{1,0,0,0,1},{1,0,0,0,1},{1,1,1,1,1}},
        /* 7 */ {{1,1,1,1,1},{0,0,0,0,1},{0,0,0,1,0},{0,0,1,0,0},{0,1,0,0,0},{0,1,0,0,0},{0,1,0,0,0}},
        /* 8 */ {{1,1,1,1,1},{1,0,0,0,1},{1,0,0,0,1},{1,1,1,1,1},{1,0,0,0,1},{1,0,0,0,1},{1,1,1,1,1}},
        /* 9 */ {{1,1,1,1,1},{1,0,0,0,1},{1,0,0,0,1},{1,1,1,1,1},{0,0,0,0,1},{0,0,0,0,1},{1,1,1,1,1}},
        /* # */ {{0,1,0,1,0},{1,1,1,1,1},{0,1,0,1,0},{1,1,1,1,1},{0,1,0,1,0},{1,1,1,1,1},{0,1,0,1,0}},
        /* : */ {{0,0,0,0,0},{0,0,1,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,1,0,0},{0,0,0,0,0},{0,0,0,0,0}}
    };

    int idx = -1;
    if (ch >= 'A' && ch <= 'Z') idx = ch - 'A';
    else if (ch >= '0' && ch <= '9') idx = 26 + (ch - '0');
    else if (ch == '#') idx = 36;
    else if (ch == ':') idx = 37;

    if (idx < 0) return;

    for (int r = 0; r < 7; ++r) {
        for (int c = 0; c < 5; ++c) {
            if (font[idx][r][c]) {
                SDL_RenderDrawPoint(renderer, x + c, y + r);
            }
        }
    }
}

void drawPixelText(SDL_Renderer* renderer, const std::string& text, int x, int y) {
    int offset = 0;
    for (char ch : text) {
        if (ch == ' ') {
            offset += 5;
            continue;
        }
        drawPixelChar(renderer, ch, x + offset, y);
        offset += 6;
    }
}

void drawNumber(SDL_Renderer* renderer, int n, int startX, int startY) {
    if (n < 0) n = 0;
    if (n > 9999) n = 9999;
    std::string s = std::to_string(n);
    int offset = 0;
    for (char ch : s) {
        drawPixelChar(renderer, ch, startX + offset, startY);
        offset += 6;
    }
}

void drawInsectSprite(SDL_Renderer* renderer, int px, int py, int type, bool isTarget, bool insideZone) {
    if (type == 0) {
        SDL_Rect body = { px - 3, py - 3, 7, 6 };
        SDL_SetRenderDrawColor(renderer, 255, 180, 0, 255);
        SDL_RenderFillRect(renderer, &body);
    } else if (type == 1) {
        SDL_Rect body = { px - 6, py - 1, 12, 2 };
        SDL_SetRenderDrawColor(renderer, 180, 200, 150, 255);
        SDL_RenderFillRect(renderer, &body);
    } else if (type == 2) {
        SDL_Rect body = { px - 3, py - 2, 6, 5 };
        SDL_SetRenderDrawColor(renderer, 80, 160, 120, 255);
        SDL_RenderFillRect(renderer, &body);
    } else {
        SDL_Rect body = { px - 2, py - 2, 4, 4 };
        SDL_SetRenderDrawColor(renderer, 140, 140, 150, 255);
        SDL_RenderFillRect(renderer, &body);
    }

    if (isTarget && insideZone) {
        SDL_SetRenderDrawColor(renderer, 50, 255, 100, 255);
        SDL_Rect box = { px - 10, py - 10, 20, 20 };
        SDL_RenderDrawRect(renderer, &box);
    } else {
        SDL_SetRenderDrawColor(renderer, 200, 70, 70, 160);
        SDL_Rect box = { px - 6, py - 6, 12, 12 };
        SDL_RenderDrawRect(renderer, &box);
    }
}

int main(int argc, char* argv[]) {
    srand(static_cast<unsigned int>(time(nullptr)));

    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;

    SDL_Window* window = SDL_CreateWindow(
        "Advanced Insect Tracking & Signature Analysis",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIDTH, HEIGHT, SDL_WINDOW_SHOWN
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    int splitY = SCREEN_H * 3 / 5;
    
    // Load both Color (outside box) and Grayscale (inside box) background textures
    SDL_Texture* colorBgTexture = loadBackground(renderer, "image.png", SCREEN_W, splitY, false);
    SDL_Texture* grayBgTexture  = loadBackground(renderer, "image.png", SCREEN_W, splitY, true);

    SDL_Texture* canvas = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, SCREEN_W, SCREEN_H
    );

    std::vector<Insect> insects(4);
    insects[0].reset(true, "HONEYBEE", {255, 200, 0, 255}, 0);     
    insects[1].reset(false, "MAYFLY", {150, 200, 255, 255}, 1);    
    insects[2].reset(false, "BLOWFLY", {100, 180, 120, 255}, 2);   
    insects[3].reset(false, "MOSQUITO", {180, 150, 150, 255}, 3);  

    float simulationSpeed = 1.0f;
    bool isDraggingSlider = false;
    int beeCounter = 0;
    float elapsedTimeSeconds = 0.0f;

    SDL_Rect detectionZone = { 30, 20, 160, 90 };
    
    SDL_Rect counterBox  = { SCREEN_W - 130, splitY + 16, 100, 18 };
    SDL_Rect timerBox    = { SCREEN_W - 130, splitY + 48, 100, 18 };
    SDL_Rect sliderTrack = { SCREEN_W - 130, splitY + 76, 100, 8 };

    bool running = true;
    SDL_Event event;
    Uint32 lastTime = SDL_GetTicks();

    while (running) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        elapsedTimeSeconds += deltaTime * simulationSpeed;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mx = event.button.x / PIXEL_SCALE;
                int my = event.button.y / PIXEL_SCALE;
                if (mx >= sliderTrack.x - 8 && mx <= sliderTrack.x + sliderTrack.w + 8 &&
                    my >= sliderTrack.y - 10 && my <= sliderTrack.y + sliderTrack.h + 10) {
                    isDraggingSlider = true;
                    float ratio = static_cast<float>(mx - sliderTrack.x) / static_cast<float>(sliderTrack.w);
                    ratio = std::clamp(ratio, 0.0f, 1.0f);
                    simulationSpeed = 0.2f + ratio * 2.8f;
                }
            } else if (event.type == SDL_MOUSEBUTTONUP) {
                isDraggingSlider = false;
            } else if (event.type == SDL_MOUSEMOTION && isDraggingSlider) {
                int mx = event.motion.x / PIXEL_SCALE;
                float ratio = static_cast<float>(mx - sliderTrack.x) / static_cast<float>(sliderTrack.w);
                ratio = std::clamp(ratio, 0.0f, 1.0f);
                simulationSpeed = 0.2f + ratio * 2.8f;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_r) {
                    for (size_t i = 0; i < insects.size(); ++i) {
                        insects[i].reset(insects[i].isTarget, insects[i].name, insects[i].color, insects[i].type);
                    }
                    beeCounter = 0;
                    elapsedTimeSeconds = 0.0f;
                }
            }
        }

        for (auto& ins : insects) {
            ins.update(simulationSpeed);

            if (ins.isTarget) {
                bool currentlyInside = (ins.x >= detectionZone.x && ins.x <= detectionZone.x + detectionZone.w &&
                                        ins.y >= detectionZone.y && ins.y <= detectionZone.y + detectionZone.h);
                
                if (currentlyInside && !ins.wasInsideLastFrame) {
                    beeCounter++;
                }
                
                if (!currentlyInside) {
                    ins.wasInsideLastFrame = false;
                } else {
                    ins.wasInsideLastFrame = true;
                }
            }
        }

        // --- RENDER TO CANVAS ---
        SDL_SetRenderTarget(renderer, canvas);
        SDL_SetRenderDrawColor(renderer, 20, 25, 35, 255);
        SDL_RenderClear(renderer);

        // 1. Render Full Color Background Outside the Box
        if (colorBgTexture) {
            SDL_Rect bgRect = { 0, 0, SCREEN_W, splitY };
            SDL_RenderCopy(renderer, colorBgTexture, nullptr, &bgRect);
        }

        // 2. Render Grayscale Background *Only Inside* the Detection Zone
        if (grayBgTexture) {
            SDL_RenderCopy(renderer, grayBgTexture, &detectionZone, &detectionZone);
        }

        // Detection Zone Border
        SDL_SetRenderDrawColor(renderer, 50, 255, 100, 255);
        SDL_RenderDrawRect(renderer, &detectionZone);

        // 3. Render Insects
        for (const auto& ins : insects) {
            bool insideZone = (ins.x >= detectionZone.x && ins.x <= detectionZone.x + detectionZone.w &&
                               ins.y >= detectionZone.y && ins.y <= detectionZone.y + detectionZone.h);
            drawInsectSprite(renderer, static_cast<int>(ins.x), static_cast<int>(ins.y), ins.type, ins.isTarget, insideZone);
        }

        // 4. Control Deck & Dashboard Panel
        SDL_SetRenderDrawColor(renderer, 40, 48, 60, 255);
        SDL_Rect deckBg = { 0, splitY, SCREEN_W, SCREEN_H - splitY };
        SDL_RenderFillRect(renderer, &deckBg);

        SDL_SetRenderDrawColor(renderer, 80, 100, 130, 255);
        SDL_RenderDrawLine(renderer, 0, splitY, SCREEN_W, splitY);

        // Draw Waveform Signatures on Left
        int rowH = (SCREEN_H - splitY) / 4;
        for (size_t i = 0; i < insects.size(); ++i) {
            int startY = splitY + (static_cast<int>(i) * rowH) + 4;
            if (insects[i].isTarget) {
                SDL_SetRenderDrawColor(renderer, 50, 255, 100, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 120, 130, 150, 255);
            }

            const auto& hist = insects[i].movementHistory;
            for (size_t j = 1; j < hist.size(); ++j) {
                int x1 = 15 + static_cast<int>(j - 1) * 3;
                int y1 = startY + rowH / 2 + static_cast<int>(hist[j - 1] * 4);
                int x2 = 15 + static_cast<int>(j) * 3;
                int y2 = startY + rowH / 2 + static_cast<int>(hist[j] * 4);
                SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
            }
        }

        // 5. Bee Counter Component (# BEES IN SAMPLE)
        SDL_SetRenderDrawColor(renderer, 150, 170, 190, 255);
        drawPixelText(renderer, "# BEES IN SAMPLE", counterBox.x, splitY + 7);

        SDL_SetRenderDrawColor(renderer, 25, 32, 42, 255);
        SDL_RenderFillRect(renderer, &counterBox);
        SDL_SetRenderDrawColor(renderer, 80, 100, 130, 255);
        SDL_RenderDrawRect(renderer, &counterBox);

        SDL_SetRenderDrawColor(renderer, 50, 255, 100, 255);
        drawNumber(renderer, beeCounter, counterBox.x + 8, counterBox.y + 5);

        // 6. Timer Component (#RUN TIME)
        SDL_SetRenderDrawColor(renderer, 150, 170, 190, 255);
        drawPixelText(renderer, "#RUN TIME", timerBox.x, splitY + 39);

        SDL_SetRenderDrawColor(renderer, 25, 32, 42, 255);
        SDL_RenderFillRect(renderer, &timerBox);
        SDL_SetRenderDrawColor(renderer, 80, 100, 130, 255);
        SDL_RenderDrawRect(renderer, &timerBox);

        int totalSecs = static_cast<int>(elapsedTimeSeconds);
        int mins = totalSecs / 60;
        int secs = totalSecs % 60;
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(2) << mins << ":" << std::setw(2) << secs;
        
        SDL_SetRenderDrawColor(renderer, 50, 255, 100, 255);
        drawPixelText(renderer, ss.str(), timerBox.x + 8, timerBox.y + 5);

        // 7. Horizontal Draggable Slider
        SDL_SetRenderDrawColor(renderer, 80, 95, 120, 255);
        SDL_RenderFillRect(renderer, &sliderTrack);

        float handleRatio = (simulationSpeed - 0.2f) / 2.8f;
        int handleX = sliderTrack.x + static_cast<int>(handleRatio * sliderTrack.w);
        SDL_Rect sliderHandle = { handleX - 4, sliderTrack.y - 3, 8, sliderTrack.h + 6 };
        SDL_SetRenderDrawColor(renderer, 220, 230, 240, 255);
        SDL_RenderFillRect(renderer, &sliderHandle);

        // --- PRESENT TO WINDOW ---
        SDL_SetRenderTarget(renderer, nullptr);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, canvas, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        SDL_Delay(25);
    }

    if (colorBgTexture) SDL_DestroyTexture(colorBgTexture);
    if (grayBgTexture) SDL_DestroyTexture(grayBgTexture);
    SDL_DestroyTexture(canvas);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}