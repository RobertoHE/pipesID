#include "pipes.h"

void killPipe(pipe_t *procesoPipe);
refrescarResult_t _refrescarPipe(pipe_t *pipeObjeto, tipoErrorPipe_t *salida);

void printfPipe(pipe_t p)
{
    printf("Pipe Pid %d, Sts: %d, CMD: %s, Timer: %d, Err: %d \n",
           p.pid,
           p.estadoPipe,
           p.command,
           p.timer, // timerid
           p.codigoSalida);
}

void initParamsPipe(pipe_t *procesoPipe)
{
    procesoPipe->pid = 0;
    memset(procesoPipe->command, '\0', sizeof(procesoPipe->command));
    procesoPipe->timer = 0;
    procesoPipe->estadoPipe = ST_PIPE_IDLE;
    procesoPipe->existeTimer = FALSEST;
    procesoPipe->_pFilePipe = NULL,
    procesoPipe->errorCB = NULL;
    procesoPipe->successCB = NULL;
    procesoPipe->timeoutCB = NULL;
    procesoPipe->codigoSalida = 0;

    return;
}

int asignarCMDPipe(pipe_t *procesoPipe, char *command)
{
    if (!command[0])
    {
        USERLOG_PIPES(LOG_ERR, "ERROR comando nulo\n\r");
        return -1;
    }
    memset(procesoPipe->command, '\0', sizeof(procesoPipe->command));
    sprintf(procesoPipe->command, "%s", command);
    return 0;
}

int iniciarPipe(pipe_t *procesoPipe, int timeout_ms)
{
    USERLOG_PIPES(LOG_DEBUG, "begin\n\r");

    // inicializa codigo salida a 0 en caso de quedar valor residual
    procesoPipe->codigoSalida = 0;
    procesoPipe->pid = crearProcesoPopen(&procesoPipe->_pFilePipe, procesoPipe->command);
    if (procesoPipe->_pFilePipe == NULL)
    {
        USERLOG_PIPES(LOG_ERR, "error _pFilePipe create \n \r");
        killPipe(procesoPipe);
        procesoPipe->estadoPipe = ST_PIPE_ERROR_PROCESSING;
        return -1;
    }
    if (procesoPipe->pid == -1 || procesoPipe->pid == -2)
    {
        procesoPipe->estadoPipe = ST_PIPE_ERROR_PROCESSING;
        killPipe(procesoPipe);
        USERLOG_PIPES(LOG_ERR, "error process create\n \r");
        return -2;
    }

    USERLOG_PIPES(LOG_DEBUG, "\t Timer valor %d\n\r", procesoPipe->timer);

    // Si no hay timer inicializado, se activa timer infinito
    if (timeout_ms == 0)
    {
        USERLOG_PIPES(LOG_DEBUG, "\t timer infinito %d\n\r", procesoPipe->timer);
        procesoPipe->existeTimer = FALSEST;
    }
    else
    {
        if (createSTimer(&procesoPipe->timer, timeout_ms, 1) == 0)
        {
            procesoPipe->estadoPipe = ST_PIPE_ERROR_PROCESSING;
            USERLOG_PIPES(LOG_ERR, "error timer create\n \r");
            return -1;
        }
        procesoPipe->existeTimer = TRUEST;
    }

    USERLOG_PIPES(LOG_DEBUG, "\tgo to: RUNNING\n\r");
    procesoPipe->estadoPipe = ST_PIPE_RUNNING;
    USERLOG_PIPES(LOG_DEBUG, "end success\n\r");

    return 0;
}

