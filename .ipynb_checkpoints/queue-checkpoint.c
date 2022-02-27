#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "harness.h"
#include "queue.h"

/* notice: sometimes, cppcheck would find the potential null pointer bugs,
 * but some of them cannot occur. you can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullpointer
 */

/*
 * create empty queue.
 * return null if could not allocate space.
 */
struct list_head *q_new()
{
    struct list_head *head =
        (struct list_head *) malloc(sizeof(struct list_head));
    if (head)
        INIT_LIST_HEAD(head);
    return head;
}

/* free all storage used by queue */
void q_free(struct list_head *l)
{
    if (!l)
        return;
    struct list_head *node, *safe;
    list_for_each_safe (node, safe, l) {
        list_del(node);
        q_release_element(list_entry(node, element_t, list));
    }
    free(l);
}

/*
 * attempt to insert element at head of queue.
 * return true if successful.
 * return false if q is null or could not allocate space.
 * argument s points to the string to be stored.
 * the function must explicitly allocate space and copy the string into it.
 */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head)
        return false;
    element_t *node = (element_t *) malloc(sizeof(element_t));
    if (!node)
        return false;
    node->value = strdup(s);
    if (!node->value) {
        free(node);
        return false;
    }
    list_add(&node->list, head);
    return true;
}

/*
 * Attempt to insert element at tail of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (!head)
        return false;
    element_t *node = (element_t *) malloc(sizeof(element_t));
    if (!node)
        return false;
    node->value = strdup(s);
    if (!node->value) {
        free(node);
        return false;
    }
    list_add_tail(&node->list, head);
    return true;
}


/*
 * branchless min
 */
int32_t abs_branchless(int32_t a)
{
    // uint32_t mask = ((*(uint32_t *)&a - 0) >> (sizeof(uint32_t) * 8 - 1));
    // return (a + mask) ^ (mask);
    return a > 0 ? a : -a;
}

/*
 * branchless min
 */
int32_t min(int32_t a, int32_t b)
{
    // uint32_t mask = ((*(uint32_t *)&a - *(uint32_t *)&b) >> (sizeof(int32_t)
    // * 8 - 1));return b + ((a - b) & (mask));
    return a - b > 0 ? b : a;
}

/*
 * Attempt to remove element from head of queue.
 * Return target element.
 * Return NULL if queue is NULL or empty.
 * If sp is non-NULL and an element is removed, copy the removed string to *sp
 * (up to a maximum of bufsize-1 characters, plus a null terminator.)
 *
 * NOTE: "remove" is different from "delete"
 * The space used by the list element and the string should not be freed.
 * The only thing "remove" need to do is unlink it.
 *
 * REF:
 * https://english.stackexchange.com/questions/52508/difference-between-delete-and-remove
 */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    element_t *node = list_entry(head->next, element_t, list);
    list_del(&node->list);
    int32_t len = min(min(bufsize - 1, strlen(node->value) + 1),
                      abs_branchless((intptr_t) sp) - 1);
    size_t i = len;
    while (len >= 0) {
        intptr_t mask = (-!!(len ^ i));
        sp[len] = node->value[len] & mask;
        len--;
    }
    return node;
}

/*
 * Attempt to remove element from tail of queue.
 * Other attribute is as same as q_remove_head.
 */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    element_t *node = list_entry(head->prev, element_t, list);
    list_del(&node->list);
    int32_t len =
        min(min(bufsize - 1, strlen(node->value) + 1), abs((intptr_t) sp) - 1);
    size_t i = len;
    while (len >= 0) {
        intptr_t mask = (-!!(len ^ i));
        sp[len] = node->value[len] & mask;
        len--;
    }
    return node;
}

/*
 * WARN: This is for external usage, don't modify it
 * Attempt to release element.
 */
void q_release_element(element_t *e)
{
    free(e->value);
    free(e);
}

/*
 * Return number of elements in queue.
 * Return 0 if q is NULL or empty
 */
int q_size(struct list_head *head)
{
    if (!head || list_empty(head))
        return 0;
    size_t len = 0;
    struct list_head *node;
    list_for_each (node, head)
        len++;
    return len;
}

/*
 * Delete the middle node in list.
 * The middle node of a linked list of size n is the
 * ⌊n / 2⌋th node from the start using 0-based indexing.
 * If there're six element, the third member should be return.
 * Return true if successful.
 * Return false if list is NULL or empty.
 */
