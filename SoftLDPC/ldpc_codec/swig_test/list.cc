#include "list.h"
#include <stdio.h>
#include <iostream>

using namespace std;


List::~List()
{
	;
}

List::List()
{
	length = 0;
	//p_head = new Node;
	//sprintf(p_head.val, "%s", "head");
	p_head.setval("head");
	p_head.setnext(&p_tail);
	//sprintf(p_tail.val, "%s", "tail");
	p_tail.setval("tail");
	p_tail.setnext(NULL);
	counter = &p_head;
/*	cout << "head val is " << p_head.getval() << endl
		<< "tail val is " << p_tail.getval() << ", constructor done" << endl;
*/
}

char *List::get(int in)
{
	int i;
	Node *ptr;
	ptr = p_head.getnext();
	if(in > length)
		cout << "out of bound" <<endl;
	else
	{
		for(i = 1; i < in; i++)
		{
			ptr = ptr->getnext();
		}
	}
	return ptr->getval();
}

void List::insert(const char *in)
{
	Node *node;
	Node *ptr;

	node = new Node;
	ptr = counter->getnext();
	//cout << "current node is " << counter->getval()
	//<< ", next one is " << ptr->getval() << endl;
	node->setnext(&p_tail);
	node->setval(in);
	
	//ptr = node.getnext();
	//cout << node.getval() << "points to " << ptr->getval() << endl;
	
	counter->setnext(node);
	counter = node;
	length++;
	
	cout << "inserted " << in << " total length is " << length << endl;
	cout << "list now looks like: " << endl;
	ptr = &p_head;
	cout << ptr->getval() << " -> ";
	//ptr = ptr->getnext();
	
	for(int i = 0; i < length; i ++)
	{
		ptr = ptr->getnext();
		cout << ptr->getval() << " -> ";
	}
	ptr = ptr->getnext();

	cout << ptr->getval() << endl;
}


Node* wrapper()
{
	List l; 
	l.insert("Ted_wrap");
	l.insert("Fred_wrap");
	Node n;
	return &n;
}

void test(char *in)
{
	cout << in << endl;
}

int main()
{
	List list;
	list.insert("ted");
	list.insert("fred");
	cout << "first element is " << list.get(1) << endl
	<< " second element is " << list.get(2) << endl;

	return 0;

}

