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
    int numeroCalculadoresFinalizados=0;
    long int numerosPrimosEncontrados=0;

    //Ficheros
    FILE *ficheroSalida, *ficheroCuentaPrimos;

    return 0;
}