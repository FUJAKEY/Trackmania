#include "raylib.h"
#include "raymath.h"
#include <string>
#include <vector>
#include <cstdio>

// Constants
#define MAX_COLUMNS 20

// Types
struct Player {
    Vector3 position;
    Vector3 velocity;
    float angle;      // Rotation around Y axis (radians)
    float speed;
    float acceleration;
    float rotationSpeed;
    bool finished;
    double startTime;
    double finishTime;
    BoundingBox bounds;
};

struct Platform {
    Vector3 position;
    Vector3 size;
    Color color;
    bool isFinish;
    bool isStart;
};

// Global State
Player player;
Camera3D camera = { 0 };
std::vector<Platform> track;
bool gameStarted = false;

void ResetGame() {
    player.position = (Vector3){ 0.0f, 1.0f, 0.0f };
    player.velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
    player.angle = 0.0f;
    player.speed = 0.0f;
    player.acceleration = 0.0f;
    player.rotationSpeed = 0.0f;
    player.finished = false;
    player.startTime = 0;
    player.finishTime = 0;
    gameStarted = false;

    // Initial camera setup
    camera.position = (Vector3){ 0.0f, 10.0f, -10.0f };
    camera.target = player.position;
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}

void InitTrack() {
    track.clear();
    // Start
    track.push_back({ (Vector3){ 0.0f, -1.0f, 0.0f }, (Vector3){ 20.0f, 2.0f, 20.0f }, GRAY, false, true });

    // Road 1
    track.push_back({ (Vector3){ 0.0f, -1.0f, 30.0f }, (Vector3){ 20.0f, 2.0f, 40.0f }, DARKGRAY, false, false });

    // Turn
    track.push_back({ (Vector3){ 15.0f, -1.0f, 60.0f }, (Vector3){ 50.0f, 2.0f, 20.0f }, DARKGRAY, false, false });

    // Road 2
    track.push_back({ (Vector3){ 40.0f, -1.0f, 60.0f }, (Vector3){ 20.0f, 2.0f, 20.0f }, DARKGRAY, false, false });

    // Jump Ramp (Upward slope approximated by steps or just a block for now)
    // Let's make a gap
    track.push_back({ (Vector3){ 40.0f, -1.0f, 90.0f }, (Vector3){ 20.0f, 2.0f, 40.0f }, DARKGRAY, false, false });

    // Landing
    track.push_back({ (Vector3){ 40.0f, -1.0f, 150.0f }, (Vector3){ 20.0f, 2.0f, 60.0f }, DARKGRAY, false, false });

    // Finish
    track.push_back({ (Vector3){ 40.0f, -1.0f, 190.0f }, (Vector3){ 20.0f, 2.0f, 20.0f }, GREEN, true, false });
}

void UpdateGame() {
    float dt = GetFrameTime();

    // Input
    float accelInput = 0.0f;
    float turnInput = 0.0f;

#if defined(PLATFORM_ANDROID)
    // Simple touch controls (screen corners)
    // Left bottom: Brake/Reverse
    // Right bottom: Accelerate
    // Left/Right center: Turn
    // For now, let's use a virtual stick or just regions
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 touch = GetMousePosition();
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();

        if (touch.x > screenWidth / 2) accelInput = 1.0f; // Right side gas
        else accelInput = -1.0f; // Left side brake

        // Tilt sensor would be better but let's use touch Y for steering?
        // Or create buttons. Let's assume simpler:
        // Top Right: Gas
        // Bottom Right: Brake
        // Bottom Left: Left
        // Bottom Center: Right?
        // Let's keep it simple:
        // 4 Zones
    }

    // Touch zones
    for (int i = 0; i < GetTouchPointCount(); i++)
    {
        Vector2 touch = GetTouchPosition(i);
        int w = GetScreenWidth();
        int h = GetScreenHeight();

        if (touch.x > w * 0.7f && touch.y > h * 0.5f) accelInput = 1.0f;
        else if (touch.x > w * 0.7f && touch.y < h * 0.5f) accelInput = -1.0f;

        if (touch.x < w * 0.3f) turnInput = 1.0f; // Left
        else if (touch.x >= w * 0.3f && touch.x < w * 0.5f) turnInput = -1.0f; // Right
    }

#else
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) accelInput = 1.0f;
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) accelInput = -1.0f;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) turnInput = 1.0f;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) turnInput = -1.0f;
    if (IsKeyPressed(KEY_R)) ResetGame();
