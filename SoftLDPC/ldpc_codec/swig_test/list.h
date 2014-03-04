#include <stdio.h>

class Node
{
private:
	char val[20];
	Node *p_next;
public:
	void setval(const char *in){ sprintf(val, "%s", in); };
	void setnext( Node *ptr){p_next = ptr;};
	char* getval(){ return val;};
	Node* getnext(){ return p_next; };
};

Node *wrapper();

void test(char *in);

class List {
private:
	Node *counter;
	int length;
public:
   List();
   ~List();
   int  search(char *value);
   void insert(const char *);
   void remove(char *);
   char *get(int n);
	Node p_head;
	Node p_tail;
   static void print(List *l);
};
