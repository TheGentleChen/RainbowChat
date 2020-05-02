/*
 * @Author: HandsomeChen
 * @Date: 2019-12-16 19:00:28
 * @LastEditTime: 2019-12-16 22:53:31
 * @LastEditors: HandsomeChen
 * @Description: 链表管理
 */
#ifndef LIST_H
#define LIST_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(_MSC_VER) && !defined(__cplusplus)
/* Microsoft compiler does not know inline
 * keyword in a pure C program
 */
#define inline __inline
#endif

#include <stddef.h> /* for offsetof */

/**
 * \struct list_head
 * \brief Doubly linked list implementation.
 *
 * Simple doubly linked list implementation inspired by include/linux/list.h.
 * \note To use it: LIST_HEAD(name_variable) to declare the variable
 * then always do INIT_LIST(name_variable).
 */
typedef struct list_head
{
  struct list_head *next; /**< Next element in the list */
  struct list_head *prev; /**< Previous element in the list */
}list_head;

/**
 * \def INIT_LIST
 * \brief Initialize a list.
 * \param name the list to initialize
 */
#define INIT_LIST(name) do { \
  (name).prev = &(name); \
  (name).next = &(name); \
}while(0);

/**
 * \def LIST_HEAD
 * \brief Used to declare a doubly linked list.
 * \param name name of list_head struct
 */
#define LIST_HEAD(name) struct list_head name

/**
 * \def LIST_ADD
 * \brief Add a new entry after the specified head.
 * \param new_entry new entry to be added
 * \param head list head to add it after
 */
#define LIST_ADD(new_entry, head) do { \
  struct list_head* next = (head)->next; \
  next->prev = (new_entry); \
  (new_entry)->next = next; \
  (new_entry)->prev = (head); \
  (head)->next = (new_entry); \
}while(0);

/**
 * \def LIST_ADD_TAIL
 * \brief Add a new entry before the specified head.
 * \param new_entry new entry to be added
 * \param head list head to add it before
 */
#define LIST_ADD_TAIL(new_entry, head) do { \
  struct list_head* prev = (head)->prev; \
  (head)->prev = (new_entry); \
  (new_entry)->next = (head); \
  (new_entry)->prev = prev; \
  prev->next = (new_entry); \
}while(0);

/**
 * \def LIST_DEL
 * \brief Delete entry from list.
 * \param rem pointer of the element to delete from the list
 * \note list_empty on entry does not return true after this, the entry is
 * in an undefined state.
 */
#define LIST_DEL(rem) do { \
  (rem)->next->prev = (rem)->prev; \
  (rem)->prev->next = (rem)->next; \
  (rem)->next = (rem); \
  (rem)->prev = (rem); \
}while(0);

/**
 * \def LIST_EMPTY
 * \brief Return whether or not the list is empty.
 * \param head pointer on the list to test
 * \return 1 if empty, 0 otherwise
 */
#define LIST_EMPTY(head) \
  ((head)->next == (head))

/**
 * \def list_get
 * \brief Get the element.
 * \param ptr the list_head pointer
 * \param type the type of the struct this is embedded in
 * \param member the name of the list_struct within the struct
 * \return pointer on the structure for this entry
 */
#define list_get(ptr, type, member) \
  (type *)((char *)(ptr) - offsetof(type, member))

/**
 * \def list_iterate
 * \brief Iterate over a list.
 * \param pos the &struct list_head to use as a loop counter
 * \param head the head for your list
 */
#define list_iterate(pos, head) \
  for((pos) = (head)->next ; (pos) != (head) ; (pos) = (pos)->next)

/**
 * \def list_iterate_safe
 * \brief Thread safe version to iterate over a list.
 * \param pos pointer on a list_head struct
 * \param n temporary variable
 * \param head the list.
 */
#define list_iterate_safe(pos, n, head) \
  for((pos) = (head)->next, (n) = (pos)->next ; (pos) != (head) ; \
      (pos) = (n), (n) = (pos)->next)

/**
 * \brief Get the number of element in the list.
 * \param head the list
 * \return size of the list
 */
static inline unsigned int list_size(struct list_head* head)
{
  struct list_head* lp = NULL;
  unsigned int size = 0;

  list_iterate(lp, head)
  {
    size++;
  }
  return size;
}

#endif /* LIST_H */

