#include <stdlib.h>
#include <stdint.h>
#include "list.h"

struct _node
{
    void* data;
    struct _node* next;
};

struct list
{
    struct _node* root;
    int len;
};

struct list* list_alloc()
{
    return (struct list*) calloc(1, sizeof(struct list));
}

void list_add(struct list* l, void* data)
{
    struct _node* node = l->root;

    if (node != NULL)
    {
        while (node->next != NULL)
        {
            node = node->next;
        }
        node->next = (struct _node*) calloc(1, sizeof(struct _node));
        node->next->data = data;
    }
    else
    {
        l->root = (struct _node*) calloc(1, sizeof(struct _node));
        l->root->data = data;
    }
    l->len++;
}

void list_apply(struct list* l, void (*func)(void*))
{
    struct _node* node = l->root;

    while (node != NULL)
    {
        func(node->data);
        node = node->next;
    }
}

void list_apply2(struct list* l, void (*func)(void*,void*), void* user)
{
    struct _node* node = l->root;

    while (node != NULL)
    {
        func(node->data, user);
        node = node->next;
    }
}

int list_length(const struct list* l)
{
    return l->len;
}

void* list_poll(struct list* l)
{
    struct _node* node = l->root;
    void* data = NULL;

    if (node != NULL)
    {
        l->root = node->next;
        l->len--;
        data = node->data;
        free(node);
    }

    return data;
}

void list_clear(struct list* l)
{
    while (list_length(l) > 0)
    {
        list_poll(l);
    }
}

void list_free(struct list* l)
{
    free(l);
}