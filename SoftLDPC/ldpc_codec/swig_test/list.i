%module list
%{
#include "list.h"
%}

// Very simple C++ example for linked list


class List {
public:
   List();
   ~List();
   void insert(const char *);
};
Node* wrapper();

void test(char *in);
