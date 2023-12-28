#pragma once

#ifndef _PIPES_DEF_LIB_
#define _PIPES_DEF_LIB_

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "soft_timer.h"

typedef enum
{
    NO_ERROR_PIPE = 0,
    ERROR_UNSUPPORTED_PROTOCOL, //curl 1
    ERROR_TIMEOUT, //timeout = 2 -> personalizado
    ERROR_URL_MALFORM, // curl 3
    ERROR_WRONG_LIBCURL, //curl 4
    ERROR_FORBIDDEN_ROUTE, // 403
    ERROR_ADDRESS_NOT_RESOLVED, // curl 6
    ERROR_FAILED_TO_CONNECT, // curl 7
    ERROR_BAD_REQUEST, // 400
    ERROR_NOT_FOUND, // 404
    ERROR_INTERNAL_ERROR, // 500
    ERROR_NO_TOKEN, // no token = 11-> personalizado
    ERROR_LAUNCHED_TEST, // ya hay un test en ejecucion
    ERROR_SOR_FILE, // fichero con resultado vacio o inexistente
    ERRORDESCONOCIDO,
    NUM_ERROR_PIPE

} tipoErrorPipe_t;

typedef enum
{
    RES_PIPE_NOT_RUNNING,
    RES_PIPE_EXIT_OK,
    RES_PIPE_EXIT_SIGNALED,
    RES_PIPE_EXIT_STOPPED,
    RES_PIPE_POPEN_ERR,
    RES_PIPE_TIMEOUT
} refrescarResult_t;

typedef enum PIPE_ESTADOS
{
    ST_PIPE_IDLE,
    ST_PIPE_INIT,
    ST_PIPE_RUNNING,
    ST_PIPE_SUCCESS_PROCESSING,
    ST_PIPE_SUCCESS,
    ST_PIPE_RETRY,
    ST_PIPE_TIMEOUT_PROCESSING,
    ST_PIPE_TIMEOUT,
    ST_PIPE_ERROR_PROCESSING,
    ST_PIPE_ERROR
} pipeState_t;

typedef void (*pipeSuccessCB_f)(pid_t p, int exitCode, char *resultOut, uint32_t resutlSize);
typedef void (*pipeErrorCB_f)(pid_t p, short int errorCode);
typedef void (*pipeTimeoutCB_f)(pid_t p, short int errorCode);

#ifndef _PIPE_BUFF_SIZE_
#define _PIPE_BUFF_SIZE_ 255
#endif

typedef struct PIPE_TYPE
{
    pid_t pid;
    FILE *_pFilePipe;
    pipeState_t estadoPipe;
    char command[_PIPE_BUFF_SIZE_]; 
    char result[_PIPE_BUFF_SIZE_];
    stimer_t timer;
    boolSTimer_t existeTimer;
    short int codigoSalida;
    pipeSuccessCB_f successCB; //func cb no se usaran porque la informacion ya se pasa hacia arriba
    pipeErrorCB_f errorCB; 
    pipeTimeoutCB_f timeoutCB;
} pipe_t;

// typedef void (*funcPipeCB_f)(pipe_t proccess);

#endif //_PIPES_DEF_LIB_