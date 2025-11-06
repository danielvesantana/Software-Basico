#include <bits/stdc++.h>
using namespace std;

// util
static inline string trim(const string& s){
    size_t i=0,j=s.size();
    while(i<j && isspace((unsigned char)s[i])) ++i;
    while(j>i && isspace((unsigned char)s[j-1])) --j;
    return s.substr(i, j-i);
}
static inline vector<string> split_commas(const string& s){
    vector<string> out; string cur; int par=0;
    for(char c: s){
        if(c==',' && par==0){ out.push_back(trim(cur)); cur.clear(); }
        else cur.push_back(c);
    }
    if(!cur.empty()) out.push_back(trim(cur));
    return out;
}
static inline string upper(string s){ for(char&c:s) c=toupper((unsigned char)c); return s; }

// opcode table
struct OpInfo{ int opcode; int nops; int sizeWords; };
unordered_map<string, OpInfo> mkOpcodes(){
    unordered_map<string, OpInfo> m;
    m["ADD"]   ={ 1,1,2}; m["SUB"]  ={ 2,1,2}; m["MULT"] ={ 3,1,2};
    m["DIV"]   ={ 4,1,2}; m["JMP"]  ={ 5,1,2}; m["JMPN"] ={ 6,1,2};
    m["JMPP"]  ={ 7,1,2}; m["JMPZ"] ={ 8,1,2}; m["COPY"] ={ 9,2,3};
    m["LOAD"]  ={10,1,2}; m["STORE"]={11,1,2}; m["INPUT"]={12,1,2};
    m["OUTPUT"]={13,1,2}; m["STOP"] ={14,0,1};
    return m;
}
static const unordered_map<string, OpInfo>& OPC(){ static unordered_map<string, OpInfo> T = mkOpcodes(); return T; }

bool isValidLabelName(const string& s){
    if(s.empty()) return false;
    if(isdigit((unsigned char)s[0])) return false;
    for(char c: s){
        if(!(isalnum((unsigned char)c) || c=='_')) return false;
    }
    return true;
}
bool tryParseInt(const string& t, long long& val){
    if(t.empty()) return false;
    char* end=nullptr;
    val = strtoll(t.c_str(), &end, 10);
    return (*end=='\0');
}

// expressao LABEL(+/-k) ou inteiro
struct LabelExpr{ string base; long long offset=0; bool isLabel=false; bool badOffset=false; };
LabelExpr parseLabelExpr(const string& tok){
    LabelExpr e;
    string s = tok;
    // remove espacos internos
    s.erase(remove_if(s.begin(), s.end(), [](char c){return isspace((unsigned char)c);}), s.end());
    long long v;
    if(tryParseInt(s,v)){ e.isLabel=false; e.base=s; e.offset=v; return e; }
    size_t pplus = s.find('+');
    size_t pminus = (s.size()>0 ? s.find('-', 1) : string::npos);
    size_t p = string::npos;
    if(pplus!=string::npos && pminus!=string::npos) p = min(pplus,pminus);
    else p = (pplus!=string::npos ? pplus : pminus);
    if(p==string::npos){ e.isLabel=true; e.base=s; e.offset=0; return e; }
    e.isLabel=true; e.base = s.substr(0,p);
    string rhs = s.substr(p);
    long long off=0;
    if(!tryParseInt(rhs, off)){ e.badOffset=true; off=0; }
    e.offset=off;
    return e;
}

// estruturas
struct LineInfo{
    int lineNo=0;
    int addr=-1;
    string raw;
    string label;
    string mnemonic;
    vector<string> ops;
    int sizeWords=0;
    bool isDirective=false;
    bool emits=false;
};
struct ErrorItem{ int lineNo; string type; string msg; };
// pendências
struct PendItem{
    string symbol;
    int addrOperand;
    int lineNoFirstUse;
};

struct Parsed{
    LineInfo li;
    vector<ErrorItem> errs;
};

static const unordered_set<string> kDirectives = {"BEGIN","END","SPACE","CONST"};

// verifica se a linha eh uma expressao nua (sem mnemonico)
static bool isBareExpression(const string& s){
    string t = trim(s);
    static const regex exprOnly(R"(^[A-Za-z_]\w*(\s*[\+\-]\s*-?\d+)?$)");
    return regex_match(t, exprOnly);
}

