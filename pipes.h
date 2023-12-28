#pragma once

#ifndef _PIPE_LIB_
#define _PIPE_LIB_

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "soft_timer.h"
#include "pipesDefines.h"
#include "typgen.h"
#include "popen2.h"

// #define DEBUG_PIPES
#ifdef DEBUG_PIPES
#include "pipe_debug.h"
#endif

// DEPURACION MACRO
#ifndef _TRAZA_
#define USERLOG_PIPES(prio, fmt, args...)
#else
//#define TRAZA_PIPES

#ifdef TRAZA_PIPES
#ifndef TRAZA_PIPE_VERBOSE
#define TRAZA_PIPE_VERBOSE LOG_DEBUG
#endif

#define USERLOG_PIPES(prio, fmt, args...)  \
    ({                                      \
        if (prio <= TRAZA_PIPE_VERBOSE)    \
        {                                  \
            char usrStr[255];\
            memset(usrStr,'\0',255);\
            strcat(usrStr, __FILE__" ");\
            strcat(usrStr, __func__);\
            strcat(usrStr, "() ");\
            strcat(usrStr, fmt);\
            USERLOG(prio, usrStr, ##args); \
        }                                  \
    })
#else
#define USERLOG_PIPES(prio, fmt, args...)
#endif
#endif

void initParamsPipe(pipe_t *procesoPipe);
void maqEstadosPipe(pipe_t *pipeObjeto);
int asignarCMDPipe(pipe_t *procesoPipe, char *command);

int iniciarPipe(pipe_t *procesoPipe, int timeout_ms);
void cerrarPipe(pipe_t *pipeObjeto);

int  returnCodePipe(pipe_t *pipeObjeto, int *rtnCode);
void procesarRespuestaPipe(pipe_t *pipeOBjeto, char *buff, uint32_t size);
void procesarRespuestaPipeRunning(pipe_t *pipeOBjeto, char *buff, uint32_t size);

#endif