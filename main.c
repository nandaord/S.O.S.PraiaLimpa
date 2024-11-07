#include "raylib.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>


#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define PLAYER_SIZE 20
#define SHARK_SIZE 30
#define SAFE_ZONE_SIZE 50
#define PLAYER_SPEED 3.0f
#define SHARK_SPEED 2.0f
#define COLLISION_RADIUS 30
#define MIN_DISTANCE 150
#define SAFE_ZONE_THRESHOLD 100  
#define SEPARATION_DISTANCE 80
#define NUM_ITEMS 5
#define MAX_POWERUPS 2
#define TEMPO_IMUNIDADE 1000
#define DISTANCIA_MINIMA_BARRREIRA 30 

int tempoDesdeUltimoTubarao = 0;   
const int intervaloTubarao = 2100;
int powerUpsCapturados = 0; 
int powerUpsGerados = 0;
int contadorTempoPowerUp = 0;
bool jogadorImune = false;
int tempoImunidadeRestante = 0;   
float tempoInicial = 0.0f;
float tempoDecorrido = 0.0f;


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
    Rectangle rect;
} Barreira;

typedef struct Lixo {
    Vector2 posicao;
    bool coletado;
    struct Lixo* prox;
} Lixo;

typedef struct PowerUp {
    Vector2 posicao;
    bool ativo;
    struct PowerUp* prox;
} PowerUp;

typedef struct{
    char nome[20];
    float tempo;
}Jogador;

