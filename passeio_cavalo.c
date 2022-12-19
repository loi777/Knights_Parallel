#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <omp.h>

#define N 5
#define M 5

#define THREADMAX 4



//--------------------------------------------------------------------------------------



// imprime o tabuleiro
// imprimindo a rodada em que o cavalo pisou na casa
void print_tabuleiro(int tabuleiro[N][M]) {
    int i, j;

    for (i=0; i < N; i++){
        for (j=0; j < M; j++)
            printf("%3d ",tabuleiro[i][j]);
        printf("\n");
    }
}

// inicia o tabuleiro zerando suas casas
// esceto a posicao de inicio
void iniciar_tabuleiro(int tabuleiro[THREADMAX][N][M], int x_inicio, int y_inicio) {
    for (int i=0; i < N; i++)
        for (int j=0; j < M; j++)
            tabuleiro[0][i][j] = 0;

    tabuleiro[0][x_inicio][y_inicio] = 1;
}

// copia o tabuleiro origem no tabuleiro copy
void copia_tabuleiro(int tabuleiro[THREADMAX][N][M], int origim, int copy) {
    for (int i=0; i < N; i++)
        for (int j=0; j < M; j++)
            tabuleiro[copy][i][j] = tabuleiro[origim][i][j];
}



//------------------------------------------------------------------



// confirma que a movimentacao esta dentro do tabuleiro
// assim como se a posicao de movimento nao foi usada ja
int jogada_valida(int x, int y, int tabuleiro[N][M]) {
    if (x < 0  || x >= N || y < 0 || y >= M)
        return 0;
        
    if(tabuleiro[x][y] != 0)
        return 0;

    return 1;
}

/* Possiveis movimentos
#1
x2 = x + 1;
y2 = y + 2;

#2
x2 = x + 1;
y2 = y - 2;

#3
x2 = x - 1;
y2 = y + 2;

#4
x2 = x - 1;
y2 = y - 2;

#5
x2 = x + 2;
y2 = y + 1;

#6
x2 = x + 2;
y2 = y - 1;

#7
x2 = x - 2;
y2 = y + 1;

#8
x2 = x - 2;
y2 = y - 1;
*/



// [1, 8] movimentos possiveis
// retorna o valor Y correspondente
int proximo_movimento_y(int y, int movimento) {
    int valor = 1;

    if( movimento < 5 )
        valor = 2;

    if (movimento % 2 == 0) // se par, eh uma subtracao
        return y - valor;
    
    return y + valor;
    
}

// [1, 8] movimentos possiveis
// retorna o valor X correspondente
int proximo_movimento_x(int x, int movimento) {
    if (movimento < 3)
        return x + 1;

    if (movimento < 5)
        return x - 1;

    if (movimento < 7)
        return x + 2;
    
    return x - 2;
}


int get_last_valid_moviment(int tabuleiro[N][M], int x, int y) {
    for (int i = 8; i > 0; i --) {
        if (jogada_valida(proximo_movimento_x(x, i), proximo_movimento_y(y, i), tabuleiro)) {
            return i;
        }
    }

    return 9;   // val impossivel
}



//----------------------------------------------------------------------------



int get_free_thread(int* threads) {
    for (int i = 1; i < THREADMAX; i ++) {
        if (threads[i] == 0) {
            threads[i] = 1;
            return i;
        }
    }

    return 0;
}


int* create_threads() {
    int* threads = malloc(THREADMAX * sizeof(int));

    for (int i = 1; i < THREADMAX; i++) {
        threads[i] = 0;
    }

    threads[0] = 1;

    return threads;
}


void clear_threads(int* threads) {
    free(threads);
}



//----------------------------------------------------------------------------



// versao recursiva do algoritmo
int passeio_cavalo(int tabuleiro[N][M], int x, int y, int jogada, int* global_condicao_parada) {
    int x2, y2;

    // condicao de parada
    if (jogada == N*M)
        return 1;


    // testa cada movimento, [1, 8] possiveis
    for (int i=1;i<9;i++){

        // verifica se nenhuma outra thread ja terminou
        if (*global_condicao_parada) {
            return 0;
        }

        x2 = proximo_movimento_x(x,i);
        y2 = proximo_movimento_y(y,i);

        // se a jogada nao for valida pula ela
        if (!jogada_valida(x2, y2, tabuleiro))
            continue;


        // se a posicao desse movimento for valida o cavalo anda
        tabuleiro[x2][y2] = jogada+1;

        // continue a recursao
        if (passeio_cavalo(tabuleiro, x2, y2, jogada+1, global_condicao_parada))
            return 1;
        

        // backtrack da recursao
        tabuleiro[x2][y2] = 0;
    }

    return 0;
}



