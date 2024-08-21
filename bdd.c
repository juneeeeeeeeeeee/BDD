#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_NODES 10000

// BDD 노드 구조체 정의
typedef struct _BDDNode {
    int var;  // 변수 이름. 나중에 문자열로 변경됨. 
    struct _BDDNode *low;  // 0-edge
    struct _BDDNode *high; // 1-edge
    int value;  // 단말 노드 값 (0 또는 1)
    int index;
} BDDNode;

BDDNode* unique_nodes[MAX_NODES];
int unique_node_count = 0;

char** input_var;
char** output_var;

// 새로운 단말 노드 생성
BDDNode* create_terminal(int value) {
    BDDNode* node = (BDDNode*)malloc(sizeof(BDDNode));
    node->var = -1;
    node->low = NULL;
    node->high = NULL;
    node->value = value;
    node->index = value;
    unique_nodes[unique_node_count++] = node;
    return node;
}

// 새로운 내부 노드 생성
BDDNode* create_node(int var, BDDNode* low, BDDNode* high) {
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
    int top_var = f->var;
    if (g->value==-1 && g->var < top_var) top_var = g->var; // terminal 제외
    if (h->value==-1 && h->var < top_var) top_var = h->var; // terminal 제외
    
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

void write_node_and_edges(FILE* file, BDDNode* node, int* visited, int* visited_count) {
    for (int i = 0; i < *visited_count; i++) {
        if (visited[i] == node->index) {
            return;
        }
    }
    // 현재 노드를 방문한 것으로 표시
    visited[(*visited_count)++] = node->index;
    if (node->value != -1) {
        fprintf(file, "    node%d [label=\"%d\", shape=box];\n", node->index, node->value);
    }
    else {
        fprintf(file, "    node%d [label=\"%s\"];\n", node->index, input_var[node->var]);
        write_node_and_edges(file, node->low, visited, visited_count);
        write_node_and_edges(file, node->high, visited, visited_count);
        fprintf(file, "    node%d -> node%d [label=\"0\"];\n", node->index, node->low->index);
        fprintf(file, "    node%d -> node%d [label=\"1\"];\n", node->index, node->high->index);
    }
}


// BDD를 DOT 파일로 변환하여 저장하는 함수
void write_bdd_to_dot(BDDNode* root1, BDDNode* root2, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Unable to open file");
        return;
    }
    int visited[MAX_NODES];
    int visited_count = 0;
    fprintf(file, "digraph BDD {\n");
    write_node_and_edges(file, root1, visited, &visited_count);
    write_node_and_edges(file, root2, visited, &visited_count);
    printf("total nodes: %d\n", visited_count);
    fprintf(file, "    node%d [label=\"%s\", style=filled, fillcolor=white, color=transparent];\n", unique_node_count, output_var[0]);
    fprintf(file, "    node%d [label=\"%s\", style=filled, fillcolor=white, color=transparent];\n", unique_node_count + 1, output_var[1]);
    fprintf(file, "    node%d -> node%d [dir=none];\n", unique_node_count, root1->index);
    fprintf(file, "    node%d -> node%d [dir=none];\n", unique_node_count + 1, root2->index);
    fprintf(file, "}\n");
    fclose(file);

    if (system("dot -Tpng bdd.dot -o graph.png") != 0) {
        fprintf(stderr, "Error: could not convert DOT to PNG\n");
    }
}

// BDD 생성 함수
BDDNode* build_bdd_from_truth_table(int** truthTable, int rows, int cols, BDDNode* terminal_0, BDDNode* terminal_1) {
    if (rows == 0) {
        return NULL;
    }

    BDDNode* root = create_node(0, NULL, NULL);

    for (int i = 0; i < rows; i++) {
        BDDNode* current = root;
        for (int j = 0; j < cols - 2; j++) {
            if (truthTable[i][j] == 0) {
                if (current->low == NULL) {
                    current->low = create_node(j + 1, NULL, NULL);
                }
                current = current->low;
            } else {
                if (current->high == NULL) {
                    current->high = create_node(j + 1, NULL, NULL);
                }
                current = current->high;
            }
        }
        if (truthTable[i][cols-2] == 0) {
            current->low = (truthTable[i][cols - 1] == 0) ? terminal_0 : terminal_1;
        } else {
            current->high = (truthTable[i][cols - 1] == 0) ? terminal_0 : terminal_1;
        }
    }
    return root;
}

