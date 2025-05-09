#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define MAX_LINE_LENGTH 1024
#define MAX_QUEUE_SIZE 100
#define NUM_PRODUCERS 4  // 生产者数量
#define NUM_CONSUMERS 4  // 消费者数量

// 队列结构
typedef struct {
    char* items[MAX_QUEUE_SIZE]; // 队列缓冲数组
    int front; // 队头index
    int rear; // 队尾index
    int enqueued;           // 入队计数器
    int dequeued;           // 出队计数器
    int done_sending;       // 已完成的生产者数量
} Queue;

// 初始化队列
void initQueue(Queue* q) {
    q->front = 0;
    q->rear = -1;
    q->enqueued = 0;
    q->dequeued = 0;
    q->done_sending = 0;
    // for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
    //     q->items[i] = NULL;
    // }
}
// 销毁队列
void destroyQueue(Queue* q) {
    for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
        free(q->items[i]);
    }
}
int getQueueSize(Queue* q) {
    int queue_size;
    int temp_enqueued, temp_dequeued;
    #pragma omp atomic read
    temp_enqueued = q->enqueued;
    #pragma omp atomic read
    temp_dequeued = q->dequeued;
    queue_size = temp_enqueued - temp_dequeued;
    return queue_size;
}
// 入队
int enqueue(Queue* q, const char* item) {
    int success = 0;
    int queue_size = getQueueSize(q);
    
    if (queue_size < MAX_QUEUE_SIZE) {
        #pragma omp critical
        {
            q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
            q->items[q->rear] = strdup(item);
            #pragma omp atomic
            q->enqueued++;
            success = 1;
        }
    }
    return success;
}

// 出队
char* dequeue(Queue* q) {
    char* item = NULL;
    int queue_size = getQueueSize(q);
    // 当队列为空时返回
    if (queue_size == 0) {
        return NULL;
    } else if (queue_size == 1) {
        #pragma omp critical
        {
            if (q->enqueued > q->dequeued) {
                item = q->items[q->front];
                q->items[q->front] = NULL;
                q->front = (q->front + 1) % MAX_QUEUE_SIZE;
                #pragma omp atomic
                q->dequeued++;
            }
        }
    } else {
        // 当有两个以上时，由于变量分离，不是临界区
        item = q->items[q->front];
        q->items[q->front] = NULL;
        q->front = (q->front + 1) % MAX_QUEUE_SIZE;
        #pragma omp atomic
        q->dequeued++;
    }
    return item;
}

// 生产者函数
void producer(Queue* q, const char* filename, int id) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Producer %d: Cannot open file %s\n", id, filename);
        return;
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        // 去除换行符
        line[strcspn(line, "\n")] = 0;
        printf("producer%d: %s\n", id, line);
        // 当队列满时等待
        while (!enqueue(q, line)) {
            #pragma omp taskyield
        }
    }
    fclose(file);
    #pragma omp atomic
    q->done_sending++;
}

// 分词函数
void tokenize(const char* line, int id) {
    char* copy = strdup(line);
    char* token = strtok(copy, " \t\n");
    while (token) {
        printf("consumer%d: %s\n", id, token);
        token = strtok(NULL, " \t\n");
    }
    free(copy);
}

// 消费者函数
void consumer(Queue* q, int id) {
    char* line;
    while (1) {
        line = dequeue(q);
        if (line) {
            tokenize(line, id);
        } else {
            int queue_size;
            int temp_enqueued, temp_dequeued;
            #pragma omp atomic read
            temp_enqueued = q->enqueued;
            #pragma omp atomic read
            temp_dequeued = q->dequeued;
            queue_size = temp_enqueued - temp_dequeued;
            int done_count;
            #pragma omp atomic read
            done_count = q->done_sending;
            
            if (queue_size == 0 && done_count == NUM_PRODUCERS) {
                break;
            }
            #pragma omp taskyield
        }
    }
    free(line);
}

int main() {
    omp_set_num_threads(NUM_PRODUCERS + NUM_CONSUMERS);
    Queue queue;
    initQueue(&queue);
    const char* files[NUM_PRODUCERS] = {
        "file1.txt", "file2.txt", "file3.txt", "file4.txt"
    };

    #pragma omp parallel
    {
        int id = omp_get_thread_num();
        if (id < NUM_PRODUCERS) {
            // 生产者线程
            producer(&queue, files[id], id);
        } else {
            // 消费者线程
            consumer(&queue, id - NUM_PRODUCERS);
        }
    }

    printf("总共计数：%d，应计数150*4=600\n", queue.enqueued);
    destroyQueue(&queue);
    return 0;
}