//aula passada: processos
fork
waiwt, waitpid
exec* //não usou
kill

//ALOCAÇÃO MEM PARTILAHDA
fifo unsafe //cria, inicializa e retorna id
//memória partilhada
//como só posso fazer um pedido
//não posso alocar memória dinamicamente
//criar estrutura com tamanho definido

//CRIAÇÃO
pshmget //lida com erros internamente
//tem permissões como em um ficheiro (por ser um recurso)
0600 //110 000 000 //leitura e escrita e nada mais
//cast para (fifo*)
shmget
shmat //Atach

//DESTRUÇÃO
shmdt //detach
shmctl //ipc_removeID

//SEMÁFOROS
int semid
semop //com base no manual para criar UP e DOWN
semget //usado para criar um array de 3 semáforos
up //NSLOTS //para inicializar o sem não bin
semctl

down(fifo->semid, NSLOTS)
down(fifo->semid, ACCESS)
{

    
}
down(fifo->semid, ACCESS)
down(fifo->semid, NITENS)

//MECANISMO DE EXCLUSÃO MÚTUA
//quando o produtor vai buscar a informação, tem o mesmo id de quando pôs lá
//id: dois dígitos menos significativos
if((value % 100) != pid)// deveria dar o id

//Está tudo a correr ao mesmo tempo
//variável que é acedida dentro do fifo (fifo->ii), por estar em memória partilhada,
//é manipulada por vários processos diferentes 