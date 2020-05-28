// This file is under the terms of the unlicense (https://github.com/DavidBuchanan314/ftpd/blob/master/LICENSE)

#define ENABLE_LOGGING 1
#include "network.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <malloc.h>
#include <math.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <switch.h>
#define lstat stat
#include "log.h"
#include "Controllers/NetworkController.h"

#define POLL_UNKNOWN (~(POLLIN | POLLPRI | POLLOUT))

int LISTEN_PORT;
//#define LISTEN_PORT 5000
#define CONFIGPATH "/config/sys-con/port_config.ini"

#include "ini.h"
#include <assert.h>

int Callback(const char* section, const char* key, const char* value, void* userdata)
{
    (void)userdata; /* this parameter is not used in this example */
    printf("    [%s]\t%s=%s\n", section, key, value);
    return 1;
}

typedef struct network_session_t network_session_t;

/*! network session */
struct network_session_t
{
    struct sockaddr_in client_addr;  /*!< listen address for PASV connection */
    int cmd_fd;                      /*!< socket for command connection */
    network_session_t* next;             /*!< link to next session */
    network_session_t* prev;             /*!< link to prev session */
};

/*! appletHook cookie */
static AppletHookCookie cookie;

/*! server listen address */
static struct sockaddr_in serv_addr;
/*! listen file descriptor */
static int listenfd = -1;
/*! list of network sessions */
static network_session_t* sessions = NULL;

/*! close a socket
 *
 *  @param[in] fd        socket to close
 *  @param[in] connected whether this socket is connected
 */
