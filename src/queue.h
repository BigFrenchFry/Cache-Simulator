
#include <stdlib.h>
#include <stdio.h>

typedef struct Node {

  int tag;
  int index;

  struct Node *next;
} Node_t;

typedef struct Queue {

  int size;

  Node_t *first;
  Node_t *last;
} Queue_t;

Queue_t* queueConstructor(void);
void enqueue(Queue_t *queue, int tag, int index);
void queuePoll(Queue_t *queue, int *dst);
void queuePeek(Queue_t *queue, int *dst);
int queueContainsBlock(Queue_t *queue, int tag, int index);
int queueFirstInstanceOfIndex(Queue_t *queue, int index);
void queueRemoveBlock(Queue_t *queue, int tag, int index);
int queueEmpty(Queue_t *queue);
void queueCleanup(Queue_t *queue);