refrescarResult_t _refrescarPipe(pipe_t *procesoPipe, tipoErrorPipe_t *salida)
{
    if (ST_PIPE_RUNNING != procesoPipe->estadoPipe)
        return RES_PIPE_NOT_RUNNING;

    USERLOG_PIPES(LOG_DEBUG, "begin\n\r");

    exitStatusChild_t tipoSalidaPipe;
    int res, pipeEstado;
    // printfPipe(*procesoPipe);
    //  printfSTimer(&procesoPipe->timer);

    unsigned int timeVal;

    // no_timer 1 si no hay timer, 0 si lo hay
    if (FALSEST == procesoPipe->existeTimer)
    {
        USERLOG_PIPES(LOG_DEBUG, "\t Sin timer \n");
        timeVal = 1;
    }
    else
    {
        USERLOG_PIPES(LOG_DEBUG, "\t Con timer \n");
        inhibitSTimer(&procesoPipe->timer);
        USERLOG_PIPES(LOG_DEBUG, " \trefrescar pipe ID: %d Time:%d Reps:%d\n", procesoPipe->timer, getTime(&procesoPipe->timer), getRepets(&procesoPipe->timer));
        printfSTimer(&procesoPipe->timer);
        timeVal = getTime(&procesoPipe->timer);
    }

    USERLOG_PIPES(LOG_DEBUG, "Llamada estadoprocesoPopen\n\r");
    res = estadoProcesoPopen(procesoPipe->pid, timeVal, &pipeEstado);
    // proceso no finalizado
    if (0 == res)
    {
        // si hay timer
        if (TRUEST == procesoPipe->existeTimer)
            activateSTimer(&procesoPipe->timer);
        procesoPipe->estadoPipe = ST_PIPE_RUNNING;
        return RES_PIPE_NOT_RUNNING;
    }

    // timeout proceso
    if (-2 == res)
    {
        USERLOG_PIPES(LOG_ERR, "fin error timeout \n\r");
        pauseSTimer(&procesoPipe->timer);
        unsigned int val;
        val = getRepets(&procesoPipe->timer);
        USERLOG_PIPES(LOG_ERR, "timeout repeticiones %d \n\r", val);
        if (val > 0)
        {
            USERLOG_PIPES(LOG_INFO, "\t--retry\n\r");
            procesoPipe->estadoPipe = ST_PIPE_RETRY;
            resumeSTimer(&procesoPipe->timer);
            activateSTimer(&procesoPipe->timer);
            killPipe(procesoPipe);
            return RES_PIPE_TIMEOUT;
        }
        *salida = ERROR_TIMEOUT; // timeout error code
        removeSTimer(&procesoPipe->timer);
        // matamos proceso si timeout
        killPipe(procesoPipe);
        USERLOG_PIPES(LOG_DEBUG, "\tgo to TIMEOUT \n\r");
        procesoPipe->estadoPipe = ST_PIPE_TIMEOUT_PROCESSING;
        return RES_PIPE_TIMEOUT;
    }
    // error de proceso
    if (-1 == res)
    {
        *salida = ERROR_INTERNAL_ERROR;
        if (TRUEST == procesoPipe->existeTimer)
            removeSTimer(&procesoPipe->timer);
        killPipe(procesoPipe);
        USERLOG_PIPES(LOG_ERR, "fin error -1\n\r");
        procesoPipe->estadoPipe = ST_PIPE_ERROR_PROCESSING;
        return RES_PIPE_POPEN_ERR; // error del proceso
    }

    // tipoSalida:ok, stopped o signaled el proceso
    tipoSalidaPipe = getExitPopen(procesoPipe->pid, pipeEstado, &res);

    if (TRUEST == procesoPipe->existeTimer)
        removeSTimer(&procesoPipe->timer);
    // Signalled (Terminado)
    if (EX_POPEN_SIGNAL == tipoSalidaPipe)
    {
        USERLOG_PIPES(LOG_ERR, "signalled \n\r");
        procesoPipe->estadoPipe = ST_PIPE_ERROR_PROCESSING;
        return RES_PIPE_EXIT_SIGNALED; // error del comando curl lanzado
    }
    // Por ahora no sabemos si hacer algo o no cuando esta stop(en suspension ctrl+z)
    if (EX_POPEN_STOP == tipoSalidaPipe)
    {
        USERLOG_PIPES(LOG_ERR, "signalled \n\r");
        procesoPipe->estadoPipe = ST_PIPE_ERROR_PROCESSING;
        return RES_PIPE_EXIT_STOPPED; // error del comando curl lanzado
    }

    if (res < NUM_ERROR_PIPE)
        *salida = res; // devolver el status de error
    else
        *salida = ERRORDESCONOCIDO;

    // exito ejecucion y resultado de la llamada al comando

    // NO ES NECESARIO TIMER ES BORRADO ANTES DE LLEGAR AQUI
    // if (TRUEST == procesoPipe->existeTimer)
    // pauseSTimer(&procesoPipe->timer);

    //*salida = res; No tenemos codigo de error
    USERLOG_PIPES(LOG_DEBUG, "end -> SUCCESS\n\r");
    procesoPipe->estadoPipe = ST_PIPE_SUCCESS_PROCESSING;
    return RES_PIPE_EXIT_OK;
}
/// @brief
/// @param pipeOBjeto pipe which contains response of the curl
/// @param buff buffer where we'll save content of the pipe
/// @param size size of buffer
void procesarRespuestaPipe(pipe_t *pipeObjeto, char *buff, uint32_t size)
{
    memset(buff, '\0', size);
    if (pipeObjeto->estadoPipe != ST_PIPE_SUCCESS)
    {
        USERLOG_PIPES(LOG_WARNING, "No se puede procesar\n\r");
        return;
    }
    //memcpy(buff, pipeObjeto->result, size);
    strncpy(buff, pipeObjeto->result, size);
}
void procesarRespuestaPipeRunning(pipe_t *pipeOBjeto, char *buff, uint32_t size)
{
    memset(buff, '\0', size);
    if (pipeOBjeto->estadoPipe != ST_PIPE_RUNNING)
    {
        USERLOG_PIPES(LOG_WARNING, "No se puede procesar\n\r");
        return;
    }

    procesaRespuestaPopen(pipeOBjeto->_pFilePipe, buff, size);
}

