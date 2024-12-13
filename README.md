# 🌊 S.O.S Praia Limpa!

## 🏖️ Sobre o jogo
O **S.O.S Praia Limpa!** é uma adaptação do Pac-Man ambientada na praia de Boa Viagem. O personagem principal é um banhista cujo objetivo é fugir de tubarões (fantasmas no original) e coletar lixos no mar. Esta versão conecta o jogador à cultura pernambucana, trazendo cenários locais e dinâmicas inovadoras.

<br>

## 🎯 Objetivo
Limpe o mar coletando lixos e evitando tubarões. Conforme o tempo passa, mais tubarões aparecem, tornando o jogo mais desafiador. Cada lixo coletado ajuda na preservação ambiental. Vença recolhendo todos os resíduos e limpando o mar completamente.

<br>

## 🎮 Como jogar
1. **WASD**: Movimente o personagem entre as barreiras de corais.
2. ♻️ **Colete os lixos** sem ser capturado pelos tubarões.
3. 🔆 **Power-ups** garantem imunidade por 5 segundos.
4. 🏆 **Vença** ao coletar todos os lixos e confira o ranking.

<br>

## 🛠️ Instalação e Execução

### Passo 1: Instale a Raylib

#### Em Linux (Debian/Ubuntu):
1. Atualize os pacotes:
   ```bash
   sudo apt update
2. Instale a Raylib:
   ```bash
   sudo apt install libraylib-dev
3. Verifique a instalação:
   ```bash
   pkg-config --libs --cflags raylib
   
#### Em Windows:
1. Acesse [Raylib](https://www.raylib.com/).
2. Baixe o pacote para MinGW/GCC.
3. Extraia os arquivos em uma pasta como C:\raylib.

<br>

### Passo 2: Instale o GCC
Siga as instruções no site oficial: [GCC](https://gcc.gnu.org/install/).

<br>

## ⚡Como executar o jogo

1. Navegue até o diretório jogoAED.
2. Escolha um método de execução:

#### Pela IDE (Visual Studio Code):
- Aperte **F5** para compilar e rodar.

#### Pelo terminal:
1. Compile o jogo:
   ```bash
   mingw32-make.exe RAYLIB_PATH=C:/raylib/raylib PLATFORM=PLATFORM_DESKTOP BUILD_MODE=RELEASE PROJECT_NAME=main OBJS=main.c
2. Execute o jogo:
   ```bash
   ./main.exe

<br>

## 🔧 Solução de Problemas

- **Erro ao compilar ou executar**:
   1. Feche e reabra a IDE.
   2. Repita os comandos.

- **Verifique se:**
  - A Raylib está instalada corretamente.
  - O GCC está configurado no PATH do sistema.
  - O diretório correto está aberto na IDE.

<br>
 
## ✍️ Autoria
- Beatriz Pereira
- Manuela Cavalcanti
- Maria Fernanda Ordonho
- Rafaela Vidal
- Ygor Rosa

  
