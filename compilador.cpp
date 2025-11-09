#include <cstdlib>
#include <iostream>
using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Uso: ./compilador nome_arquivo.asm\n";
        return 1;
    }

    string arquivo = argv[1];

    size_t dot = arquivo.find_last_of('.');
    string base = (dot == string::npos) ? arquivo : arquivo.substr(0, dot);

    if (system("g++ -std=c++17 -O2 pre_processamento.cpp -o pre_processador") != 0) {
        cerr << "Erro ao compilar pre_processamento.cpp\n";
        return 1;
    }

    string arquivo_pre = "./pre_processador " + arquivo;
    if (system(arquivo_pre.c_str()) != 0) {
        cerr << "Erro ao executar pré-processador\n";
        return 1;
    }

    if (system("g++ -std=c++17 -O2 monta_o1.cpp -o monta_o1") != 0) {
        cerr << "Erro ao compilar monta_o1.cpp\n";
        return 1;
    }

    string arquivo_o1 = "./monta_o1 " + base + ".pre";
    if (system(arquivo_o1.c_str()) != 0) {
        cerr << "Erro ao executar monta_o1\n";
        return 1;
    }

    if (system("g++ -std=c++17 -O2 monta_o2.cpp -o monta_o2") != 0) {
        cerr << "Erro ao compilar monta_o2.cpp\n";
        return 1;
    }

    string arquivo_o2 = "./monta_o2 " + base + ".o1";
    if (system(arquivo_o2.c_str()) != 0) {
        cerr << "Erro ao executar monta_o2\n";
        return 1;
    }

    return 0;
}