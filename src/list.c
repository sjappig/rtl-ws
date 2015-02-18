#include <stdlib.h>
#include "list.h"

struct _node
{
    void* data;
    struct _node* next;
};

struct list
{
    struct _node* root;
    volatile int len;
};

static inline void internal_list_add(struct list* l, struct _node* new_node)
{
    struct _node* node = l->root;

    if (node != NULL)
    {
        while (node->next != NULL)
        {
            node = node->next;
        }
        node->next = new_node;
    }
    else
    {
        l->root = new_node;
    }
    l->len++;
}

struct list* list_alloc()
{
    return (struct list*) calloc(1, sizeof(struct list));
}

void list_add(struct list* l, void* data)
{
    struct _node* node = (struct _node*) calloc(1, sizeof(struct _node));
    node->data = data;
    internal_list_add(l, node);
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

void* list_peek(struct list* l)
{
    struct _node* node = l->root;
    void* data = NULL;

    if (node != NULL)
    {
        data = node->data;
    }

    return data;
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

void list_poll_to_list(struct list* from, struct list* to)
{
    struct _node* node = from->root;

    if (node != NULL)
    {
        from->root = node->next;
        from->len--;
        node->next = NULL;
        internal_list_add(to, node);
    }
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