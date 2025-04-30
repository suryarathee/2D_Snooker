#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>

// Constants
const float PI = 3.14159f;
const int NUM_BALLS = 16; // 15 colored balls + 1 cue ball

// Ball structure
struct Ball {
    float x, y;           // Position
    float vx, vy;         // Velocity
    float radius;         // Radius
    bool active;          // Is ball active (not pocketed)
    int color[3];         // RGB color
	int player;         // Player number (1 or 2)
};

// Pocket structure
struct Pocket {
    float x, y;           // Position
    float radius;         // Radius
};

// Game state
struct GameState {
    // Table properties
    float tableWidth = 1.6f;
    float tableHeight = 0.8f;
    float cushionThickness = 0.05f;

    // Balls
    std::vector<Ball> balls;
    int activeBalls = NUM_BALLS;

    // Pockets
    std::vector<Pocket> pockets;

    // Cue properties
    float cueAngle = 0.0f;
    float cueLength = 0.5f;
    float cuePower = 0.0f;
    float maxCuePower = 0.05f;  // REDUCED maximum power (was 0.1f)
    bool cueDragging = false;
    bool cueAiming = true;

    // Game state
    bool ballsMoving = false;
    int player1Score = 0;
    int player2Score = 0;
    int shots = 0;
    bool gameOver = false;
	int currentPlayer = 1; // Player 1 starts first
    bool player1Solids = false;
    bool player2Solids = false;
	bool ballTypeAssigned = false; // Flag to check if ball type is assigned
    bool potted = false;
    bool foul = false;
    int winner = -1;
    std::string message = "";

    // Friction coefficient - INCREASED for more friction (was 0.992f)
    float friction = 0.9992f;

    // Minimum velocity threshold - INCREASED to stop balls sooner
    float minVelocity = 0.008f;

} game;

// Initialize ball positions in triangle formation
void initializeBalls() {
    game.balls.clear();

    // Colors for balls (RGB values between 0-255, will be normalized)
    int colors[NUM_BALLS][3] = {
        {255, 255, 255},  // Cue ball (white)
        {255, 255, 0},    // Yellow
        {0, 0, 255},      // Blue
        {255, 0, 0},      // Red
        {128, 0, 128},    // Purple
        {255, 165, 0},    // Orange
        {0, 128, 0},      // Green
        {128, 0, 0},      // Maroon
        {0, 0, 0},        // Black (8-ball)
        {255, 255, 0},    // Yellow striped
        {0, 0, 255},      // Blue striped
        {255, 0, 0},      // Red striped
        {128, 0, 128},    // Purple striped
        {255, 165, 0},    // Orange striped
        {0, 128, 0},      // Green striped
        {128, 0, 0}       // Maroon striped
    };

    float ballRadius = 0.03f;
    float spacing = ballRadius * 2.1f; // Slight gap between balls

    // Create cue ball
    Ball cueBall;
    cueBall.x = -0.4f;
    cueBall.y = 0.0f;
    cueBall.vx = 0.0f;
    cueBall.vy = 0.0f;
    cueBall.radius = ballRadius;
    cueBall.active = true;
    cueBall.color[0] = colors[0][0];
    cueBall.color[1] = colors[0][1];
    cueBall.color[2] = colors[0][2];
    game.balls.push_back(cueBall);

    // Create other balls in triangle formation
    float startX = 0.4f;
    float startY = 0.0f;

    int ballIndex = 1;
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col <= row; col++) {
            if (ballIndex < NUM_BALLS) {
                Ball ball;
                ball.x = startX + row * spacing * 0.866f; // 0.866 = cos(30°)
                ball.y = startY + (col - row / 2.0f) * spacing;
                ball.vx = 0.0f;
                ball.vy = 0.0f;
                ball.radius = ballRadius;
                ball.active = true;
                ball.color[0] = colors[ballIndex][0];
                ball.color[1] = colors[ballIndex][1];
                ball.color[2] = colors[ballIndex][2];
                // Assign player based on ball type (1-7 solids, 9-15 stripes, 8 neutral)
                if (ballIndex >= 1 && ballIndex <= 7) ball.player = -1;  // Solids (player to be determined)
                else if (ballIndex >= 9 && ballIndex <= 15) ball.player = -2;  // Stripes (player to be determined)
                else ball.player = 0;  // 8-ball is neutral
                game.balls.push_back(ball);
                ballIndex++;
            }
        }
    }
}

