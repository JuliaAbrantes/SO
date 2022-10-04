/*------------------------------------
SO aula 2 revisão de C/C++
//JAVA---------
//C/C++--------
------------------------------------*/

//JAVA---------
A a1;
a1 = new A();
//C/C++--------
A a1; //instancia um objeto da classe A
A *p = new A(); //cria uma referencia para um objeto instanciado;


a1.x //pega o campo da instancia
p->x // (*p).x //pega o campo que o ponteiro aponta
//consequencia:
//JAVA objeto não contem objeto, contém uam referência
//C/C++ objeto contém de facto o objeto
class A{
    B b;
}


//JAVA---------
//array é objeto
int x[10]
x.legth
//C/C++--------
//array aponta para um endereço (geralmente inicial)
int x[10]
int *p = &x[2]
p[1] = 3;
//ver sizeof

//C/C++--------
//locally global, só no ficheiro
static int n;
//realy global
int n;


//C---------
//maloc?
//C++--------
//new
int *p = new int(10); //new int devolve o endereço onde o novo int está instanciado
//lembrar de delete



//JAVA---------
>>>
//shift logico/aritimético
//C/C++--------
>>
//definido pelo tipo


//JAVA---------
classe.campo
//C/C++--------
classe.campo
ponteiro->campo
&,*
//namespace::
std::cout<<"saida"

//operador de derivação de classes
//JAVA--------- extends
//C/C++-------- :


//JAVA--------- privacidade default
//package
//C/C++--------
//public


//JAVA---------
yes function overloading
no operator overloading
//C/C++--------
no funciton overloading
yes operator overloading


//JAVA---------
//C/C++--------
OUTTUT INPUT ???



//JAVA---------
void tx(int n, A a); // n por valor, a por referencia (valor do ponteiro na vdd)
//C/C++--------
void tx(int n, A &a); //
void tx(int n, A *a);


//duplicar uma string na memória
#include <string.h>

/* ----------------ajuda aula 1 -------*/


//man Scanf
//getline();
 ///*
//Criar um elemento
Register *p;
//percorrer os elementos para pegar info
for (p = head; p != NULL; p = p->next) {
    //p é um ponteiro por isso usa seta eqv: (*p).campo
    print(p->reg.nmec);
}
//procurar ponto de inserção
Node *p1, *p2;
for(p1 = NULL, p2 = head; p2!=NULL; p1 = p2, p2->p2.next) {
    if(p2->reg.nmec > p->reg.nmec) break;
}
//Se p1 é null, inserir no começo da lista,
//head aponta para p e p aponta para p2
//Se p2 é null, inserir no fim da lista
//se p1!= null && p2!= null, p1, aponta pra p, e p aponta pra p2
//*/

Node nodeNew;
        nodeNew.reg.nmec = regNew;
        nodeNew.reg.name = name;

        node.next = 