// ---------- helper para imprimir erros (arquivo ou tela) ----------
static void renderErrors(ostream& os, const vector<ErrorItem>& errs){
    os << "\n=== ERROS ===\n";
    if(errs.empty()){
        os << "(nenhum)\n";
        return;
    }
    // imprimir ordenado por linha (estável)
    vector<ErrorItem> sorted = errs;
    stable_sort(sorted.begin(), sorted.end(),
        [](const ErrorItem& a, const ErrorItem& b){
            return a.lineNo < b.lineNo;
        });
    for(const auto& e: sorted){
        if(e.lineNo>0) os << "[linha " << e.lineNo << "] ";
        else           os << "[apos leitura] ";
        os << e.type << ": " << e.msg << "\n";
    }
}

// parser de linha
Parsed parseLine(const string& line, int lineNo){
    Parsed P;
    P.li.lineNo = lineNo;
    P.li.raw = line;

    string s = trim(line);
    if(s.empty()){ P.li.emits=false; return P; }

    // rotulo opcional
    string label, rest=s;
    size_t p = s.find(':');
    if(p!=string::npos){
        label = trim(s.substr(0,p));
        rest  = trim(s.substr(p+1));
        if(!isValidLabelName(label)){
            P.errs.push_back({lineNo,"Léxico","Nome de rótulo inválido: '"+label+"'"});
        }
        P.li.label = label;
        if(rest.empty()){ P.li.emits=false; return P; }
    }

    // separa primeiro token
    string first, tail;
    {
        stringstream ss(rest);
        ss >> first;
        getline(ss, tail);
    }
    if(first.empty()){ P.li.emits=false; return P; }

    string M = upper(first);
    P.li.mnemonic = M;
    tail = trim(tail);

    // se nao eh diretiva nem instrucao, ve se eh expressao nua
    if(!kDirectives.count(M) && !OPC().count(M)){
        if(isBareExpression(rest)){
            P.li.emits=false; return P;
        }
    }

    // operandos
    if(!tail.empty()) P.li.ops = split_commas(tail);

    // diretivas
    if(kDirectives.count(M)) P.li.isDirective=true;

    // BEGIN/END não emitem
    if(M=="BEGIN" || M=="END"){ P.li.emits=false; return P; }

    // SPACE/CONST ou instrução emitem
    P.li.emits=true;
    return P;
}