void drawBackground() {
    // Save the current projection and modelview matrices
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Draw in normalized device coordinates (-1 to 1) which will always
    // cover the entire screen regardless of projection
    glBegin(GL_QUADS);
    glColor3f(0.2f, 0.1f, 0.05f); // Dark wood color
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();

    // Add wood grain pattern
    glColor3f(0.3f, 0.15f, 0.07f);
    for (int i = 0; i < 20; i++) {
        float y = -1.0f + i * 0.1f;
        glBegin(GL_LINES);
        glVertex2f(-1.0f, y);
        glVertex2f(1.0f, y);
        glEnd();
    }

    // Restore the original matrices
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}
// Initialize pockets
void initializePockets() {
    game.pockets.clear();

    float pocketRadius = 0.08f;
    float tableWidth = game.tableWidth;
    float tableHeight = game.tableHeight;

    // Add 6 pockets (4 corners, 2 middle sides)
    Pocket pocket;

    // Top-left pocket
    pocket.x = -tableWidth / 2 - game.cushionThickness / 2;
    pocket.y = tableHeight / 2 + game.cushionThickness / 2;
    pocket.radius = pocketRadius;
    game.pockets.push_back(pocket);

    // Top-middle pocket
    pocket.x = 0;
    pocket.y = tableHeight / 2 + game.cushionThickness / 2;
    game.pockets.push_back(pocket);

    // Top-right pocket
    pocket.x = tableWidth / 2 + game.cushionThickness / 2;
    pocket.y = tableHeight / 2 + game.cushionThickness / 2;
    game.pockets.push_back(pocket);

    // Bottom-right pocket
    pocket.x = tableWidth / 2 + game.cushionThickness / 2;
    pocket.y = -tableHeight / 2 - game.cushionThickness / 2;
    game.pockets.push_back(pocket);

    // Bottom-middle pocket
    pocket.x = 0;
    pocket.y = -tableHeight / 2 - game.cushionThickness / 2;
    game.pockets.push_back(pocket);

    // Bottom-left pocket
    pocket.x = -tableWidth / 2 - game.cushionThickness / 2;
    pocket.y = -tableHeight / 2 - game.cushionThickness / 2;
    game.pockets.push_back(pocket);
}

// Initialize the game
void initializeGame() {
    initializeBalls();
    initializePockets();
    game.player1Score = 0;
    game.player2Score = 0;
    game.shots = 0;
    game.gameOver = false;
    game.ballsMoving = false;
    game.cueAiming = true;
    game.currentPlayer = 1;
    game.player1Solids = false;
    game.player2Solids = false;
    game.ballTypeAssigned = false;
    game.potted = false;
    game.foul = false;
    game.message = "Player 1's turn";
}

void switchPlayer() {
    if (game.currentPlayer == 1) {
        game.currentPlayer = 2;
        game.message = "Player 2's turn";
    }
    else {
        game.currentPlayer = 1;
        game.message = "Player 1's turn";
    }
    game.potted = false;
    game.foul = false;
}
void assignBallTypes(int pottedBallIndex) {
    if (game.ballTypeAssigned) return;

    // Determine if the potted ball is solid or striped
    bool isSolid = (pottedBallIndex >= 1 && pottedBallIndex <= 7);

    if (game.currentPlayer == 1) {
        game.player1Solids = isSolid;
        game.player2Solids = !isSolid;
    }
    else {
        game.player2Solids = isSolid;
        game.player1Solids = !isSolid;
    }

    // Assign balls to players
    for (size_t i = 1; i < game.balls.size(); i++) {
        if (i == 8) continue; // 8-ball is neutral

        if (i >= 1 && i <= 7) { // Solids
            game.balls[i].player = game.player1Solids ? 1 : 2;
        }
        else { // Stripes
            game.balls[i].player = game.player1Solids ? 2 : 1;
        }
    }

    game.ballTypeAssigned = true;

    // Update message to inform players of their ball types
    if (game.player1Solids) {
        game.message = "Player 1: Solids, Player 2: Stripes";
    }
    else {
        game.message = "Player 1: Stripes, Player 2: Solids";
    }
}

