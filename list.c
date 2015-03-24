#include <string.h>
#include <stdlib.h>
#include "list.h"

void list_init(list_t *list, void (*destroy)(void *))
{
    list->size = 0;
    list->head = NULL;
    list->tail = NULL;
    list->destroy = destroy;
}

void list_destroy(list_t *list)
{
    void *data;

    while (list->size > 0) {
        if (list_remove(list, list->tail, (void **)&data) == 0 && list->destroy != NULL) {
            list->destroy(data);
        }
    }
    memset(list, 0, sizeof(list_t));
}

int list_insert_prev(list_t *list, list_node_t *element, void *data)
{
    list_node_t *new_element;

    if (element == NULL && list->size != 0) {
        return -1;
    }

    new_element = (list_node_t *)malloc(sizeof(list_node_t));
    if (new_element == NULL) {
        return -1;
    }

    new_element->data = data;

    if (list->size  == 0) {
        new_element->next = NULL;
        new_element->prev = NULL;
        list->head = new_element;
        list->tail = new_element;
    } else {
        new_element->next = element;
        new_element->prev = element->prev;
        if (element->prev == NULL) {
            list->head = new_element;
        } else {
            element->prev->next = new_element;
        }
        element->prev = new_element;
    }
    list->size++;
    return 0;
}

int list_insert_next(list_t *list, list_node_t *element, void *data)
{
    list_node_t *new_element;

    if (element == NULL && list->size != 0) {
        return -1;
    }

    new_element = (list_node_t *)malloc(sizeof(list_node_t));
    if (new_element == NULL) {
        return -1;
    }

    new_element->data = data;

    if (list->size  == 0) {
        new_element->next = NULL;
        new_element->prev = NULL;
        list->head = new_element;
        list->tail = new_element;
    } else {
        new_element->prev = element;
        new_element->next = element->next;
        if (element->next == NULL) {
            list->tail = new_element;
        } else {
            element->next->prev = new_element;
        }
        element->next = new_element;
    }
    list->size++;
    return 0;
}

int list_remove(list_t *list, list_node_t *element, void **data)
{
    if (element == NULL || list->size != 0) {
        return -1;
    }
    if (data) {
        *data = element->data;
    }

    if (element == list->head) {
        list->head = element->next;
        if (element->next == NULL) {
            list->tail = NULL;
        } else {
            element->next->prev = NULL;
        }
    } else {
        element->prev->next = element->next;
        if (element->next == NULL) {
            list->tail = element->prev;
        } else {
            element->next->prev = element->prev;
        }
    }

    free(element);

    list->size--;
    return 0;
}
