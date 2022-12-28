//Proyecto II de la asignatura de Sistemas Operativos
//Alumno: Alejandro Gómez García, 2ºA MAIS

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>

#define LONGITUD_MSG 100
#define LONGITUD_MSG_INFO_ERR 200

//Diferentes códigos de error para diferentes errores
#define ERR_GET_QUEUE_KEY  1
#define ERR_CREATE_QUEUE   2
#define ERR_SEND    3
#define ERR_RECV    4
#define ERR_FSAL    5
#define ERR_MSG_NOT_EXPECTED 6

//Definimos los rangos y las operaciones de Limite y rango
#define BASE 800000000
#define RANGO 2000
#define LIMITE (BASE+RANGO)
#define RANGO_BUSQUEDA (LIMITE-BASE)

#define MSG_COD_ESTOY_AQUI 2
#define MSG_COD_LIMITES 3
#define MSG_COD_RESULTADOS 4
#define MSG_COD_FIN 5

///Definimos ya los nombres de los ficheros a usar
#define NOMBRE_FICHERO_SALIDA "primos.txt"
#define NOMBRE_FICHERO_CUENTA_PRIMOS "cuentaprimos.txt"
#define FRECUENCIA_ESCRITURA_CUENTA_PRIMOS 5

///Temporizador
#define INTERVALO_TIMER  5

//Aqui definimos la estructura de mensages
typedef struct {
    long msg_type;
    char msg_text[LONGITUD_MSG];
} T_MSG_BUFFER;

int  comprobarSiEsPrimo(long int numero);
void imprimirJerarquiaPorcesos(int pidRaiz, int pidServidor, int *pidHijos, int numHijos);
void informar(char *texto, int verboso);
long int contarLineas();
void alarmHandler(int signo);

//Variables para el tiempo de ejecución
int    computoTotalSegundos=0;
int    messageQueueId;