bool checkWin(int player) {
    // Check if all of the player's balls are pocketed
    for (size_t i = 1; i < game.balls.size(); i++) {
        if (i == 8) continue; // Skip 8-ball

        if (game.balls[i].player == player && game.balls[i].active) {
            return false; // Player still has active balls
        }
    }

    // Check if 8-ball is pocketed (should be the last ball)
    return !game.balls[8].active;
}

bool checkLoss(int player) {
    // If the player pockets the 8-ball but still has their balls on the table
    if (!game.balls[8].active) {
        for (size_t i = 1; i < game.balls.size(); i++) {
            if (i == 8) continue;

            if (game.balls[i].player == player && game.balls[i].active) {
                return true; // Player pocketed 8-ball too early
            }
        }
    }

    // If the player pockets the cue ball and the 8-ball in the same shot
    if (!game.balls[0].active && !game.balls[8].active) {
        return true;
    }

    return false;
}
// Check if a ball is pocketed
void checkPockets() {
    for (size_t i = 0; i < game.balls.size(); i++) {
        if (!game.balls[i].active) continue;

        for (size_t j = 0; j < game.pockets.size(); j++) {
            float dx = game.balls[i].x - game.pockets[j].x;
            float dy = game.balls[i].y - game.pockets[j].y;
            float distance = sqrt(dx * dx + dy * dy);

            if (distance < game.pockets[j].radius) {
                // Ball is pocketed
                game.balls[i].active = false;
                game.activeBalls--;

                // Special case for cue ball - respawn it
                if (i == 0) {
                    game.foul = true;
                    // Respawn cue ball in a legal position
                    game.balls[0].x = -0.4f;
                    game.balls[0].y = 0.0f;
                    game.balls[0].vx = 0.0f;
                    game.balls[0].vy = 0.0f;
                    game.balls[0].active = true;
                    game.activeBalls++;
                    game.message = "Foul! Scratched the cue ball";
                }
                // Special case for 8-ball (black ball)
                else if (i == 8) {
                    // Check if the player has potted all their assigned balls
                    bool allAssignedBallsPotted = true;
                    for (size_t k = 1; k < game.balls.size(); k++) {
                        if (game.balls[k].player == game.currentPlayer && game.balls[k].active) {
                            allAssignedBallsPotted = false;
                            break;
                        }
                    }

                    if (allAssignedBallsPotted) {
                        // Player wins by potting the 8-ball after potting all their assigned balls
                        game.gameOver = true;
						game.winner = game.currentPlayer;
                        game.message = "Player " + std::to_string(game.currentPlayer) + " wins by potting the black ball!";
                    }
                    else {
                        // Player loses for potting the 8-ball too early
                        game.gameOver = true;
						game.winner = game.currentPlayer == 1 ? 2 : 1; // Opponent wins
                        game.message = "Player " + std::to_string(game.currentPlayer) + " loses! Potted the black ball too early.";
                    }
                }
                // Regular balls
                else {
                    // Assign ball types if not already assigned
                    if (!game.ballTypeAssigned) {
                        assignBallTypes(i);
                    }

                    // Check if the player potted their own ball
                    if (game.balls[i].player == game.currentPlayer) {
                        // Add to score
                        if (game.currentPlayer == 1) {
                            game.player1Score++;
                        }
                        else {
                            game.player2Score++;
                        }
                        game.potted = true;
                        game.message = "Good shot! Go again";
                    }
                    else {
                        // Player potted opponent's ball
                        if (game.currentPlayer == 1) {
                            game.player2Score++;
                        }
                        else {
                            game.player1Score++;
                        }
                        game.message = "Potted opponent's ball";
                    }
                }
            }
        }
    }
}

