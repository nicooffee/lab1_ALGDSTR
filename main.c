/*--------------------------------------------------------------------------------------------------------------------**/
/************************************************************************************************************************
    Autores        : NICOLAS HONORATO DROGUETT, OSCAR FRITIS LOBOS.
	Proposito      : Laboratorio 1 algoritmos distribuidos.
    Programa       : Movimiento de 4 particulas en forma concurrente.
    Fecha          : Santiago de Chile, 24 de mayo de 2019
	Compilador C   : gcc (Ubuntu 5.4.0-6ubuntu1~16.04.9) 5.4.0
************************************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "libraries/participant_list.h"
#include "libraries/participant.h"

#define ROJO        1
#define VERDE       2
#define AMARILLO    3
#define CYAN        4
#define MAGENTA     5
#define BLANCO      6
#define AZUL        7
#define BLANCO_ROJO     8
#define BLANCO_VERDE    9
#define BLANCO_AMARILLO 10
#define BLANCO_CYAN     11
#define BLANCO_MAGENTA  12
#define BLANCO_NEGRO    13
#define BLANCO_AZUL     14

/**-------------------------------------------ESTRUCTURAS------------------------------------------------------------**/
/**
 * struct Message_w
 * Estructura para enviar los datos que utilizan las funciones del manejo de la ventana 1.
 * 
 * listp    : Lista de participantes.
 * w        : ventana donde se muestran los participantes.
 * delay    : Delay de refresco de la ventana.
 */
struct Message_w{
    Participant_list listp;
    WINDOW *w1;
    WINDOW *w2;
    int delay;
    int maxInstant;
    struct Colision **colisiones;
};


/**
 * Estructura para enviar los datos que utilizan las funciones del movimiento de un participante.
 * 
 * listp        : Lista de participantes.
 * id           : ID del participante a manejar
 * delay        : Delay del participante.
 * maxInstant   : Máximo de instantes permitidos en la simulación.
 */
struct Message_p{
    Participant_list listp;
    Participant p;
    int delay;
    int maxInstant;
    struct Colision **colisiones;
};


/**
 * Estructura para almacenar los datos de una colisión
 * 
 * instante : Instante en el que ocurre la colisión
 * posX     : Eje x del lugar de colisión
 * posY     : Eje y del lugar de colisión
 * id1      : ID del participante que genera la colisión
 * id2      : ID del participante que recibe la colisión
 */
struct Colision{
    int instante;
    int posX;
    int posY;
    int id1;
    int id2;
};
/**-----------------------------------------------PROTOTIPOS---------------------------------------------------------**/
void init_setup();
void print_participant(Participant p, WINDOW *w);
void *print_participant_list(void *message);
void print_info(WINDOW *w,struct Colision **colisiones,int restantes);
void *move_participant_list(void * message);
struct Message_p *create_message_p(Participant_list listp,Participant p,int delay,int maxInstantm,struct Colision **colisiones);
struct Message_w *create_message_w(Participant_list listp, WINDOW *w1,WINDOW *w2,int d,int i,struct Colision **colisiones);
struct Colision *create_colision(int inst,int posx,int posy,int id1,int id2);
void *stop_ejec(void *message);

/**-------------------------------------------VARIABLES GLOBALES-----------------------------------------------------**/
int INSTANT = 0;                                                //Instante de la simulación.
int COLISIONES = 0;                                             //Cantidad de colisiones total.
int first_coll;                                                 //Posición de la última colisión
int length_coll;                                                //Cantidad de colisiones a guardar (las n ultimas).
pthread_mutex_t mutex_screen=PTHREAD_MUTEX_INITIALIZER;         //Mutex para sincronizar las ventanas.
pthread_mutex_t mutex_participant=PTHREAD_MUTEX_INITIALIZER;    //Mutex para sincronizar el uso de lista de participantes.
int ejec_flag = 1;
/**
 * Parametros de inicio:
 *  - n:    Cantidad de particulas de cada participante.
 *  - k:    Cantidad de participantes.
 *  - i:    Iteraciones del proceso.
 *  - d:    Delay de refresco de ventana.
 */
