#include <stdio.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/sendfile.h>

#define BUFLEN 1024
#define PORTNUMBER 8000
 
/* Manejador de señal de salida de los procesos hijos 
 * Imprime el PID y el estado de salida del mismo */

void hijo(int sig){
    pid_t pid;
    int estado;
    while ((pid = waitpid(-1, &estado, WNOHANG)) != -1){
        if (pid != 0){
            printf("hijo: %d\nestado %d\n", pid, WEXITSTATUS(estado));
        }
    }
}


/* Función servicio donde se encuentra la ruta y el llamado al programa que atiende la petición
 * Separar el servicio en un programa nos permite modificarlo sin tener de dar de baja el Servidor. */
void servicio(void){ 
    char *const parmList[] = {"HTTP", NULL}; 
 
    execv("./servicio", parmList);
}

int main(void){ 
//creacion de variables
    char buf[10];
    int s, n, ns, len, yes=1;
    
    struct sockaddr_in direcc; //estructura de conexión

    /* Creación del punto límite de conexión
     * AF_INET: familia de Internet Protocol IPv4
     * SOCK_STREAM: tipo de comunicación, coneccion bidireccional en base a cadenas de bytes
     * 0: tipo de protocolo, en este caso se da por la familia por lo que se setea a 0 */
    
    s = socket(AF_INET, SOCK_STREAM, 0);
    
    if (s < 0) {
        perror("socket");
        exit(1);	
    }

    /* Establecemos las opciones del socket, le pasamos nuestro socket, luego SOL_SOCKET 
     * SO_REUSEADDR especifica las reglas de validación
     * de las direcciones suministradas a bind() de forma que sea reutilizable. */
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    bzero((char *) &direcc, sizeof(direcc)); 	//Inicialización a cero de la estructura de conexión
    /*Se cargan los parametros de la estructura del servidor*/
    direcc.sin_family = AF_INET;		//familia de protocolo TCP/IP
    direcc.sin_port = htons(PORTNUMBER);	//número de puerto asociado [CONSTANTE]
    direcc.sin_addr.s_addr = htonl(INADDR_ANY);	//INADDR_ANY: admision de datos de cualquier IP

    len = sizeof(struct sockaddr_in);

    /*asociacion del socket con la estructura de dirección*/
    if (bind(s, (struct sockaddr *) &direcc, len) == -1) {
        perror("bind");
        close(s);
        exit(1);
    }

    /* Prepara el socket para aceptar información externa y encola hasta n clientes
     * Por la implementación de concurrencia donde cada proceso se atiende con un nuevo 
     * proceso hijo no es necesario mantener una cola  por lo que se setea a 1 */
    if (listen(s, 1) == -1) {
        perror("listen"); 
        close(s);
        exit(1);
    }

    /* La llamada Signal asocia el manejador hijo como respuesta a la señal
     * SIGGCHLD que se produce en la finalización de un proceso hijo, de forma que estos
     * no queden como procesos zombies */
    signal(SIGCHLD, hijo);

    while (1){
	/* Al recibir una petición el Servidor crea un nuevo socket asociado al cliente que
	 * la realizó y hace la llamada a fork() para crear un nuevo proceso que la atienda */
        ns = accept(s, (struct sockaddr *) &direcc, &len);
        if (fork() == 0){	//si se trata de un proceso hijo
            close(s); 		//se cierra la conección al socket de escucha del padre
            dup2(ns,0);		//Crea dos copias del descriptor de archivo hijo y lo redefine como la salida 
            dup2(ns,1);		//y la entrada estandar para llamarlo en el "programa" que se ocupa del servicio.
            servicio();		//se llama al servicio.
            close(ns);		//Se cierra el socket hijo y termina enviando la SIGCHLD (0) al padre
            exit(0);
        }
        close(ns);
    } 
    close(s);
    exit(0);
}

