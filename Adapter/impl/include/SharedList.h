/*
 Copyright (c) 2013, Oracle and/or its affiliates. All rights
 reserved.
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; version 2 of
 the License.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 02110-1301  USA
 */

#include <string.h>

#include "node.h"
#include "uv.h"

#include "legacy_uv_compat.h"


/* Simple LIFO sharable list.
   Has no built-in signaling.
   Uses uv_mutex_t for synchronization.
*/

/* The note can serve to document a list item,
   and also as cache-line padding.
*/
#define VPSZ sizeof(void *)
#define LIST_ITEM_NOTE_SIZE 64 - ( 2 * VPSZ)

template<typename T> class ListNode {
public:
  ListNode<T> * next;
  T * item;
private:
  char note[LIST_ITEM_NOTE_SIZE];

public:
  /* Constructor */
  ListNode<T>(T *t) : item(t)
  {
    note[0] = '\0';
  };
  
  /* Methods */
  void setNote(const char *txt) {
    strncpy(note, txt, LIST_ITEM_NOTE_SIZE);
    /* If txt is too long, strncpy() leaves it unterminated */
    note[LIST_ITEM_NOTE_SIZE] = '\0';
  };

  const char * getNote() const {
    return note;
  };
};


template<typename T> class SharedList {
private:
  uv_mutex_t lock;
  ListNode<T> * head;
  
public:
  SharedList<T>() : head(0)
  {
    int i = uv_mutex_init(& lock);
    assert(i == 0);
  };
  
  
  ~SharedList<T>()
  {
    uv_mutex_destroy(& lock);
  };
  
  
  void produce(ListNode<T> * node) {
    /* Find the tail */
    ListNode<T> * tail;
    for(tail = node; tail->next; tail = tail->next);
    
    uv_mutex_lock(& lock);
    tail->next = head;
    head = tail;
    uv_mutex_unlock(& lock);
  };
  
  
  ListNode<T> * consumeAll() {
    uv_mutex_lock(& lock);
    ListNode<T> * result = head;
    head = 0;
    uv_mutex_unlock(& lock);
    return result;
  };
};