float calcularDistancia(Vector2 a, Vector2 b) {
    return sqrtf((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}
void pushPowerUp(PowerUp** head, Vector2 posicao) {
    PowerUp* novo = (PowerUp*)malloc(sizeof(PowerUp));
    novo->posicao = posicao;
    novo->ativo = true;
    novo->prox = *head;
    *head = novo;
}

void popPowerUp(PowerUp** head) {
    if (*head != NULL) {
        PowerUp* temp = *head;
        *head = (*head)->prox;
        free(temp);
    }
}
bool posicaoEmBarreira(Vector2 posicao, Barreira* barreiras, int numBarreiras) {
    for (int i = 0; i < numBarreiras; i++) {
            // Verifica se está dentro da barreira
            if (CheckCollisionPointRec(posicao, barreiras[i].rect)) {
                return true;
            }
            // Verifica a distância mínima da barreira
            float distanciaX = fabs(posicao.x - (barreiras[i].rect.x + barreiras[i].rect.width / 2));
            float distanciaY = fabs(posicao.y - (barreiras[i].rect.y + barreiras[i].rect.height / 2));
            if (distanciaX < DISTANCIA_MINIMA_BARRREIRA && distanciaY < DISTANCIA_MINIMA_BARRREIRA) {
                return true;
            }
        }
        return false;
}

bool coletarPowerUp(Player player, PowerUp** headPowerUp, Tubarao** tubaroes) {
  if (*headPowerUp != NULL) {
        PowerUp* powerUpAtual = *headPowerUp;
        if (powerUpAtual->ativo && calcularDistancia(player.posicao, powerUpAtual->posicao) < PLAYER_SIZE) {
            popPowerUp(headPowerUp);  // Remove o power-up da pilha
            powerUpsCapturados++;

            // Ativa imunidade e define tempo restante
            jogadorImune = true;
            tempoImunidadeRestante = TEMPO_IMUNIDADE;

            return true;  // Power-up coletado com sucesso
        }
    }
    return false;
}

void gerarPowerUpAleatorio(PowerUp** headPowerUp, Barreira* barreiras, int numBarreiras) {
    int intervaloPowerUp = 1000;

    if (powerUpsGerados < MAX_POWERUPS  && contadorTempoPowerUp >= intervaloPowerUp) {  // Probabilidade baixa de aparecer
        Vector2 posicao;
         // Tenta gerar uma posição fora das barreiras
        do {
            posicao = (Vector2){ GetRandomValue(50, SCREEN_WIDTH - 50), GetRandomValue(50, SCREEN_HEIGHT - 50) };
        } while (posicaoEmBarreira(posicao, barreiras, numBarreiras));
        
        pushPowerUp(headPowerUp, posicao);
        powerUpsGerados++;
        contadorTempoPowerUp = 0;
    }

    contadorTempoPowerUp++;
}

void desenharPowerUps(PowerUp* head) {
    PowerUp* temp = head;
    while (temp != NULL) {
        if (temp->ativo) {
            DrawCircleV(temp->posicao, 10, YELLOW);  // Desenha o power-up em amarelo
        }
        temp = temp->prox;
    }
}

void atualizarImunidade() {
    if (jogadorImune) {
        tempoImunidadeRestante--;
        if (tempoImunidadeRestante <= 0) {
            jogadorImune = false;
        }
    }
}


void ordenarLixosPorPosicaoY(Lixo** head) {
    Lixo* atual = *head;
    Lixo* prox;

    while (atual != NULL) {
        prox = atual->prox;
        int j = 0;

        while (prox != NULL && prox->posicao.y < atual->posicao.y && j > 0) {
            Vector2 aux = atual->posicao;
            atual->posicao = prox->posicao;
            prox->posicao = aux;
            prox = atual->prox;
            j--;
        }
        atual = atual->prox;
    }
}

// Função para inicializar itens em posições aleatórias e adicionar na lista encadeada
void inicializarItens(Lixo** head, Barreira* barreiras, int numBarreiras) {
     for (int i = 0; i < NUM_ITEMS; i++) {
        Lixo* novoLixo = (Lixo*)malloc(sizeof(Lixo));
        do {
            // Gera uma posição aleatória
            novoLixo->posicao = (Vector2){GetRandomValue(50, SCREEN_WIDTH - 50), GetRandomValue(50, SCREEN_HEIGHT - 50)};
        } while (posicaoEmBarreira(novoLixo->posicao, barreiras, numBarreiras)); // Repete enquanto a posição estiver em uma barreira
        
        novoLixo->coletado = false;
        novoLixo->prox = *head;
        *head = novoLixo;
    }
}

void inicializarItensOrdenados(Lixo** head, Barreira* barreiras, int numBarreiras) {
    inicializarItens(head, barreiras, numBarreiras);
    ordenarLixosPorPosicaoY(head);
}

// Função para verificar se o jogador colidiu com algum item e marcar como coletado
void verificarColetaItens(Player player, Lixo* head) {
    Lixo* temp = head;
    while (temp != NULL) {
        if (!temp->coletado && calcularDistancia(player.posicao, temp->posicao) < PLAYER_SIZE) {
            temp->coletado = true;
        }
        temp = temp->prox;
    }
}

// Função para verificar se todos os itens foram coletados
bool todosItensColetados(Lixo* head) {
    Lixo* temp = head;
    while (temp != NULL) {
        if (!temp->coletado) {
            return false;
        }
        temp = temp->prox;
    }
    return true;
}

// Função para liberar a memória alocada para os itens ao reiniciar o jogo
void liberarItens(Lixo** head) {
    Lixo* temp = *head;
    while (temp != NULL) {
        Lixo* prox = temp->prox;
        free(temp);
        temp = prox;
    }
    *head = NULL;
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

void reiniciarJogo(Player* player, Tubarao** head, Lixo** lixo, bool* gameOver, bool* vitoria, bool* telaInicial, bool* aumentoVelocidade, Barreira* barreiras, int numBarreiras, char* nomeJogador, int* caractereAtual, bool* adicionouAoRanking) {
    // Reiniciar posição do jogador
    player->posicao = (Vector2){100, 100};
    *gameOver = false;
    *vitoria = false;
    *aumentoVelocidade = false;
    *telaInicial = true;
    *adicionouAoRanking = false;  // Permite salvar a pontuação apenas uma vez

    // Resetar o tempo do jogo
    tempoInicial = GetTime();
    tempoDecorrido = 0.0f;

    // Resetar os power-ups
    tempoDesdeUltimoTubarao = 0;
    powerUpsGerados = 0;
    powerUpsCapturados = 0;
    contadorTempoPowerUp = 0;

    // Limpar o nome do jogador e permitir reentrada
    nomeJogador[0] = '\0';
    *caractereAtual = 0;

    // Liberar tubarões e itens coletados
    Tubarao* aux = *head;
    while (aux != NULL) {
        Tubarao* prox = aux->prox;
        free(aux);
        aux = prox;
    }
    *head = NULL;

    inicializarTubarao(head, 5, *player);
    liberarItens(lixo);
    inicializarItens(lixo, barreiras, numBarreiras);
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

        // Limita o movimento do tubarão às bordas da tela
        if (temp->posicao.x < 0) temp->posicao.x = 0;
        if (temp->posicao.x > SCREEN_WIDTH - SHARK_SIZE) temp->posicao.x = SCREEN_WIDTH - SHARK_SIZE;
        if (temp->posicao.y < 0) temp->posicao.y = 0;
        if (temp->posicao.y > SCREEN_HEIGHT - SHARK_SIZE) temp->posicao.y = SCREEN_HEIGHT - SHARK_SIZE;
        
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

void moverJogador(Player* player) {
    if (IsKeyDown(KEY_RIGHT)) player->posicao.x += player->speed;
    if (IsKeyDown(KEY_LEFT)) player->posicao.x -= player->speed;
    if (IsKeyDown(KEY_UP)) player->posicao.y -= player->speed;
    if (IsKeyDown(KEY_DOWN)) player->posicao.y += player->speed;

    // Limita o jogador dentro dos limites da tela
    if (player->posicao.x < 0) player->posicao.x = 0;
    if (player->posicao.x > SCREEN_WIDTH - PLAYER_SIZE) player->posicao.x = SCREEN_WIDTH - PLAYER_SIZE;
    if (player->posicao.y < 0) player->posicao.y = 0;
    if (player->posicao.y > SCREEN_HEIGHT - PLAYER_SIZE) player->posicao.y = SCREEN_HEIGHT - PLAYER_SIZE;
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
        if (temp->posicao.x > SCREEN_WIDTH - SHARK_SIZE) temp->posicao.x = SCREEN_WIDTH - SHARK_SIZE;
        if (temp->posicao.y < 0) temp->posicao.y = 0;
        if (temp->posicao.y > SCREEN_HEIGHT - SHARK_SIZE) temp->posicao.y = SCREEN_HEIGHT - SHARK_SIZE;

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
    *numBarreiras = 8;
    barreiras[0].rect = (Rectangle){ 200, 150, 10, 300 }; // Barreira vertical
    barreiras[1].rect = (Rectangle){ 400, 100, 300, 10 }; // Barreira horizontal
    barreiras[2].rect = (Rectangle){ 600, 300, 10, 200 }; // Barreira vertical
    barreiras[3].rect = (Rectangle){ 300, 400, 200, 10 }; // Barreira horizontal
    barreiras[4].rect = (Rectangle){ 600, 200, 10, 300 };   // Vertical direita

    // Pequenas barreiras para formar obstáculos no meio
    barreiras[5].rect = (Rectangle){ 300, 300, 60, 10 };    // Horizontal pequena no meio
    barreiras[6].rect = (Rectangle){ 500, 150, 10, 120 };   // Vertical pequena no canto superior direito

    // Barreira na parte superior direita
    barreiras[7].rect = (Rectangle){ 650, 50, 100, 10 };  
}


void desenharBarreiras(Barreira* barreiras, int numBarreiras) {
    for (int i = 0; i < numBarreiras; i++) {
        DrawRectangleRec(barreiras[i].rect, DARKGRAY);
    }
}

void adicionarRanking(const char* nomeJogador, float tempoDecorrido){
    Jogador jogadores[100];
    int totalJogadores = 0;

    FILE *arquivo = fopen64("ranking.txt" , "r");
    if(arquivo != NULL){
        while (fscanf(arquivo, "Nome: %s | Tempo: %f segundos\n", jogadores[totalJogadores].nome, &jogadores[totalJogadores].tempo) != EOF) {
        totalJogadores++;
    }
    fclose(arquivo);
}

    strcpy(jogadores[totalJogadores].nome, nomeJogador);
    jogadores[totalJogadores].tempo = tempoDecorrido;
    totalJogadores++;

    // Ordenar os jogadores pelo tempo
    for (int i = 0; i < totalJogadores - 1; i++) {
        for (int j = 0; j < totalJogadores - 1 - i; j++) {
            if (jogadores[j].tempo > jogadores[j + 1].tempo) {
                // Trocar jogadores
                Jogador temp = jogadores[j];
                jogadores[j] = jogadores[j + 1];
                jogadores[j + 1] = temp;
            }
        }
    }

    arquivo = fopen("ranking.txt", "w");
    if (arquivo != NULL) {
        for (int i = 0; i < totalJogadores; i++) {
            fprintf(arquivo, "Nome: %s | Tempo: %.2f segundos\n", jogadores[i].nome, jogadores[i].tempo);
        }
        fclose(arquivo);
    }

}

bool nomeExiste(const char *nome) {
    FILE *arquivo = fopen("ranking.txt", "r");
    if (!arquivo) return false;

    char linha[256];
    while (fgets(linha, sizeof(linha), arquivo)) {
        // Verifica se a linha contém o nome do jogador
        if (strstr(linha, nome) != NULL) {
            fclose(arquivo);
            return true; // Nome já existe
        }
    }
    fclose(arquivo);
    return false; // Nome não existe
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "S.O.S. Praia Limpa!");

    Texture2D background = LoadTexture("assets/background/Captura de tela 2024-11-05 092632.png");
    Texture2D fundoJogo = LoadTexture("assets/background/Captura de tela 2024-11-05 170948.png");

    Color transparente = (Color){255, 255, 255, 128};
    
    Player player = { .posicao = {100, 100}, .speed = PLAYER_SPEED };
    Tubarao* head = NULL;
    PowerUp* headPowerUp = NULL;
    Lixo *lixo = NULL;


    inicializarTubarao(&head, 5, player);

    Barreira barreiras[10];
    int numBarreiras = 0;

    inicializarBarreiras(barreiras, &numBarreiras);
    inicializarItensOrdenados(&lixo, barreiras, numBarreiras);


    bool gameOver = false;
    bool vitoria = false;
    bool aumentoVelocidade = false;
    bool telaInicial = true;
    bool telaRanking = false;
    bool telaInstrucoes = false;
    bool mostrarMensagem = false;
    int tempoMensagem = 0;
    bool telaNome = false;
    bool adicionouAoRanking = false;

    char nomeJogador[20] = "";
    int caractereAtual = 0;

    Font myFont = LoadFont("assets/fonts/Story Milky.ttf");
    Font myFont2 = LoadFont("assets/fonts/Nexa-Heavy.ttf");
    SetTargetFPS(60);

        while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (telaInicial) {
     DrawTexturePro(
            background,
            (Rectangle){0, 0, background.width, background.height},  // Região da textura
            (Rectangle){0, 0, GetScreenWidth(), GetScreenHeight()},  // Região de destino na tela
            (Vector2){0, 0},                                         // Origem
            0.0f,                                                    // Rotação
            transparente                                             // Cor com transparência
        );

    

    Vector2 titleSize = MeasureTextEx(myFont, "S.O.S. Praia Limpa!", 70, 2);
    DrawTextEx(myFont, "S.O.S. Praia Limpa!", (Vector2){(SCREEN_WIDTH - titleSize.x) / 2 + 2, 152}, 70, 2, (Color){0, 0, 0, 200}); // Sombra preta
    DrawTextEx(myFont, "S.O.S. Praia Limpa!", (Vector2){(SCREEN_WIDTH - titleSize.x) / 2, 150}, 70, 2, (Color){11, 143, 170,255}); // Texto azul

    // Definindo cores com base na paleta fornecida
    Color corBotao = (Color){28, 194, 215, 200}; // Azul claro
    Color corTexto = (Color){255, 255, 255, 255}; // Branco
    Color corBorda = (Color){28, 194, 215, 255};

        // Definindo a altura e largura dos botões
    // Definindo a altura e largura dos botões
    const int botaoLargura = 200;
    const int botaoAltura = 50;
    const int espacoEntreBotoes = 20; // Espaçamento entre os botões

// Botão Iniciar
Rectangle botaoIniciar = {
    SCREEN_WIDTH / 2 - botaoLargura / 2, // Centraliza horizontalmente
    SCREEN_HEIGHT / 2 - 30, // Mover o botão para cima
    botaoLargura,
    botaoAltura
};
DrawRectangleRounded(botaoIniciar, 0.3f, 16, corBotao);
DrawRectangleRoundedLines(botaoIniciar, 0.3f, 16, 2,  corBorda);
const char *textoBotaoIniciar = "Iniciar";
Vector2 textSizeIniciar = MeasureTextEx(myFont2, textoBotaoIniciar, 30, 2);
DrawTextEx(myFont2, textoBotaoIniciar,
           (Vector2){ botaoIniciar.x + (botaoIniciar.width - textSizeIniciar.x) / 2, botaoIniciar.y + (botaoIniciar.height - textSizeIniciar.y) / 2 },
           30, 2, corTexto);

// Botão Ranking
Rectangle botaoRanking = {
    SCREEN_WIDTH / 2 - botaoLargura / 2, // Centraliza horizontalmente
    botaoIniciar.y + botaoAltura + espacoEntreBotoes, // Abaixo do botão Iniciar
    botaoLargura,
    botaoAltura
};
DrawRectangleRounded(botaoRanking, 0.3f, 16, corBotao);
DrawRectangleRoundedLines(botaoRanking, 0.3f, 16, 2, corBorda);
const char *textoBotaoRanking = "Ranking";
Vector2 textSizeRanking = MeasureTextEx(myFont2, textoBotaoRanking, 30, 2);
DrawTextEx(myFont2, textoBotaoRanking,
           (Vector2){ botaoRanking.x + (botaoRanking.width - textSizeRanking.x) / 2, botaoRanking.y + (botaoRanking.height - textSizeRanking.y) / 2 },
           30, 2, corTexto);

// Botão Instruções
Rectangle botaoInstrucoes = {
    SCREEN_WIDTH / 2 - botaoLargura / 2, // Centraliza horizontalmente
    botaoRanking.y + botaoAltura + espacoEntreBotoes, // Abaixo do botão Ranking
    botaoLargura,
    botaoAltura
};
DrawRectangleRounded(botaoInstrucoes, 0.3f, 16, corBotao);
DrawRectangleRoundedLines(botaoInstrucoes, 0.3f, 16, 2, corBorda);
const char *textoBotaoInstrucoes = "Como Jogar";
Vector2 textSizeInstrucoes = MeasureTextEx(myFont2, textoBotaoInstrucoes, 30, 2);
DrawTextEx(myFont2, textoBotaoInstrucoes,
           (Vector2){ botaoInstrucoes.x + (botaoInstrucoes.width - textSizeInstrucoes.x) / 2, botaoInstrucoes.y + (botaoInstrucoes.height - textSizeInstrucoes.y) / 2 },
           30, 2, corTexto);

// Botão Sair
Rectangle botaoSair = {
    SCREEN_WIDTH / 2 - botaoLargura / 2, // Centraliza horizontalmente
    botaoInstrucoes.y + botaoAltura + espacoEntreBotoes, // Abaixo do botão Instruções
    botaoLargura,
    botaoAltura
};
DrawRectangleRounded(botaoSair, 0.3f, 16, corBotao);
DrawRectangleRoundedLines(botaoSair, 0.3f, 16, 2, corBorda);
const char *textoBotaoSair = "Sair";
Vector2 textSizeSair = MeasureTextEx(myFont2, textoBotaoSair, 30, 2);
DrawTextEx(myFont2, textoBotaoSair,
           (Vector2){ botaoSair.x + (botaoSair.width - textSizeSair.x) / 2, botaoSair.y + (botaoSair.height - textSizeSair.y) / 2 },
           30, 2, corTexto);

    
    // Verificar cliques
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        if (CheckCollisionPointRec(mousePos, botaoIniciar)) {
            telaInicial = false;
            telaNome = true;
        } else if (CheckCollisionPointRec(mousePos, botaoRanking)) {
            telaInicial = false;
            telaRanking = true;
        } else if (CheckCollisionPointRec(mousePos, botaoInstrucoes)) {
            telaInicial = false;
            telaInstrucoes = true;
        }else if (CheckCollisionPointRec(mousePos, botaoSair)) {
            CloseWindow();
        }
    }
}

else if(telaNome) {

    const int botaoLargura = 200;
    const int botaoAltura = 50;

    Color corBotaoNome = (Color){173, 216, 230, 230}; // Azul de mar (Dodger Blue)
    Color corTexto = (Color){255, 255, 255, 255}; // Branco
    Color corBorda = (Color){70, 130, 180, 255}; // Azul escuro (Dark Blue)
    Color corCaixaTexto = (Color){255, 255, 255, 255};

    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){173, 216, 230, 255});

    Vector2 titleSize = MeasureTextEx(myFont, "Insira seu Nome:", 40, 1); // Reduzindo o tamanho para 60 e espaçamento para 1
    
    DrawTextEx(myFont, "Insira seu Nome:",(Vector2){(SCREEN_WIDTH - titleSize.x) / 2 + 2, 102}, 40, 1, (Color){0, 0, 0, 145}); // Texto azul, fonte menor
    DrawTextEx(myFont, "Insira seu Nome:",(Vector2){(SCREEN_WIDTH - titleSize.x) / 2, 100}, 40, 1, (Color){70, 130, 180, 200}); // Texto azul, fonte menor

    const int caixaTextoLargura = 300;
    const int caixaTextoAltura = 40;
    Rectangle caixaTexto = {
        SCREEN_WIDTH / 2 - caixaTextoLargura / 2, 
        200, 
        caixaTextoLargura, 
        caixaTextoAltura
    };

    DrawRectangleRec(caixaTexto,corCaixaTexto);
    DrawRectangleLinesEx(caixaTexto,2,corBorda);

    int key = GetCharPressed();
    while(key > 0){
        if((key>=32) && (key<=125) && (caractereAtual < 19)){
            nomeJogador[caractereAtual] = (char)key;
            caractereAtual++;
            nomeJogador[caractereAtual] = '\0';
        }
        key = GetCharPressed();
    }

    if(IsKeyPressed(KEY_BACKSPACE) && (caractereAtual > 0)){
        caractereAtual--;
        nomeJogador[caractereAtual] = '\0';
    }

    DrawText(nomeJogador,caixaTexto.x + 5, caixaTexto.y + 10, 20, DARKGRAY);

    Rectangle botaoNome = {
    SCREEN_WIDTH / 2 - botaoLargura / 2, 
    270, 
    botaoLargura,
    botaoAltura
    };

DrawRectangleRounded(botaoNome, 0.3f, 10, (Color){80, 155, 157, 100});
DrawRectangleRoundedLines(botaoNome, 0.3f , 16 , 2 , (Color){80, 155, 157, 200});
const char *texto = "Jogar";
float fontSize = 30;      // Tamanho da fonte
float spacing = 2;        // Espaçamento entre letras

Vector2 textSize = MeasureTextEx(myFont2, texto, fontSize, spacing);

Vector2 textPos = (Vector2){
    botaoNome.x + (botaoNome.width - textSize.x) / 2,
    botaoNome.y + (botaoNome.height - textSize.y) / 2
};

DrawTextEx(myFont2, texto, textPos, fontSize, spacing, corTexto);

 static bool exibirMensagemErro = false;

    // Verifica se o botão "Jogar" pode ser clicado
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        if (CheckCollisionPointRec(mousePos, botaoNome)) {
            // Verifica se o nome é válido
            bool nomeValido = (caractereAtual > 0 && !nomeExiste(nomeJogador));
            if (nomeValido) {
                telaNome = false;
                telaInicial = false;
                telaRanking = false;
                exibirMensagemErro = false;  // Limpa a mensagem de erro

                tempoInicial = GetTime();  // Reinicia o cronômetro do jogo
                // Limpa a mensagem de erro se o nome for válido
            } else {
                exibirMensagemErro = true; // Define para exibir mensagem de erro
            }
        } 
    }

    // Exibir mensagem de erro se o nome não for válido
    if (exibirMensagemErro) {
        const char *mensagemErro = "Nome usado ou vazio. Tente outro!";
        float fontSizeErro = 20; // Tamanho da fonte
        float spacingErro = 2;   

        Vector2 textSizeErro = MeasureTextEx(myFont2, mensagemErro, fontSizeErro, spacingErro);

        Vector2 textPosErro = (Vector2){
        SCREEN_WIDTH / 2 - textSizeErro.x / 2,
        330 // Y fixo
        };
        DrawTextEx(myFont2, mensagemErro, textPosErro, fontSizeErro, spacingErro, RED);

}

}

 else if (telaRanking) {

        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){173, 216, 230, 255});
        Vector2 titleSize = MeasureTextEx(myFont, "Ranking - Top 10:", 40, 1); // Reduzindo o tamanho para 60 e espaçamento para 1
        DrawTextEx(myFont, "Ranking - Top 10:",(Vector2){(SCREEN_WIDTH - titleSize.x) / 2+2, 70}, 40, 1, (Color){0, 0, 0, 145}); // Texto azul, fonte menor
        DrawTextEx(myFont, "Ranking - Top 10:",(Vector2){(SCREEN_WIDTH - titleSize.x) / 2, 68}, 40, 1, (Color){70, 130, 180, 200}); // Texto azul, fonte menor

        FILE *arquivo = fopen("ranking.txt", "r");
        char linha[100];
        char nomeJogador[20];
        float tempo;

      if (arquivo != NULL) {
    for (int i = 0; i < 10; i++) {
        if (fgets(linha, sizeof(linha), arquivo) != NULL) {
            // Tente extrair o nome e o tempo da linha
            if (sscanf(linha, "Nome: %s | Tempo: %f segundos", nomeJogador, &tempo) == 2) {
                int yPosition = 130 + i * 40; // Ajustado para começar mais acima
                char rankingText[100];
                sprintf(rankingText, "%d. %s - %.2f segundos", i + 1, nomeJogador, tempo);
                
                Vector2 textSize = MeasureTextEx(myFont2, rankingText, 20, 0); // Medir o texto com a fonte 'myfont2'
                Vector2 position = {(SCREEN_WIDTH - textSize.x) / 2, (float)yPosition}; // Criar um Vector2 para a posição
                
                DrawTextEx(myFont2, rankingText, position, 20, 0, (Color){0, 0, 0, 200}); // Texto preto
            } else {
                break; // Se a linha não estiver no formato correto, saia do loop
            }
        } else {
            break; // Se não houver mais linhas, saia do loop
        }
    }
    fclose(arquivo);
}

