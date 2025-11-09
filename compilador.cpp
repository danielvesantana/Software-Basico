#include <cstdlib>
#include <iostream>
using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Uso: ./compilador nome_arquivo.asm\n";
        return 1;
    }

    string arquivo = argv[1];

    if (system("g++ -std=c++17 -O2 pre_processamento.cpp -o pre_processador.out") != 0) {
        cerr << "Erro ao compilar pre_processamento.cpp\n";
        return 1;
    }

    if (system(("./pre_processador.out " + arquivo).c_str()) != 0) {
        cerr << "Erro ao executar pré-processador\n";
        return 1;
    }

    if (system("g++ -std=c++17 -O2 monta_o1.cpp -o monta_o1") != 0) {
        cerr << "Erro ao compilar monta_o1.cpp\n";
        return 1;
    }

    if (system("./monta_o1 pre_processado.pre") != 0) {
        cerr << "Erro ao executar monta_o1\n";
        return 1;
    }

    if (system("g++ -std=c++17 -O2 monta_o2.cpp -o monta_o2") != 0) {
        cerr << "Erro ao compilar monta_o2.cpp\n";
        return 1;
    }

    if (system("./monta_o2 pre_processado.o1") != 0) {
        cerr << "Erro ao executar monta_o2\n";
        return 1;
    }

    return 0;
}