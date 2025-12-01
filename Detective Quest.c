#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#define MAX_NOME 50
#define MAX_PISTA 120
#define HASH_SIZE 101    


// nó da árvore de cômodos
typedef struct Sala {
    char nome[MAX_NOME];
    char pista[MAX_PISTA];     // pista associada (string vazia = sem pista)
    struct Sala *esquerda;
    struct Sala *direita;
} Sala;

// nó da BST de pistas coletadas
typedef struct PistaNode {
    char pista[MAX_PISTA];
    struct PistaNode *esquerda;
    struct PistaNode *direita;
} PistaNode;


typedef struct HashNode {
    char *chave;       // pista
    char *suspeito;    // nome do suspeito
    struct HashNode *next;
} HashNode;


typedef struct HashTable {
    HashNode *buckets[HASH_SIZE];
} HashTable;

// cria uma sala
Sala* criarSala(const char nome[], const char pista[]);

// anda pela árvore (mansão) e ativa o sistema de pistas
void explorarSalas(Sala* atual, PistaNode **bstRoot, HashTable *ht);

// insere a pista coletada na árvore de pistas
PistaNode* inserirPista(PistaNode *root, const char pista[]);

// exibe todas as pistas
void exibirPistas(PistaNode *root);

// insere associação pista -> suspeito na tabela hash
void inserirNaHash(HashTable *ht, const char pista[], const char suspeito[]);

// consulta o suspeito correspondente a uma pista (retorna NULL se não existir)
char* encontrarSuspeito(HashTable *ht, const char pista[]);

// verifica se o suspeito acusado possui pelo menos 2 pistas entre as coletadas e exibe o resultado do julgamento
void verificarSuspeitoFinal(PistaNode *bstRoot, HashTable *ht, const char acusado[]);

unsigned long hash_str(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash % HASH_SIZE;
}


char* my_strdup(const char *s) {
    if (!s) return NULL;
    char *r = malloc(strlen(s) + 1);
    if (r) strcpy(r, s);
    return r;
}

/* -----------------------------
   Implementação das funções principais
   ----------------------------- */

/*
 criarSala() – cria dinamicamente um quarto.
 cria um nó sala com nome e pista (pista pode ser string vazia).
 retorna ponteiro para a Sala alocada.
*/
Sala* criarSala(const char nome[], const char pista[]) {
    Sala *s = (Sala*) malloc(sizeof(Sala));
    if (!s) {
        fprintf(stderr, "Erro de alocacao para sala.\n");
        exit(EXIT_FAILURE);
    }
    strncpy(s->nome, nome, MAX_NOME-1);
    s->nome[MAX_NOME-1] = '\0';
    if (pista && strlen(pista) > 0) {
        strncpy(s->pista, pista, MAX_PISTA-1);
        s->pista[MAX_PISTA-1] = '\0';
    } else {
        s->pista[0] = '\0';
    }
    s->esquerda = s->direita = NULL;
    return s;
}

/*
 inserirPista() / adicionarPista() – insere a pista coletada na árvore de pistas.
 Insere em BST por ordem alfabética. Não insere duplicatas (ignora igualdade exata).
*/
PistaNode* inserirPista(PistaNode *root, const char pista[]) {
    if (pista == NULL || strlen(pista) == 0) return root; // nada a inserir
    if (root == NULL) {
        PistaNode *novo = (PistaNode*) malloc(sizeof(PistaNode));
        if (!novo) { fprintf(stderr, "Erro de alocacao para PistaNode.\n"); exit(EXIT_FAILURE); }
        strncpy(novo->pista, pista, MAX_PISTA-1);
        novo->pista[MAX_PISTA-1] = '\0';
        novo->esquerda = novo->direita = NULL;
        return novo;
    }
    int cmp = strcmp(pista, root->pista);
    if (cmp < 0) root->esquerda = inserirPista(root->esquerda, pista);
    else if (cmp > 0) root->direita = inserirPista(root->direita, pista);
    // se igual, já existe: não insere duplicata
    return root;
}

/*
 inserirNaHash() – insere associação pista/suspeito na tabela hash.
 Se a chave já existir, sobrescreve o suspeito (comportamento simples).
*/
void inserirNaHash(HashTable *ht, const char pista[], const char suspeito[]) {
    if (!ht || !pista) return;
    unsigned long idx = hash_str(pista);
    HashNode *node = ht->buckets[idx];
    // procurar se já existe
    while (node) {
        if (strcmp(node->chave, pista) == 0) {
            // sobrescrever suspeito
            free(node->suspeito);
            node->suspeito = my_strdup(suspeito);
            return;
        }
        node = node->next;
    }
    // inserir novo nó no bucket (no início)
    HashNode *novo = (HashNode*) malloc(sizeof(HashNode));
    if (!novo) { fprintf(stderr, "Erro de alocacao para HashNode.\n"); exit(EXIT_FAILURE); }
    novo->chave = my_strdup(pista);
    novo->suspeito = my_strdup(suspeito);
    novo->next = ht->buckets[idx];
    ht->buckets[idx] = novo;
}