Rectangle botaoVoltar = {SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT - 60, 100, 40};

// Desenhar o botão arredondado
DrawRectangleRounded(botaoVoltar, 0.3f, 10, (Color){80, 155, 157, 100});
DrawRectangleRoundedLines(botaoVoltar, 0.3f, 16, 2, (Color){80, 155, 157, 200});

// Texto do botão
const char *textoVoltar = "Voltar";
float fontSizeVoltar = 20; // Tamanho da fonte
float spacingVoltar = 2;    // Espaçamento entre letras

Vector2 textSizeVoltar = MeasureTextEx(myFont2, textoVoltar, fontSizeVoltar, spacingVoltar);

Vector2 textPosVoltar = (Vector2){
    botaoVoltar.x + (botaoVoltar.width - textSizeVoltar.x) / 2,
    botaoVoltar.y + (botaoVoltar.height - textSizeVoltar.y) / 2
};

// Desenhar o texto do botão "Voltar"
DrawTextEx(myFont2, textoVoltar, textPosVoltar, fontSizeVoltar, spacingVoltar, WHITE);

// Verifica se o usuário clicou no botão de Voltar
if (CheckCollisionPointRec(GetMousePosition(), botaoVoltar) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    reiniciarJogo(&player, &head, &lixo, &gameOver, &vitoria, &telaInicial, &aumentoVelocidade, barreiras, numBarreiras, nomeJogador, &caractereAtual, &adicionouAoRanking);
    telaInicial = true;
}

        }

