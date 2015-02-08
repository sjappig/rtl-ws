#include <stdlib.h>
#include <stdint.h>
#include "list.h"

struct _callback_node
{
    void* data;
    struct _callback_node* next;
};

struct list
{
    struct _callback_node* root;
};

struct list* list_alloc()
{
    return (struct list*) calloc(1, sizeof(struct list));
}

void list_add(struct list* l, void* data)
{
    struct _callback_node* node = l->root;

    if (node != NULL)
    {
        while (node->next != NULL)
        {
            node = node->next;
        }
        node->next = (struct _callback_node*) calloc(1, sizeof(struct _callback_node));
        node->next->data = data;
    }
    else
    {
        l->root = (struct _callback_node*) calloc(1, sizeof(struct _callback_node));
        l->root->data = data;
    }
}

void list_apply(struct list* l, void (*func)(void*,void*), void* user)
{
    struct _callback_node* node = l->root;

    while (node != NULL)
    {
        func(node->data, user);
        node = node->next;
    }
}

void list_clear(struct list* l)
{
    struct _callback_node* node = l->root;
    struct _callback_node* prev_node = NULL;

    while (node != NULL)
    {
        prev_node = node;
        node = node->next;
        free(prev_node);
    }

    l->root = NULL;
}

void list_free(struct list* l)
{
    free(l);
}