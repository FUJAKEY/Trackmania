/**
 * Trackmania Clone for Android
 *
 * Note on Libraries:
 * While the goal is to use minimal dependencies, Raylib is used here as a thin
 * abstraction layer over the Android NDK (NativeActivity, EGL, OpenGL ES).
 * This allows for a robust C++ implementation of the game logic without
 * reinventing the entire windowing and input handling subsystems from scratch,
 * ensuring the game is playable and performant on the target architecture.
 */

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>

// --- Constants & Config ---
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define CAR_SIZE (Vector3){ 2.0f, 1.0f, 4.0f }
#define GRAVITY -25.0f // Stronger gravity for arcade feel

// --- Structs ---
struct Car {
    Vector3 position;
    Vector3 velocity;
    float yaw;          // Rotation around Y axis
    float speed;
    float acceleration;
    float turnSpeed;
    float maxSpeed;
    float friction;

    // Simplistic physics state
    bool onGround;
    BoundingBox bounds;
};

struct TrackSegment {
    Vector3 position;
    Vector3 size;
    Color color;
    BoundingBox bounds;
};

// --- Globals ---
Car g_player;
std::vector<TrackSegment> g_track;
Camera3D g_camera;

// --- Physics Helper ---
bool CheckCollisionBox(BoundingBox box1, BoundingBox box2) {
    return (box1.max.x >= box2.min.x && box1.min.x <= box2.max.x) &&
           (box1.max.y >= box2.min.y && box1.min.y <= box2.max.y) &&
           (box1.max.z >= box2.min.z && box1.min.z <= box2.max.z);
}

void InitGame() {
    // Initialize Player
    g_player.position = (Vector3){ 0.0f, 2.0f, 0.0f };
    g_player.velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
    g_player.yaw = 0.0f;
    g_player.speed = 0.0f;
    g_player.acceleration = 30.0f;
    g_player.turnSpeed = 2.0f;
    g_player.maxSpeed = 60.0f;
    g_player.friction = 15.0f;
    g_player.onGround = false;

    // Build Track (A simple loop/straight)
    // Start Platform
    g_track.push_back({ (Vector3){ 0.0f, -1.0f, 0.0f }, (Vector3){ 20.0f, 2.0f, 20.0f }, DARKGRAY });

    // Straight
    for(int i=0; i<10; i++) {
        g_track.push_back({ (Vector3){ 0.0f, -1.0f, 20.0f + (i * 20.0f) }, (Vector3){ 20.0f, 2.0f, 20.0f }, GRAY });
    }

    // Ramp Up
    for(int i=0; i<5; i++) {
        float height = i * 2.0f;
        g_track.push_back({ (Vector3){ 0.0f, -1.0f + height, 220.0f + (i * 10.0f) }, (Vector3){ 20.0f, 2.0f, 10.0f }, RED });
    }

    // Jump landing
    g_track.push_back({ (Vector3){ 0.0f, -1.0f, 350.0f }, (Vector3){ 40.0f, 2.0f, 40.0f }, DARKGREEN });

    // Initialize Bounds
    for(auto& seg : g_track) {
        seg.bounds = (BoundingBox){
            (Vector3){ seg.position.x - seg.size.x/2, seg.position.y - seg.size.y/2, seg.position.z - seg.size.z/2 },
            (Vector3){ seg.position.x + seg.size.x/2, seg.position.y + seg.size.y/2, seg.position.z + seg.size.z/2 }
        };
    }

    // Initialize Camera
    g_camera.position = (Vector3){ 0.0f, 10.0f, -10.0f };
    g_camera.target = g_player.position;
    g_camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    g_camera.fovy = 60.0f;
    g_camera.projection = CAMERA_PERSPECTIVE;
}

