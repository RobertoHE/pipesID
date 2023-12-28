#pragma once

#ifndef _PIPE_DEBUG_LIB_
#define _PIPE_DEBUG_LIB_

#include "pipesDefines.h"
#include "pipes.h"



typedef struct TIPOS_ERRORES
{	int numEjecuciones;
    int error_n[NUM_ERROR_PIPE];
 
} tiposErroresPipe_t;

tiposErroresPipe_t erroresPipe;

void procesarErroresPipe(int n_error);
void aumentaNumPruebasPipe();

void guardaErroresPipeJSON();
void setTimeoutError();
void initTiposErroresPipes();


#endif