/*
 encontrarSuspeito() – consulta o suspeito correspondente a uma pista.
 Retorna ponteiro para string com o suspeito (armazenado na hash) ou NULL se não achar.
*/
char* encontrarSuspeito(HashTable *ht, const char pista[]) {
    if (!ht || !pista) return NULL;
    unsigned long idx = hash_str(pista);
    HashNode *node = ht->buckets[idx];
    while (node) {
        if (strcmp(node->chave, pista) == 0)
            return node->suspeito;
        node = node->next;
    }
    return NULL;
}

/*
 explorarSalas() – navega pela árvore e ativa o sistema de pistas.
 AO ENTRAR numa sala, se houver pista, exibe e insere na BST de pistas (evita duplicatas).
 O usuário escolhe 'e' (esquerda), 'd' (direita) ou 's' (sair).
*/
void explorarSalas(Sala* atual, PistaNode **bstRoot, HashTable *ht) {
    if (!atual) return;
    char escolha;
    while (1) {
        printf("\nVocê entrou em: %s\n", atual->nome);

        // coleta de pista (se existir)
        if (atual->pista && strlen(atual->pista) > 0) {
            printf("Pista encontrada: %s\n", atual->pista);
            // inserir na BST de pistas (não duplica)
            *bstRoot = inserirPista(*bstRoot, atual->pista);
        } else {
            printf("Nenhuma pista nesta sala.\n");
        }

        // mostrar opções
        printf("Opções: esquerda (e), direita (d), sair (s)\n");
        printf("Escolha: ");
        if (scanf(" %c", &escolha) != 1) {
            // entrada inválida: limpar e repetir
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) ;
            printf("Entrada inválida. Tente novamente.\n");
            continue;
        }

        if (escolha == 'e' || escolha == 'E') {
            if (atual->esquerda) atual = atual->esquerda;
            else printf("Não há caminho à esquerda.\n");
        } else if (escolha == 'd' || escolha == 'D') {
            if (atual->direita) atual = atual->direita;
            else printf("Não há caminho à direita.\n");
        } else if (escolha == 's' || escolha == 'S') {
            printf("Exploração encerrada pelo jogador.\n");
            return;
        } else {
            printf("Opção inválida.\n");
        }
    }
}


void exibirPistas(PistaNode *root) {
    if (!root) return;
    exibirPistas(root->esquerda);
    printf("- %s\n", root->pista);
    exibirPistas(root->direita);
}

// percorre a BST e conta quantas pistas apontam para 'suspeitoAlvo'
// retorna contagem (inteiro)
int contarPistasParaSuspeito(PistaNode *root, HashTable *ht, const char *suspeitoAlvo) {
    if (!root) return 0;
    int count = 0;
    // esquerda
    count += contarPistasParaSuspeito(root->esquerda, ht, suspeitoAlvo);
    // raiz
    char *s = encontrarSuspeito(ht, root->pista);
    if (s != NULL && strcmp(s, suspeitoAlvo) == 0) count++;
    // direita
    count += contarPistasParaSuspeito(root->direita, ht, suspeitoAlvo);
    return count;
}

/*
 verificarSuspeitoFinal() – conduz à fase de julgamento final.
 recebe a BST de pistas coletadas (mapa pista->suspeito) e o acusado.
 conta quantas pistas apontam para o acusado. Se >= 2 -> acusacao sustentada.
*/
void verificarSuspeitoFinal(PistaNode *bstRoot, HashTable *ht, const char acusado[]) {
    if (!acusado || strlen(acusado) == 0) {
        printf("Nome do acusado inválido.\n");
        return;
    }
    int matches = contarPistasParaSuspeito(bstRoot, ht, acusado);
    printf("\nResultado da investigação para: %s\n", acusado);
    printf("Pistas que apontam para %s: %d\n", acusado, matches);
    if (matches >= 2) {
        printf("Decisão: Acusação SUSTENTADA! Há evidências suficientes.\n");
    } else {
        printf("Decisão: Acusação NÃO SUSTENTADA. Pistas insuficientes.\n");
    }
}