// inicia a recursao para multiplas threads
void inicia_recursao_multithread(int tabuleiro[THREADMAX][N][M], int x, int y, int jogada, int* threads, int thread_free, int* global_condicao_parada) {

    // marca a movimentacao
    tabuleiro[thread_free][x][y] = jogada + 1;

    // inicia a recursao, caso ela tenha dado certo, salva
    if (passeio_cavalo(tabuleiro[thread_free], x, y, jogada + 1, global_condicao_parada)) {
        *global_condicao_parada = 1;

        copia_tabuleiro(tabuleiro, thread_free, 0);

        return;
    }
    
    return;
}



//------------------------------------------------------------------------------------------



// versao recursiva do algoritmo
void passeio_cavalo_main(int tabuleiro[THREADMAX][N][M], int too_deep, int x, int y, int jogada, int* threads, int* global_condicao_parada) {
    int x2, y2, thread_free;

    // condicao de parada
    if (jogada == N*M) {
        *global_condicao_parada = 1;

        return;
    }


    int main_garantee = get_last_valid_moviment(tabuleiro[0], x, y);


    // testa cada movimento, [1, 8] possiveis
    for (int i=1; i<9; i++){
        // condicao de parada caso uma multi-thread chegou na resposta
        if (*global_condicao_parada)
            return;


        x2 = proximo_movimento_x(x,i);
        y2 = proximo_movimento_y(y,i);


        // verifica se a posicao x2,y2 eh valida
        if (!jogada_valida(x2, y2, tabuleiro[0])) {
            continue;
        }


        thread_free = 0;
        // se a main foi prolongada // obtem uma thread livre
        if (i < main_garantee && jogada < too_deep) {
            thread_free = get_free_thread(threads);
        }


        // amenos que nao tenha
        if (thread_free) {

            copia_tabuleiro(tabuleiro, 0, thread_free);

            // inicia multi-thread
            #pragma omp task shared(tabuleiro, threads, global_condicao_parada) firstprivate(thread_free, x2, y2, jogada)
            {
                inicia_recursao_multithread(tabuleiro, x2, y2, jogada, threads, thread_free, global_condicao_parada);

                // libera a thread usada
                threads[thread_free] = 0;
            }

            continue;
        }
        

        // caso nao estejamos profundos ainda
        tabuleiro[0][x2][y2] = jogada+1;

        // continua a recursao
        passeio_cavalo_main(tabuleiro, too_deep, x2,y2, jogada+1, threads, global_condicao_parada);
        
        // verifica se a main terminou a recursao
        if (*global_condicao_parada) {
            return;
        }

        // backtrack da recursao
        tabuleiro[0][x2][y2] = 0;
    }

    return;
}



//--------------------------------------------------------------------------------------



/* abordagem tomada:
    cada thread possui uma informacao se ela ja esta processando uma recursao ou nao.
    
    a thread principal (0) eh especial, ela tera sua propria recursao, 
    mas sempre que possivel ela tentara usar mais threads para incurtar o seu trabalho */


// main
int main() {
    printf("Resolvendo para N=%d e M=%d\n", N, M);
    printf("Resolvendo com %d Threads\n", THREADMAX);

    int tabuleiro[THREADMAX][N][M];

    // se a main estiver muito profunda, nao vale a pena
    // criar novas threads
    int too_deep = N*M / 4;

    clock_t start = clock();


    // zera o tabuleiro da main, localizado na posicao 1
    iniciar_tabuleiro(tabuleiro, 0, 0);


    // variaveis globais que devem ser repassadas para a recursao main
    int *threads = create_threads();

    int global_condicao_parada = 0;

    omp_set_num_threads(4);
    
    #pragma omp parallel shared(tabuleiro, threads, global_condicao_parada)
    #pragma omp single
    {
        // nossa recursao principal
        passeio_cavalo_main(tabuleiro, too_deep, 0, 0, 1, threads, &global_condicao_parada);
    }


    

    // verifica se alguma thread achou uma resposta valida
    if (global_condicao_parada) {
        print_tabuleiro(tabuleiro[0]); // tabuleiro de resposta
    } else {
        printf("Nao existe solucao\n");
    }


    clear_threads(threads);


    clock_t end = clock();
    double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("\n\n%f seconds\n",cpu_time_used);
}