// montagem (passagem única para .o1)
int main(int argc, char** argv){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if(argc<2){
        cerr << "Uso: ./monta_o1 <pre_processado.pre>\n";
        return 1;
    }
    string inPre = argv[1];
    ifstream in(inPre);
    if(!in.is_open()){
        cerr << "Nao foi possivel abrir " << inPre << "\n";
        return 1;
    }

    auto dot = inPre.find_last_of('.');
    string base = (dot==string::npos)? inPre : inPre.substr(0,dot);
    string outO1 = base + ".o1";

    unordered_map<string,int> symtab; // TS
    vector<LineInfo> listing;
    vector<ErrorItem> errors;
    vector<PendItem> pend;

    int PC=0;
    string line; int ln=0;
    unordered_map<string,int> firstDef;

    while(getline(in,line)){
        ++ln;
        Parsed P = parseLine(line, ln);
        for(auto&e:P.errs) errors.push_back(e);

        // define rótulo no PC atual
        if(!P.li.label.empty()){
            if(symtab.count(P.li.label) && symtab[P.li.label]!=PC){
                errors.push_back({ln,"Semântico","Rótulo declarado duas vezes em endereços diferentes: "+P.li.label});
            }
            // registrar na TS mesmo com erro léxico
            symtab[P.li.label]=PC;
            if(!firstDef.count(P.li.label)) firstDef[P.li.label]=ln;
        }

        if(!P.li.emits) continue;

        LineInfo li = P.li;
        li.addr = PC;

        // diretivas de dados
        if(li.isDirective){
            if(li.mnemonic=="SPACE"){
                if(li.ops.size()>1){
                    errors.push_back({ln,"Sintático","SPACE aceita no máx. 1 argumento"});
                }
                long long n=1;
                if(!li.ops.empty()){
                    if(!tryParseInt(li.ops[0], n) || n<=0){
                        errors.push_back({ln,"Sintático","SPACE requer inteiro positivo"});
                        n=1;
                    }
                }
                li.sizeWords=(int)n;
                listing.push_back(li);
                PC += li.sizeWords;
                continue;
            }else if(li.mnemonic=="CONST"){
                if(li.ops.size()!=1){
                    errors.push_back({ln,"Sintático","CONST requer 1 argumento"});
                    li.sizeWords=1;
                }else{
                    long long v;
                    if(!tryParseInt(li.ops[0], v)){
                        errors.push_back({ln,"Sintático","CONST requer inteiro (ex.: CONST -1)"});
                    }
                    li.sizeWords=1;
                }
                listing.push_back(li);
                PC += li.sizeWords;
                continue;
            }
        }

        // instrução
        if(!OPC().count(li.mnemonic)){
            errors.push_back({ln,"Sintático","Instrução inexistente: "+li.mnemonic});
            li.sizeWords=1;
            listing.push_back(li);
            PC += li.sizeWords;
            continue;
        }
        auto info = OPC().at(li.mnemonic);
        if((int)li.ops.size()!=info.nops){
            errors.push_back({ln,"Sintático","Número de operandos incorreto em "+li.mnemonic});
        }
        li.sizeWords = info.sizeWords;

        // pendências e léxico dos operandos
        for(size_t i=0;i<li.ops.size();++i){
            LabelExpr ex = parseLabelExpr(li.ops[i]);
            if(ex.badOffset){
                errors.push_back({ln,"Sintático","Offset inválido em '"+li.ops[i] +"'"});
            }
            if(ex.isLabel){
                if(!isValidLabelName(ex.base)){
                    errors.push_back({ln,"Léxico","Identificador inválido em operando: '"+ex.base+"'"});
                }
                if(!symtab.count(ex.base)){
                    int addrOp = li.addr + 1 + (int)i;
                    pend.push_back({ex.base, addrOp, ln});
                }
            }else{
                long long dummy;
                if(!tryParseInt(li.ops[i], dummy)){
                    errors.push_back({ln,"Léxico","Token de operando inválido: '"+li.ops[i]+"'"});
                }
            }
        }

        listing.push_back(li);
        PC += li.sizeWords;
    }

    // rótulos usados e nunca definidos -> erro na linha da 1ª utilização
    {
        unordered_map<string,int> firstUseLine; // simbolo -> menor linha de uso
        for(const auto& p: pend){
            if(!firstUseLine.count(p.symbol)) firstUseLine[p.symbol] = p.lineNoFirstUse;
            else firstUseLine[p.symbol] = min(firstUseLine[p.symbol], p.lineNoFirstUse);
        }
        for(const auto& [sym, lnuse] : firstUseLine){
            if(!symtab.count(sym)){
                errors.push_back({lnuse,"Semântico","Rótulo não declarado: "+sym});
            }
        }
    }

    ofstream o(outO1);
    if(!o.is_open()){
        cerr << "Nao foi possivel criar " << outO1 << "\n";
        return 1;
    }

    o << "=== LISTAGEM (.o1) — passagem unica, sem corrigir pendencias ===\n";
    for(const auto& li: listing){
        o << setw(4) << li.addr << "  ";
        if(!li.label.empty()) o << li.label << ": ";
        o << li.mnemonic;
        if(!li.ops.empty()){
            o << " ";
            for(size_t i=0;i<li.ops.size();++i){
                if(i) o << ", ";
                o << li.ops[i];
            }
        }
        o << "   ; tamanho="<<li.sizeWords << "\n";
    }

    o << "\n=== TABELA DE SIMBOLOS ===\n";
    vector<pair<string,int>> syms(symtab.begin(), symtab.end());
    sort(syms.begin(), syms.end(), [](auto&a,auto&b){return a.first<b.first;});
    for(auto& [name,addr]: syms){
        o << setw(10) << name << " = " << addr << "\n";
    }

    o << "\n=== LISTA DE PENDENCIAS (simbolo -> end+) ===\n";
    if(pend.empty()){
        o << "(sem pendencias)\n";
    }else{
        unordered_map<string, vector<int>> by;
        for(auto&p: pend) by[p.symbol].push_back(p.addrOperand);
        for(auto& [sym,vec]: by){
            sort(vec.begin(), vec.end());
            o << setw(10) << sym << " -> ";
            for(size_t i=0;i<vec.size();++i){
                if(i) o << ", ";
                o << vec[i] << "+";
            }
            o << "\n";
        }
    }

    // ---- ERROS (arquivo) ----
    renderErrors(o, errors);
    o.close();

    // ---- ERROS (tela) ----
    renderErrors(cout, errors);
    cout.flush();

    cout << "Gerado: " << outO1 << "\n";
    return 0;
}