void UpdateGame(float dt) {
    // --- Input (Touch regions for simplicity) ---
    // Left side of screen: Steer
    // Right side of screen: Gas/Brake
    float steerInput = 0.0f;
    float gasInput = 0.0f;

#if defined(PLATFORM_ANDROID)
    // Touch controls
    int touchCount = GetTouchPointCount();
    for (int i = 0; i < touchCount; i++)
    {
        Vector2 pos = GetTouchPosition(i);
        if (pos.x < GetScreenWidth() / 2) {
            // Steering
            if (pos.x < GetScreenWidth() / 4) steerInput = 1.0f; // Left
            else steerInput = -1.0f; // Right (Logic inverted? Let's check visual)
            // Wait, standard is Left <-> Right.
            // 0..1/4 width -> Left. 1/4..1/2 -> Right.
        } else {
            // Gas/Brake
            if (pos.y < GetScreenHeight() / 2) gasInput = 1.0f; // Top right -> Gas
            else gasInput = -1.0f; // Bottom right -> Brake
        }
    }
#else
    // Keyboard for testing
    if (IsKeyDown(KEY_W)) gasInput = 1.0f;
    if (IsKeyDown(KEY_S)) gasInput = -1.0f;
    if (IsKeyDown(KEY_A)) steerInput = 1.0f; // Left
    if (IsKeyDown(KEY_D)) steerInput = -1.0f; // Right
#endif

    // --- Physics ---

    // Rotation
    if (g_player.onGround) {
         g_player.yaw += steerInput * g_player.turnSpeed * (g_player.speed / g_player.maxSpeed) * dt;
    }

    // Acceleration
    if (gasInput > 0.1f) {
        g_player.speed += g_player.acceleration * dt;
    } else if (gasInput < -0.1f) {
        g_player.speed -= g_player.acceleration * dt;
    } else {
        // Natural friction
        if (g_player.speed > 0) g_player.speed -= g_player.friction * dt;
        if (g_player.speed < 0) g_player.speed += g_player.friction * dt;
        if (fabs(g_player.speed) < 1.0f) g_player.speed = 0.0f;
    }

    // Clamp speed
    if (g_player.speed > g_player.maxSpeed) g_player.speed = g_player.maxSpeed;
    if (g_player.speed < -g_player.maxSpeed/2) g_player.speed = -g_player.maxSpeed/2;

    // Calculate Velocity Vector based on Yaw
    // X = sin(yaw), Z = cos(yaw)
    g_player.velocity.x = sinf(g_player.yaw) * g_player.speed;
    g_player.velocity.z = cosf(g_player.yaw) * g_player.speed;

    // Gravity
    g_player.velocity.y += GRAVITY * dt;

    // Proposed new position
    Vector3 nextPos = Vector3Add(g_player.position, Vector3Scale(g_player.velocity, dt));

    // --- Collision Detection ---
    g_player.onGround = false;

    // Player Bounds at next position
    BoundingBox playerBox = {
        Vector3Subtract(nextPos, Vector3Scale(CAR_SIZE, 0.5f)),
        Vector3Add(nextPos, Vector3Scale(CAR_SIZE, 0.5f))
    };

    // Check Ground Collision
    float groundHeight = -999.0f;
    bool collided = false;

    for (const auto& seg : g_track) {
        if (CheckCollisionBox(playerBox, seg.bounds)) {
            // Simple: Snap to top of box if we are falling onto it
            // We only care about floor collision for now
            if (g_player.position.y >= seg.bounds.max.y - 0.5f) { // If we were above it
                 if (seg.bounds.max.y > groundHeight) {
                     groundHeight = seg.bounds.max.y;
                     collided = true;
                 }
            }
        }
    }

    if (collided && nextPos.y <= groundHeight + (CAR_SIZE.y/2.0f)) {
        nextPos.y = groundHeight + (CAR_SIZE.y/2.0f);
        g_player.velocity.y = 0;
        g_player.onGround = true;
    }

    // Kill plane
    if (nextPos.y < -50.0f) {
        // Reset
        g_player.position = (Vector3){ 0.0f, 2.0f, 0.0f };
        g_player.velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
        g_player.speed = 0.0f;
        g_player.yaw = 0.0f;
        nextPos = g_player.position;
    }

    g_player.position = nextPos;

    // --- Camera Follow ---
    Vector3 idealCamPos;
    idealCamPos.x = g_player.position.x - (sinf(g_player.yaw) * 15.0f);
    idealCamPos.z = g_player.position.z - (cosf(g_player.yaw) * 15.0f);
    idealCamPos.y = g_player.position.y + 7.0f;

    // Smooth camera (Lerp)
    g_camera.position.x += (idealCamPos.x - g_camera.position.x) * 5.0f * dt;
    g_camera.position.y += (idealCamPos.y - g_camera.position.y) * 5.0f * dt;
    g_camera.position.z += (idealCamPos.z - g_camera.position.z) * 5.0f * dt;
    g_camera.target = g_player.position;
}

void DrawGame() {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    BeginMode3D(g_camera);

        // Draw Track
        for (const auto& seg : g_track) {
            DrawCube(seg.position, seg.size.x, seg.size.y, seg.size.z, seg.color);
            DrawCubeWires(seg.position, seg.size.x, seg.size.y, seg.size.z, DARKGRAY);
        }

        // Draw Car
        DrawCube(g_player.position, CAR_SIZE.x, CAR_SIZE.y, CAR_SIZE.z, BLUE);
        DrawCubeWires(g_player.position, CAR_SIZE.x, CAR_SIZE.y, CAR_SIZE.z, BLACK);

        // Front indicators
        Vector3 frontPos = Vector3Add(g_player.position, Vector3Scale((Vector3){sinf(g_player.yaw), 0, cosf(g_player.yaw)}, 1.5f));
        DrawCube(frontPos, 0.5f, 0.5f, 0.5f, YELLOW);

    EndMode3D();

    DrawFPS(10, 10);
    DrawText("CONTROLS: LEFT SCREEN = STEER | RIGHT SCREEN = GAS/BRAKE", 10, 40, 20, BLACK);
    DrawText(TextFormat("SPEED: %.1f", g_player.speed), 10, 70, 20, BLACK);

    EndDrawing();
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Trackmania Clone");
    InitGame();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        UpdateGame(dt);
        DrawGame();
    }

    CloseWindow();
    return 0;
}
