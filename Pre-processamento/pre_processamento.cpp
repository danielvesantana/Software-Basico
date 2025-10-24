#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <unordered_map>
using namespace std;

vector<string> split(const string& linha, char expressao){
    vector<string> arg;
    size_t inicio, fim;
    inicio = fim = 0;

    while ((fim = linha.find(expressao, inicio)) != string::npos){
        arg.push_back(linha.substr(inicio, fim - inicio));
        inicio = fim + 1;
    }

    arg.push_back(linha.substr(inicio));
    return arg;
}

unordered_map<string, int> cria_tabelaMNT(unordered_map<string, int>& tabelaMNT, smatch match){
    vector<string> arg = split(match[2].str(), ',');   //encontra os argumentos da macro
    int n_arg = arg.size();     
    tabelaMNT.insert({match[1].str(), n_arg});  //insere a quantidade de argumentos

    return tabelaMNT;
}

unordered_map<string, vector<string>> cria_tabelaMDT(unordered_map<string, vector<string>>& tabelaMDT,
    smatch match, ifstream& arquivo, string linha, streampos& inicio_programa){

    regex reg("ENDMACRO");
    vector<string> escopo_macro;   //armazena as linhas da macro

    regex espacos("([^\\s]) {2,}");    //consome espaços desnecessários
    while(getline(arquivo, linha)){

        if(regex_search(linha, reg)){   //faz a leitura até encontrar ENDMACRO
            inicio_programa = arquivo.tellg();
            break;
        }

        escopo_macro.push_back(linha + "\n");   //adiciona as linhas da macro na MDT
    }

    linha = regex_replace(linha, espacos, "$1 ");   //tambem para consumir espaços
    tabelaMDT.insert({match[1].str(), escopo_macro});

    return tabelaMDT;
}

void primeira_passagem(unordered_map<string, int>& tabelaMNT,
    unordered_map<string, vector<string>>& tabelaMDT, ifstream& arquivo,
    streampos& inicio_programa){

    string linha;

    while(getline(arquivo, linha)){
        smatch match;
        regex reg(R"((\w+):\s*MACRO\s*([\w&, ]*))");  // expressão regular para procurar macros

        if(regex_search(linha, match, reg)){
            tabelaMNT = cria_tabelaMNT(tabelaMNT, match);
            tabelaMDT = cria_tabelaMDT(tabelaMDT, match, arquivo, linha, inicio_programa);
        }
    }
}

void segunda_passagem(unordered_map<string, int>& tabelaMNT, unordered_map<string, vector<string>>& tabelaMDT, ifstream& arquivo){
    string linha;
    ofstream pre_processado;
    pre_processado.open("pre_processado.pre");

    regex reg("([^\\s]) {2,}");

    while(getline(arquivo, linha)){
        linha = regex_replace(linha, reg, "$1 ");
        bool flag = false;

        for(auto &par1 : tabelaMNT){
            string macro1 = par1.first;
            regex reg1("\\b" + macro1 + "\\b");
            smatch match1;

            if(regex_search(linha, match1, reg1)){
                flag = true;
                vector<string> corpo1 = tabelaMDT[macro1];

                for(string linha_macro1 : corpo1){
                    bool expandiu = false;

                    for(auto &par2 : tabelaMNT){
                        string macro2 = par2.first;
                        regex reg2("\\b" + macro2 + "\\b");
                        smatch match2;

                        if(regex_search(linha_macro1, match2, reg2)){
                            vector<string> corpo2 = tabelaMDT[macro2];

                            for(string linha_macro2 : corpo2){
                                pre_processado << linha_macro2; 
                            }
                            expandiu = true;
                        }
                    }
                    if(!expandiu) 
                        pre_processado << linha_macro1; 
                }
                break;
            }
        }

        if(!flag){
            pre_processado << linha << "\n";
        }
    }

    pre_processado.close();
}

int main(){
    unordered_map<string, int> tabelaMNT;
    unordered_map<string, vector<string>> tabelaMDT;
    streampos inicio_programa;

    ifstream arquivo("test.asm");
    if(arquivo.is_open()){
        primeira_passagem(tabelaMNT, tabelaMDT, arquivo, inicio_programa);
        arquivo.close();

        arquivo.open("test.asm");
        arquivo.seekg(inicio_programa);
        segunda_passagem(tabelaMNT, tabelaMDT, arquivo);
    }else{
        cout << "nao foi possivel abrir o arquivo\n";
    }

    return 0;
}