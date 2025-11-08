#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <unordered_map>
using namespace std;

vector<string> macro_declarada;

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
    smatch match, ifstream& arquivo, string linha){

    regex reg("ENDMACRO");
    vector<string> escopo_macro;   //armazena as linhas da macro

    regex espacos("([^\\s]) {2,}");    //consome espaços desnecessários
    while(getline(arquivo, linha)){

        if(regex_search(linha, reg)){   //faz a leitura até encontrar ENDMACRO
            break;
        }

        escopo_macro.push_back(linha + "\n");   //adiciona as linhas da macro na MDT
    }

    linha = regex_replace(linha, espacos, "$1 ");   //tambem para consumir espaços
    tabelaMDT.insert({match[1].str(), escopo_macro});

    return tabelaMDT;
}

void primeira_passagem(unordered_map<string, int>& tabelaMNT,
    unordered_map<string, vector<string>>& tabelaMDT, ifstream& arquivo){

    string linha;

    while(getline(arquivo, linha)){
        smatch match;
        regex reg(R"((\w+):\s*MACRO\s*([\w&, ]*))");  // expressão regular para procurar macros

        if(regex_search(linha, match, reg)){
            macro_declarada.push_back(linha);

            tabelaMNT = cria_tabelaMNT(tabelaMNT, match);
            tabelaMDT = cria_tabelaMDT(tabelaMDT, match, arquivo, linha);
        }
    }
}

streampos inicio_segunda_passagem(ifstream& arquivo,unordered_map<string, int> tabelaMNT){
    string linha;
    regex reg("ENDMACRO");  //para capturar a última macro
    int contador = 0;
    streampos posicao;   //salva a posição que inicia o programa

    while(true){
        posicao = arquivo.tellg();

        if (!getline(arquivo, linha))
            break;

        if(regex_search(linha, reg)){
            contador++;
        }

        if(contador == tabelaMNT.size() && linha.length() != 0 && !regex_search(linha, reg)){
            break;   //caso tenha lido a última macro e tenha aparecido uma linha 
        }
    }

    return posicao;
}

string troca_argumentos(string linha_escopo, string linha_chamada){
    //macro_declarada e o nome da macro quando foi declarada

    for(int i = 0; i < macro_declarada.size(); i++){
        regex reg(R"((\w+):\s*MACRO\s*([\w&, ]*))");
        smatch match;

        //ve se a macro compreende ao regex de macro
        if(regex_search(macro_declarada[i], match, reg)){
            regex reg1("\\b" + match[1].str() + "\\b");
            
            //ve se o nome da macro declarada e igual a macro chamada
            if(regex_search(linha_chamada, reg1)){
                regex reg2(R"((\w+)\s*([\w&, ]*))");  //captura os arg da macro chamada
                smatch match1;

                vector<string> arg_chamada;
                if(regex_search(linha_chamada, match1, reg2))
                    arg_chamada = split(match1[2].str(), ',');  //separa os argumentos na macro chamada
                vector<string> arg_declarado = split(match[2].str(), ','); //separa os argumentos na macro declarada

                // limpeza de espaços dos vetores de argumentos
                for (string& s : arg_chamada) {
                    s = regex_replace(s, regex("^\\s+|\\s+$"), "");
                }
                for (string& s : arg_declarado) {
                    s = regex_replace(s, regex("^\\s+|\\s+$"), "");
                }

                // substitui cada argumento formal pelo real
                for (size_t j = 0; j < arg_declarado.size() && j < arg_chamada.size(); j++) {
                    regex arg_regex("\\b" + arg_declarado[j] + "\\b");
                    linha_escopo = regex_replace(linha_escopo, arg_regex, arg_chamada[j]);
                }
            }else continue;
        }
    }

    return linha_escopo;
}

void segunda_passagem(unordered_map<string, int>& tabelaMNT, unordered_map<string, vector<string>>& tabelaMDT, ifstream& arquivo, string filename){
    string linha;
    ofstream pre_processado;
    pre_processado.open("pre_processado.pre");

    arquivo.clear();
    arquivo.seekg(0);

    //faz com que a leitura do arquivo comece apos as declaracoes de macros
    streampos inicio_pos = inicio_segunda_passagem(arquivo, tabelaMNT);

    arquivo.clear();
    arquivo.seekg(inicio_pos);

    regex reg("([^\\s]) {2,}");  //remove espaços em brancos

    while(getline(arquivo, linha)){
        linha = regex_replace(linha, reg, "$1 ");  //remove espaços em brancos
        bool flag = false;

        for(auto &par1 : tabelaMNT){
            string macro1 = par1.first;
            regex reg1("\\b" + macro1 + "\\b");
            smatch match1;

            //testa se alguma macro da tabela MNT corresponde esta na linha lida
            if(regex_search(linha, match1, reg1)){
                flag = true;
                vector<string> corpo1 = tabelaMDT[macro1];   //armazena o escopo da macro

                for(string linha_macro1 : corpo1){
                    bool expandiu = false;
                    linha_macro1 = troca_argumentos(linha_macro1, linha);

                    //testa agora se dentro dessa macro há outra macro
                    for(auto &par2 : tabelaMNT){
                        string macro2 = par2.first;
                        regex reg2("\\b" + macro2 + "\\b");
                        smatch match2;

                        //se houver outra macro faz a substituição das linhas dessa macro que foi chamada
                        if(regex_search(linha_macro1, match2, reg2)){
                            vector<string> corpo2 = tabelaMDT[macro2];

                            for(string linha_macro2 : corpo2){
                                linha_macro2 = troca_argumentos(linha_macro2, linha_macro1);

                                pre_processado << linha_macro2; 
                            }
                            expandiu = true;
                        }
                    }
                    if(!expandiu){ 
                        pre_processado << linha_macro1; 
                    }
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

int main(int argc, char* argv[]){
    unordered_map<string, int> tabelaMNT;
    unordered_map<string, vector<string>> tabelaMDT;
    string filename;

    if (argc != 2) {
        cout << "Uso: ./pre_processador.out <arquivo.asm>\n";
        return 1;
    }

    filename = argv[1];
    ifstream arquivo(filename);
    if(arquivo.is_open()){
        primeira_passagem(tabelaMNT, tabelaMDT, arquivo);
        arquivo.close();

        arquivo.open(filename);
        segunda_passagem(tabelaMNT, tabelaMDT, arquivo, filename);
    }else{
        cout << "nao foi possivel abrir o arquivo\n";
    }

    return 0;
}