int main(int argc, char *argv[]){


    //Comprobación de parametros
    //Si son un número incorrecto no se ejecutará el programa
    //Si los hijos son <= 0 da ERROR
    //Si la verbosidad es distinta de 1 o 0 da ERROR
    if((argc != 3) || strtol(argv[1], NULL, 10) <= 0 || strtol(argv[2], NULL, 10) < 0 || strtol(argv[2], NULL, 10) > 1){
        printf("[ERROR]> Recuerda que para ejecutar el programa tienes que introducir los siguientes parámetros:\n");
        printf("./MainPrimos.exe (numero de hijos a crear mayor que 0) (1 ó 0 para habilitar o deshabilitar mensajes en consola)");
        exit(1);
    }

    int numHijos = (int) strtol(argv[1], NULL, 10);
    int verbosity = (int) strtol(argv[2], NULL, 10); //Visibilidad por consola

    int   rootPid, serverPid, currentPid, parentPid, pid, pidCalculador;
    int   *pidHijos;     //Pid de los hijos
    int   i,j;

    //Gestor de mensages en cola
    key_t keyQueue;
    T_MSG_BUFFER message;

    ///Registros de tiempos
    time_t startTime, endTime;

    //Busqueda
    ///intervalo_hijo = RANGO_BUSQUEDA/num_hijos. Cada hijo busca en [base_hijo,limite_hijo)
    long int intervalo_hijo, base_hijo, limite_hijo;

    char info[LONGITUD_MSG_INFO_ERR];

    //Control de finalización
    int numeroCalculadoresFinalizados = 0;
    long int numerosPrimosEncontrados = 0;

    //Ficheros
    FILE *ficheroSalida, *ficheroCuentaPrimos;

    
    rootPid = getpid();
    //Creación del Server
    pid = fork();

    if (pid == 0) {             //Server es raiz si pid==0
        pid = getpid();
        serverPid = pid;        //Pid del Server
        currentPid = serverPid; //Decimos que se está ejecutando el server
        
        //Creación de cola de mensajes
        if ((keyQueue = ftok("/tmp",'C') == -1)){
            perror("Error al crear la System V IPC key");
            exit(ERR_GET_QUEUE_KEY);
        }
        

        if ((messageQueueId = msgget(keyQueue, IPC_CREAT | 0666) ) == -1) {
            perror("Fallo al crear la System V IPC queue");
            exit(ERR_CREATE_QUEUE);
        }

        //Visualización de la Message Queue Id
        printf("Server: Message queue id =%u\n",messageQueueId);

        //Se crean los procesos calculadores hijos del Server
        i = 0;

        while(i<numHijos){
            if (pid > 0){                   //Aseguramos que el padre es el Server
                pid = fork();
                if (pid == 0){              ///Rama hijo CALCulador
                    parentPid = getppid();  // Asignamos la Pid del padre
                    currentPid = getpid();  // Decimos que se está ejecutando un calculador
                    printf("Ha nacido un hijo calculdador %d, ordinal de hijo %d\n",currentPid,i);
                }
            }
            i++;
        }

        if (currentPid != serverPid){                   //Solo si es un calculador debe iterar

            message.msg_type = MSG_COD_ESTOY_AQUI;      //Indicamos que el mensaje es el de aviso de listo para funcionar
            sprintf(message.msg_text,"%d",currentPid);
            msgsnd(messageQueueId,&message, sizeof(message), IPC_NOWAIT);

            message.msg_type = MSG_COD_LIMITES;

            //Espera a recibir los límites de iteración

            msgrcv(messageQueueId,&message, sizeof(message), MSG_COD_LIMITES,0);
            sscanf(message.msg_text,"%ld %ld",&base_hijo,&limite_hijo);
            printf("Child says: He recibido mi intervalo de iteración [%ld,%ld)\n",base_hijo,limite_hijo);

            for (long int numero = base_hijo; numero < limite_hijo; numero++) {
                if (comprobarSiEsPrimo(numero)) {
                    //Se manda el mensaje de haber encontrado un primo
                    message.msg_type = MSG_COD_RESULTADOS;
                    sprintf(message.msg_text, "%d %ld", currentPid, numero);
                    msgsnd(messageQueueId, &message, sizeof(message), IPC_NOWAIT);
                }
            }
            //Enviamos un mensaje al finalizar
            message.msg_type = MSG_COD_FIN;
            sprintf(message.msg_text, "%d", currentPid);
            msgsnd(messageQueueId, &message, sizeof(message), IPC_NOWAIT);


        } else { //Si no es un calculador es el server
        
            pidHijos = (int*) malloc(numHijos* sizeof(int));
            printf("Hijos creados pero a la espera de enviarles trabajo\n");

            for (j = 0; j<numHijos;j++){ //Recibe los mensajes del calculador listo
                msgrcv(messageQueueId,&message,sizeof(message),MSG_COD_ESTOY_AQUI,0);
                sscanf(message.msg_text,"%d",&pidHijos[j]);
            }

            //Cuando están listos indica los intervalos de iteración

            intervalo_hijo = (int) (RANGO_BUSQUEDA/numHijos);

            message.msg_type = MSG_COD_LIMITES;
            for(j = 0; j<numHijos; j++){
                
                base_hijo = BASE + (intervalo_hijo*j);
                limite_hijo = base_hijo + intervalo_hijo;

                if (j == (numHijos-1)){
                    limite_hijo = LIMITE+1;
                }

                sprintf(message.msg_text,"%ld %ld",base_hijo, limite_hijo);
                msgsnd(messageQueueId,&message, sizeof(message), IPC_NOWAIT);
                printf("Server says: send limits to my child%d, [%ld,%ld) \n",j,base_hijo,limite_hijo);

            }

            //Se va escribiendo la jerarquía de procesos
            imprimirJerarquiaPorcesos(rootPid,serverPid,pidHijos,numHijos);
            

            if((ficheroSalida = fopen(NOMBRE_FICHERO_SALIDA,"w")) == NULL){
                perror("Error al crear el fichero de salida");
                exit(ERR_FSAL);
            }
            printf("Se ha creado el fichero de salida primos.txt\n");

            time(&startTime); //Se inicia el timer 

            while(numeroCalculadoresFinalizados < numHijos){

                if(msgrcv(messageQueueId, &message, sizeof(message),0,0) == -1){
                    perror("Server: msgrcv failed\n");
                    exit(ERR_RECV);
                }
                
                if (message.msg_type == MSG_COD_RESULTADOS){

                    int numeroPrimo;

                    //Dividimos el Pid del calculador y el número primo
                    sscanf(message.msg_text,"%d %d",&pidCalculador,&numeroPrimo);

                    //Imprimimos por consola
                    sprintf(info,"MSG %ld : %s\n" ,++numerosPrimosEncontrados,message.msg_text);
                    informar(info,verbosity);
                    fprintf(ficheroSalida,"%d\n",numeroPrimo);

                    ///Si se ha recibido un número de primos dvisible por 5 se escribe en cuentaprimos.txt
                    if (numerosPrimosEncontrados % FRECUENCIA_ESCRITURA_CUENTA_PRIMOS ==0){
                        ficheroCuentaPrimos = fopen(NOMBRE_FICHERO_CUENTA_PRIMOS,"w");
                        fprintf(ficheroCuentaPrimos,"%ld\n",numerosPrimosEncontrados);
                        fclose(ficheroCuentaPrimos);
                    }

                } else if (message.msg_type == MSG_COD_FIN){

                    //Imprimimos por consola
                    numeroCalculadoresFinalizados++;
                    sprintf(info,"FIN %d %s\n",numeroCalculadoresFinalizados,message.msg_text);
                    informar(info,verbosity);
                } else {
                    perror("Server se ha encontrado con un mensaje no esperado\n");
                    exit(ERR_MSG_NOT_EXPECTED);
                }
            }

            time(&endTime); //Al finalizar se para el timer
            double dif = difftime(endTime,startTime);
            printf("Server: Tiempo total de computación: %.2lf seconds,\n",dif);
            msgctl(messageQueueId, IPC_RMID, NULL);
            fflush(ficheroSalida);
            fclose(ficheroSalida);
            exit(0);
        }
    } else { //Rama del proceso raíz

        alarm(INTERVALO_TIMER);
        signal(SIGALRM,alarmHandler);
        wait(NULL); //Espera a Server

        //Cuenta el número de lineas que es igual al numero de prímos encontrados
        printf("Resultado: %ld primos detectados\n",contarLineas());
        exit(0);
    }
}

