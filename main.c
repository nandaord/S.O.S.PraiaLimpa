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
    }
}

void reiniciarJogo(Player* player, Tubarao** head, bool* gameOver, bool* vitoria, bool* telaInicial, bool* aumentoVelocidade) {
    // Reinicia a posição e estado do jogador
    player->posicao = (Vector2){100, 100};
    *gameOver = false;
    *vitoria = false;
    *aumentoVelocidade = false;
    *telaInicial = true;

    // Limpa a lista de tubarões e reinicializa
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

    forcaSeparacaoTubaroes(head);
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "S.O.S. Praia Limpa!");

    Player player = { .posicao = {100, 100}, .speed = PLAYER_SPEED };
    Tubarao* head = NULL;
    ZonaSegura zonaSegura = { .posicao = {SCREEN_WIDTH - SAFE_ZONE_SIZE, SCREEN_HEIGHT - SAFE_ZONE_SIZE}, .raio = SAFE_ZONE_SIZE };

    inicializarTubarao(&head, 5, player);

    bool gameOver = false;
    bool vitoria = false;
    bool aumentoVelocidade = false;
    bool telaInicial = true;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (telaInicial) {
    // Desenhar fundo
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){173, 216, 230, 255}); // Fundo azul claro

    Font myFont = LoadFont("C:/jogoAED/story_milky/Story Milky.ttf");
    Vector2 titleSize = MeasureTextEx(myFont, "S.O.S. Praia Limpa!", 80, 2);
    DrawTextEx(myFont, "S.O.S. Praia Limpa!", (Vector2){(SCREEN_WIDTH - titleSize.x) / 2, 150}, 80, 2, (Color){70, 130, 180, 255}); // Texto azul

    // Definindo cores com base na paleta fornecida
Color corBotaoIniciar = (Color){135, 206, 250, 255}; // Azul claro
Color corBotaoRanking = (Color){100, 149, 237, 255}; // Azul intermediário (Cornflower Blue)
Color corBotaoInstrucoes = (Color){70, 130, 180, 255}; // Azul médio (Steel Blue)
Color corBotaoSair = (Color){30, 144, 255, 255}; // Azul de mar (Dodger Blue)
Color corTexto = (Color){255, 255, 255, 255}; // Branco
Color corBorda = (Color){0, 0, 139, 255}; // Azul escuro (Dark Blue)

    // Definindo a altura e largura dos botões
// Definindo a altura e largura dos botões
const int botaoLargura = 200;
const int botaoAltura = 50;
const int espacoEntreBotoes = 20; // Espaçamento entre os botões

// Botão Iniciar
// Botão Iniciar
Rectangle botaoIniciar = {
    SCREEN_WIDTH / 2 - botaoLargura / 2, // Centraliza horizontalmente
    SCREEN_HEIGHT / 2 - 30, // Mover o botão para cima
    botaoLargura,
    botaoAltura
};
DrawRectangleRec(botaoIniciar, corBotaoIniciar);
DrawRectangleLines(botaoIniciar.x, botaoIniciar.y, botaoIniciar.width, botaoIniciar.height, corBorda);
DrawText("Iniciar", botaoIniciar.x + 10, botaoIniciar.y + 10, 20, corTexto);

// Botão Ranking
Rectangle botaoRanking = {
    SCREEN_WIDTH / 2 - botaoLargura / 2, // Centraliza horizontalmente
    botaoIniciar.y + botaoAltura + espacoEntreBotoes, // Abaixo do botão Iniciar
    botaoLargura,
    botaoAltura
};
DrawRectangleRec(botaoRanking, corBotaoRanking);
DrawRectangleLines(botaoRanking.x, botaoRanking.y, botaoRanking.width, botaoRanking.height, corBorda);
DrawText("Ranking", botaoRanking.x + 10, botaoRanking.y + 10, 20, corTexto);

// Botão Instruções
Rectangle botaoInstrucoes = {
    SCREEN_WIDTH / 2 - botaoLargura / 2, // Centraliza horizontalmente
    botaoRanking.y + botaoAltura + espacoEntreBotoes, // Abaixo do botão Ranking
    botaoLargura,
    botaoAltura
};
DrawRectangleRec(botaoInstrucoes, corBotaoRanking);
DrawRectangleLines(botaoInstrucoes.x, botaoInstrucoes.y, botaoInstrucoes.width, botaoInstrucoes.height, corBorda);
DrawText("Instruções", botaoInstrucoes.x + 10, botaoInstrucoes.y + 10, 20, corTexto);

// Botão Sair
Rectangle botaoSair = {
    SCREEN_WIDTH / 2 - botaoLargura / 2, // Centraliza horizontalmente
    botaoInstrucoes.y + botaoAltura + espacoEntreBotoes, // Abaixo do botão Instruções
    botaoLargura,
    botaoAltura
};
DrawRectangleRec(botaoSair, corBotaoSair);
DrawRectangleLines(botaoSair.x, botaoSair.y, botaoSair.width, botaoSair.height, corBorda);
DrawText("Sair", botaoSair.x + 10, botaoSair.y + 10, 20, corTexto);

    
    // Verificar cliques
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        if (CheckCollisionPointRec(mousePos, botaoIniciar)) {
            telaInicial = false;
        } else if (CheckCollisionPointRec(mousePos, botaoRanking)) {
            // Código para abrir o ranking
        } else if (CheckCollisionPointRec(mousePos, botaoSair)) {
            CloseWindow();
        }
    }
}

 else if (!gameOver && !vitoria) {
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

            DrawCircleV(player.posicao, PLAYER_SIZE, BLUE);
            temp = head;
            while (temp != NULL) {
                DrawCircleV(temp->posicao, SHARK_SIZE, RED);
                temp = temp->prox;
            }

            DrawCircleV(zonaSegura.posicao, zonaSegura.raio, GREEN);
            DrawText("Área Segura", zonaSegura.posicao.x - 20, zonaSegura.posicao.y, 10, DARKGREEN);

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

    // Liberação dos recursos
    Tubarao* aux = head;
    while (aux != NULL) {
        Tubarao* prox = aux->prox;
        free(aux);
        aux = prox;
    }

    CloseWindow();
    return 0;
}