// 진리표 파일 읽기 함수
int** read_truth_table(const char* filename, int* rows, int* cols) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    int input_count = 0;
    static int output_count;
    fgets(line, sizeof(line), file);

    // input, output 변수 이름 저장
    char *token = strtok(line, " ");
    char *last_token = NULL;

    while (token != NULL) {
        last_token = token;  // 현재 토큰을 마지막으로 기록
        token = strtok(NULL, " ");
        if (token != NULL) {  // 다음 토큰이 NULL이 아닌 경우 input_var에 저장
            input_var = (char**)realloc(input_var, sizeof(char*) * (input_count + 1));
            input_var[input_count++] = strdup(last_token);  // 문자열 복사
        }
    }

    // 마지막 토큰 개행 문자 제거
    if (last_token != NULL) {
        size_t len = strlen(last_token);
        if (last_token[len - 1] == '\n') {
            last_token[len - 1] = '\0';
        }
    }

    output_var = (char**)realloc(output_var, sizeof(char*) * (output_count + 1));
    output_var[output_count++] = strdup(last_token);  // 문자열 복사

    int** truthTable = (int**)malloc(sizeof(int*) * 512);
    *rows = 0;
    *cols = 0;

    while (fgets(line, sizeof(line), file)) {
        // 줄 바꿈 문자 제거
        line[strcspn(line, "\r\n")] = 0;

        truthTable[*rows] = (int*)malloc(sizeof(int) * 10);
        char* token = strtok(line, " ");
        int col = 0;
        char *endptr;

        while (token != NULL) {
            truthTable[*rows][col++] = strtol(token, &endptr, 10);
            if (*endptr != '\0' && *endptr != '\n' && *endptr != '\r') {
                fprintf(stderr, "Conversion error, non-convertible part: %s\n", endptr);
                exit(EXIT_FAILURE);
            }
            token = strtok(NULL, " ");
        }
        *cols = col;
        (*rows)++;
    }

    fclose(file);

    return truthTable;
}

int main() {
    // 단말 노드 생성
    BDDNode* terminal_0 = create_terminal(0);
    BDDNode* terminal_1 = create_terminal(1);

    // 파일 읽기
    int rows, cols;
    char* filename = "s3_table.txt";
    int** truthTable = read_truth_table(filename, &rows, &cols);
    printf("truthTable done, rows: %d, cols: %d\n", rows, cols);
    BDDNode* bdd1 = build_bdd_from_truth_table(truthTable, rows, cols, terminal_0, terminal_1);
    printf("truthTable to bdd done\n");
    BDDNode* root1 = ite(bdd1, terminal_1, terminal_0);
    printf("bdd to reduced bdd done\n");

    // 다른 파일 읽기
    filename = "carry_table.txt";
    truthTable = read_truth_table(filename, &rows, &cols);
    printf("truthTable done, rows: %d, cols: %d\n", rows, cols);
    BDDNode* bdd2 = build_bdd_from_truth_table(truthTable, rows, cols, terminal_0, terminal_1);
    printf("truthTable to bdd done\n");
    BDDNode* root2 = ite(bdd2, terminal_1, terminal_0);
    printf("bdd to reduced bdd done\n");
    write_bdd_to_dot(root1, root2, "bdd.dot");

    // 메모리 해제
    for(int i=0;i<unique_node_count;i++)
    {
        free(unique_nodes[i]);
    }
    for(int i=0;i<=rows-1;i++)
    {
        free(truthTable[i]);
    }
    free(truthTable);
    for(int i=0;i<=cols - 1;i++)
    {
        free(input_var[i]);
    }
    free(input_var);
    for(int i=0;i<=1;i++) // can be changed if output variable changes
    {
        free(output_var[i]);
    }
    free(output_var);
    // order 변경 기능 아직 안넣음
    return 0;
}