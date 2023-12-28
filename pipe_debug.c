#include "pipe_debug.h"

void aumentaNumPruebasPipes()
{
    erroresPipe.numEjecuciones++;
}
void initTiposErroresPipes()
{
    erroresPipe.numEjecuciones = 0;

    tipoErrorPipe_t i;
    for (i = 0; i < NUM_ERROR_PIPE; i++)
    {
        erroresPipe.error_n[i] = 0;
    }
};

void procesarErroresPipe(int n_error)
{
    int n = n_error-1;
    if (n >= ERRORDESCONOCIDO)
        n = ERRORDESCONOCIDO;

    erroresPipe.error_n[n]++;
}