else if (telaInstrucoes) {

        float startY = 150; 
        float lineSpacing = 60;
        float lineSpacingTopic = 50;
        float fontSize = 25;

        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){173, 216, 230, 255});
        Vector2 titleSize = MeasureTextEx(myFont, "Como Jogar:", 50, 1); // Reduzindo o tamanho para 60 e espaçamento para 1
        DrawTextEx(myFont, "Como Jogar:\n\n\n",(Vector2){(SCREEN_WIDTH - titleSize.x) / 2, 100}, 50, 1, (Color){70, 130, 180, 255}); // Texto azul, fonte menor
        DrawTextEx(myFont, "1 - Use as setas para mover o personagem", 
           (Vector2){(SCREEN_WIDTH - titleSize.x) / 5, startY}, fontSize, 1, (Color){70, 130, 180, 255});
        DrawTextEx(myFont, "entre as barreiras de corais", 
           (Vector2){(SCREEN_WIDTH - titleSize.x) / 5, startY + lineSpacingTopic}, fontSize, 1, (Color){70, 130, 180, 255});

// Segunda instrução
        DrawTextEx(myFont, "2 - Seu objetivo é coletar todos os lixos do mar", 
           (Vector2){(SCREEN_WIDTH - titleSize.x) / 5, startY + 2 * lineSpacing}, fontSize, 1, (Color){70, 130, 180, 255});
        DrawTextEx(myFont, "sem ser capturado pelos tubarões", 
           (Vector2){(SCREEN_WIDTH - titleSize.x) / 5, startY + 2 * lineSpacing + lineSpacingTopic}, fontSize, 1, (Color){70, 130, 180, 255});

