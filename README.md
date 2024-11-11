# S.O.S Praia Limpa!

## Sobre o jogo 

O jogo é uma adaptação do Pac-Man, ambientado na praia de Boa Viagem. O personagem principal é um banhista que tem como objetivo fugir de tubarões, representando os fantasmas do jogo original.
Essa versão traz um toque local ao cenário, utilizando um dos principais pontos turísticos do Recife, e oferece uma nova dinâmica e temática que conecta o jogador à cultura praiana da região. 
O jogador se movimenta entre corais que simulam as barreiras do Pac-Man e durante a partida ele pode coletar Power-ups que lhe darão imunidade temporária. 
O público-alvo do jogo inclui adolescentes, jovens adultos e fãs de jogos que buscam uma experiência casual e divertida, além de turistas e moradores interessados na cultura pernambucana.


## Objetivo

O objetivo principal do jogo é "limpar" o mar, coletando os lixos espalhados pela água no menor tempo possível. O jogador precisa navegar entre corais e evitar tubarões enquanto remove resíduos flutuantes. A cada tempo que passa, a quantidade de tubarões aumenta, dificultando o jogo.
Cada peça de lixo coletada contribui para a limpeza das águas, refletindo um ambiente mais saudável e menos poluído.
A meta é recolher todos os detritos presentes, deixando o mar limpo e incentivando a preservação ambiental.

## Como jogar:

1. Use as setas para mover o personagem entre as barreiras de corais

2. Seu objetivo será coletar todos os lixos do mar sem ser capturado pelos tubarões

3. Powerups podem aparecer a qualquer momento, se coletá-lo você ganha imortalidade por 5 segundos

4. Se você coletar todos os lixos, vencerá e poderá consultar o ranking dos jogadores

## Instruções antes de executar:
Instalar a Raylib (biblioteca utilizada para a interface gráfica do jogo):
&nbsp;

### Em Linux (Debian/Ubuntu):
Atualize os pacotes:

      sudo apt update

Instale a Raylib:

    sudo apt install libraylib-dev

Verifique a instalação para garantir que a Raylib foi instalada corretamente:

    pkg-config --libs --cflags raylib

Esse comando deve retornar os caminhos e flags de compilação da Raylib, indicando que ela está pronta para uso.
&nbsp;
&nbsp;

### Em Windows:
Acesse o site oficial da Raylib.

    https://www.raylib.com/

Baixe o pacote adequado para MinGW/GCC.
&nbsp;

Extraia os arquivos em uma pasta de fácil acesso, como C:\raylib.
&nbsp;

## Como executar:
1. Entre no diretório **jogoAED**. Todos os comandos de compilação e execução do jogo devem ser executados a partir desse diretório, pois ele contém os arquivos e as configurações necessárias
2. O diretório aberto na IDE deve ser o citado acima, não funciona se entrar nele apenas pelo terminal
3. Aperte F5 para rodar
4. Outra maneira de iniciar o jogo é pelo terminal, digitando os comandos diretamente no diretório jogoAED:
  &nbsp;

4.1 Para compilar: 

        mingw32-make.exe RAYLIB_PATH=C:/raylib/raylib PLATFORM=PLATFORM_DESKTOP BUILD_MODE=RELEASE PROJECT_NAME=main OBJS=main.c
  
4.2 Para executar:
  
         ./main.exe
 
5. Em caso de erro em ambos os casos, feche a aba da IDE (aconselha-se o Visual Studio Code) e abra novamente, depois repita o comando.
   