int comprobarSiEsPrimo(long int numero) {
    if (numero < 2) {
        return 0; // Por convenio 0 y 1 no son primos ni compuestos
    }else{
        for (int x = 2; x <= (numero / 2); x++){
            if (numero % x == 0) return 0;
        }
    }
    return 1;
}

void imprimirJerarquiaPorcesos(int pidRaiz, int pidServidor, int *pidHijos, int numHijos){
    printf("\n");
    printf("RAIZ\tSERV\tCALC\n"); //Cabecera
    printf("%d\t%d\t%d\n",pidRaiz,pidServidor,pidHijos[0]);

    for (int k=1; k < numHijos; k++){
        printf("\t\t\t%d\n",pidHijos[k]); ///Resto de hijos
    }
    printf("\n");
}

//Si verboso == 1, imprime por consola
void informar(char *texto, int verboso){
    if(verboso){
        printf("%s",texto);
    }
}

long int contarLineas(){

    long int count = 0;
    long int numeroPrimo;
    FILE *ficheroPrimos;

    ficheroPrimos = fopen(NOMBRE_FICHERO_SALIDA,"r");
    while(fscanf(ficheroPrimos,"%ld",&numeroPrimo) != EOF){
        count++;   
    }
    fclose(ficheroPrimos);
    return count;
}



//Maneja las alarmas del proceso raíz
void alarmHandler(int signo){

    FILE *ficheroCuentaPrimos;
    int numeroPrimosEncontrados = 0;
    computoTotalSegundos += INTERVALO_TIMER;

    if ((ficheroCuentaPrimos = fopen(NOMBRE_FICHERO_CUENTA_PRIMOS,"r"))!= NULL){
        fscanf(ficheroCuentaPrimos,"%d",&numeroPrimosEncontrados);
        fclose(ficheroCuentaPrimos);
    }
    printf("%02d (segs): %d primos encontrados\n",computoTotalSegundos,numeroPrimosEncontrados);
    alarm(INTERVALO_TIMER);
}