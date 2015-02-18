#ifndef LIST_H
#define LIST_H

struct list;

struct list* list_alloc();

void list_add(struct list* l, void* data);

void list_apply(struct list* l, void (*func)(void*));

void list_apply2(struct list* l, void (*func)(void*,void*), void* user);

int list_length(const struct list* l);

void* list_peek(struct list* l);

void* list_poll(struct list* l);

void list_poll_to_list(struct list* from, struct list* to);

void list_clear(struct list* l);

void list_free(struct list* l);

#endif