int  returnCodePipe(pipe_t *pipeObjeto, int *rtnCode){
    if (pipeObjeto->estadoPipe != ST_PIPE_SUCCESS)
    {
        USERLOG_PIPES(LOG_WARNING, "No se puede procesar\n\r");
        return -1;
    }
    *rtnCode = pipeObjeto->codigoSalida;
    return 0;
}

void killPipe(pipe_t *procesoPipe)
{

    USERLOG_PIPES(LOG_DEBUG, "begin\n\r");
    if (procesoPipe->_pFilePipe != NULL)
    {
        USERLOG_PIPES(LOG_DEBUG, "\tClose File and Kill Child\n");
        // Cierra file y mata proceso hijo
        pclose2(procesoPipe->_pFilePipe, &procesoPipe->pid);
    }
    procesoPipe->_pFilePipe = NULL;

    USERLOG_PIPES(LOG_DEBUG, " end success\n\r");
}

void cerrarPipe(pipe_t *procesoPipe)
{

    USERLOG_PIPES(LOG_DEBUG, "begin\n\r");
    removeSTimer(&procesoPipe->timer);
    killPipe(procesoPipe);

    // para eliminar procesos hijos zombie
    waitpid(-1, NULL, WNOHANG);
    memset(procesoPipe->command, '\0', sizeof(procesoPipe->command));
    procesoPipe->successCB = NULL;
    procesoPipe->timeoutCB = NULL;
    procesoPipe->errorCB = NULL;
    procesoPipe->codigoSalida = 0;
    USERLOG_PIPES(LOG_DEBUG, " end success\n\r");
}

void maqEstadosPipe(pipe_t *procesoPipe)
{
    tipoErrorPipe_t pipeExitCode = 0;
    // Actualizacion estado
    _refrescarPipe(procesoPipe, &pipeExitCode);
    procesoPipe->codigoSalida = pipeExitCode;

    switch (procesoPipe->estadoPipe)
    {
    case ST_PIPE_RETRY:
        USERLOG_PIPES(LOG_INFO, "retry \n\r");
        killPipe(procesoPipe);
        // NO break
    case ST_PIPE_INIT:
        USERLOG_PIPES(LOG_DEBUG, "init \n\r");
        pipeExitCode = iniciarPipe(procesoPipe, 0);
        if (pipeExitCode == -1)
        {
            USERLOG_PIPES(LOG_ERR, "init error \n\r");
            procesoPipe->estadoPipe = ST_PIPE_ERROR_PROCESSING;
            break;
        }
        USERLOG_PIPES(LOG_DEBUG, "\tgo to RUNNING \n\r");
        procesoPipe->estadoPipe = ST_PIPE_RUNNING;
        break;
    case ST_PIPE_RUNNING:
        break;
    case ST_PIPE_SUCCESS_PROCESSING:

        memset(procesoPipe->result, '\0', sizeof(procesoPipe->result));
        procesaRespuestaPopen(procesoPipe->_pFilePipe, procesoPipe->result, sizeof(procesoPipe->result));

        if (procesoPipe->successCB != NULL)
        {
            USERLOG_PIPES(LOG_DEBUG, "\t --- execute pipe success callback--\n");
            procesoPipe->successCB(procesoPipe->pid, procesoPipe->codigoSalida, procesoPipe->result, sizeof(procesoPipe->result));
            USERLOG_PIPES(LOG_DEBUG, "\t ---end exec success CB\n\r");
        }
        procesoPipe->estadoPipe = ST_PIPE_SUCCESS;
        // NO BREAK
    case ST_PIPE_SUCCESS:
        // SUCCESS significa que el proceso ha finalizado bien pero
        // eso no significa que el resultado del proceso sea correcto
        // por eso aqui no se puede tratar la respuesta del proceso
        break;
    case ST_PIPE_TIMEOUT_PROCESSING:
        USERLOG_PIPES(LOG_ERR, "time out \n\r");
        if (procesoPipe->timeoutCB != NULL)
        {
            USERLOG_PIPES(LOG_DEBUG, "\t--execute pipe timeout callback--\n");
            procesoPipe->timeoutCB(procesoPipe->pid, ERROR_TIMEOUT);
            procesoPipe->timeoutCB = NULL;
        }

        procesoPipe->estadoPipe = ST_PIPE_TIMEOUT;
        // No break;
    case ST_PIPE_TIMEOUT:
        break;
    case ST_PIPE_ERROR_PROCESSING:
        procesoPipe->codigoSalida = pipeExitCode;
        USERLOG_PIPES(LOG_ERR, "error \n\r");
        if (procesoPipe->errorCB != NULL)
        {
            USERLOG_PIPES(LOG_DEBUG, "\t--execute pipe error callback--\n");
            procesoPipe->errorCB(procesoPipe->pid, pipeExitCode);
            procesoPipe->errorCB = NULL;
        }
        USERLOG_PIPES(LOG_DEBUG, "\t--estado finish error\n");
        procesoPipe->estadoPipe = ST_PIPE_ERROR;
        // No break;
    case ST_PIPE_ERROR:
        break;
    case ST_PIPE_IDLE:
    default:
        break;
    }
}