static void
network_closesocket(int fd,
                bool connected)
{
    int rc;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    struct pollfd pollinfo;

    //  WriteToLog("0x%X\n", socketGetLastBsdResult());

    if (connected)
    {
        /* get peer address and print */
        rc = getpeername(fd, (struct sockaddr*)&addr, &addrlen);
        if (rc != 0)
        {
            WriteToLog("getpeername: %d %s\n", errno, strerror(errno));
            WriteToLog("closing connection to fd=%d\n", fd);
        }
        else
            WriteToLog("closing connection to %s:%u\n",
                          inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

        /* shutdown connection */
        rc = shutdown(fd, SHUT_WR);
        if (rc != 0)
            WriteToLog("shutdown: %d %s\n", errno, strerror(errno));

        /* wait for client to close connection */
        pollinfo.fd = fd;
        pollinfo.events = POLLIN;
        pollinfo.revents = 0;
        rc = poll(&pollinfo, 1, 250);
        if (rc < 0)
            WriteToLog("poll: %d %s\n", errno, strerror(errno));
    }

    /* set linger to 0 */
    struct linger linger;
    linger.l_onoff = 1;
    linger.l_linger = 0;
    rc = setsockopt(fd, SOL_SOCKET, SO_LINGER,
                    &linger, sizeof(linger));
    if (rc != 0)
        WriteToLog("setsockopt: SO_LINGER %d %s\n",
                      errno, strerror(errno));

    /* close socket */
    rc = close(fd);
    if (rc != 0)
        WriteToLog("close: %d %s\n", errno, strerror(errno));
}

/*! close command socket on network session
 *
 *  @param[in] session network session
 */
static void
network_session_close_cmd(network_session_t* session)
{
    /* close command socket */
    if (session->cmd_fd >= 0)
        network_closesocket(session->cmd_fd, true);

    session->cmd_fd = -1;
}

/*! destroy network session
 *
 *  @param[in] session network session
 *
 *  @returns the next session in the list
 */
static network_session_t*
network_session_destroy(network_session_t* session)
{
    network_session_t* next = session->next;

    /* close all sockets/files */
    network_session_close_cmd(session);

    /* unlink from sessions list */
    if (session->next)
        session->next->prev = session->prev;
    if (session == sessions)
        sessions = session->next;
    else
    {
        session->prev->next = session->next;
        if (session == sessions->prev)
            sessions->prev = session->prev;
    }

    /* deallocate */
    free(session);

    return next;
}

/*! allocate new network session
 *
 *  @param[in] listen_fd socket to accept connection from
 */
static int
network_session_new(int listen_fd)
{
    ssize_t rc;
    int new_fd;
    network_session_t* session;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    /* accept connection */
    new_fd = accept(listen_fd, (struct sockaddr*)&addr, &addrlen);
    if (new_fd < 0)
    {
        WriteToLog("accept: %d %s\n", errno, strerror(errno));
        return -1;
    }

    WriteToLog("accepted connection from %s:%u\n",
                  inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

    registerNetworkController(new_fd);

    /* allocate a new session */
    session = (network_session_t*)calloc(1, sizeof(network_session_t));
    if (session == NULL)
    {
        WriteToLog("failed to allocate session\n");
        network_closesocket(new_fd, true);
        return -1;
    }

    /* initialize session */
    session->cmd_fd = new_fd;

    /* link to the sessions list */
    if (sessions == NULL)
    {
        sessions = session;
        session->prev = session;
    }
    else
    {
        sessions->prev->next = session;
        session->prev = sessions->prev;
        sessions->prev = session;
    }

    /* copy socket address to pasv address */
    addrlen = sizeof(session->client_addr);
    rc = getsockname(new_fd, (struct sockaddr*)&session->client_addr, &addrlen);
    if (rc != 0)
    {
        WriteToLog("getsockname: %d %s\n", errno, strerror(errno));
        network_session_destroy(session);
        return -1;
    }

    return 0;
}

/*! poll sockets for network session
 *
 *  @param[in] session network session
 *
 *  @returns next session
 */
static network_session_t*
network_session_poll(network_session_t* session)
{
    int rc;
    struct pollfd pollinfo[2];
    nfds_t nfds = 1;

    /* the first pollfd is the command socket */
    pollinfo[0].fd = session->cmd_fd;
    pollinfo[0].events = POLLIN | POLLPRI;
    pollinfo[0].revents = 0;

    /* poll the selected sockets */
    rc = poll(pollinfo, nfds, 0);
    if (rc < 0)
    {
        WriteToLog("poll: %d %s\n", errno, strerror(errno));
        network_session_close_cmd(session);
    }
    else if (rc > 0)
    {
        /* check the command socket */
        if (pollinfo[0].revents != 0)
        {
            /* handle command */
            if (pollinfo[0].revents & POLL_UNKNOWN)
                WriteToLog("cmd_fd: revents=0x%08X\n", pollinfo[0].revents);

            /* we need to read a new command */
            if (pollinfo[0].revents & (POLLERR | POLLHUP))
            {
                WriteToLog("cmd revents=0x%x\n", pollinfo[0].revents);
                network_session_close_cmd(session);
            }

            ssize_t count;
            uint8_t input_bytes[64];
            count = recv(session->cmd_fd, input_bytes, sizeof(input_bytes), MSG_PEEK);
            if (count == 0)
            {
                removeNetworkController(session->cmd_fd);
                network_session_close_cmd(session);
            }
        }
    }

    /* still connected to peer; return next session */
    if (session->cmd_fd >= 0)
        return session->next;

    /* disconnected from peer; destroy it and return next session */
    WriteToLog("disconnected from peer\n");

    return network_session_destroy(session);
}

/*! Handle applet events
 *
 *  @param[in] type    Event type
 *  @param[in] closure Callback closure
 */
static void
applet_hook(AppletHookType type,
            void* closure)
{
    (void)closure;
    (void)type;
    /* stubbed for now */
    switch (type)
    {
    default:
        break;
    }
}

void network_pre_init(void)
{
    /* register applet hook */
    appletHook(&cookie, applet_hook, NULL);
}

static int network_parse_config_line(void *dummy, const char *section, const char *name, const char *value)
{
    if (strncmp(name, "port:", 5) == 0)
    {
        LISTEN_PORT = atoi(value);
    }

    return 0;
}

/*! initialize network subsystem */
int network_init(void)
{
    int rc = 0;

    /* allocate socket to listen for clients */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0)
    {
        WriteToLog("socket: %d %s\n", errno, strerror(errno));
        network_exit();
        return -1;
    }

    /* get address to listen on */
    serv_addr.sin_family = AF_INET;

    serv_addr.sin_addr.s_addr = INADDR_ANY;
    ini_parse(CONFIGPATH, network_parse_config_line, NULL);
    serv_addr.sin_port = htons(LISTEN_PORT);

    /* reuse address */
    {
        int yes = 1;
        rc = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
        if (rc != 0)
        {
            WriteToLog("setsockopt: %d %s\n", errno, strerror(errno));
            network_exit();
            return -1;
        }
    }

    /* bind socket to listen address */
    rc = bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (rc != 0)
    {
        WriteToLog("bind: %d %s\n", errno, strerror(errno));
        network_exit();
        return -1;
    }

    /* listen on socket */
    rc = listen(listenfd, 5);
    if (rc != 0)
    {
        WriteToLog("listen: %d %s\n", errno, strerror(errno));
        network_exit();
        return -1;
    }

    return 0;
}

/*! deinitialize network subsystem */
void network_exit(void)
{

    WriteToLog("exiting network server\n");

    /* clean up all sessions */
    while (sessions != NULL)
        network_session_destroy(sessions);

    /* stop listening for new clients */
    if (listenfd >= 0)
        network_closesocket(listenfd, false);

    /* deinitialize socket driver */
    WriteToLog("Waiting for socketExit()...\n");
}

void network_post_exit(void)
{
}

/*! network look
 *
 *  @returns whether to keep looping
 */
loop_status_t
network_loop(void)
{
    int rc;
    struct pollfd pollinfo;
    network_session_t* session;

    /* we will poll for new client connections */
    pollinfo.fd = listenfd;
    pollinfo.events = POLLIN;
    pollinfo.revents = 0;

    /* poll for a new client */
    rc = poll(&pollinfo, 1, 0);
    if (rc < 0)
    {
        /* wifi got disabled */
        WriteToLog("poll: FAILED!\n");

        if (errno == ENETDOWN)
            return LOOP_RESTART;

        WriteToLog("poll: %d %s\n", errno, strerror(errno));
        return LOOP_EXIT;
    }
    else if (rc > 0)
    {
        if (pollinfo.revents & POLLIN)
        {
            /* we got a new client */
            if (network_session_new(listenfd) != 0)
            {
                return LOOP_RESTART;
            }
        }
        else
        {
            WriteToLog("listenfd: revents=0x%08X\n", pollinfo.revents);
        }
    }

    /* poll each session */
    session = sessions;
    while (session != NULL)
        session = network_session_poll(session);

    return LOOP_CONTINUE;
}

