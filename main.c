#include "raylib.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

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
#define MAX_POWERUPS 1
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
int powerUpsGeradosTotal = 0;

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
    PowerUp* powerUpAtual = *headPowerUp;
    PowerUp* anterior = NULL;

    while (powerUpAtual != NULL) {
        if (powerUpAtual->ativo && calcularDistancia(player.posicao, powerUpAtual->posicao) < PLAYER_SIZE) {
            // Remove o power-up da lista ao coletá-lo
            if (anterior == NULL) {
                popPowerUp(headPowerUp);  // Remove o primeiro elemento
            } else {
                anterior->prox = powerUpAtual->prox;
                free(powerUpAtual);
            }

            powerUpsCapturados++;

            // Ativa imunidade e define tempo restante
            jogadorImune = true;
            tempoImunidadeRestante = TEMPO_IMUNIDADE;

            return true;  // Power-up coletado com sucesso
        }
        anterior = powerUpAtual;
        powerUpAtual = powerUpAtual->prox;
    }
    return false;
}

void gerarPowerup(PowerUp** headPowerUp, Barreira* barreiras, int numBarreiras) {
    int intervaloPowerUp = 1000;

    // Verifica se já atingimos o limite de dois power-ups totais
    if (powerUpsGeradosTotal >= 2) return;

    // Contar o número de power-ups ativos atualmente
    int powerUpsAtivos = 0;
    PowerUp* temp = *headPowerUp;
    while (temp != NULL) {
        if (temp->ativo) powerUpsAtivos++;
        temp = temp->prox;
    }

    // Gera um novo power-up se o intervalo foi alcançado e se o total não excede o limite
    if (powerUpsAtivos < MAX_POWERUPS && contadorTempoPowerUp >= intervaloPowerUp) {
        Vector2 posicao;

        // Gera uma posição fora das barreiras
        do {
            posicao = (Vector2){ GetRandomValue(50, SCREEN_WIDTH - 50), GetRandomValue(50, SCREEN_HEIGHT - 50) };
        } while (posicaoEmBarreira(posicao, barreiras, numBarreiras));

        pushPowerUp(headPowerUp, posicao); // Adiciona o novo power-up
        contadorTempoPowerUp = 0;          // Reseta o contador para o próximo intervalo
        powerUpsGeradosTotal++;            // Incrementa o total de power-ups gerados
    }

    contadorTempoPowerUp++;
}

