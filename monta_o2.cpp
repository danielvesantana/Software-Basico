#include <bits/stdc++.h>
using namespace std;

// util
static inline string trim(const string& s){
    size_t i=0, j=s.size();
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

static inline string upper(string s){ for(char& c : s) c = toupper((unsigned char)c); return s; }

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


// struct para guardar expressão de operando: inteiro ou LABEL(+/-k)
struct LabelExpr{
    string base;
    long long offset = 0;
    bool isLabel = false;
    bool badOffset = false; //
};

static bool tryParseInt(const string& intValid, long long& val){
    if(intValid.empty()) return false;
    char* end=nullptr; val = strtoll(intValid.c_str(), &end, 10);
    return (*end=='\0');
}

LabelExpr parseLabelExpr(const string& tok){
    LabelExpr e;
    string s = tok;
    s.erase(remove_if(s.begin(), s.end(), [](char c){return isspace((unsigned char)c);}), s.end());
    long long v;
    if(tryParseInt(s, v)){ e.isLabel=false; e.base=s; e.offset=v; return e; }
    size_t pplus = s.find('+');
    size_t pminus = (s.size()>0 ? s.find('-', 1) : string::npos);
    size_t p = string::npos;
    if(pplus!=string::npos && pminus!=string::npos) p = min(pplus, pminus);
    else p = (pplus!=string::npos ? pplus : pminus);
    if(p==string::npos){ e.isLabel=true; e.base=s; e.offset=0; return e; }
    e.isLabel=true; e.base = s.substr(0,p);
    string rhs = s.substr(p);
    long long off=0;
    if(!tryParseInt(rhs, off)){ e.badOffset=true; off=0; }
    e.offset=off;
    return e;
}

// p/ parsear o .o1
struct LinhaO1{
    int addr=-1;
    string mnemonic;
    vector<string> ops;
    int sizeWords=0;
    bool isDirective=false;
};

static bool isDirectiveName(const string& M){
    string U = upper(M);
    return (U=="SPACE" || U=="CONST" || U=="BEGIN" || U=="END");
}

// parse da LISTAGEM do <arquivo>.o1
static void leitorListaO1(istream& in, vector<LinhaO1>& linhasLista){
    string line;
    while(getline(in, line)){
        if(line.find("=== LISTAGEM (.o1)")!=string::npos) break;
    }
    while(getline(in, line)){
        string s = trim(line);
        if(s.empty()) break;
        if(s.rfind("===",0)==0) break;
        static const regex R(R"(^\s*(\d+)\s+(?:\w+\s*:\s*)?([A-Za-z]+)([^;]*?)\s*;\s*tamanho\s*=\s*(\d+)\s*$)");
        smatch m;
        if(!regex_match(line, m, R)) continue;

        LinhaO1 linh;
        linh.addr = stoi(m[1].str());
        linh.mnemonic = upper(trim(m[2].str()));
        string opsStr = trim(m[3].str());
        linh.sizeWords = stoi(m[4].str());
        linh.isDirective = isDirectiveName(linh.mnemonic);

        if(!opsStr.empty()){
            linh.ops = split_commas(opsStr);
        }
        linhasLista.push_back(move(linh));
    }
}

// parse da TABELA DE SIMBOLOS
static void leitorSimb(istream& in, unordered_map<string,int>& symtab){
    string line;
    while(getline(in, line)){
        if(line.find("=== TABELA DE SIMBOLOS")!=string::npos) break;
    }
    static const regex R(R"(^\s*([A-Za-z_]\w*)\s*=\s*(-?\d+)\s*$)");
    while(getline(in, line)){
        string s = trim(line);
        if(s.empty()) break;
        if(s.rfind("===",0)==0) break;
        smatch m;
        if(regex_match(s, m, R)){
            string name = m[1].str();
            int addr = stoi(m[2].str());
            symtab[name]=addr;
        }
    }
}

// resolve operando usando a tabela de simbolos
static long long resolveOperand(const string& tok,
                                const unordered_map<string,int>& symtab){
    LabelExpr ex = parseLabelExpr(tok);
    if(!ex.isLabel) return ex.offset;
    auto it = symtab.find(ex.base);
    if(it==symtab.end()){
        throw runtime_error("Simbolo indefinido: " + ex.base);
    }
    return (long long)it->second + ex.offset;
}

// parte de gerar o o2
static void GeradorO2(const string& base,
                      const vector<LinhaO1>& linhasLista,
                      const unordered_map<string,int>& symtab)
{
    string outName = base + ".o2";
    ofstream o(outName);
    if(!o.is_open()) throw runtime_error("Nao foi possivel criar " + outName);

    vector<long long> out;

    for(const LinhaO1& r : linhasLista){
        if(r.isDirective){
            if(r.mnemonic=="BEGIN" || r.mnemonic=="END"){
                continue;
            }
            if(r.mnemonic=="SPACE"){
                long long n=1;
                if(!r.ops.empty()){
                    long long v=1;
                    if(!tryParseInt(r.ops[0], v) || v<=0) v=1;
                    n=v;
                }
                for(long long i=0;i<n;++i) out.push_back(0);
                continue;
            }
            if(r.mnemonic=="CONST"){
                long long v=0;
                if(!r.ops.empty() && tryParseInt(r.ops[0], v)) out.push_back(v);
                else out.push_back(0);
                continue;
            }
            continue;
        }

        auto it = OPC().find(r.mnemonic);
        if(it==OPC().end()){
            continue;
        }
        const OpInfo& info = it->second;

        out.push_back(info.opcode);
        for(const string& op : r.ops){
            out.push_back( resolveOperand(op, symtab) );
        }
    }

    for(size_t i=0;i<out.size();++i){
        if(i) o << ' ';
        o << out[i];
    }
    o << "\n";
    o.close();
    cerr << "Gerado: " << outName << "\n";
}

int main(int argc, char** argv){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if(argc<2){
        cerr << "Uso: ./monta_o2 <base>.o1\n";
        return 1;
    }
    string inO1 = argv[1];
    ifstream in(inO1);
    if(!in.is_open()){
        cerr << "Nao foi possivel abrir " << inO1 << "\n";
        return 1;
    }

    auto dot = inO1.find_last_of('.');
    string base = (dot==string::npos)? inO1 : inO1.substr(0,dot);

    vector<LinhaO1> linhasLista;
    leitorListaO1(in, linhasLista);
    in.close();

    unordered_map<string,int> symtab;
    ifstream in2(inO1);
    leitorSimb(in2, symtab);
    in2.close();

    try{
        GeradorO2(base, linhasLista, symtab);
    }catch(const exception& ex){
        cerr << "Falha ao gerar .o2: " << ex.what() << "\n";
        return 2;
    }
    return 0;
}
