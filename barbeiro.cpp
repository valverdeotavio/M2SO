#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <queue>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstdlib>

using namespace std;
using namespace chrono;


enum EstadoBarbeiro {
    DORME,
    ATENDE
};


queue<int> filaClientes;

int capacidadeFila;

int clientesAtendidos = 0;
int clientesDesistentes = 0;

bool simulacaoAtiva = true;

EstadoBarbeiro estadoBarbeiro = DORME;


int chegadaMin;
int chegadaMax;

int atendimentoMin;
int atendimentoMax;

int duracaoSimulacao;


sem_t semClientes;
sem_t semBarbeiro;

pthread_mutex_t mutexFila;


steady_clock::time_point inicioSimulacao;


string tempoAtual() {
    auto agora = steady_clock::now();

    auto ms = duration_cast<milliseconds>(agora - inicioSimulacao).count();

    int horas = ms / 3600000;
    ms %= 3600000;

    int minutos = ms / 60000;
    ms %= 60000;

    int segundos = ms / 1000;
    ms %= 1000;

    stringstream ss;

    ss << "["
       << setfill('0') << setw(2) << horas << ":"
       << setw(2) << minutos << ":"
       << setw(2) << segundos << "."
       << setw(3) << ms
       << "]";

    return ss.str();
}


string filaVisual() {
    string s = "[";

    int ocupadas = filaClientes.size();

    for (int i = 0; i < ocupadas; i++)
        s += "#";

    for (int i = ocupadas; i < capacidadeFila; i++)
        s += ".";

    s += "]";

    return s;
}


void imprimirEstado(string evento) {

    cout << tempoAtual() << " " << evento << endl;

    // Estado barbeiro
    if (estadoBarbeiro == DORME)
        cout << "Barbeiro: DORME" << endl;
    else
        cout << "Barbeiro: ATENDE" << endl;

    // Fila
    cout << "Fila: "
         << filaVisual()
         << " (" << filaClientes.size()
         << "/" << capacidadeFila << ") -> ";

    queue<int> copia = filaClientes;

    while (!copia.empty()) {
        cout << "C" << copia.front() << " ";
        copia.pop();
    }

    cout << endl;

    // Contadores
    cout << "Contadores: "
         << "atendidos=" << clientesAtendidos
         << " | desistentes=" << clientesDesistentes
         << " | em_espera=" << filaClientes.size()
         << endl;

    cout << "------------------------------------------------------------"
         << endl;
}


void* barbeiro(void* arg) {

    while (simulacaoAtiva) {

        estadoBarbeiro = DORME;

        sem_wait(&semClientes);

        pthread_mutex_lock(&mutexFila);

        if (!filaClientes.empty()) {

            int clienteAtual = filaClientes.front();
            filaClientes.pop();

            estadoBarbeiro = ATENDE;

            imprimirEstado(
                "Barbeiro iniciou atendimento do cliente C"
                + to_string(clienteAtual)
            );

            pthread_mutex_unlock(&mutexFila);

            sem_post(&semBarbeiro);

            // Tempo aleatório de atendimento
            int tempo =
                atendimentoMin +
                rand() % (atendimentoMax - atendimentoMin + 1);

            sleep(tempo);

            pthread_mutex_lock(&mutexFila);

            clientesAtendidos++;

            imprimirEstado(
                "Barbeiro concluiu atendimento do cliente C"
                + to_string(clienteAtual)
            );
        }

        pthread_mutex_unlock(&mutexFila);
    }

    return NULL;
}


void* cliente(void* arg) {

    int id = *(int*)arg;

    delete (int*)arg;

    pthread_mutex_lock(&mutexFila);

    if ((int)filaClientes.size() < capacidadeFila) {

        filaClientes.push(id);

        imprimirEstado(
            "Cliente C" + to_string(id)
            + " chegou e entrou na fila"
        );

        pthread_mutex_unlock(&mutexFila);

        sem_post(&semClientes);

        sem_wait(&semBarbeiro);

    } else {

        clientesDesistentes++;

        imprimirEstado(
            "Cliente C" + to_string(id)
            + " chegou, mas desistiu por falta de cadeira"
        );

        pthread_mutex_unlock(&mutexFila);
    }

    return NULL;
}


int main() {

    srand(time(NULL));

    cout << "========== BARBEIRO DORMINHOCO ==========" << endl;

    cout << "Numero de cadeiras: ";
    cin >> capacidadeFila;

    cout << "Tempo MIN chegada clientes (segundos): ";
    cin >> chegadaMin;

    cout << "Tempo MAX chegada clientes (segundos): ";
    cin >> chegadaMax;

    cout << "Tempo MIN atendimento (segundos): ";
    cin >> atendimentoMin;

    cout << "Tempo MAX atendimento (segundos): ";
    cin >> atendimentoMax;

    cout << "Duracao da simulacao (segundos): ";
    cin >> duracaoSimulacao;

    // Inicialização
    sem_init(&semClientes, 0, 0);
    sem_init(&semBarbeiro, 0, 0);

    pthread_mutex_init(&mutexFila, NULL);

    inicioSimulacao = steady_clock::now();

    // Thread barbeiro
    pthread_t threadBarbeiro;

    pthread_create(
        &threadBarbeiro,
        NULL,
        barbeiro,
        NULL
    );

    int idCliente = 1;

    auto inicio = steady_clock::now();

    // Loop da simulação
    while (true) {

        auto agora = steady_clock::now();

        int tempoPassado =
            duration_cast<seconds>(agora - inicio).count();

        if (tempoPassado >= duracaoSimulacao)
            break;

        pthread_t threadCliente;

        int* id = new int(idCliente++);

        pthread_create(
            &threadCliente,
            NULL,
            cliente,
            id
        );

        pthread_detach(threadCliente);

        // Tempo aleatório entre chegadas
        int tempoChegada =
            chegadaMin +
            rand() % (chegadaMax - chegadaMin + 1);

        sleep(tempoChegada);
    }

    // Finaliza
    simulacaoAtiva = false;

    cout << endl;
    cout << "=========== FIM DA SIMULACAO ===========" << endl;

    cout << "Clientes atendidos: "
         << clientesAtendidos << endl;

    cout << "Clientes desistentes: "
         << clientesDesistentes << endl;

    // Libera recursos
    sem_destroy(&semClientes);
    sem_destroy(&semBarbeiro);

    pthread_mutex_destroy(&mutexFila);

    return 0;
}