// Handle ball-ball collisions
void handleBallCollisions() {
    for (size_t i = 0; i < game.balls.size(); i++) {
        if (!game.balls[i].active) continue;

        for (size_t j = i + 1; j < game.balls.size(); j++) {
            if (!game.balls[j].active) continue;

            // Calculate distance between balls
            float dx = game.balls[j].x - game.balls[i].x;
            float dy = game.balls[j].y - game.balls[i].y;
            float distance = sqrt(dx * dx + dy * dy);

            // Check for collision
            if (distance < game.balls[i].radius + game.balls[j].radius) {
                // Normalize the displacement vector
                float nx = dx / distance;
                float ny = dy / distance;

                // Calculate relative velocity
                float dvx = game.balls[j].vx - game.balls[i].vx;
                float dvy = game.balls[j].vy - game.balls[i].vy;

                // Calculate velocity along the normal
                float velAlongNormal = dvx * nx + dvy * ny;

                // Don't resolve if balls are moving away from each other
                if (velAlongNormal > 0) continue;

                // Collision response (elasticity coefficient = 0.8) - REDUCED elasticity (was 0.9f)
                float elasticity = 0.1f;
                float impulse = -(1 + elasticity) * velAlongNormal;

                // Apply impulse to both balls
                game.balls[i].vx -= nx * impulse;
                game.balls[i].vy -= ny * impulse;
                game.balls[j].vx += nx * impulse;
                game.balls[j].vy += ny * impulse;

                // Separate the balls to prevent sticking
                float overlap = (game.balls[i].radius + game.balls[j].radius - distance) / 2.0f;
                game.balls[i].x -= nx * overlap;
                game.balls[i].y -= ny * overlap;
                game.balls[j].x += nx * overlap;
                game.balls[j].y += ny * overlap;
            }
        }
    }
}

// Handle ball-cushion collisions
void handleCushionCollisions() {
    float tableLeft = -game.tableWidth / 2;
    float tableRight = game.tableWidth / 2;
    float tableTop = game.tableHeight / 2;
    float tableBottom = -game.tableHeight / 2;

    for (size_t i = 0; i < game.balls.size(); i++) {
        if (!game.balls[i].active) continue;

        // Left cushion
        if (game.balls[i].x - game.balls[i].radius < tableLeft) {
            game.balls[i].x = tableLeft + game.balls[i].radius;
            game.balls[i].vx = -game.balls[i].vx * 0.8f; // REDUCED elasticity (was 0.9f)
        }

        // Right cushion
        if (game.balls[i].x + game.balls[i].radius > tableRight) {
            game.balls[i].x = tableRight - game.balls[i].radius;
            game.balls[i].vx = -game.balls[i].vx * 0.8f; // REDUCED elasticity
        }

        // Bottom cushion
        if (game.balls[i].y - game.balls[i].radius < tableBottom) {
            game.balls[i].y = tableBottom + game.balls[i].radius;
            game.balls[i].vy = -game.balls[i].vy * 0.8f; // REDUCED elasticity
        }

        // Top cushion
        if (game.balls[i].y + game.balls[i].radius > tableTop) {
            game.balls[i].y = tableTop - game.balls[i].radius;
            game.balls[i].vy = -game.balls[i].vy * 0.8f; // REDUCED elasticity
        }
    }
}

// Check if all balls have stopped moving
bool allBallsStopped() {
    for (size_t i = 0; i < game.balls.size(); i++) {
        if (!game.balls[i].active) continue;

        if (fabs(game.balls[i].vx) > game.minVelocity || fabs(game.balls[i].vy) > game.minVelocity) {
            return false;
        }
    }
    return true;
}

