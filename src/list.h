#ifndef LIST_H
#define LIST_H

struct list;

struct list* list_alloc();

void list_add(struct list* l, void* data);

void list_apply(struct list* l, void (*func)(void*,void*), void* user);

void list_clear(struct list* l);

void list_free(struct list* l);

#endif