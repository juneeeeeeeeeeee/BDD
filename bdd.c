#include <stdio.h>
#include <stdlib.h>
#define MAX_NODES 1000

// BDD 노드 구조체 정의
typedef struct BDDNode {
    char var;  // 변수 이름
    struct BDDNode *low;  // 0-edge
    struct BDDNode *high; // 1-edge
    int value;  // 단말 노드 값 (0 또는 1)
    int index;
} BDDNode;

BDDNode* unique_nodes[MAX_NODES];
int unique_node_count = 0;

// 새로운 단말 노드 생성
BDDNode* create_terminal(int value) {
    BDDNode* node = (BDDNode*)malloc(sizeof(BDDNode));
    node->var = 0;
    node->low = NULL;
    node->high = NULL;
    node->value = value;
    node->index = (value) ? 1 : 0;
    unique_nodes[unique_node_count++] = node;
    return node;
}

// 새로운 내부 노드 생성
BDDNode* create_node(char var, BDDNode* low, BDDNode* high) {
    // 동일한 노드가 있는지 확인
    for (int i = 2; i < unique_node_count; i++) { // 0과 1은 terminal node
        if (unique_nodes[i]->var == var && unique_nodes[i]->low == low && unique_nodes[i]->high == high) {
            return unique_nodes[i];
        }
    }
    BDDNode* node = (BDDNode*)malloc(sizeof(BDDNode));
    node->var = var;
    node->low = low;
    node->high = high;
    node->value = -1; // 내부 노드는 값이 없음
    node->index = unique_node_count;
    unique_nodes[unique_node_count++] = node;
    return node;
}

// ITE 연산
BDDNode* ite(BDDNode* f, BDDNode* g, BDDNode* h) {
    // f가 단말 노드인 경우
    if (f->value != -1) {
        return f->value ? g : h;
    }
    // Recursive case
    char top_var = f->var;
    if (g->var!=0 && g->var < top_var) top_var = g->var; // terminal 제외
    if (h->var!=0 && h->var < top_var) top_var = h->var; // terminal 제외
    
    BDDNode* f_high = (f->var == top_var) ? f->high : f;
    BDDNode* f_low = (f->var == top_var) ? f->low : f;
    BDDNode* g_high = (g->var == top_var) ? g->high : g;
    BDDNode* g_low = (g->var == top_var) ? g->low : g;
    BDDNode* h_high = (h->var == top_var) ? h->high : h;
    BDDNode* h_low = (h->var == top_var) ? h->low : h;
    
    BDDNode* t_high = ite(f_high, g_high, h_high);
    BDDNode* t_low = ite(f_low, g_low, h_low);
    if (t_high == t_low) return t_high;
    return create_node(top_var, t_low, t_high);
}

// AND 연산
BDDNode* and_op(BDDNode* f, BDDNode* g, BDDNode* terminal_0) {
    return ite(f, g, terminal_0);
}

// OR 연산
BDDNode* or_op(BDDNode* f, BDDNode* g, BDDNode* terminal_1) {
    return ite(f, terminal_1, g);
}

// NOT 연산
BDDNode* not_op(BDDNode* f, BDDNode* terminal_0, BDDNode* terminal_1) {
    return ite(f, terminal_0, terminal_1);
}

// XOR 연산
BDDNode* xor_op(BDDNode* f, BDDNode* g, BDDNode* terminal_0, BDDNode* terminal_1){
    return ite(f, not_op(g, terminal_0, terminal_1), g);
}


void write_node_and_edges(FILE* file, BDDNode* node, int* visited, int* visited_count) {
    for (int i = 0; i < *visited_count; i++) {
        if (visited[i] == node->index) {
            return;
        }
    }
    // 현재 노드를 방문한 것으로 표시
    visited[*visited_count] = node->index;
    (*visited_count)++;
    if (node->value != -1) {
        fprintf(file, "    \"%p\" [label=\"%d\", shape=box];\n", (void*)node, node->value);
    }
    else {
        fprintf(file, "    \"%p\" [label=\"%c\"];\n", (void*)node, node->var);
        write_node_and_edges(file, node->low, visited, visited_count);
        write_node_and_edges(file, node->high, visited, visited_count);
        fprintf(file, "    \"%p\" -> \"%p\" [label=\"0\"];\n", (void*)node, (void*)node->low);
        fprintf(file, "    \"%p\" -> \"%p\" [label=\"1\"];\n", (void*)node, (void*)node->high);
    }
}


// BDD를 DOT 파일로 변환하여 저장하는 함수
void write_bdd_to_dot(BDDNode* root, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Unable to open file");
        return;
    }
    int visited[MAX_NODES];
    int visited_count = 0;
    fprintf(file, "digraph BDD {\n");
    write_node_and_edges(file, root, visited, &visited_count);
    fprintf(file, "}\n");
    fclose(file);

    if (system("dot -Tpng bdd.dot -o graph.png") != 0) {
        fprintf(stderr, "Error: could not convert DOT to PNG\n");
    }
}

int main() {
    // 단말 노드 생성
    BDDNode* terminal_0 = create_terminal(0);
    BDDNode* terminal_1 = create_terminal(1);

    // 변수 노드 생성
    BDDNode* a = create_node('a', terminal_0, terminal_1);
    BDDNode* b = create_node('b', terminal_0, terminal_1);
    BDDNode* c = create_node('c', terminal_0, terminal_1);
    BDDNode* d = create_node('d', terminal_0, terminal_1);
    
    // Boolean 함수 f(a, b, c, d) = a XOR b XOR c XOR d
    BDDNode* f1 = xor_op(a, b, terminal_0, terminal_1);
    BDDNode* f2 = xor_op(f1, c, terminal_0, terminal_1);
    BDDNode* f = xor_op(f2, d, terminal_0, terminal_1);
    // BDDNode* g = and_op(f2, d, terminal_0);
    write_bdd_to_dot(f, "bdd.dot");

    // 결과 출력
    printf("BDD for f(a, b, c, d) = a XOR b XOR c XOR d:\n");
    printf("Root: %c\n", f->var);
    printf("Low: %c, High: %c\n", f->low->var, f->high->var);

    // 메모리 해제
    for(int i=0;i<unique_node_count;i++)
    {
        free(unique_nodes[i]);
    }
    free(terminal_0);
    free(terminal_1);

    printf("umjunsik\n");

    // order 변경 기능 아직 안넣음
    // text에서 식으로 변환하는 기능 아직 안넣음
    // 진리표에서 bdd로 변환하는 기능 아직 없음
    return 0;
}