// Handle mouse motion for aiming the cue
void mouseMotion(int x, int y) {
    if (!game.ballsMoving && game.cueAiming) {
        // Convert mouse coordinates to OpenGL coordinates
        float windowWidth = glutGet(GLUT_WINDOW_WIDTH);
        float windowHeight = glutGet(GLUT_WINDOW_HEIGHT);
        float glX = (2.0f * x / windowWidth) - 1.0f;
        float glY = 1.0f - (2.0f * y / windowHeight);

        // Calculate angle between cue ball and mouse position
        float dx = glX - game.balls[0].x;
        float dy = glY - game.balls[0].y;
        game.cueAngle = atan2(dy, dx);

        // Adjust power based on distance from cue ball (if dragging)
        if (game.cueDragging) {
            float distance = sqrt(dx * dx + dy * dy);
            game.cuePower = std::min(distance, game.maxCuePower);
        }
    }
}
void drawTable() {
    // Draw table border (dark brown)
    glColor3f(0.4f, 0.2f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(-game.tableWidth / 2 - game.cushionThickness, -game.tableHeight / 2 - game.cushionThickness);
    glVertex2f(game.tableWidth / 2 + game.cushionThickness, -game.tableHeight / 2 - game.cushionThickness);
    glVertex2f(game.tableWidth / 2 + game.cushionThickness, game.tableHeight / 2 + game.cushionThickness);
    glVertex2f(-game.tableWidth / 2 - game.cushionThickness, game.tableHeight / 2 + game.cushionThickness);
    glEnd();

    // Draw table felt (green gradient)
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.4f, 0.0f); // Dark green
    glVertex2f(-game.tableWidth / 2, -game.tableHeight / 2);
    glVertex2f(game.tableWidth / 2, -game.tableHeight / 2);
    glColor3f(0.0f, 0.6f, 0.0f); // Light green
    glVertex2f(game.tableWidth / 2, game.tableHeight / 2);
    glVertex2f(-game.tableWidth / 2, game.tableHeight / 2);
    glEnd();
}

void drawBall(const Ball& ball) {
    if (!ball.active) return;

    // Draw ball (shaded)
    glBegin(GL_POLYGON);
    for (int j = 0; j < 36; j++) {
        float angle = j * 10 * PI / 180.0f;
        float x = ball.x + ball.radius * cos(angle);
        float y = ball.y + ball.radius * sin(angle);
        glColor3f(ball.color[0] / 255.0f * 0.8f, ball.color[1] / 255.0f * 0.8f, ball.color[2] / 255.0f * 0.8f); // Shaded color
        glVertex2f(x, y);
    }
    glEnd();

    // Draw ball outline
    glColor3f(0.0f, 0.0f, 0.0f); // Black outline
    glBegin(GL_LINE_LOOP);
    for (int j = 0; j < 36; j++) {
        float angle = j * 10 * PI / 180.0f;
        float x = ball.x + ball.radius * cos(angle);
        float y = ball.y + ball.radius * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();
}
// Handle mouse clicks for shooting the cue
void mouseClick(int button, int state, int x, int y) {
    if (game.gameOver || game.ballsMoving) return;

    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            game.cueDragging = true;
        }
        else if (state == GLUT_UP && game.cueDragging) {
            // Shoot the cue ball
            float shotAngle = game.cueAngle + PI; // Reverse the angle

            // REDUCED the velocity factor by 50% to make shots slower
            float velocityFactor = 0.5f;
            game.balls[0].vx = cos(shotAngle) * game.cuePower * velocityFactor;
            game.balls[0].vy = sin(shotAngle) * game.cuePower * velocityFactor;

            game.ballsMoving = true;
            game.cueAiming = false;
            game.cueDragging = false;
            game.shots++;
        }
    }
}