bool q_delete_mid(struct list_head *head)
{
    // https://leetcode.com/problems/delete-the-middle-node-of-a-linked-list/
    if (!head || list_empty(head))
        return false;
    struct list_head *node = NULL, *next = head->next, *prev = head->prev;
    while (!node) {
        if (next == prev)
            node = next;
        if (next->next == prev)
            node = prev;
        next = next->next;
        prev = prev->prev;
    }
    list_del(node);
    q_release_element(list_entry(node, element_t, list));
    return true;
}

/*
 * Delete all nodes that have duplicate string,
 * leaving only distinct strings from the original list.
 * Return true if successful.
 * Return false if list is NULL.
 *
 * Note: this function always be called after sorting, in other words,
 * list is guaranteed to be sorted in ascending order.
 */
bool q_delete_dup(struct list_head *head)
{
    // https://leetcode.com/problems/remove-duplicates-from-sorted-list-ii/
    if (!head)
        return false;
    char *del_patten = NULL;
    element_t *node, *safe;
    for (node = list_entry(head->next, element_t, list),
        safe = list_entry(node->list.next, element_t, list);
         &safe->list != head;
         node = safe, safe = list_entry(node->list.next, element_t, list)) {
        intptr_t mask =
            -(!strcmp(node->value, safe->value)) | (node->value == del_patten);
        del_patten = (char *) ((uintptr_t) safe->value & mask);
        if (del_patten && !strcmp(del_patten, node->value)) {
            list_del(&node->list);
            q_release_element(node);
        }
    }
    if (del_patten && !strcmp(del_patten, node->value)) {
        list_del(&node->list);
        q_release_element(node);
    }
    return true;
}

/*
 * Attempt to swap every two adjacent nodes.
 */
void q_swap(struct list_head *head)
{
    // https://leetcode.com/problems/swap-nodes-in-pairs/
    if (!head)
        return;
    struct list_head *node, *safe;
    list_for_each_safe (node, safe, head) {
        if (safe == head)
            return;
        list_del(node);
        list_add(node, safe);
        safe = node->next;
    }
}

/*
 * Reverse elements in queue
 * No effect if q is NULL or empty
 * This function should not allocate or free any list elements
 * (e.g., by calling q_insert_head, q_insert_tail, or q_remove_head).
 * It should rearrange the existing ones.
 */
void q_reverse(struct list_head *head)
{
    if (!head)
        return;
    struct list_head *node, *safe;
    list_for_each_safe (node, safe, head) {
        node->next = node->prev;
        node->prev = safe;
    }
    node->next = node->prev;
    node->prev = safe;
}

/*
 * Merge two sorted list
 */
struct list_head *merge(struct list_head *a, struct list_head *b)
{
    struct list_head *sorted = NULL, **tail = &sorted;
    while (a && b) {
        struct list_head **temp =
            (strcmp(list_entry(a, element_t, list)->value,
                    list_entry(b, element_t, list)->value) <= 0)
                ? &a
                : &b;
        *tail = *temp;
        *temp = (*temp)->next;
        tail = &((*tail)->next);
    }
    *tail = (struct list_head *) ((intptr_t) a | (intptr_t) b);
    return sorted;
}


/*
 * dividing two sort
 */

struct list_head *merge_sort(struct list_head *head)
{
    if (!head->next)
        return head;
    struct list_head *mid, *last;
    for (mid = head, last = head->next; last && last->next;
         mid = mid->next, last = last->next->next)
        ;
    last = mid;
    mid = mid->next;
    last->next = NULL;
    head = merge_sort(head);
    mid = merge_sort(mid);
    return merge(head, mid);
}


/*
 * Sort elements of queue in ascending order
 * No effect if q is NULL or empty. In addition, if q has only one
 * element, do nothing.
 */
void q_sort(struct list_head *head)
{
    if (!head || (head->next == head->prev))
        return;
    struct list_head *tail;
    head->prev->next = NULL;
    tail = merge_sort(head->next);
    head->next = tail;
    tail->prev = head;
    while (tail->next) {
        tail->next->prev = tail;
        tail = tail->next;
    }
    head->prev = tail;
    tail->next = head;
}
