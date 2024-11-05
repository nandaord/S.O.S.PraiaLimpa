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

    Texture2D background = LoadTexture("assets/resources/background.png");

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
    SetTargetFPS(60);

        while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (telaInicial) {
    DrawTexture(background, 0, 0, WHITE);
    

    Vector2 titleSize = MeasureTextEx(myFont, "S.O.S. Praia Limpa!", 70, 2);
    DrawTextEx(myFont, "S.O.S. Praia Limpa!", (Vector2){(SCREEN_WIDTH - titleSize.x) / 2, 150}, 70, 2, (Color){70, 130, 180, 255}); // Texto azul

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
DrawRectangleRec(botaoInstrucoes, corBotaoInstrucoes);
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

    Color corBotaoNome = (Color){30, 144, 255, 255}; // Azul de mar (Dodger Blue)
    Color corTexto = (Color){255, 255, 255, 255}; // Branco
    Color corBorda = (Color){0, 0, 139, 255}; // Azul escuro (Dark Blue)
    Color corCaixaTexto = (Color){255, 255, 255, 255};

    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){173, 216, 230, 255});

    Vector2 titleSize = MeasureTextEx(myFont, "Insira seu Nome:", 40, 1); // Reduzindo o tamanho para 60 e espaçamento para 1
    
    DrawTextEx(myFont, "Insira seu Nome:",(Vector2){(SCREEN_WIDTH - titleSize.x) / 2, 100}, 40, 1, (Color){70, 130, 180, 255}); // Texto azul, fonte menor

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

DrawRectangleRec(botaoNome, corBotaoNome);
DrawRectangleLinesEx(botaoNome, 2, corBorda);
DrawText("Jogar", botaoNome.x + 10, botaoNome.y + 10, 20, corTexto);
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
        DrawText("Nome inválido ou já existente", SCREEN_WIDTH / 2 - 100, 330, 20, RED);
    }
}

 else if (telaRanking) {

        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){173, 216, 230, 255});
        Vector2 titleSize = MeasureTextEx(myFont, "Ranking - Top 10:", 40, 1); // Reduzindo o tamanho para 60 e espaçamento para 1
        DrawTextEx(myFont, "Ranking - Top 10:",(Vector2){(SCREEN_WIDTH - titleSize.x) / 2, 100}, 40, 1, (Color){70, 130, 180, 255}); // Texto azul, fonte menor

        FILE *arquivo = fopen("ranking.txt", "r");
        char linha[100];
        char nomeJogador[20];
        float tempo;

        if (arquivo != NULL) {
                for (int i = 0; i < 10; i++) {
                    if (fgets(linha, sizeof(linha), arquivo) != NULL) {
                        // Tente extrair o nome e o tempo da linha
                        if (sscanf(linha, "Nome: %s | Tempo: %f segundos", nomeJogador, &tempo) == 2) {
                            int yPosition = 150 + i * 40; // Espaçamento de 40 pixels entre as linhas
                            char rankingText[100];
                            sprintf(rankingText, "%dº: %s - %.2f segundos", i + 1, nomeJogador, tempo);
                            DrawText(rankingText, (SCREEN_WIDTH - MeasureText(rankingText, 20)) / 2, yPosition, 20, (Color){0, 0, 0, 255}); // Texto preto
                        } else {
                            break; // Se a linha não estiver no formato correto, saia do loop
                        }
                    } else {
                        break; // Se não houver mais linhas, saia do loop
                    }
                }
                fclose(arquivo);
            }


            // Botão de Voltar
            Rectangle botaoVoltar = {SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT - 60, 100, 40};
            DrawRectangleRec(botaoVoltar, GRAY);
            DrawText("Voltar", botaoVoltar.x + 10, botaoVoltar.y + 10, 20, WHITE);

            // Verifica se o usuário clicou no botão de Voltar
            if (CheckCollisionPointRec(GetMousePosition(), botaoVoltar) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                reiniciarJogo(&player, &head, &lixo, &gameOver, &vitoria, &telaInicial, &aumentoVelocidade, barreiras, numBarreiras, nomeJogador, &caractereAtual, &adicionouAoRanking);
                telaInicial = true;
            }
        }

else if (telaInstrucoes) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){173, 216, 230, 255});
        Vector2 titleSize = MeasureTextEx(myFont, "Como Jogar:", 50, 1); // Reduzindo o tamanho para 60 e espaçamento para 1
        DrawTextEx(myFont, "Como Jogar:",(Vector2){(SCREEN_WIDTH - titleSize.x) / 2, 100}, 50, 1, (Color){70, 130, 180, 255}); // Texto azul, fonte menor

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
            DrawText(vitoria ? ("Parabéns! Você coletou todos os lixos do mar!") : "Você foi pego pelos tubarões! Fim de jogo!", 150, SCREEN_HEIGHT / 2 - 20, 20, RED);
            
            if (vitoria && !adicionouAoRanking) {
        
            DrawText(TextFormat("Tempo final: %.2f segundos", tempoDecorrido), 250, 300, 20, DARKGRAY);

            Rectangle botaoVerRanking = {SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 + 40, 150, 40};
            DrawRectangleRec(botaoVerRanking,BLUE);
            DrawText("Ver Ranking",botaoVerRanking.x + 10,botaoVerRanking.y +10,20,WHITE);

            Vector2 mousePos = GetMousePosition();

            if (CheckCollisionPointRec(mousePos, botaoVerRanking) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                adicionarRanking(nomeJogador, tempoDecorrido);
                adicionouAoRanking = true;
                telaInicial = false;
                telaRanking = true;
            }
            
    
            } else {
            
            Rectangle botaoReiniciar = {SCREEN_WIDTH / 2 - 140, SCREEN_HEIGHT / 2 + 40, 100, 40};
            DrawRectangleRec(botaoReiniciar, GREEN);
            DrawText("Reiniciar", botaoReiniciar.x + 10, botaoReiniciar.y + 10, 20, WHITE);

            // Botão de Voltar para a Tela Inicial, posicionado mais à direita
            Rectangle botaoVoltar = {SCREEN_WIDTH / 2 + 40, SCREEN_HEIGHT / 2 + 40, 100, 40};
            DrawRectangleRec(botaoVoltar, BLUE);
            DrawText("Voltar", botaoVoltar.x + 10, botaoVoltar.y + 10, 20, WHITE);

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
    CloseWindow();
    return 0;
}