void drawGameOverScreen() {
    // Semi-transparent background
    glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();

    // Game Over text
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(-0.2f, 0.1f);
    std::string gameOverText = "GAME OVER";
    for (char c : gameOverText) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
    }

    // Restart prompt
    glRasterPos2f(-0.3f, -0.1f);
    std::string restartText = "Press 'R' to restart the game";
    for (char c : restartText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }
}
void drawCueStick() {
    if (!game.ballsMoving && game.cueAiming && game.balls[0].active && !game.gameOver) {
        float cueStartX = game.balls[0].x + cos(game.cueAngle + PI) * game.balls[0].radius;
        float cueStartY = game.balls[0].y + sin(game.cueAngle + PI) * game.balls[0].radius;
        float cueEndX = game.balls[0].x + cos(game.cueAngle) * (game.cueLength + game.cuePower * 3.0f);
        float cueEndY = game.balls[0].y + sin(game.cueAngle) * (game.cueLength + game.cuePower * 3.0f);

        // Draw cue stick
        glColor3f(0.8f, 0.6f, 0.4f);
        glLineWidth(4.0f);
        glBegin(GL_LINES);
        glVertex2f(cueStartX, cueStartY);
        glVertex2f(cueEndX, cueEndY);
        glEnd();
        glLineWidth(1.0f);
    }
}
// Display function
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    drawBackground();
    glLoadIdentity();

	drawTable();

    // Draw pockets
    glColor3f(0.0f, 0.0f, 0.0f);
    for (size_t i = 0; i < game.pockets.size(); i++) {
        glBegin(GL_POLYGON);
        for (int j = 0; j < 36; j++) {
            float angle = j * 10 * PI / 180.0f;
            float x = game.pockets[i].x + game.pockets[i].radius * cos(angle);
            float y = game.pockets[i].y + game.pockets[i].radius * sin(angle);
            glVertex2f(x, y);
        }
        glEnd();
    }

    // Draw balls
    for (size_t i = 0; i < game.balls.size(); i++) {
        if (!game.balls[i].active) continue;

        // Draw ball
        glColor3f(game.balls[i].color[0] / 255.0f, game.balls[i].color[1] / 255.0f, game.balls[i].color[2] / 255.0f);
        glBegin(GL_POLYGON);
        for (int j = 0; j < 36; j++) {
            float angle = j * 10 * PI / 180.0f;
            float x = game.balls[i].x + game.balls[i].radius * cos(angle);
            float y = game.balls[i].y + game.balls[i].radius * sin(angle);
            glVertex2f(x, y);
        }
        glEnd();

        // Draw stripes for balls 9-15 (indices 9-15)
        if (i >= 9) {
            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_POLYGON);
            for (int j = 0; j < 36; j++) {
                float angle = j * 10 * PI / 180.0f;
                // Make stripes only cover top half
                if (sin(angle) > 0) {
                    float x = game.balls[i].x + game.balls[i].radius * cos(angle);
                    float y = game.balls[i].y + game.balls[i].radius * sin(angle);
                    glVertex2f(x, y);
                }
                else {
                    float x = game.balls[i].x + game.balls[i].radius * 0.5f * cos(angle);
                    float y = game.balls[i].y + game.balls[i].radius * 0.5f * sin(angle);
                    glVertex2f(x, y);
                }
            }
            glEnd();
        }

        // Draw number on ball 8 (black ball)
        if (i == 8) {
            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_POLYGON);
            for (int j = 0; j < 36; j++) {
                float angle = j * 10 * PI / 180.0f;
                float x = game.balls[i].x + game.balls[i].radius * 0.3f * cos(angle);
                float y = game.balls[i].y + game.balls[i].radius * 0.3f * sin(angle);
                glVertex2f(x, y);
            }
            glEnd();
        }
    }

    // Draw cue stick
    if (!game.ballsMoving && game.cueAiming && game.balls[0].active && !game.gameOver) {
        glColor3f(0.8f, 0.6f, 0.4f);

        // Determine cue color based on current player
        if (game.currentPlayer == 1) {
            glColor3f(0.8f, 0.2f, 0.2f); // Red for player 1
        }
        else {
            glColor3f(0.2f, 0.2f, 0.8f); // Blue for player 2
        }

        float cueStartX = game.balls[0].x + cos(game.cueAngle + PI) * game.balls[0].radius;
        float cueStartY = game.balls[0].y + sin(game.cueAngle + PI) * game.balls[0].radius;
        float cueEndX = game.balls[0].x + cos(game.cueAngle) * (game.cueLength + game.cuePower * 3.0f);
        float cueEndY = game.balls[0].y + sin(game.cueAngle) * (game.cueLength + game.cuePower * 3.0f);

        glLineWidth(4.0f);
        glBegin(GL_LINES);
        glVertex2f(cueStartX, cueStartY);
        glVertex2f(cueEndX, cueEndY);
        glEnd();
        glLineWidth(1.0f);

        // Draw cue tip
        glColor3f(0.9f, 0.9f, 0.9f); // Light gray tip
        glBegin(GL_POLYGON);
        for (int j = 0; j < 36; j++) {
            float angle = j * 10 * PI / 180.0f;
            float x = cueEndX + game.balls[0].radius * 0.3f * cos(angle);
            float y = cueEndY + game.balls[0].radius * 0.3f * sin(angle);
            glVertex2f(x, y);
        }
        glEnd();
    }
    // Display score and shots
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(-0.95f, 0.92f);

    std::stringstream scoreStream;
    scoreStream << "Player 1: " << game.player1Score << " | Player 2: " << game.player2Score << " | Shots: " << game.shots;
    std::string scoreText = scoreStream.str();

    for (char c : scoreText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    glRasterPos2f(-0.95f, 0.85f);
	if (game.currentPlayer == 1) {
		std::string playerText = "Player 1's Turn";
		for (char c : playerText) {
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
		}
	}
	else {
		std::string playerText = "Player 2's Turn";
		for (char c : playerText) {
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
		}

	}
    

    // Display controls
    glRasterPos2f(-0.95f, -0.92f);
    std::string controlsText = "Controls: Click and drag to aim and shoot";
    for (char c : controlsText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }

    // Display game over message
    if (game.gameOver) {
        glColor3f(1.0f, 0.0f, 0.0f);
        glRasterPos2f(-0.25f, 0.0f);
        std::string gameOverText = "GAME OVER";
        for (char c : gameOverText) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
        }

        glRasterPos2f(-0.3f, -0.1f);
        std::stringstream finalScoreStream;
		finalScoreStream << "Final Score: Player 1: " << game.player1Score << " | Player 2: " << game.player2Score;
		
			finalScoreStream << game.winner<<" Wins!";
        std::string finalScoreText = finalScoreStream.str();
        for (char c : finalScoreText) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }

        glRasterPos2f(-0.4f, -0.2f);
        std::string restartText = "Press 'R' to restart the game";
        for (char c : restartText) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }
    }

    glutSwapBuffers();
}