int main(int args, char **argv){
    pthread_t thread_screen1,*thread_participant,thread_ejec;
    struct Message_w *screen_message;
    struct Message_p **participant_message;
    struct Colision **colisiones=NULL;
    Participant_list list;
    WINDOW *w_particle,*w_data;
    int n,k,i,d;
    int j;
    int maxx,maxy;
    int maxx_p,maxy_p,maxx_d,maxy_d;
    if(args < 5){
        printf("Parametros de ejecucion incorrectos.");
        exit(0);
    }
    init_setup();
    getmaxyx(stdscr,maxy,maxx);
    w_particle = newwin(maxy,maxx*5/10,0,0);
    w_data = newwin(maxy,maxx*5/10,0,maxx*5/10);
    getmaxyx(w_particle,maxy_p,maxx_p);
    getmaxyx(w_particle,maxy_d,maxx_d);
    n = atoi(argv[1]);
    k = atoi(argv[2]);
    i = atoi(argv[3]);
    d = atoi(argv[4]);
    first_coll = 0;
    length_coll = maxy/2-5;
    colisiones = (struct Colision**) calloc(length_coll,sizeof(struct Colision*));
    list = participant_list_create(k,n,maxx_p-1,maxy_p-1,1,1);
    screen_message =        create_message_w(list,w_particle,w_data,d,i,colisiones);
    thread_participant =    (pthread_t*) calloc(k,sizeof(pthread_t));
    participant_message =   (struct Message_p**) calloc(k,sizeof(struct Message_p*));
    pthread_create(&thread_screen1,NULL,&print_participant_list,(void *) screen_message);
    pthread_create(&thread_ejec,NULL,&stop_ejec,(void *) screen_message);
    for(j=0;j<k;j++){
        participant_message[j] = create_message_p(list,participant_list_get(list,j),d,i,colisiones);
        pthread_create(&thread_participant[j],NULL,&move_participant_list,(void *)participant_message[j]);
    }
    pthread_join(thread_screen1, NULL);
    pthread_join(thread_ejec,NULL);;
    for(j=0;j<k;j++)
        pthread_join(thread_participant[j],NULL);
    participant_list_free(list);
    pthread_mutex_destroy(&mutex_screen);
    pthread_mutex_destroy(&mutex_participant);
    free(thread_participant);
    free(screen_message);
    for(j=0;j<k;j++)
        free(participant_message[j]);
    for(j=0;j<length_coll;j++)
        if(colisiones[j]!=NULL)
            free(colisiones[j]);
    free(colisiones);
    free(participant_message);
    delwin(w_data);
    delwin(w_particle);
    endwin();
    return 0;
}





/**
 * move_participant_list:
 * 
 * Función que mueve al participante según los datos del mensaje. Se utiliza
 * un mutex para asegurar la exclusión mutua al mover o colisionar con otra
 * partícula. Esta función finaliza su ejecución cuando el participante no posee
 * partículas o cuando el instante de la simulación supera al máximo definido.
 */
void *move_participant_list(void * message){
    Participant aux=NULL;
    Participant p;
    Participant_list listp;
    int delay;
    int maxInstant;
    int flag=1;
    struct Colision **colisiones=NULL;
    if(message==NULL) return NULL;
    listp           = ((struct Message_p*) message)->listp;
    delay           = ((struct Message_p*) message)->delay;
    maxInstant      = ((struct Message_p*) message)->maxInstant;
    colisiones      = ((struct Message_p*) message)->colisiones;
    p               = ((struct Message_p*) message)->p;
    while(flag!=0 && (maxInstant==-1 || INSTANT<=maxInstant) && ejec_flag){
        pthread_mutex_lock(&mutex_participant);
        flag=participant_list_move(listp,p,&aux);
        if(aux != NULL){
            first_coll = ((first_coll - 1)<0?length_coll+(first_coll - 1):first_coll - 1)% length_coll;
            if(colisiones[first_coll]!=NULL) free(colisiones[first_coll]);
            colisiones[first_coll] = \
                create_colision(INSTANT,\
                                participant_get_x(p),\
                                participant_get_y(p),\
                                participant_get_id(p),\
                                participant_get_id(aux));
            COLISIONES = COLISIONES + 1;
            aux=NULL;
        }
        INSTANT = INSTANT + 1;
        pthread_mutex_unlock(&mutex_participant);
        usleep(delay);
    }
    return NULL;
}





