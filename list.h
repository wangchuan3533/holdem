#ifndef _LIST_H
#define _LIST_H
typedef struct list_node_s {
    void *data;
    struct list_node_s *next;
    struct list_node_s *prev;
} list_node_t;

typedef struct list_s {
    list_node_t *head;
    list_node_t *tail;
    int size;
    void (*destroy)(void *);
} list_t;

void list_init(list_t *list, void (*destroy)(void *));
void list_destroy(list_t *list);
int list_insert_prev(list_t *list, list_node_t *element, void *data);
int list_insert_next(list_t *list, list_node_t *element, void *data);
int list_remove(list_t *list, list_node_t *element, void **data);

#endif

