#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/sendfile.h>

#define CHARSIZE 1024
#define MIMETYPE "mime-types.tsv"

#define CABECERA_ERROR "HTTP/1.0 %s %s\n"\
                       "Content-type: text/html\n"\
                     
#define CABECERA_OK "HTTP/1.1 200 OK\n"\
                    "Date: %s\n"\
                    "Content-length: %ld\n"\
                    "Content-type: %s\n"

void enviar_cabecera_ok(const char *fecha_hora, size_t st_size, const char *tipo);
void enviar_cabecera_error(char *codigo, char *mensaje_corto);
const char* tipo_mime(const char *archivo);
void enviar_archivo(const char *archivo);
const char* fecha_hora(void);


int main(int argc, char **argv)
{
    char buf[CHARSIZE], metodo[CHARSIZE], uri[CHARSIZE], protocolo[CHARSIZE];
    char archivo[CHARSIZE] = ".";
    struct stat stat_buf;

    //Cabecera
    read(0, buf, CHARSIZE-1);
    fprintf(stderr, "\n%s", buf);
    sscanf(buf, "%s %s %s\n", metodo, uri, protocolo);

    //Protocolo
    if (strcasecmp(protocolo, "HTTP/1.0") && strcasecmp(protocolo, "HTTP/1.1")){ 
        enviar_cabecera_error("505", "HTTP version not supported");
        enviar_archivo("./PaginasError/505.html");
        exit(0);
    }
    //Metodo
    if (strcasecmp(metodo, "GET") && strcasecmp(metodo, "HEAD")){
        enviar_cabecera_error("501", "Not Implemented");
        enviar_archivo("./PaginasError/501.html");
        exit(0);
    }
    //Añade separador para dirección del archivo
    if (uri[0] != '/') strcat(archivo, "/");
    strcat(archivo, uri);
    //Asigna el index.html de no recibir direccion raiz
    if (uri[strlen(uri)-1] == '/') strcat(archivo, "index.html");

    int file_cargado = stat (archivo, &stat_buf);

    //Falla de carga o inexistencia
    if (file_cargado < 0) {  
        enviar_cabecera_error("404", "Not found");
        enviar_archivo("./PaginasError/404.html");
        exit(0);
    }

    if (!(strcasecmp(metodo, "GET"))) { 
        enviar_cabecera_ok(fecha_hora(), stat_buf.st_size, tipo_mime(archivo));
        enviar_archivo(archivo);
    }

    if (!(strcasecmp(metodo, "HEAD"))) { 
        enviar_cabecera_ok(fecha_hora(), stat_buf.st_size, tipo_mime(archivo));
    }
    exit(0);
}

void enviar_cabecera_ok(const char *fecha_hora, size_t st_size, const char *tipo) {
    printf(CABECERA_OK, fecha_hora, st_size, tipo);
}

void enviar_cabecera_error(char *codigo, char *mensaje_corto) {
    printf(CABECERA_ERROR, codigo, mensaje_corto);
}

/*Conversion del tiempo en una cadena de texto*/
const char* fecha_hora(void) {
    time_t tiempo = time(0);

    struct tm *tlocal = gmtime(&tiempo);
    static char output[128];
    strftime(output,128,"%a, %d %b %Y %X %Z",tlocal);

    return output;
}

//Envio de archivo segun ruta especificada y comprobaciones
void enviar_archivo(const char *archivo){
    int fd;
    char *p;
    off_t len;
    struct stat sb;

    fd = open(archivo, O_RDONLY);
    if (fd == -1) perror ("open");
    if (fstat(fd, &sb) == -1) perror("fstat");
    p = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (p == MAP_FAILED) perror ("mmap");
    fwrite(p, 1,  sb.st_size, stdout);
    if (munmap(p, sb.st_size) == -1) perror ("munmap");
}

//Obtencion del tipo MIME
const char* tipo_mime(const char *archivo) {
    static char tipo[CHARSIZE];
    char *ext = strrchr(archivo, '.');
    //char delimitador[] = " ";
    char linea[128];
    char *cadena;
    int contador_linea = 1;
    ext++; // sacar el punto;
    bzero(linea, sizeof linea );
    FILE *mf = fopen(MIMETYPE, "r");
    if (mf != NULL) {
        while(fgets(linea, sizeof linea, mf) != NULL) {
            if (contador_linea > 1) {
                if((cadena = strtok(linea, " ")) != NULL) {
                    if(strcmp(cadena,ext) == 0) {
                        cadena = strtok(NULL, " ");
                        strcpy(tipo, cadena);
                        break;
                    }
                }
            }
            contador_linea++;
        }
        fclose(mf);
    } else {
        perror("fopen");
    }
    return tipo;
}