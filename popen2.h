#pragma once

#ifndef _POPEN_DEF_LIB_
#define _POPEN_DEF_LIB_


#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "userlog.h"

#ifndef _TRAZA_
#define USERLOG_POPEN(prio, fmt, args...) 
#else
//#define TRAZA_POPEN

#ifdef TRAZA_POPEN

#ifndef POPEN_VERBOSE
#define POPEN_VERBOSE LOG_DEBUG
#endif



#define USERLOG_POPEN(prio, fmt, args...)  \
    ({\
        if(prio <= POPEN_VERBOSE){ \
           char usrStr[255];\
            memset(usrStr,'\0',255);\
            strcat(usrStr, __FILE__" ");\
            strcat(usrStr, __func__);\
            strcat(usrStr, "() ");\
            strcat(usrStr, fmt);\
            USERLOG(prio, usrStr, ##args); \
        }\
     })
#else
#define USERLOG_POPEN(prio, fmt, args...) 
#endif
#endif


typedef unsigned char 		BOOL;


#define READ_PIPE 0
#define WRITE_PIPE 1


//Estados proceso hijo
typedef enum FIN_HIJO{
    EX_POPEN_OK,
    EX_POPEN_STOP,
    EX_POPEN_SIGNAL,
    EX_POPEN_ERROR, // no se usa ahora mismo
    EX_POPEN_ERROR_UNDEFINED
}exitStatusChild_t;


FILE *popen2(const char *command, const char *type, pid_t *pid);
int pclose2(FILE *fp, pid_t *pid);
exitStatusChild_t getExitPopen(pid_t pid, int status, int *exitRes);
int crearProcesoPopen(FILE **file, const char *command); // devuelve child pid
int estadoProcesoPopen(pid_t pid, int timer, int *status);
void procesaRespuestaPopen(FILE *file, char *buffOut, uint32_t buffSize);

#endif // _POPEN_DEF_LIB_