// Terceira instrução
        DrawTextEx(myFont, "3 - Powerups podem aparecer a qualquer momento,", 
           (Vector2){(SCREEN_WIDTH - titleSize.x) / 5, startY + 4 * lineSpacing}, fontSize, 1, (Color){70, 130, 180, 255});
        DrawTextEx(myFont, "se coletá-lo você ganha imortalidade por 5 segundos", 
           (Vector2){(SCREEN_WIDTH - titleSize.x) / 5, startY + 4 * lineSpacing + lineSpacingTopic}, fontSize, 1, (Color){70, 130, 180, 255});

// Quarta instrução
        DrawTextEx(myFont, "4 - Se você coletar todos os lixos, vencerá", 
           (Vector2){(SCREEN_WIDTH - titleSize.x) / 5, startY + 6 * lineSpacing}, fontSize, 1, (Color){70, 130, 180, 255});
        DrawTextEx(myFont, "e poderá consultar o ranking dos jogadores", 
           (Vector2){(SCREEN_WIDTH - titleSize.x) / 5, startY + 6 * lineSpacing + lineSpacingTopic}, fontSize, 1, (Color){70, 130, 180, 255});
            
            // teste teste teste
            // Botão de Voltar
            Rectangle botaoVoltar = {SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT - 60, 100, 40};
            DrawRectangleRec(botaoVoltar, GRAY);
            DrawText("Voltar", botaoVoltar.x + 10, botaoVoltar.y + 10, 20, WHITE);

            // Verifica se o usuário clicou no botão de Voltar
            if (CheckCollisionPointRec(GetMousePosition(), botaoVoltar) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                telaInstrucoes = false;
                telaInicial = true;
            }
        }

 else if (!gameOver && !vitoria) {

   DrawTexture(fundoJogo,0,0,WHITE);

    tempoDecorrido = GetTime() - tempoInicial;
    DrawText(TextFormat("Tempo decorrido: %.2f segundos", tempoDecorrido), 10, 10, 20, BLACK);

            moverJogador(&player);
            gerarPowerUpAleatorio(&headPowerUp, barreiras, numBarreiras);
            verificarColetaItens(player, lixo);

            if (todosItensColetados(lixo)) {
                vitoria = true;
            }

            if (coletarPowerUp(player, &headPowerUp, &head)) {
                mostrarMensagem = true;
                tempoMensagem = 300;  // Exibe a mensagem por 2 segundos
            }

            atualizarImunidade();
            desenharPowerUps(headPowerUp);

            if (mostrarMensagem) {
                DrawText("Power-up capturado! Imunidade ativada por 5 segundos!", 90, 50, 20, RED);
                tempoMensagem--;
                if (tempoMensagem <= 0) mostrarMensagem = false;
            }

            // Verifica colisão com as barreiras
            for (int i = 0; i < numBarreiras; i++) {
                if (verificaColisaoBarreira(player, barreiras[i])) {
                    // Se houver colisão, impede o movimento do jogador
                    player.posicao.x -= (IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT)) * player.speed;
                    player.posicao.y -= (IsKeyDown(KEY_DOWN) - IsKeyDown(KEY_UP)) * player.speed;
                    break;
                }
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
            float distanciaParaColisao = (PLAYER_SIZE / 2.0f) + (SHARK_SIZE / 2.0f);
            while (temp != NULL) {
                if (!jogadorImune && calcularDistancia(player.posicao, temp->posicao) < distanciaParaColisao) {
                    gameOver = true;
                    break;
                }
                temp = temp->prox;
            }

            DrawCircleV(player.posicao, PLAYER_SIZE, BLUE);
            temp = head;
            while (temp != NULL) {
                DrawCircleV(temp->posicao, SHARK_SIZE, RED);
                temp = temp->prox;
            }

            desenharBarreiras(barreiras, numBarreiras);
            Lixo* aux = lixo;
            while (aux != NULL) {
                if (!aux->coletado) {
                    DrawCircleV(aux->posicao, 10, GREEN);
                }
                aux = aux->prox;
            }

        } else {

            tempoInicial = GetTime();
            Color backgroundColor = (Color){173, 216, 230, 255};
            const char *mensagem = vitoria ? "Todos os lixos do mar foram coletados!" : "Fim de jogo!";
            Vector2 textSize = MeasureTextEx(myFont2, mensagem, 30, 2);
            Vector2 textPos = (Vector2){
                (SCREEN_WIDTH - textSize.x) / 2,
                SCREEN_HEIGHT / 2 - 20
            };

            ClearBackground(backgroundColor);
            DrawTextEx(myFont2, mensagem, textPos, 30, 2, RED);
            
            if (vitoria && !adicionouAoRanking) {

                Rectangle botaoVerRanking = {
                    SCREEN_WIDTH / 2 - 75, 
                    SCREEN_HEIGHT / 2 + 40, 
                    150, 
                    40
                };
        
                const char *mensagem = TextFormat("Tempo final: %.2f segundos", tempoDecorrido);

                Vector2 textSize = MeasureTextEx(myFont2,mensagem,20,2);
                Vector2 textPos = (Vector2){
                    (SCREEN_WIDTH-textSize.x) / 2,
                    SCREEN_HEIGHT / 2 + 10
                };

                DrawTextEx(myFont2,mensagem,textPos,20,2,DARKGRAY);

                DrawRectangleRounded(botaoVerRanking,0.3f,10,(Color){80,155,157,100});
                DrawRectangleRoundedLines(botaoVerRanking, 0.3f , 16 , 2 , (Color){80, 155, 157, 200});

const char *texto = "Ver Ranking";
float fontSize = 20;      // Tamanho da fonte
float spacing = 2;        // Espaçamento entre letras

Vector2 textSize2 = MeasureTextEx(myFont2, texto, fontSize, spacing);

Vector2 textPos2 = (Vector2){
    botaoVerRanking.x + (botaoVerRanking.width - textSize2.x) / 2,
    botaoVerRanking.y + (botaoVerRanking.height - textSize2.y) / 2
};

DrawTextEx(myFont2, texto, textPos2, fontSize, spacing, WHITE);

            Vector2 mousePos = GetMousePosition();

            if (CheckCollisionPointRec(mousePos, botaoVerRanking) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                adicionarRanking(nomeJogador, tempoDecorrido);
                adicionouAoRanking = true;
                telaInicial = false;
                telaRanking = true;
            }
            
    
            } else {

            Rectangle botaoReiniciar = {
                SCREEN_WIDTH / 2-140,
                SCREEN_HEIGHT / 2 + 40,
                100,
                40
            };

            Rectangle botaoVoltar = {
                SCREEN_WIDTH / 2 + 40,
                SCREEN_HEIGHT / 2 + 40,
                100,
                40
            };

            DrawRectangleRounded(botaoReiniciar, 0.3f, 10, (Color){80, 155, 157, 100});
            DrawRectangleRoundedLines(botaoReiniciar, 0.3f, 16, 2, (Color){80, 155, 157, 200});

            DrawRectangleRounded(botaoVoltar, 0.3f, 10, (Color){40, 120, 160, 100});  // Cor mais escura e azulado para o botão
DrawRectangleRoundedLines(botaoVoltar, 0.3f, 16, 2, (Color){40, 120, 160, 200});  // Cor mais escura e azulado para a borda

            const char *textoReiniciar = "Reiniciar";
            const char *textoVoltar = "Voltar";
            float fontSize = 20;      
            float spacing = 2; 

            Vector2 textSizeReiniciar = MeasureTextEx(myFont2, textoReiniciar, fontSize, spacing);
            Vector2 textSizeVoltar = MeasureTextEx(myFont2, textoVoltar, fontSize, spacing);

            Vector2 textPosReiniciar = (Vector2){
                botaoReiniciar.x + (botaoReiniciar.width - textSizeReiniciar.x) / 2,
                botaoReiniciar.y + (botaoReiniciar.height - textSizeReiniciar.y) / 2
            };

            Vector2 textPosVoltar = (Vector2){
                botaoVoltar.x + (botaoVoltar.width - textSizeVoltar.x) / 2,
                botaoVoltar.y + (botaoVoltar.height - textSizeVoltar.y) / 2
            };

            DrawTextEx(myFont2, textoReiniciar, textPosReiniciar, fontSize, spacing, WHITE);
            DrawTextEx(myFont2, textoVoltar, textPosVoltar, fontSize, spacing, WHITE);

            Vector2 mousePos = GetMousePosition();
            if (CheckCollisionPointRec(mousePos, botaoReiniciar) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                reiniciarJogo(&player, &head, &lixo, &gameOver, &vitoria, &telaInicial, &aumentoVelocidade, barreiras, numBarreiras,nomeJogador, &caractereAtual, &adicionouAoRanking);
                telaNome = true;
                telaInicial = false;
            }
            
            else if (CheckCollisionPointRec(mousePos, botaoVoltar) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                reiniciarJogo(&player, &head, &lixo, &gameOver, &vitoria, &telaInicial, &aumentoVelocidade, barreiras, numBarreiras, nomeJogador, &caractereAtual, &adicionouAoRanking);
                telaInicial = true;
            }
        }}

        EndDrawing();
    }
    
    UnloadTexture(background);
    UnloadTexture(fundoJogo);

    CloseWindow();
    return 0;
}
