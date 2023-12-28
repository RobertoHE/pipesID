#include "popen2.h"
#include <stdint.h>

FILE *popen2(const char *command, const char *type, pid_t *pid)
{
    pid_t child_pid;
    int fd[2];
    pipe(fd);
    USERLOG_POPEN(LOG_DEBUG, " begin\n\r");

    if ((child_pid = fork()) == -1)
    {
        perror("fork");
        USERLOG_POPEN(LOG_ERR, " error fork\n\r");
        exit(1);
    }

    /* child process */
    if (child_pid == 0)
    {
        if (type == "r")
        {
            close(fd[READ_PIPE]);    // Close the READ_PIPE end of the pipe since the child's fd is write-only
            dup2(fd[WRITE_PIPE], 1); // Redirect stdout to pipe
        }
        else
        {
            close(fd[WRITE_PIPE]);  // Close the WRITE_PIPE end of the pipe since the child's fd is read-only
            dup2(fd[READ_PIPE], 0); // Redirect stdin to pipe
        }
        
        setpgid(child_pid, child_pid); // Needed so negative PIDs can kill children of /bin/sh
        execl("/bin/sh", "/bin/sh", "-c", command, NULL);
        // printf("POPEN_PID_0\n\r:%d",child_pid); para depuracion
        USERLOG_POPEN(LOG_DEBUG, "\tchild end \n\r");
        exit(0);
    }
    else
    {
        if (type == "r")
        {
            close(fd[WRITE_PIPE]); // Close the WRITE_PIPE end of the pipe since parent's fd is read-only
        }
        else
        {
            close(fd[READ_PIPE]); // Close the READ_PIPE end of the pipe since parent's fd is write-only
        }
        // printf("POPEN_PID_x:%d\n\r",child_pid); PARA DEPURACION
    }

    *pid = child_pid;

    if (type == "r")
    {
        USERLOG_POPEN(LOG_DEBUG, " end success\n\r");
        return fdopen(fd[READ_PIPE], "r");
    }

    USERLOG_POPEN(LOG_DEBUG, " end success\n\r");
    return fdopen(fd[WRITE_PIPE], "w");
};

int pclose2(FILE *fp, pid_t *pid)
{
    USERLOG_POPEN(LOG_DEBUG, " begin\n\r");

    int stat = 0;

    if (NULL != fp)
    {
        USERLOG_POPEN(LOG_DEBUG, " %s()\t fclose\n\r");
        fclose(fp);
        fp = NULL;
    }
    if (*pid != 0)
    {
        USERLOG_POPEN(LOG_DEBUG, " %s()\t kill proc\n\r");
        kill(*pid, SIGKILL);
    }
    //#warning interesa dejar pid a 0 ?
    ///*pid = 0;
    USERLOG_POPEN(LOG_DEBUG, " end success\n\r");

    return stat;
}

exitStatusChild_t getExitPopen(pid_t pid, int status, int *exitRes)
{

    USERLOG_POPEN(LOG_DEBUG, "begin\n\r");
    // tipo exit 0 normal, 1 signaled y 2 stopped
    if (WIFEXITED(status))
    {
        //Devolvemos el resultado exit hacia arriba
        *exitRes = WEXITSTATUS(status);
        // SUCCESS
        USERLOG_POPEN(LOG_DEBUG, "ok \n\r");
        return EX_POPEN_OK;
    }
    else if (WIFSIGNALED(status))
    {
        USERLOG_POPEN(LOG_ERR, " signaled\n\r");
        *exitRes = WTERMSIG(status);
        return EX_POPEN_SIGNAL;
    }
    else if (WIFSTOPPED(status))
    {
        USERLOG_POPEN(LOG_ERR, " stopped\n\r");
        *exitRes = WSTOPSIG(status);
        return EX_POPEN_STOP;
    }

    USERLOG_POPEN(LOG_ERR, " error undefined\n\r");
    return EX_POPEN_ERROR_UNDEFINED;
}

int crearProcesoPopen(FILE **file, const char *command)
{
    int c_pid;
    // Lanzamos orden en un pipe
    USERLOG_POPEN(LOG_DEBUG, " begin\n\r");
    if ((*file = popen2(command, "r", &c_pid)) == NULL)
    {
        // file a nulo para que no se realice fclose despues
        *file = NULL;
        USERLOG_POPEN(LOG_ERR, " popen2 error\n\r");
        return -1;
    }
    USERLOG_POPEN(LOG_DEBUG, "\tPopen ok \n\r");

    // Flujo no bloqueante
    if ((fcntl(fileno(*file), F_SETFL, O_NONBLOCK)) == -1)
    {
        USERLOG_POPEN(LOG_ERR, " fcntl error\n\r");
        return -2;
    }
    USERLOG_POPEN(LOG_DEBUG, "fcntl ok \n\r");
    // devolvemos el pid del hijo
    USERLOG_POPEN(LOG_DEBUG, " end success\n\r");
    return c_pid;
}

int estadoProcesoPopen(pid_t pid, int timeout, int *status)
{
    USERLOG_POPEN(LOG_DEBUG, " begin\n\r");

    // Caso timeout no tenemos respuesta aun, matamos al proceso hijo
    if (timeout == 0)
    {
        USERLOG_POPEN(LOG_ERR, " timeout\n\r");
        if (pid != 0)
            kill(pid, SIGKILL);
        return -2; // para timeout
    }

    int wpid;
    //int time = 0;

    //  recibe el estado del pid (no bloqueante)
    // kill zombie process

    wpid = waitpid(pid, status, WNOHANG);

    // Sigue en ejecucion
    if (wpid == 0)
    {
    } // Cuando el waitpid deja de ser 0 el proceso ha sufrido cambios
    else if (-1 == wpid)
    {
        kill(pid, SIGKILL);
        USERLOG_POPEN(LOG_ERR, " popen2 error\n\r");
        return -1; // Para error de proceso
    }
    else
    {
        USERLOG_POPEN(LOG_DEBUG, "SUCCESS\n\r");
        // volvemos a matar el proceso otra vez por precaucion, pero ya ha finalizado
        kill(pid, SIGKILL);
    }
    // No usamos enum para devolver pues se devuelve el pid

    return wpid;
}

void procesaRespuestaPopen(FILE *file, char *buffOut, uint32_t buffSize)
{
    USERLOG_POPEN(LOG_DEBUG, "begin\n\r");

    if (buffOut == NULL)
        return;
    if (file == NULL)
    {
        USERLOG_POPEN(LOG_ERR, "%s()\tnull file\n\r");
        return;
    }

    memset(buffOut, '\0', buffSize);
    fread(buffOut,1,buffSize,file);
    // while (fgets(buffOut, buffSize, file) != EOF)
    // {
    // };
    USERLOG_POPEN(LOG_DEBUG, "end success\n\r");
    return;
}