void desenharPowerUps(PowerUp* head, Texture2D powerUpTexture) {
    PowerUp* temp = head;
    while (temp != NULL) {
        if (temp->ativo) {
            printf("Desenhando PowerUp na posição: (%f, %f)\n", temp->posicao.x, temp->posicao.y);
            DrawTextureEx(powerUpTexture, (Vector2){temp->posicao.x - (powerUpTexture.width * 0.5f), temp->posicao.y - (powerUpTexture.height * 0.5f)}, 0.0f, 2.0f, WHITE);
        }
        temp = temp->prox;
    }
    //UnloadTexture(powerUp);
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

void reiniciarJogo(Player* player, Tubarao** head, Lixo** lixo, bool* gameOver, bool* vitoria, bool* telaInicial, bool* aumentoVelocidade, Barreira* barreiras, int numBarreiras, char* nomeJogador, int* caractereAtual, bool* adicionouAoRanking, PowerUp **headPowerUp) {
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
    powerUpsGeradosTotal = 0;
    powerUpsCapturados = 0;
    contadorTempoPowerUp = 0;

    jogadorImune = false;
    tempoImunidadeRestante = 0;

    // Limpar o nome do jogador e permitir reentrada
    nomeJogador[0] = '\0';
    *caractereAtual = 0;

    while (*headPowerUp != NULL) {
        popPowerUp(headPowerUp);  // Remove cada power-up até a lista estar vazia
    }
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

    *numBarreiras = 7;
    barreiras[0].rect = (Rectangle){ 92, 146, 30, 300 }; // Barreira vertical
    barreiras[1].rect = (Rectangle){ 220, 299, 131, 30 }; // Barreira horizontal

    barreiras[2].rect = (Rectangle){ 298, 446, 203, 30 }; // Barreira vertical
    barreiras[3].rect = (Rectangle){ 238, 69, 252, 30 }; // Barreira horizontal
    barreiras[4].rect = (Rectangle){ 560, 195, 30, 200 }; // Vertical direita

    barreiras[5].rect = (Rectangle){ 608, 32, 131, 30 };  // Vertical pequena no canto superior direito

    // Barreira na parte superior direita
    barreiras[6].rect = (Rectangle){ 637, 530, 131, 30 }; 
}

void desenharBarreiras(Barreira* barreiras, int numBarreiras, Texture2D barreira1, Texture2D barreira2) {
    float escalaVisual = 4.0; // Ajuste este valor para o tamanho visual desejado das barreiras

    for (int i = 0; i < numBarreiras; i++) {
        Texture2D texturaAtual = (i == 0 || i == 4) ? barreira2 : barreira1; // Usa 'barreira2' para as barreiras[0] e barreiras[4]

        Rectangle destRect = {
            barreiras[i].rect.x - (barreiras[i].rect.width * (escalaVisual - 1)) / 2,
            barreiras[i].rect.y - (barreiras[i].rect.height * (escalaVisual - 1)) / 2,
            barreiras[i].rect.width * escalaVisual,
            barreiras[i].rect.height * escalaVisual
        };

        Rectangle sourceRect = { 0, 0, texturaAtual.width, texturaAtual.height };

        // Desenha a textura com a escala visual maior
        DrawTexturePro(texturaAtual, sourceRect, destRect, (Vector2){0, 0}, 0.0f, WHITE);
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
    if (arquivo == NULL) {
    printf("Erro ao abrir o arquivo ranking.txt para escrita!\n");
    return;
}
    if (arquivo != NULL) {
        for (int i = 0; i < totalJogadores; i++) {
            fprintf(arquivo, "Nome: %s | Tempo: %.2f segundos\n", jogadores[i].nome, jogadores[i].tempo);
             printf("Escrevendo jogador: %s com tempo %.2f\n", jogadores[i].nome, jogadores[i].tempo);  // Linha de depuração
        }
        fclose(arquivo);
    }

}

bool nomeExiste(const char *nome) {
    FILE *arquivo = fopen("ranking.txt", "r");
    if (!arquivo) return false;

    char nomeComEspaco[21];
    snprintf(nomeComEspaco,sizeof(nomeComEspaco),"%s ",nome);

    char linha[256];
    char nomeArquivo[20];
    float tempoArquivo;

    while (fgets(linha, sizeof(linha), arquivo)) {
        // Verifica se a linha contém o nome do jogador
        sscanf(linha, "Nome: %[^|] | Tempo: %f segundos", nomeArquivo, &tempoArquivo);
        if (strcmp(nomeArquivo, nomeComEspaco) == 0) {
                fclose(arquivo); // Fecha o arquivo antes de retornar
                return true;
            }
        }
    fclose(arquivo);
    return false;
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "S.O.S. Praia Limpa!");

    Texture2D background = LoadTexture("assets/background/Captura de tela 2024-11-05 092632.png");
    Texture2D fundoJogo = LoadTexture("assets/background/background.png");
    Texture2D powerUpTexture = LoadTexture("assets/powerUp/pocaoVermelha.png");
    Texture2D barreira1 = LoadTexture("assets/barreiras/barreira1.png");
    Texture2D barreira2 = LoadTexture("assets/barreiras/barreira2.png");


    Texture2D banhistaUp = LoadTexture("assets/characters/banhistaCima.png");
    Texture2D banhistaDown = LoadTexture("assets/characters/banhistaBaixo.png");
    Texture2D banhistaRight = LoadTexture("assets/characters/banhistaDir.png");
    Texture2D banhistaLeft = LoadTexture("assets/characters/banhistaEsq.png");

    Texture2D lixo1 = LoadTexture("assets/trash/lixo1.png");
    Texture2D lixo2 = LoadTexture("assets/trash/lixo2.png");
    Texture2D lixo3 = LoadTexture("assets/trash/lixo3.png");
    Texture2D lixo4 = LoadTexture("assets/trash/lixo4.png");
    Texture2D lixo5 = LoadTexture("assets/trash/lixo5.png");

    Texture2D sharkLeft, sharkRight, sharkUp, sharkDown;

    sharkLeft = LoadTexture("assets/characters/tubaraoEsq .png");
    sharkRight = LoadTexture("assets/characters/tubaraoDir.png");
    sharkUp = LoadTexture("assets/characters/tubaraoCima.png");
    sharkDown = LoadTexture("assets/characters/tubaraoBaixo.png");


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
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);

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

        Color corBotao = (Color){28, 194, 215, 200}; // Azul claro
        Color corTexto = (Color){255, 255, 255, 255}; // Branco
        Color corBorda = (Color){28, 194, 215, 255};

        const int botaoLargura = 200;
        const int botaoAltura = 50;
        const int espacoEntreBotoes = 20; // Espaçamento entre os botões

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

        Vector2 mousePos = GetMousePosition();
    
        if (CheckCollisionPointRec(mousePos, botaoIniciar)) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                telaInicial = false;
                telaNome = true;
                telaRanking = false;
                telaInstrucoes = false;
            }
        }
        if (CheckCollisionPointRec(mousePos, botaoRanking)) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                telaInicial = false;
                telaRanking = true;
                telaInstrucoes = false;
                telaNome = false;
            }
        }

        if (CheckCollisionPointRec(mousePos, botaoInstrucoes)) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                telaInicial = false;
                telaInstrucoes = true;
                telaRanking = false;
                telaNome = false;
            }
        }

        if (CheckCollisionPointRec(mousePos, botaoSair)) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                CloseWindow();
            }
        }
        
    }

    else if (telaNome) {

        const int botaoLargura = 200;
        const int botaoAltura = 50;

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

        Vector2 mousePos = GetMousePosition();
        if (CheckCollisionPointRec(mousePos, botaoNome)) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
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
        if (CheckCollisionPointRec(GetMousePosition(), botaoVoltar)){
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                reiniciarJogo(&player, &head, &lixo, &gameOver, &vitoria, &telaInicial, &aumentoVelocidade, barreiras, numBarreiras, nomeJogador, &caractereAtual, &adicionouAoRanking, &headPowerUp);
                telaInicial = true;
            }    
        }

    }

    else if (telaInstrucoes) {
        float startY = 120;  // Subindo a posição inicial para o texto das instruções
        float lineSpacing = 50;  // Diminuindo o espaçamento entre instruções
        float lineSpacingTopic = 40;
        float fontSize = 20;  // Reduzindo o tamanho da fonte das instruções

        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){173, 216, 230, 255});
        
        // Ajustando o título mais acima e reduzindo o tamanho da fonte
        Vector2 titleSize = MeasureTextEx(myFont, "Como Jogar", 40, 1); // Reduzindo o tamanho para 60 e espaçamento para 1
                DrawTextEx(myFont, "Como Jogar",(Vector2){(SCREEN_WIDTH - titleSize.x) / 2+2, 70}, 40, 1, (Color){0, 0, 0, 145}); // Texto azul, fonte menor
                DrawTextEx(myFont, "Como Jogar",(Vector2){(SCREEN_WIDTH - titleSize.x) / 2, 68}, 40, 1, (Color){70, 130, 180, 200}); // Texto azul, fonte menor

        // Calcula a largura do texto
        Vector2 line1Size = MeasureTextEx(myFont2, "1 - Use as setas para mover o personagem", fontSize, 1);
        Vector2 line2Size = MeasureTextEx(myFont2, "entre as barreiras de corais", fontSize, 1);
        Vector2 line3Size = MeasureTextEx(myFont2, "2 - Seu objetivo sera coletar todos os lixos do mar", fontSize, 1);
        Vector2 line4Size = MeasureTextEx(myFont2, "sem ser capturado pelos tubaroes", fontSize, 1);
        Vector2 line5Size = MeasureTextEx(myFont2, "3 - Powerups podem aparecer a qualquer momento,", fontSize, 1);
        Vector2 line6Size = MeasureTextEx(myFont2, "se coleta-lo voce ganha imortalidade por 5 segundos", fontSize, 1);
        Vector2 line7Size = MeasureTextEx(myFont2, "4 - Se voce coletar todos os lixos, vencera", fontSize, 1);
        Vector2 line8Size = MeasureTextEx(myFont2, "e podera consultar o ranking dos jogadores", fontSize, 1);

        // Primeira instrução
        DrawTextEx(myFont2, "1 - Use as setas para mover o personagem", 
                (Vector2){(SCREEN_WIDTH - line1Size.x) / 2, startY}, fontSize, 1, (Color){0, 0, 0, 255});
        DrawTextEx(myFont2, "entre as barreiras de corais", 
                (Vector2){(SCREEN_WIDTH - line2Size.x) / 2, startY + lineSpacingTopic}, fontSize, 1, (Color){0, 0, 0, 255});

        // Segunda instrução
        DrawTextEx(myFont2, "2 - Seu objetivo sera coletar todos os lixos do mar", 
                (Vector2){(SCREEN_WIDTH - line3Size.x) / 2, startY + 2 * lineSpacing}, fontSize, 1, (Color){0, 0, 0, 255});
        DrawTextEx(myFont2, "sem ser capturado pelos tubaroes", 
                (Vector2){(SCREEN_WIDTH - line4Size.x) / 2, startY + 2 * lineSpacing + lineSpacingTopic}, fontSize, 1, (Color){0, 0, 0, 255});

        // Terceira instrução
        DrawTextEx(myFont2, "3 - Powerups podem aparecer a qualquer momento,", 
                (Vector2){(SCREEN_WIDTH - line5Size.x) / 2, startY + 4 * lineSpacing}, fontSize, 1, (Color){0, 0, 0, 255});
        DrawTextEx(myFont2, "se coleta-lo voce ganha imortalidade por 5 segundos", 
                (Vector2){(SCREEN_WIDTH - line6Size.x) / 2, startY + 4 * lineSpacing + lineSpacingTopic}, fontSize, 1, (Color){0, 0, 0, 255});

        // Quarta instrução
        DrawTextEx(myFont2, "4 - Se voce coletar todos os lixos, vencera", 
                (Vector2){(SCREEN_WIDTH - line7Size.x) / 2, startY + 6 * lineSpacing}, fontSize, 1, (Color){0, 0, 0, 255});
        DrawTextEx(myFont2, "e podera consultar o ranking dos jogadores", 
                (Vector2){(SCREEN_WIDTH - line8Size.x) / 2, startY + 6 * lineSpacing + lineSpacingTopic}, fontSize, 1, (Color){0, 0, 0, 255});
                    // teste teste teste
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
        if (CheckCollisionPointRec(GetMousePosition(), botaoVoltar)){
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                reiniciarJogo(&player, &head, &lixo, &gameOver, &vitoria, &telaInicial, &aumentoVelocidade, barreiras, numBarreiras, nomeJogador, &caractereAtual, &adicionouAoRanking, &headPowerUp);
                telaInicial = true;
            }    
        }
    }

    else if (!gameOver && !vitoria) {

        DrawTexture(fundoJogo,0,0,WHITE);

        tempoDecorrido = GetTime() - tempoInicial;
        DrawTextEx(myFont2, TextFormat("Tempo decorrido: %.2f segundos", tempoDecorrido), (Vector2){10, 10}, 20, 0, BLACK);

        moverJogador(&player);

        verificarColetaItens(player, lixo);
        if (todosItensColetados(lixo)) {
            vitoria = true;
        }

        if (coletarPowerUp(player, &headPowerUp, &head)) {
            mostrarMensagem = true;
            tempoMensagem = 300; 
        }
        gerarPowerup(&headPowerUp, barreiras, numBarreiras);
        atualizarImunidade();
        desenharPowerUps(headPowerUp, powerUpTexture);

        if (mostrarMensagem) {
            DrawTextEx(myFont2, "Power-up capturado! Imunidade ativada por 5 segundos!", (Vector2){90, 50}, 20, 0, RED);
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

    // Defina uma variável para armazenar a última direção
        int lastDirection; // Comece com uma direção padrão, como direita

    // Dentro do loop principal
        if (IsKeyDown(KEY_RIGHT)) {
            lastDirection = KEY_RIGHT; // Atualize a direção
            DrawTextureEx(banhistaRight, (Vector2){player.posicao.x - PLAYER_SIZE, player.posicao.y - PLAYER_SIZE}, 0.0f, 0.15f, WHITE);
        } else if (IsKeyDown(KEY_LEFT)) {
            lastDirection = KEY_LEFT;
            DrawTextureEx(banhistaLeft, (Vector2){player.posicao.x - PLAYER_SIZE, player.posicao.y - PLAYER_SIZE}, 0.0f, 0.15f, WHITE);
        } else if (IsKeyDown(KEY_UP)) {
            lastDirection = KEY_UP;
            DrawTextureEx(banhistaUp, (Vector2){player.posicao.x - PLAYER_SIZE, player.posicao.y - PLAYER_SIZE}, 0.0f, 0.15f, WHITE);
        } else if (IsKeyDown(KEY_DOWN)) {
            lastDirection = KEY_DOWN;
            DrawTextureEx(banhistaDown, (Vector2){player.posicao.x - PLAYER_SIZE, player.posicao.y - PLAYER_SIZE}, 0.0f, 0.15f, WHITE);
        } else {
            // Nenhuma tecla pressionada, mantenha a última direção
            switch (lastDirection) {
                case KEY_RIGHT:
                    DrawTextureEx(banhistaRight, (Vector2){player.posicao.x - PLAYER_SIZE, player.posicao.y - PLAYER_SIZE}, 0.0f, 0.15f, WHITE);
                    break;
                case KEY_LEFT:
                    DrawTextureEx(banhistaLeft, (Vector2){player.posicao.x - PLAYER_SIZE, player.posicao.y - PLAYER_SIZE}, 0.0f, 0.15f, WHITE);
                    break;
                case KEY_UP:
                    DrawTextureEx(banhistaUp, (Vector2){player.posicao.x - PLAYER_SIZE, player.posicao.y - PLAYER_SIZE}, 0.0f, 0.15f, WHITE);
                    break;
                case KEY_DOWN:
                    DrawTextureEx(banhistaDown, (Vector2){player.posicao.x - PLAYER_SIZE, player.posicao.y - PLAYER_SIZE}, 0.0f, 0.15f, WHITE);
                    break;
            }
        }

        temp = head;

        while (temp != NULL) {
                // Seleciona a textura do tubarão com base na direção
            if (fabs(temp->direcao.x) > fabs(temp->direcao.y)) {
                if (temp->direcao.x > 0) {
                    DrawTextureEx(sharkRight, (Vector2){temp->posicao.x - SHARK_SIZE, temp->posicao.y - SHARK_SIZE}, 0.0f, 0.4f, WHITE);
                } else {
                    DrawTextureEx(sharkLeft, (Vector2){temp->posicao.x - SHARK_SIZE, temp->posicao.y - SHARK_SIZE}, 0.0f, 0.4f, WHITE);
                }
            } else {
                if (temp->direcao.y > 0) {
                    DrawTextureEx(sharkDown, (Vector2){temp->posicao.x - SHARK_SIZE, temp->posicao.y - SHARK_SIZE}, 0.0f, 0.4f, WHITE);
                } else {
                    DrawTextureEx(sharkUp, (Vector2){temp->posicao.x - SHARK_SIZE, temp->posicao.y - SHARK_SIZE}, 0.0f, 0.4f, WHITE);
                }
            }

            temp = temp->prox;
        }

        desenharBarreiras(barreiras, numBarreiras, barreira1,barreira2);

        Lixo* aux = lixo;
        int i = 1;
        while (aux != NULL) {
            if (!aux->coletado) {
                switch (i) {
                    case 1:
                        DrawTextureEx(lixo1, aux->posicao, 0.0f, 0.4f, WHITE);
                        break;
                    case 2:
                        DrawTextureEx(lixo2, aux->posicao, 0.0f, 0.4f, WHITE);
                        break;
                    case 3:
                        DrawTextureEx(lixo3, aux->posicao, 0.0f, 0.4f, WHITE);
                        break;
                    case 4:
                        DrawTextureEx(lixo4, aux->posicao, 0.0f, 0.4f, WHITE);
                        break;
                    case 5:
                        DrawTextureEx(lixo5, aux->posicao, 0.0f, 0.4f, WHITE);
                        break;
                }
            }
            aux = aux->prox;
            i++;
        }


    } else {

        tempoInicial = GetTime();
        Color backgroundColor = (Color){173, 216, 230, 255};
        const char *mensagem = vitoria ? "Parabens! Todos os lixos do mar foram coletados!" : "Voce foi pego pelos tubaroes :(. Fim de jogo!";
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
            float fontSize = 20;      
            float spacing = 2;       

            Vector2 textSize2 = MeasureTextEx(myFont2, texto, fontSize, spacing);

            Vector2 textPos2 = (Vector2){
                botaoVerRanking.x + (botaoVerRanking.width - textSize2.x) / 2,
                botaoVerRanking.y + (botaoVerRanking.height - textSize2.y) / 2
            };

            DrawTextEx(myFont2, texto, textPos2, fontSize, spacing, WHITE);

            Vector2 mousePos = GetMousePosition();

            if (CheckCollisionPointRec(mousePos, botaoVerRanking)){
                SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    adicionarRanking(nomeJogador, tempoDecorrido);
                    adicionouAoRanking = true;
                    telaInicial = false;
                    telaRanking = true;
                    telaInstrucoes = false;
                }    
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
            if (CheckCollisionPointRec(mousePos, botaoReiniciar)){
                SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    reiniciarJogo(&player, &head, &lixo, &gameOver, &vitoria, &telaInicial, &aumentoVelocidade, barreiras, numBarreiras,nomeJogador, &caractereAtual, &adicionouAoRanking, &headPowerUp);
                    telaNome = true;
                    telaInicial = false;
                }    
            }
            else if (CheckCollisionPointRec(mousePos, botaoVoltar)){
                SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    reiniciarJogo(&player, &head, &lixo, &gameOver, &vitoria, &telaInicial, &aumentoVelocidade, barreiras, numBarreiras, nomeJogador, &caractereAtual, &adicionouAoRanking, &headPowerUp);
                    telaInicial = true;
                }    
            }
        }
    }

        EndDrawing();
    }
    
    UnloadTexture(background);
    UnloadTexture(fundoJogo);
    UnloadTexture(sharkDown);
    UnloadTexture(sharkLeft);    
    UnloadTexture(sharkRight);
    UnloadTexture(sharkUp);
    UnloadTexture(banhistaDown);
    UnloadTexture(banhistaLeft);    
    UnloadTexture(banhistaRight);
    UnloadTexture(banhistaUp);
    UnloadTexture(powerUpTexture);
    UnloadTexture(lixo1);
    UnloadTexture(lixo2);
    UnloadTexture(lixo3);    
    UnloadTexture(lixo4);
    UnloadTexture(lixo5);
    UnloadTexture(barreira1);
    UnloadTexture(barreira2);
    
    CloseWindow();
    return 0;
}