#endif

    if (player.finished) {
        if (IsKeyPressed(KEY_ENTER) || (IsGestureDetected(GESTURE_TAP) && GetTouchPointCount() > 0)) {
            ResetGame();
        }
        return;
    }

    // Physics
    float maxSpeed = 50.0f;
    float accelRate = 30.0f;
    float friction = 10.0f;
    float turnRate = 3.0f;

    // Start Timer
    if (!gameStarted && (accelInput != 0 || turnInput != 0)) {
        gameStarted = true;
        player.startTime = GetTime();
    }

    // Acceleration
    player.speed += accelInput * accelRate * dt;

    // Friction
    if (accelInput == 0) {
        if (player.speed > 0) player.speed -= friction * dt;
        if (player.speed < 0) player.speed += friction * dt;
        if (fabs(player.speed) < 1.0f) player.speed = 0;
    }

    // Cap speed
    if (player.speed > maxSpeed) player.speed = maxSpeed;
    if (player.speed < -maxSpeed / 2) player.speed = -maxSpeed / 2;

    // Turning (only when moving)
    if (fabs(player.speed) > 1.0f) {
        float turnFactor = player.speed / maxSpeed; // Turn slower at low speeds? Or inverse?
        // Let's make constant turning for arcade feel
        player.angle += turnInput * turnRate * dt;
    }

    // Update Velocity
    player.velocity.x = sinf(player.angle) * player.speed;
    player.velocity.z = cosf(player.angle) * player.speed;

    // Gravity (Simple)
    player.velocity.y -= 50.0f * dt;

    // Predict position
    Vector3 nextPos = Vector3Add(player.position, Vector3Scale(player.velocity, dt));

    // Collision with Track
    bool onGround = false;
    // Car bounds (approximate 2x1x2)
    BoundingBox carBox = {
        Vector3Subtract(nextPos, (Vector3){1.0f, 1.0f, 1.0f}),
        Vector3Add(nextPos, (Vector3){1.0f, 1.0f, 1.0f})
    };

    for (const auto& p : track) {
        BoundingBox platformBox = {
            Vector3Subtract(p.position, (Vector3){p.size.x/2, p.size.y/2, p.size.z/2}),
            Vector3Add(p.position, (Vector3){p.size.x/2, p.size.y/2, p.size.z/2})
        };

        if (CheckCollisionBoxes(carBox, platformBox)) {
            // Check if we are landing on top
            if (player.position.y >= p.position.y + p.size.y/2 - 0.5f) {
                onGround = true;
                nextPos.y = p.position.y + p.size.y/2 + 1.0f;
                player.velocity.y = 0;

                // Finish check
                if (p.isFinish) {
                    player.finished = true;
                    player.finishTime = GetTime();
                }
            } else {
                // Side collision - simple stop
                 player.speed = -player.speed * 0.5f; // Bounce
                 nextPos = player.position;
            }
        }
    }

    // Apply movement
    player.position = nextPos;

    // Fall check
    if (player.position.y < -20.0f) {
        ResetGame();
    }

    // Camera Follow
    float camDist = 12.0f;
    float camHeight = 6.0f;
    float camLerp = 5.0f * dt;

    Vector3 idealCamPos;
    idealCamPos.x = player.position.x - sinf(player.angle) * camDist;
    idealCamPos.y = player.position.y + camHeight;
    idealCamPos.z = player.position.z - cosf(player.angle) * camDist;

    camera.position = Vector3Lerp(camera.position, idealCamPos, camLerp);
    camera.target = Vector3Lerp(camera.target, player.position, 10.0f * dt);
}

void DrawGame() {
    BeginDrawing();
    ClearBackground(SKYBLUE);

    BeginMode3D(camera);
        // Draw Track
        for (const auto& p : track) {
            DrawCube(p.position, p.size.x, p.size.y, p.size.z, p.color);
            DrawCubeWires(p.position, p.size.x, p.size.y, p.size.z, BLACK);
        }

        // Draw Car
        Vector3 carSize = { 2.0f, 1.0f, 3.0f };
        DrawCube(player.position, carSize.x, carSize.y, carSize.z, RED);
        DrawCubeWires(player.position, carSize.x, carSize.y, carSize.z, MAROON);

        // Draw "Wheels" or orientation indicator
        Vector3 frontPos = Vector3Add(player.position, (Vector3){ sinf(player.angle)*1.0f, 0, cosf(player.angle)*1.0f });
        DrawCube(frontPos, 0.5f, 0.5f, 0.5f, YELLOW);

    EndMode3D();

    // UI
    if (!gameStarted) {
        DrawText("PRESS ARROWS/WASD TO START", 20, 20, 20, WHITE);
        DrawText("PRESS R TO RESTART", 20, 50, 20, WHITE);
#if defined(PLATFORM_ANDROID)
        DrawText("TOUCH RIGHT TO GAS, LEFT TO STEER", 20, 80, 20, WHITE);
#endif
    } else {
        double currentTime = player.finished ? player.finishTime : GetTime();
        double time = currentTime - player.startTime;
        char timeStr[32];
        sprintf(timeStr, "TIME: %.3f", time);
        DrawText(timeStr, 20, 20, 30, WHITE);
    }

    if (player.finished) {
        DrawText("FINISHED!", GetScreenWidth()/2 - 100, GetScreenHeight()/2 - 40, 40, GOLD);
        DrawText("PRESS ENTER/TAP TO RESTART", GetScreenWidth()/2 - 150, GetScreenHeight()/2 + 10, 20, WHITE);
    }

    DrawFPS(10, GetScreenHeight() - 30);
    EndDrawing();
}

int main() {
    InitWindow(800, 450, "Raylib TrackMania Clone");
    SetTargetFPS(60);

    InitTrack();
    ResetGame();

    while (!WindowShouldClose()) {
        UpdateGame();
        DrawGame();
    }

    CloseWindow();
    return 0;
}
