#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <omp.h>

#define N 8
#define M 8

#define THREADMAX 4

int MOVY[8] = {2, -2, 2, -2, 1, -1, 1, -1};
int MOVX[8] = {1, 1, -1, -1, 2, 2, -2, -2};

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
void iniciar_tabuleiro(int tabuleiro[N][M], int x_inicio, int y_inicio) {
    for (int i=0; i < N; i++)
        for (int j=0; j < M; j++)
            tabuleiro[i][j] = 0;

    tabuleiro[x_inicio][y_inicio] = 1;
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
    if (x < 0 || x >= N || y < 0 || y >= M)
        return 0;
        
    if (tabuleiro[x][y] != 0)
        return 0;

    return 1;
}


// [1, 8] movimentos possiveis
// retorna o valor Y correspondente
int proximo_movimento_y(int y, int movimento) {
    return y + MOVY[movimento];
}


// [1, 8] movimentos possiveis
// retorna o valor X correspondente
int proximo_movimento_x(int x, int movimento) {
    return x + MOVX[movimento];
}


void get_mov_possiveis(int tabuleiro[N][M], int x, int y, int mov_possiveis[8], int* mov_possiveis_qtd) {
    int x2, y2;

    for (int i = 0; i < 8; i ++) {
        x2 = proximo_movimento_x(x, i);
        y2 = proximo_movimento_y(y, i);

        if (jogada_valida(x2, y2, tabuleiro)) {
            mov_possiveis[*mov_possiveis_qtd] = i;
            *mov_possiveis_qtd += 1;
        }
    }
}



//----------------------------------------------------------------------------


// obtenha o id de uma thread que esta atualmente livre
int get_free_thread(int* threads) {
    for (int i = 1; i < THREADMAX; i ++) {
        if (threads[i] == 0) {
            threads[i] = 1;
            return i;
        }
    }

    return 0;
}


// aloca espaco e zera as posicoes para determinar threads livres
// posicao 0 sempre cheia por ser a main
int* create_threads() {
    int* threads = malloc(THREADMAX * sizeof(int));

    for (int i = 1; i < THREADMAX; i++) {
        threads[i] = 0;
    }

    threads[0] = 1;

    return threads;
}


// limpa o espaco alocado pelas threads
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
    for (int i=0;i<8;i++){

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
void passeio_cavalo_main(int tabuleiro[THREADMAX][N][M], int too_deep, int x, int y, int jogada, int* threads, int* global_condicao_parada, clock_t* unparallel_time) {
    int x2, y2, thread_free;
    clock_t temp_time;

    // condicao de parada
    if (jogada == N*M) {
        *global_condicao_parada = 1;

        return;
    }


    int mov_possiveis[8];
    int mov_possiveis_qtd = 0;
    get_mov_possiveis(tabuleiro[0], x, y, mov_possiveis, &mov_possiveis_qtd);


    if (mov_possiveis_qtd > 0) {

        // testa cada movimento, [1, 8] possiveis
        for (int i = 0; i < mov_possiveis_qtd-1; i++) {
            // condicao de parada caso uma multi-thread chegou na resposta
            if (*global_condicao_parada)
                return;


            x2 = proximo_movimento_x(x, mov_possiveis[i]);
            y2 = proximo_movimento_y(y, mov_possiveis[i]);


            thread_free = 0;
            // se a main foi prolongada // obtem uma thread livre
            if (jogada < too_deep) {
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
            passeio_cavalo_main(tabuleiro, too_deep, x2,y2, jogada+1, threads, global_condicao_parada, unparallel_time);

            // verifica se a main terminou a recursao
            if (*global_condicao_parada) {
                return;
            }

            // backtrack da recursao
            tabuleiro[0][x2][y2] = 0;
        }

    
        x2 = proximo_movimento_x(x, mov_possiveis[mov_possiveis_qtd-1]);
        y2 = proximo_movimento_y(y, mov_possiveis[mov_possiveis_qtd-1]);

        // caso nao estejamos profundos ainda
        tabuleiro[0][x2][y2] = jogada+1;

        // continua a recursao
        passeio_cavalo_main(tabuleiro, too_deep, x2,y2, jogada+1, threads, global_condicao_parada, unparallel_time);

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
    // o tempo de copia sera maior que o tempo economizado
    int too_deep = N*M / 5;

    clock_t start = clock();
    clock_t unparallel_time = 0;


    // zera o tabuleiro da main, localizado na posicao 0
    iniciar_tabuleiro(tabuleiro[0], 0, 0);


    // variaveis globais que devem ser repassadas para a recursao main
    int *threads = create_threads();

    int global_condicao_parada = 0;

    omp_set_num_threads(4);
    
    #pragma omp parallel shared(tabuleiro, threads, global_condicao_parada, unparallel_time)
    #pragma omp single
    {
        // nossa recursao principal
        passeio_cavalo_main(tabuleiro, too_deep, 0, 0, 1, threads, &global_condicao_parada, &unparallel_time);
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
    printf("\n\nTOTAL %f seconds\n", cpu_time_used);

    printf("NOT PARALEL %f seconds\n", ((double) unparallel_time / CLOCKS_PER_SEC));
}