void free_pistas(PistaNode *root) {
    if (!root) return;
    free_pistas(root->esquerda);
    free_pistas(root->direita);
    free(root);
}

void free_sala_tree(Sala *root) {
    if (!root) return;
    free_sala_tree(root->esquerda);
    free_sala_tree(root->direita);
    free(root);
}

void free_hash(HashTable *ht) {
    for (int i = 0; i < HASH_SIZE; ++i) {
        HashNode *n = ht->buckets[i];
        while (n) {
            HashNode *up = n->next;
            free(n->chave);
            free(n->suspeito);
            free(n);
            n = up;
        }
        ht->buckets[i] = NULL;
    }
}

   /* -----------------------------
   MAIN: monta mansão fixa, tabela hash e inicia exploração
   ----------------------------- */
int main() {
   
    HashTable ht;
    for (int i = 0; i < HASH_SIZE; ++i) ht.buckets[i] = NULL;

    // criando a mansão
    // cada sala tem uma pista estática (ou string vazia)
    Sala *hall        = criarSala("Hall de Entrada",     "Pegada de sapato");
    Sala *estar       = criarSala("Sala de Estar",       "Vidro com impressão");
    Sala *biblioteca  = criarSala("Biblioteca",         "Carta rasgada");
    Sala *cozinha     = criarSala("Cozinha",            "Copo quebrado");
    Sala *jogos       = criarSala("Sala de Jogos",      "");
    Sala *jardim      = criarSala("Jardim",             "Luvas sujas");
    Sala *escritorio  = criarSala("Escritório",         "Documento suspeito");
    Sala *adega       = criarSala("Adega",              "Garrafa com resíduos");
    Sala *quarto      = criarSala("Quarto",             "Cabelo loiro");
    Sala *banheiro    = criarSala("Banheiro",           "Pó branco");

    // Montagem manual da árvore (um exemplo razoável)
    //                hall
    //              /      \
    //           estar    biblioteca
    //          /   \     /      \
    //      cozinha jogos jardim  escritorio
    //      /   \
    //   adega quarto
    //            \
    //           banheiro

    hall->esquerda = estar;
    hall->direita  = biblioteca;

    estar->esquerda = cozinha;
    estar->direita  = jogos;

    cozinha->esquerda = adega;
    cozinha->direita = quarto;

    quarto->direita = banheiro;

    biblioteca->esquerda = jardim;
    biblioteca->direita = escritorio;

   

  
    inserirNaHash(&ht, "Pegada de sapato", "Sr. Silva");
    inserirNaHash(&ht, "Vidro com impressão", "Sra. Pereira");
    inserirNaHash(&ht, "Carta rasgada", "Dr. Ramos");
    inserirNaHash(&ht, "Copo quebrado", "Sra. Pereira");
    inserirNaHash(&ht, "Luvas sujas", "Sr. Costa");
    inserirNaHash(&ht, "Documento suspeito", "Dr. Ramos");
    inserirNaHash(&ht, "Garrafa com resíduos", "Sr. Costa");
    inserirNaHash(&ht, "Cabelo loiro", "Sra. Rodrigues");
    inserirNaHash(&ht, "Pó branco", "Sra. Rodrigues");

    // BST para pistas coletadas (vazia no inicio)
    PistaNode *bstPistas = NULL;

    // mensagem inicial
    printf("=== Detective Quest - Nível Mestre ===\n");
    printf("Explore a mansão, recolha pistas e acuse o suspeito.\n");

    // exploração
    explorarSalas(hall, &bstPistas, &ht);

    // exibir todas as pistas coletadas
    printf("\n=== Pistas coletadas (ordem alfabética) ===\n");
    if (bstPistas == NULL) {
        printf("Nenhuma pista foi coletada.\n");
    } else {
        exibirPistas(bstPistas);
    }

    //  pedir nome do suspeito
    char acusado[80];
    printf("\nQuem você acusa como culpado? (digite o nome completo): ");
  
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF); // limpar buffer restante
    if (fgets(acusado, sizeof(acusado), stdin) != NULL) {
     
        size_t ln = strlen(acusado);
        if (ln > 0 && acusado[ln-1] == '\n') acusado[ln-1] = '\0';
    } else {
        acusado[0] = '\0';
    }


 

    // Verificar se há pelo menos 2 pistas apontando para o acusado
    verificarSuspeitoFinal(bstPistas, &ht, acusado);


    free_pistas(bstPistas);
    free_sala_tree(hall); 
    free_hash(&ht);

    printf("\nObrigado por jogar Detective Quest!\n");
    return 0;
}
