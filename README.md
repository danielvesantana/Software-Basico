# Software-Basico
# Compilador em C++ — Projeto de Software Básico

## Integrantes do Grupo

| Nome                         | Responsabilidade                                                                                  |
|------------------------------|--------------------------------------------------------------------------------------------------|
| **Aluno 1 – _Daniel Alves - 251031382 _** | Implementação do **pré-processador** (`pre_processamento.cpp`): expansão de macros e geração do `.pre`. |
| **Aluno 2 – _Bianca Neves - 231013583 _** | Desenvolvimento do **montador da 1ª passagem** (`monta_o1.cpp`): geração do `.o1`, TS, pendências e erros. |
| **Aluno 3 – _Felipe Augusto - 251038534 _** | Implementação do **montador da 2ª passagem** (`monta_o2.cpp`): leitura do `.o1` e geração do `.o2`.        |

> O projeto foi modularizado em arquivos separados para organizar melhor cada etapa do processo de compilação.

---

## Estrutura do Projeto

- `pre_processamento.cpp`  
  Pré-processador:
  - Expande macros.
  - Remove diretivas de macro (`MACRO` / `ENDMACRO`).
  - Gera o arquivo intermediário: **`<nome>.pre`**.

- `monta_o1.cpp`  
  Montador — 1ª passagem:
  - Lê `<nome>.pre`.
  - Monta **Tabela de Símbolos (TS)**.
  - Gera listagem com endereços e tamanhos.
  - Gera **Lista de Pendências**.
  - Detecta e registra erros:
    - Léxicos (nomes inválidos),
    - Sintáticos (mnemônicos/operandos incorretos),
    - Semânticos (rótulos não declarados, duplicidades).
  - Saída: **`<nome>.o1`**.

- `monta_o2.cpp`  
  Montador — 2ª passagem:
  - Lê `<nome>.o1`.
  - Usa a TS para resolver pendências.
  - Converte mnemônicos + operandos em código objeto numérico.
  - Saída: **`<nome>.o2`**.

- `compilador.cpp`  
  Arquivo “driver”:
  - Compila e executa automaticamente:
    1. `pre_processamento.cpp`
    2. `monta_o1.cpp`
    3. `monta_o2.cpp`
  - Faz o encadeamento:
    ```text
    <nome>.asm → <nome>.pre → <nome>.o1 → <nome>.o2
    ```

---

## Como Compilar e Executar

### 1. Executar o compilador

Na pasta do projeto:

```bash
./compilador nome_arquivo.asm

```
Isso irá gerar na mesma pasta:
1. nome_arquivo.pre
2. nome_arquivo.o1
3. nome_arquivo.o2

### Arquivos de Teste

No repositório estão incluídos:

test.asm -> exemplo sem erros, usado para validar o fluxo completo:
test2.asm -> exemplo com erros léxicos, sintáticos e semânticos, usado para testar:

Para compilar o compilador:
```bash
g++ -std=c++17 -O2 -Wall -Wextra -o compilador compilador.cpp
```

## Tecnologias Utilizadas

Linguagem: C++17

Compilador: g++

Ambiente alvo: Linux, Windows

Organização modular:

cada etapa do processo (pré-processamento, 1ª passagem, 2ª passagem) em um arquivo separado.

## Repositório do projeto:
[text](https://github.com/danielvesantana/Software-Basico)

