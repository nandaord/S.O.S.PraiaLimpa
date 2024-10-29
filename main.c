#include "raylib.h"
#include <stdlib.h>
#include <math.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define PLAYER_SIZE 20
#define SHARK_SIZE 30
#define SAFE_ZONE_SIZE 50
#define PLAYER_SPEED 3.0f
#define SHARK_SPEED 2.0f
#define COLLISION_RADIUS 20
#define MIN_DISTANCE 150
#define SAFE_ZONE_THRESHOLD 100  
#define SEPARATION_DISTANCE 80   

typedef struct {
    Vector2 posicao;
    float speed;
} Player;

typedef struct Tubarao {
    Vector2 posicao;
    float speed;
    struct Tubarao* prox;
    struct Tubarao* prev;
} Tubarao;

typedef struct {
    Vector2 posicao;
    float raio;
} ZonaSegura;

float calcularDistancia(Vector2 a, Vector2 b) {
    return sqrtf((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}

void forcaSeparacaoTubaroes(Tubarao* head) {
    Tubarao* temp = head;
    while (temp != NULL) {
        Tubarao* other = head;
        Vector2 separationForce = { 0, 0 };
        
        while (other != NULL) {
            if (other != temp) {
                float distance = calcularDistancia(temp->posicao, other->posicao);
                if (distance < SEPARATION_DISTANCE) {
                    Vector2 repel = {
                        temp->posicao.x - other->posicao.x,
                        temp->posicao.y - other->posicao.y
                    };
                    float magnitude = sqrtf(repel.x * repel.x + repel.y * repel.y);
                    repel.x /= magnitude;
                    repel.y /= magnitude;

                    // Aumenta a força de repulsão para evitar sobreposição
                    separationForce.x += repel.x / (distance * 0.3f);
                    separationForce.y += repel.y / (distance * 0.3f);
                }
            }
            other = other->prox;
        }
        
        // Aplica a força de separação mais intensa
        temp->posicao.x += separationForce.x * 1.5f;
        temp->posicao.y += separationForce.y * 1.5f;
        
        temp = temp->prox;
    }
}

Tubarao* criarTubarao(float x, float y, float speed) {
    Tubarao* novo = (Tubarao*)malloc(sizeof(Tubarao));
    novo->posicao = (Vector2){ x, y };
    novo->speed = speed;
    novo->prox = NULL;
    novo->prev = NULL;
    return novo;
}

void addTubarao(Tubarao** head, float x, float y, float speed) {
    Tubarao* novo = criarTubarao(x, y, speed);
    if (*head == NULL) {
        *head = novo;
    } else {
        Tubarao* temp = *head;
        while (temp->prox != NULL) {
            temp = temp->prox;
        }
        temp->prox = novo;
        novo->prev = temp;
    }
}

void inicializarTubarao(Tubarao** head, int numSharks, Player player) {
    for (int i = 0; i < numSharks; i++) {
        float x, y;
        do {
            x = GetRandomValue(0, SCREEN_WIDTH);
            y = GetRandomValue(0, SCREEN_HEIGHT);
        } while (calcularDistancia((Vector2){x, y}, player.posicao) < MIN_DISTANCE);  // Garante distância mínima

        addTubarao(head, x, y, SHARK_SPEED);
    }
}

void velocidadeAleatoriaTubarao(Tubarao* head, float increment) {
    Tubarao* aux = head;
    while (aux != NULL) {
        if (GetRandomValue(0, 1) == 1) {
            aux->speed += increment;
        }
        aux = aux->prox;
    }
}

void moverTubarao(Tubarao* head, Player player) {
    Tubarao* temp = head;

    while (temp != NULL) {
        Vector2 direction = {
            player.posicao.x - temp->posicao.x + ((float)GetRandomValue(-10, 10) / 100.0f),
            player.posicao.y - temp->posicao.y + ((float)GetRandomValue(-10, 10) / 100.0f)
        };
        float magnitude = sqrtf(direction.x * direction.x + direction.y * direction.y);

        direction.x /= magnitude;
        direction.y /= magnitude;

        temp->posicao.x += direction.x * temp->speed;
        temp->posicao.y += direction.y * temp->speed;

        temp = temp->prox;
    }

    forcaSeparacaoTubaroes(head);  // Aplica a força de separação após movimentar os tubarões
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Fuga dos Tubarões");
    SetWindowState(FLAG_WINDOW_RESIZABLE);  // Permite alternar para tela cheia
    ToggleFullscreen(); 

    Player player = { .posicao = {100, 100}, .speed = PLAYER_SPEED };
    Tubarao* head = NULL;
    ZonaSegura zonaSegura = { .posicao = {SCREEN_WIDTH - SAFE_ZONE_SIZE, SCREEN_HEIGHT - SAFE_ZONE_SIZE}, .raio = SAFE_ZONE_SIZE };

    inicializarTubarao(&head, 5, player);

    bool gameOver = false;
    bool vitoria = false;
    bool aumentoVelocidade = false;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (!gameOver && !vitoria) {
            if (IsKeyDown(KEY_RIGHT)) player.posicao.x += player.speed;
            if (IsKeyDown(KEY_LEFT)) player.posicao.x -= player.speed;
            if (IsKeyDown(KEY_UP)) player.posicao.y -= player.speed;
            if (IsKeyDown(KEY_DOWN)) player.posicao.y += player.speed;

            float distanciaZonaSegura = calcularDistancia(player.posicao, zonaSegura.posicao);
            if (distanciaZonaSegura < SAFE_ZONE_THRESHOLD && !aumentoVelocidade) {
                velocidadeAleatoriaTubarao(head, 1.0f);
                aumentoVelocidade = true;
            }

            moverTubarao(head, player);

            Tubarao* temp = head;
            while (temp != NULL) {
                if (calcularDistancia(player.posicao, temp->posicao) < COLLISION_RADIUS) {
                    gameOver = true;
                    break;
                }
                temp = temp->prox;
            }

            if (distanciaZonaSegura < zonaSegura.raio) {
                vitoria = true;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (gameOver) {
            DrawText("Você foi pego pelos tubarões! Fim de jogo!", 150, SCREEN_HEIGHT / 2, 20, RED);
        } else if (vitoria) {
            DrawText("Parabéns! Você chegou à área segura!", 150, SCREEN_HEIGHT / 2, 20, GREEN);
        } else {
            DrawCircleV(player.posicao, PLAYER_SIZE, BLUE);
            Tubarao* temp = head;
            while (temp != NULL) {
                DrawCircleV(temp->posicao, SHARK_SIZE, RED);
                temp = temp->prox;
            }

            DrawCircleV(zonaSegura.posicao, zonaSegura.raio, GREEN);
            DrawText("Área Segura", zonaSegura.posicao.x - 20, zonaSegura.posicao.y, 10, DARKGREEN);
        }

        EndDrawing();
    }

    Tubarao* aux = head;
    while (aux != NULL) {
        Tubarao* prox = aux->prox;
        free(aux);
        aux = prox;
    }

    CloseWindow();
    return 0;
}