/**
 * print_participant_list:
 * 
 * Función que imprime los datos de una lista de participantes dentro
 * de una ventana, según corresponda a los datos del mensaje. Se utilizan
 * dos mutex, uno para asegurar el uso único de la ventana y el otro para
 * asegurar el uso único de la lista de participantes. 
 */
void *print_participant_list(void *message){
    int j,delay,maxInstant,w2_mx,w2_my;
    Participant_list listp;
    Participant aux;
    listp       = ((struct Message_w*) message)->listp;
    WINDOW *w1  = ((struct Message_w*) message)->w1;
    WINDOW *w2  = ((struct Message_w*) message)->w2;
    delay       = ((struct Message_w*) message)->delay;
    maxInstant  = ((struct Message_w*) message)->maxInstant;
    getmaxyx(w2,w2_my,w2_mx);
    while(ejec_flag){
        wclear(w1);
        wclear(w2);
        wattron(w1,COLOR_PAIR(BLANCO));
        box(w1,'*','*');
        wattroff(w1,COLOR_PAIR(BLANCO));
        pthread_mutex_lock(&mutex_screen);
        pthread_mutex_lock(&mutex_participant);
        for(j=0;j<participant_list_get_length(listp);j++){
            aux = participant_list_get(listp,j);
            if(participant_get_particleNum(aux)!=0){
                print_participant(aux,w1);
                wnoutrefresh(w1);
            }
        }
        print_info(w2,((struct Message_w*) message)->colisiones,participant_list_get_numActive(listp));
        wattron(w2,COLOR_PAIR(BLANCO_VERDE));
        mvwhline(w2,w2_my-7,1,'-',w2_mx-2);
        wattroff(w2,COLOR_PAIR(BLANCO_VERDE));
        mvwprintw(w2,w2_my-6,2," Ejecucion en modo: ");
        if(maxInstant==-1){
            wattron(w2,COLOR_PAIR(BLANCO_ROJO));
            wprintw(w2,"indefinido.");
            wattroff(w2,COLOR_PAIR(BLANCO_ROJO));
        } 
        else{
            wattron(w2,COLOR_PAIR(BLANCO_VERDE));
            wprintw(w2,"limitado");
            wattroff(w2,COLOR_PAIR(BLANCO_VERDE));
            wprintw(w2,", en %d iteraciones.",maxInstant);
        } 
        mvwprintw(w2,w2_my-5,2," Presione ESC para detener.");
        wnoutrefresh(w2);
        pthread_mutex_unlock(&mutex_screen);
        pthread_mutex_unlock(&mutex_participant);
        doupdate();
        usleep(delay/2);
    }
    mvwhline(w2,w2_my-6,2,' ',w2_mx-3);
    mvwhline(w2,w2_my-5,2,' ',w2_mx-3);
    mvwprintw(w2,w2_my-6,2," Simulacion ejecutada con exito.");
    mvwprintw(w2,w2_my-5,2," Presione ESC para salir.");
    wrefresh(w2);
    while(wgetch(w2)!=27);
}





/**
 * stop_ejec:
 * 
 * Funcion para finalizar la ejecución general de las demás hebras.
 */
void *stop_ejec(void *message){
    WINDOW *w2  = ((struct Message_w*) message)->w2;
    while(wgetch(w2)!=27);
    ejec_flag = 0;
}




/**
 * print_info:
 * 
 * Función que imprime los datos de las ultimas length_coll colisiones
 * dentro de la simulación en la pantalla w. Además muestra datos globales
 * relacionados con la simulación.
 */