// Update function for game logic
void update(int value) {
    if (!game.gameOver) {
        if (game.ballsMoving) {
            // Update ball positions based on velocity
            for (size_t i = 0; i < game.balls.size(); i++) {
                if (!game.balls[i].active) continue;

                game.balls[i].x += game.balls[i].vx;
                game.balls[i].y += game.balls[i].vy;

                // Apply friction - higher value = more friction = faster slowdown
                game.balls[i].vx *= game.friction;
                game.balls[i].vy *= game.friction;

                // Stop ball if velocity is very low
                if (fabs(game.balls[i].vx) < game.minVelocity) game.balls[i].vx = 0.0f;
                if (fabs(game.balls[i].vy) < game.minVelocity) game.balls[i].vy = 0.0f;
            }

            // Handle collisions
            handleBallCollisions();
            handleCushionCollisions();

            // Check for pocketed balls
            checkPockets();

            // Check if all balls have stopped
            if (allBallsStopped()) {
                game.ballsMoving = false;
                game.cueAiming = true;
                game.cuePower = 0.0f;
                if (!game.potted || game.foul) {
                    switchPlayer();
                }
                game.potted = false;
            }
            
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0); // ~60 FPS
}

// Keyboard function for key presses
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 'r':
    case 'R':
        // Reset the game
        initializeGame();
        break;
    case 27: // ESC key
        exit(0);
        break;
    }
}

// Reshape function
void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Maintain aspect ratio
    if (width <= height) {
        glOrtho(-1.0, 1.0, -1.0 * height / width, 1.0 * height / width, -1.0, 1.0);
    }
    else {
        glOrtho(-1.0 * width / height, 1.0 * width / height, -1.0, 1.0, -1.0, 1.0);
    }

    glMatrixMode(GL_MODELVIEW);
}

// Main function
int main(int argc, char** argv) {
    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("2D Pool Game");
    // Register callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMotionFunc(mouseMotion);
    glutPassiveMotionFunc(mouseMotion);
    glutMouseFunc(mouseClick);
    glutTimerFunc(16, update, 0);

    // Set clear color
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Initialize game
    initializeGame();

    // Start the main loop
    glutMainLoop();

    return 0;
}
