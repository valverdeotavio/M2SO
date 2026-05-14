    #include <iostream>
    #include <pthread.h>
    #include <unistd.h>
    #include <vector>
    #include <chrono>
    #include <random>
    #include <iomanip>

    using namespace std;
    using namespace chrono;

    enum Estado {
        PENSANDO,
        FOME,
        COMENDO
    };

    int N;
    int tempoSimulacao;

    int pensarMin;
    int pensarMax;

    int comerMin;
    int comerMax;

    vector<Estado> estados;
    vector<int> refeicoes;
    vector<bool> garfos;

    pthread_mutex_t mutexMesa;
    vector<pthread_cond_t> condicoes;

    bool executando = true;

    steady_clock::time_point inicio;

    // retorna filósofo da esquerda
    int esquerda(int i) {
        return (i + N - 1) % N;
    }

    // retorna filósofo da direita
    int direita(int i) {
        return (i + 1) % N;
    }

    // converte estado para texto
    string nomeEstado(Estado e) {

        if (e == PENSANDO)
            return "PENS";

        if (e == FOME)
            return "FOME";

        return "COME";
    }

    // imprime tempo formatado
    void imprimirTempo() {

        auto agora = steady_clock::now();

        auto tempo =
            duration_cast<milliseconds>(agora - inicio).count();

        int horas = tempo / 3600000;
        tempo %= 3600000;

        int minutos = tempo / 60000;
        tempo %= 60000;

        int segundos = tempo / 1000;
        tempo %= 1000;

        cout << "[";

        cout << setfill('0') << setw(2) << horas << ":";
        cout << setw(2) << minutos << ":";
        cout << setw(2) << segundos << ".";
        cout << setw(3) << tempo;

        cout << "]";
    }

    // imprime estado completo
    void mostrarSistema(int id, string mudanca) {

        imprimirTempo();

        cout << " F" << id << ": "
            << mudanca << endl;

        // garfos
        cout << "Garfos: ";

        for (int i = 0; i < N; i++) {

            if (garfos[i])
                cout << "[X]";
            else
                cout << "[O]";
        }

        cout << endl;

        // filósofos
        cout << "Filosofos: ";

        for (int i = 0; i < N; i++) {

            cout << "F" << i
                << ":" << nomeEstado(estados[i]);

            if (i != N - 1)
                cout << " | ";
        }

        cout << endl;

        // refeições
        cout << "Refeicoes: ";

        for (int i = 0; i < N; i++) {

            cout << "F" << i
                << ":" << refeicoes[i];

            if (i != N - 1)
                cout << " | ";
        }

        cout << endl;

        cout << "--------------------------------------------------"
            << endl;
    }

    // verifica se pode comer
    void testar(int i) {

        int esq = esquerda(i);
        int dir = direita(i);

        if (estados[i] == FOME &&
            estados[esq] != COMENDO &&
            estados[dir] != COMENDO) {

            estados[i] = COMENDO;

            garfos[esq] = true;
            garfos[i] = true;

            refeicoes[i]++;

            mostrarSistema(i, "FOME -> COME");

            pthread_cond_signal(&condicoes[i]);
        }
    }

    // tenta pegar garfos
    void pegarGarfos(int i) {

        pthread_mutex_lock(&mutexMesa);

        estados[i] = FOME;

        mostrarSistema(i, "PENS -> FOME");

        testar(i);

        while (estados[i] != COMENDO) {
            pthread_cond_wait(&condicoes[i], &mutexMesa);
        }

        pthread_mutex_unlock(&mutexMesa);
    }

    // devolve garfos
    void devolverGarfos(int i) {

        pthread_mutex_lock(&mutexMesa);

        estados[i] = PENSANDO;

        garfos[esquerda(i)] = false;
        garfos[i] = false;

        mostrarSistema(i, "COME -> PENS");

        testar(esquerda(i));
        testar(direita(i));

        pthread_mutex_unlock(&mutexMesa);
    }

    // gera número aleatório
    int aleatorio(int min, int max) {

        static thread_local mt19937 gerador(random_device{}());

        uniform_int_distribution<int> distribuicao(min, max);

        return distribuicao(gerador);
    }

    // thread filósofo
    void* filosofo(void* arg) {

        int id = *((int*)arg);

        while (executando) {

            // pensando
            usleep(
                aleatorio(pensarMin, pensarMax) * 1000
            );

            pegarGarfos(id);

            // comendo
            usleep(
                aleatorio(comerMin, comerMax) * 1000
            );

            devolverGarfos(id);
        }

        pthread_exit(NULL);
    }

    // resumo final
    void resumo() {

        cout << endl;

        cout << "============= RESUMO FINAL ============="
            << endl;

        for (int i = 0; i < N; i++) {

            cout << "Filosofo F"
                << i
                << " comeu "
                << refeicoes[i]
                << " vezes."
                << endl;
        }
    }

    int main(int argc, char* argv[]) {

        if (argc != 7) {

            cout << "Uso:" << endl;

            cout << "./jantar "
                << "N duracao "
                << "pensarMin pensarMax "
                << "comerMin comerMax"
                << endl;

            return 1;
        }

        N = atoi(argv[1]);

        tempoSimulacao = atoi(argv[2]);

        pensarMin = atoi(argv[3]);
        pensarMax = atoi(argv[4]);

        comerMin = atoi(argv[5]);
        comerMax = atoi(argv[6]);

        if (N < 3) {

            cout << "Numero minimo de filosofos = 3"
                << endl;

            return 1;
        }

        estados.resize(N, PENSANDO);
        refeicoes.resize(N, 0);
        garfos.resize(N, false);

        condicoes.resize(N);

        pthread_mutex_init(&mutexMesa, NULL);

        for (int i = 0; i < N; i++) {
            pthread_cond_init(&condicoes[i], NULL);
        }

        vector<pthread_t> threads(N);
        vector<int> ids(N);

        inicio = steady_clock::now();

        // cria threads
        for (int i = 0; i < N; i++) {

            ids[i] = i;

            pthread_create(
                &threads[i],
                NULL,
                filosofo,
                &ids[i]
            );
        }

        // tempo da simulação
        sleep(tempoSimulacao);

        executando = false;

        // espera threads terminarem
        for (int i = 0; i < N; i++) {
            pthread_join(threads[i], NULL);
        }

        resumo();

        pthread_mutex_destroy(&mutexMesa);

        for (int i = 0; i < N; i++) {
            pthread_cond_destroy(&condicoes[i]);
        }

        return 0;
    }