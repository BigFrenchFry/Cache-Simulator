#include "queue.h"

Queue_t* queueConstructor() {
  
  Queue_t *newQueue;
  newQueue = malloc(sizeof(Queue_t));
  newQueue->size = 0;
  newQueue->first = NULL;
  newQueue->last = NULL;
  
  return newQueue;
}

void enqueue(Queue_t *queue, int tag, int index) {
  Node_t *newNode;
  newNode = malloc(sizeof(Node_t));
  newNode->tag = tag;
  newNode->index = index;
  if (queue->size) {
    (queue->last)->next = newNode;
    (queue->last) = newNode;
    ++(queue->size);
  } else {
    (queue->first) = newNode;
    (queue->last) = newNode;
    ++(queue->size);
  }
}

void queuePoll(Queue_t *queue, int *dst) {

  if (!(queue->size)) { return; }
  dst[0] = (queue->first)->tag;
  dst[1] = (queue->first)->index;

  Node_t *oldFirst;
  Node_t *newFirst;
  oldFirst = (queue->first);
  if ((queue->size) > 1) {
    newFirst = (queue->first)->next;
  } else {
    newFirst = NULL;
  }
  free(oldFirst);
  (queue->first) = newFirst;

  --(queue->size);
  
  if (!(queue->size) || (queue->size) == 1) {
    (queue->last) = (queue->first); 
  }
}

void queuePeek(Queue_t *queue, int *dst) {
  if (!(queue->size)) { return; }
  
  dst[0] = (queue->first)->tag;
  dst[1] = (queue->first)->index;
}

int queueContainsBlockRec(Node_t *node, int tag, int index, int itr, int max) {
  if (itr < max) { 
    if ((node->tag) == tag && (node->index) == index) {
      return 1;
    } else {
      return queueContainsBlockRec(node->next, tag, index, itr+1, max);
    }
  } else {
    return 0;
  }
}

int queueContainsBlock(Queue_t *queue, int tag, int index) {
  if (!(queue->size)) { return 0; }
  return queueContainsBlockRec(queue->first, tag, index, 0, queue->size);
}

int queueFirstInstanceOfIndexRec(Node_t *node, int index, int itr, int max) {
  if (itr < max) { 
    if (node->index == index) {
      return (node->tag);
    } else {
      return queueFirstInstanceOfIndexRec(node->next, index, itr+1, max);
    }
  } else {
    return 0;
  }
}

int queueFirstInstanceOfIndex(Queue_t *queue, int index) {
  if (!(queue->size)) { return 0; }
  return queueFirstInstanceOfIndexRec(queue->first, index, 0, queue->size);
}

void queueRemoveBlockRec(Queue_t *queue, Node_t *node, int tag, int index, int itr, int max) {
  if (itr+1 < max) { 
    if ((node->next)->tag == tag) {
      Node_t *oldNext;
      oldNext = node->next;
      Node_t *newNext;
      newNext = (node->next)->next;
      free(oldNext);
      (node->next) = newNext;

      --(queue->size);
      if (itr+1 == max-1) {
        (queue->last) = newNext;
      }
    } else {
      queueRemoveBlockRec(queue, node->next, tag, index, itr+1, max);
    }
  } else {
    fprintf(stderr, "queueRemove likely messed up somewhere\n");
  }
}

void queueRemoveBlock(Queue_t *queue, int tag, int index) {
  if (!(queue->size)) { return; }
  if ((queue->first)->tag == tag) {
    int t[2];
    queuePoll(queue, t);
  } else {
    queueRemoveBlockRec(queue, queue->first, tag, index, 0, queue->size);
  }
}

int queueEmpty(Queue_t *queue) {
  return !(queue->size);
}

void queueCleanup(Queue_t *queue) {
  while ((queue->size)) {
    int t[2];
    queuePoll(queue, t);
  }
  free(queue);
}

