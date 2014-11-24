#include "chat_ll.h"
#include "stdio.h"

void test1() {
  chat_ll list = chat_ll_create();
  
  printf("Should be empty:\n");
  chat_ll_print(&list);

  chat_info u;
  u.lts.ts = 1;
  u.lts.pid = 1;

  chat_ll_append(&list, u);
  printf("Should have (1,1)\n");
  chat_ll_print(&list);

  u.lts.ts = 2;
  chat_ll_append(&list, u);
  printf("Should have (1,1) and (2,1)\n");
  chat_ll_print(&list);

  u.lts.ts = 1;
  u.lts.pid = 2;
  chat_ll_insert_inorder(&list, u);
  printf("Should have (1,1) (1,2) (2,1)\n");
  chat_ll_print(&list);

  u.lts.ts = 0;
  chat_ll_insert_inorder(&list, u);
  printf("Should have (0,2) (1,1) (1,2) (2,1)\n");
  chat_ll_print(&list);

  u.lts.ts = 3;
  chat_ll_insert_inorder(&list, u);
  printf("Should have (0,1) (1,1) (1,2) (2,1) (3,2)\n");
  chat_ll_print(&list);

}

void test2() {
  chat_ll list = chat_ll_create();
  
  printf("Should be empty:\n");
  chat_ll_print(&list);

  chat_info u;
  u.lts.ts = 1;
  u.lts.pid = 1;

  chat_ll_append(&list, u);
  printf("Should have (1,1)\n");
  chat_ll_print(&list);

  u.lts.ts = 2;
  chat_ll_append(&list, u);
  printf("Should have (1,1) and (2,1)\n");
  chat_ll_print(&list);

  u.lts.ts = 1;
  u.lts.pid = 2;
  chat_ll_insert_inorder_fromback(&list, u);
  printf("Should have (1,1) (1,2) (2,1)\n");
  chat_ll_print(&list);

  u.lts.ts = 0;
  chat_ll_insert_inorder_fromback(&list, u);
  printf("Should have (0,2) (1,1) (1,2) (2,1)\n");
  chat_ll_print(&list);

  u.lts.ts = 3;
  chat_ll_insert_inorder_fromback(&list, u);
  printf("Should have (0,1) (1,1) (1,2) (2,1) (3,2)\n");
  chat_ll_print(&list);
}

void test3() {
  chat_ll list = chat_ll_create();

  chat_info u;
  u.lts.ts = 1;
  u.lts.pid = 1;

  chat_ll_append(&list, u);

  u.lts.ts = 2;
  chat_ll_append(&list, u);

  u.lts.ts = 1;
  u.lts.pid = 2;
  chat_ll_insert_inorder_fromback(&list, u);

  u.lts.ts = 0;
  chat_ll_insert_inorder_fromback(&list, u);

  u.lts.ts = 3;
  chat_ll_insert_inorder_fromback(&list, u);

  printf("Current List:\n");
  chat_ll_print(&list);

  lts_entry lts;
  lts.ts = 0;
  lts.pid = 0;
  
  chat_info* result;
  printf("Get (0,0): Expect 3 misses:\n");
  result = chat_ll_get(&list, lts);
  if (result) {
    printf("HIT\n");
  }
  else {
    printf("MISS\n");
  }
  
  result = chat_ll_get_inorder(&list, lts);
  if (result) {
    printf("HIT\n");
  }
  else {
    printf("MISS\n");
  }

  result = chat_ll_get_inorder_fromback(&list, lts);
  if (result) {
    printf("HIT\n");
  }
  else {
    printf("MISS\n");
  }

  lts.ts = 1;
  lts.pid = 2;
  
  printf("Get (1,2): Expect 3 hits:\n");
  result = chat_ll_get(&list, lts);
  if (result) {
    printf("HIT\n");
  }
  else {
    printf("MISS\n");
  }
  
  result = chat_ll_get_inorder(&list, lts);
  if (result) {
    printf("HIT\n");
  }
  else {
    printf("MISS\n");
  }

  result = chat_ll_get_inorder_fromback(&list, lts);
  if (result) {
    printf("HIT\n");
  }
  else {
    printf("MISS\n");
  }


  lts.ts = 0;
  lts.pid = 2;
  
  printf("Get (0,2): Expect 3 hits:\n");
  result = chat_ll_get(&list, lts);
  if (result) {
    printf("HIT\n");
  }
  else {
    printf("MISS\n");
  }
  
  result = chat_ll_get_inorder(&list, lts);
  if (result) {
    printf("HIT\n");
  }
  else {
    printf("MISS\n");
  }

  result = chat_ll_get_inorder_fromback(&list, lts);
  if (result) {
    printf("HIT\n");
  }
  else {
    printf("MISS\n");
  }

  lts.ts = 3;
  lts.pid = 2;
  
  printf("Get (3,2): Expect 3 hits:\n");
  result = chat_ll_get(&list, lts);
  if (result) {
    printf("HIT\n");
  }
  else {
    printf("MISS\n");
  }
  
  result = chat_ll_get_inorder(&list, lts);
  if (result) {
    printf("HIT\n");
  }
  else {
    printf("MISS\n");
  }

  result = chat_ll_get_inorder_fromback(&list, lts);
  if (result) {
    printf("HIT\n");
  }
  else {
    printf("MISS\n");
  }


}


int main() {
  test1();
  test2();
  test3();
  return 0;
}
