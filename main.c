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

int tempoDesdeUltimoTubarao = 0;      // Tempo em quadros desde o último tubarão adicionado
const int intervaloTubarao = 2100; 

typedef struct {
    Vector2 posicao;
    float speed;
} Player;

typedef struct Tubarao {
    Vector2 posicao;
    Vector2 direcao;
    float speed;
    struct Tubarao* prox;
    struct Tubarao* prev;
} Tubarao;

typedef struct {
    Vector2 posicao;
    float raio;
} ZonaSegura;

typedef struct {
    Rectangle rect;
} Barreira;

float calcularDistancia(Vector2 a, Vector2 b) {
    return sqrtf((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}

void addTubarao(Tubarao** head, float x, float y, float speed) {
    Tubarao* novo = (Tubarao*)malloc(sizeof(Tubarao));
    novo->posicao = (Vector2){ x, y };
    novo->speed = speed;
    novo->prox = NULL;
    novo->prev = NULL;

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
        } while (calcularDistancia((Vector2){x, y}, player.posicao) < MIN_DISTANCE);

        addTubarao(head, x, y, SHARK_SPEED);
        
        // Define uma direção inicial aleatória para o tubarão
        (*head)->direcao = (Vector2){ ((float)GetRandomValue(-10, 10)) / 10.0f, ((float)GetRandomValue(-10, 10)) / 10.0f };
    }
}

void reiniciarJogo(Player* player, Tubarao** head, bool* gameOver, bool* vitoria, bool* telaInicial, bool* aumentoVelocidade) {
    player->posicao = (Vector2){100, 100};
    *gameOver = false;
    *vitoria = false;
    *aumentoVelocidade = false;
    *telaInicial = true;

    Tubarao* aux = *head;
    while (aux != NULL) {
        Tubarao* prox = aux->prox;
        free(aux);
        aux = prox;
    }
    *head = NULL;
    inicializarTubarao(head, 5, *player);
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

                    separationForce.x += repel.x / (distance * 0.3f);
                    separationForce.y += repel.y / (distance * 0.3f);
                }
            }
            other = other->prox;
        }
        
        temp->posicao.x += separationForce.x * 1.5f;
        temp->posicao.y += separationForce.y * 1.5f;
        
        temp = temp->prox;
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

void moverTubaraoAleatoriamente(Tubarao* head) {
    Tubarao* temp = head;

    while (temp != NULL) {
        // Altera a direção aleatoriamente a cada 60 quadros (aproximadamente 1 segundo)
        if (GetRandomValue(0, 59) == 0) {
            temp->direcao.x = ((float)GetRandomValue(-10, 10)) / 10.0f;
            temp->direcao.y = ((float)GetRandomValue(-10, 10)) / 10.0f;

            // Normaliza a nova direção
            float magnitude = sqrtf(temp->direcao.x * temp->direcao.x + temp->direcao.y * temp->direcao.y);
            temp->direcao.x /= magnitude;
            temp->direcao.y /= magnitude;
        }

        // Atualiza a posição do tubarão com a direção atual
        temp->posicao.x += temp->direcao.x * temp->speed;
        temp->posicao.y += temp->direcao.y * temp->speed;

        // Limita o movimento do tubarão às bordas da tela
        if (temp->posicao.x < 0) temp->posicao.x = 0;
        if (temp->posicao.x > SCREEN_WIDTH) temp->posicao.x = SCREEN_WIDTH;
        if (temp->posicao.y < 0) temp->posicao.y = 0;
        if (temp->posicao.y > SCREEN_HEIGHT) temp->posicao.y = SCREEN_HEIGHT;

        temp = temp->prox;
    }

    // Aplica a força de separação para evitar que os tubarões fiquem muito próximos uns dos outros
    forcaSeparacaoTubaroes(head);
}



// Função para verificar colisões entre o jogador e barreiras
bool verificaColisaoBarreira(Player player, Barreira barreira) {
    return CheckCollisionCircleRec(player.posicao, PLAYER_SIZE, barreira.rect);
}

void inicializarBarreiras(Barreira* barreiras, int* numBarreiras) {
    *numBarreiras = 4;
    barreiras[0].rect = (Rectangle){ 200, 150, 10, 300 }; // Barreira vertical
    barreiras[1].rect = (Rectangle){ 400, 100, 300, 10 }; // Barreira horizontal
    barreiras[2].rect = (Rectangle){ 600, 300, 10, 200 }; // Barreira vertical
    barreiras[3].rect = (Rectangle){ 300, 400, 200, 10 }; // Barreira horizontal
}

