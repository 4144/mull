#include "test_and_or_operators.h"

#include <stdio.h>

enum { SUCCESS = 0, FAILURE = 1 };

int testee_AND_operator_2branches(int a, int b, int c) {
  if (a < b && b < c) {
    printf("left branch\n");
    return a;
  } else {
    printf("right branch\n");
    return b;
  }
}

int testee_AND_operator_1branch(int a, int b, int c) {
  if (a < b && b < c) {
    printf("left branch\n");
    return a;
  }

  printf("right branch\n");
  return b;
}

int testee_OR_operator_2branches(int a, int b, int c) {
  if (a < b || b < c) {
    printf("left branch\n");
    return a;
  } else {
    printf("right branch\n");
    return b;
  }
}

int testee_OR_operator_1branch(int a, int b, int c) {
  if (a < b || b < c) {
    printf("left branch\n");
    return a;
  }

  printf("right branch\n");
  return b;
}

int testee_compound_AND_then_OR_operator(int a, int b, int c) {
  if (a < b && (b < c || a < c)) {
    printf("left branch\n");
    return a;
  }

  printf("right branch\n");
  return b;
}

int testee_compound_AND_then_AND_operator(int A, int B, int C) {
  if ((!A && B) && C) {
    printf("left branch\n");
    return 1;
  } else {
    printf("right branch\n");
    return 0;
  }
}

int testee_compound_OR_then_AND_operator(int a, int b, int c) {
  if (a < b || (b < c && a < c)) {
    printf("left branch\n");
    return a;
  }

  printf("right branch\n");
  return b;
}

int testee_compound_OR_then_OR_operator(int A, int B, int C) {
  if ((!A || B) || C) {
    printf("left branch\n");
    return 1;
  } else {
    printf("right branch\n");
    return 0;
  }
}

int test_AND_operator_2branches() {
  if (testee_AND_operator_2branches(1, 3, 2) == 3) {
    return SUCCESS;
  }
  return FAILURE;
}

int test_AND_operator_1branch() {
  if (testee_AND_operator_1branch(1, 3, 2) == 3) {
    return SUCCESS;
  }
  return FAILURE;
}

int test_OR_operator_2branches() {
  if (testee_OR_operator_2branches(1, 3, 2) == 1) {
    return SUCCESS;
  }
  return FAILURE;
}

int test_OR_operator_1branch() {
  if (testee_OR_operator_1branch(1, 3, 2) == 1) {
    return SUCCESS;
  }
  return FAILURE;
}

int test_compound_AND_then_OR_operator() {
  if (testee_compound_AND_then_OR_operator(1, 3, 2) == 1) {
    return SUCCESS;
  }
  return FAILURE;
}

int test_compound_AND_then_AND_operator() {
  if (testee_compound_AND_then_AND_operator(1, 1, 1) == 0) {
    return SUCCESS;
  }
  return FAILURE;
}

int test_compound_OR_then_AND_operator() {
  if (testee_compound_OR_then_AND_operator(1, 3, 2) == 1) {
    return SUCCESS;
  }
  return FAILURE;
}

int test_compound_OR_then_OR_operator() {
  if (testee_compound_OR_then_OR_operator(0, 0, 0) == 1) {
    return SUCCESS;
  }
  return FAILURE;
}