void print_info(WINDOW *w,struct Colision **colisiones,int restantes){
    int line;
    int j;
    int maxx,maxy;
    struct Colision *aux_col;
    getmaxyx(w,maxy,maxx);
    mvwvline(w,1,1,'|',maxy-2);
    wattron(w,COLOR_PAIR(BLANCO_CYAN));
    box(w,'*','*');
    wattroff(w,COLOR_PAIR(BLANCO_CYAN));
    line = 4;
    mvwhline(w,1,2,' ',maxx-3);
    mvwprintw(w,1,2," Instante actual: %10d",INSTANT);
    mvwhline(w,2,2,' ',maxx-3);
    mvwprintw(w,2,2," Colisiones: %5d| Participantes restantes: %2d",COLISIONES,restantes);
    mvwhline(w,3,2,'-',maxx-3);
    j = first_coll;
    do{
        if(colisiones[j]!=NULL){
            aux_col = colisiones[j];
            mvwhline(w,line,2,' ',maxx-3);
            mvwprintw(w,line,2," Colision -> I: %8d|(x,y): (%3d,%3d)| %2d -><- %2d",aux_col->instante,aux_col->posX,aux_col->posY,aux_col->id1,aux_col->id2);
            line+=2;
        }
        j=(j+1)%length_coll; 
    }while(j!=first_coll && colisiones[j]!=NULL);
}



/**
 * print_participant:
 * 
 * Función utilizada por print_participant_list. Imprime la posición
 * de un participante en la ventana w.
 */
void print_participant(Participant p, WINDOW *w){
    wattron(w,COLOR_PAIR(participant_get_id(p)%13+1));
    mvwprintw(w,participant_get_y(p),participant_get_x(p),"*");
    wattroff(w,COLOR_PAIR(participant_get_id(p)%13+1));
}




/**
 * init_setup:
 * 
 * Función para inicializar las ventanas y números aleatorios.
 */
void init_setup(){
    initscr();
    srand((unsigned) time(NULL));   
    noecho();
    start_color();
    init_pair(ROJO,COLOR_RED,COLOR_RED);
    init_pair(VERDE,COLOR_GREEN,COLOR_GREEN);
    init_pair(AMARILLO,COLOR_YELLOW,COLOR_YELLOW);
    init_pair(CYAN,COLOR_CYAN,COLOR_CYAN);
    init_pair(MAGENTA,COLOR_MAGENTA,COLOR_MAGENTA);
    init_pair(BLANCO,COLOR_WHITE,COLOR_WHITE);
    init_pair(AZUL,COLOR_BLUE,COLOR_BLUE);
    init_pair(BLANCO_ROJO,COLOR_WHITE,COLOR_RED);
    init_pair(BLANCO_VERDE,COLOR_WHITE,COLOR_GREEN);
    init_pair(BLANCO_AMARILLO,COLOR_WHITE,COLOR_YELLOW);
    init_pair(BLANCO_CYAN,COLOR_WHITE,COLOR_CYAN);
    init_pair(BLANCO_MAGENTA,COLOR_WHITE,COLOR_MAGENTA);
    init_pair(BLANCO_NEGRO,COLOR_WHITE,COLOR_BLACK);
    init_pair(BLANCO_AZUL,COLOR_WHITE,COLOR_BLUE);
    curs_set(FALSE);
}


struct Message_p *create_message_p(Participant_list listp,Participant p,int delay,int maxInstant,struct Colision **colisiones){
    struct Message_p *participant_message = (struct Message_p*) calloc(1,sizeof(struct Message_p));
    participant_message->listp = listp;
    participant_message->p = p;
    participant_message->delay = delay;
    participant_message->maxInstant = maxInstant;
    (* participant_message).colisiones = colisiones;
    return participant_message;
}

struct Message_w *create_message_w(Participant_list listp, WINDOW* w1, WINDOW *w2,int d,int i,struct Colision **colisiones){
    struct Message_w *screen_message = (struct Message_w*) calloc(1,sizeof(struct Message_w));
    screen_message->listp = listp;
    screen_message->w1 = w1;
    screen_message->w2 = w2;
    screen_message->delay = d;
    screen_message->maxInstant = i;
    (* screen_message).colisiones = colisiones;
    return screen_message;
}

struct Colision *create_colision(int inst,int posx,int posy,int id1,int id2){
    struct Colision *new = (struct Colision*) calloc(1,sizeof(struct Colision));
    new->instante = inst;
    new->posX = posx;
    new->posY = posy;
    new->id1 = id1;
    new->id2 = id2;
    return new;
}