void desenharBarreiras(Barreira* barreiras, int numBarreiras) {
    for (int i = 0; i < numBarreiras; i++) {
        DrawRectangleRec(barreiras[i].rect, DARKGRAY);
    }
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Fuga dos Tubarões");

    Player player = { .posicao = {100, 100}, .speed = PLAYER_SPEED };
    Tubarao* head = NULL;
    ZonaSegura zonaSegura = { .posicao = {SCREEN_WIDTH - SAFE_ZONE_SIZE, SCREEN_HEIGHT - SAFE_ZONE_SIZE}, .raio = SAFE_ZONE_SIZE };

    inicializarTubarao(&head, 5, player);

    Barreira barreiras[10];
    int numBarreiras = 0;
    inicializarBarreiras(barreiras, &numBarreiras);

    bool gameOver = false;
    bool vitoria = false;
    bool aumentoVelocidade = false;
    bool telaInicial = true;

    SetTargetFPS(60);

        while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (telaInicial) {
            DrawText("Fuga dos Tubarões", SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 4, 30, DARKBLUE);

            // Botão Iniciar
            Rectangle botaoIniciar = {SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 - 20, 100, 40};
            DrawRectangleRec(botaoIniciar, BLUE);
            DrawText("Iniciar", botaoIniciar.x + 10, botaoIniciar.y + 10, 20, WHITE);

            // Botão Sair
            Rectangle botaoSair = {SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 + 50, 100, 40};
            DrawRectangleRec(botaoSair, RED);
            DrawText("Sair", botaoSair.x + 20, botaoSair.y + 10, 20, WHITE);

            Vector2 mousePos = GetMousePosition();
            if (CheckCollisionPointRec(mousePos, botaoIniciar) || CheckCollisionPointRec(mousePos, botaoSair)) {
                SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            } else {
                SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            }

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (CheckCollisionPointRec(mousePos, botaoIniciar)) {
                    telaInicial = false;
                } else if (CheckCollisionPointRec(mousePos, botaoSair)) {
                    CloseWindow();
                    return 0;
                }
            }

        } else if (!gameOver && !vitoria) {
            if (IsKeyDown(KEY_RIGHT)) player.posicao.x += player.speed;
            if (IsKeyDown(KEY_LEFT)) player.posicao.x -= player.speed;
            if (IsKeyDown(KEY_UP)) player.posicao.y -= player.speed;
            if (IsKeyDown(KEY_DOWN)) player.posicao.y += player.speed;

            // Verifica colisão com as barreiras
            for (int i = 0; i < numBarreiras; i++) {
                if (verificaColisaoBarreira(player, barreiras[i])) {
                    // Se houver colisão, impede o movimento do jogador
                    player.posicao.x -= (IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT)) * player.speed;
                    player.posicao.y -= (IsKeyDown(KEY_DOWN) - IsKeyDown(KEY_UP)) * player.speed;
                    break;
                }
            }

            float distanciaZonaSegura = calcularDistancia(player.posicao, zonaSegura.posicao);
            if (distanciaZonaSegura < SAFE_ZONE_THRESHOLD && !aumentoVelocidade) {
                velocidadeAleatoriaTubarao(head, 1.0f);
                aumentoVelocidade = true;
            }

            // Incrementa o tempo desde o último tubarão adicionado
            tempoDesdeUltimoTubarao++;

            // Adiciona um novo tubarão a cada 35 segundos (2100 quadros)
            if (tempoDesdeUltimoTubarao >= intervaloTubarao) {
                float x, y;
                do {
                    x = GetRandomValue(0, SCREEN_WIDTH);
                    y = GetRandomValue(0, SCREEN_HEIGHT);
                } while (calcularDistancia((Vector2){x, y}, player.posicao) < MIN_DISTANCE);

                // Adiciona o novo tubarão e define uma direção inicial
                addTubarao(&head, x, y, SHARK_SPEED);
                head->direcao = (Vector2){ ((float)GetRandomValue(-10, 10)) / 10.0f, ((float)GetRandomValue(-10, 10)) / 10.0f };

                // Reinicia o contador
                tempoDesdeUltimoTubarao = 0;
            }

            moverTubaraoAleatoriamente(head);

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

            // Desenha o jogador, tubarões e a área segura
            DrawCircleV(player.posicao, PLAYER_SIZE, BLUE);
            temp = head;
            while (temp != NULL) {
                DrawCircleV(temp->posicao, SHARK_SIZE, RED);
                temp = temp->prox;
            }

            DrawCircleV(zonaSegura.posicao, zonaSegura.raio, GREEN);
            DrawText("Área Segura", zonaSegura.posicao.x - 20, zonaSegura.posicao.y, 10, DARKGREEN);

            // Desenha as barreiras
            desenharBarreiras(barreiras, numBarreiras);

        } else {
            DrawText(gameOver ? "Você foi pego pelos tubarões! Fim de jogo!" : "Parabéns! Você chegou à área segura!", 150, SCREEN_HEIGHT / 2 - 20, 20, RED);
            
            // Botão de Reiniciar
            Rectangle botaoReiniciar = {SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 + 40, 100, 40};
            DrawRectangleRec(botaoReiniciar, GREEN);
            DrawText("Reiniciar", botaoReiniciar.x + 10, botaoReiniciar.y + 10, 20, WHITE);

            Vector2 mousePos = GetMousePosition();
            if (CheckCollisionPointRec(mousePos, botaoReiniciar) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                reiniciarJogo(&player, &head, &gameOver, &vitoria, &telaInicial, &aumentoVelocidade);
            }
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
