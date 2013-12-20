#include <xcopy.h>
#include <tc_epoll_module.h>

typedef struct tc_select_multiplex_io_s   tc_select_multiplex_io_t;

struct tc_select_multiplex_io_s {
#ifdef USE_EPOLL
    struct epoll_event   event;
    struct epoll_event  *events;
    int                  efd;
#else
    int             max_fd;
    int             last;
    fd_set          r_set;
    fd_set          w_set;
#endif
    tc_event_t    **evs;
};

int tc_epoll_create(tc_event_loop_t *loop)
{
    tc_event_t               **evs;
    tc_select_multiplex_io_t  *io;
    struct epoll_event        *events; 
    int  efd;

    evs = malloc(loop->size * sizeof(tc_event_t *));
    if (evs == NULL) {
        return TC_EVENT_ERROR;
    }

    io = malloc(sizeof(tc_select_multiplex_io_t));
    if (io == NULL) {
        goto bad;
    }

    efd = epoll_create1(0);
    if(efd == -1)
        goto bad;
 
    events = calloc(MAX_FD_NUM, sizeof(struct epoll_event));
    if(NULL == events)
        goto bad;

    io->efd = efd;
    io->events = events;
    io->evs = evs;

    loop->io = io;

    return TC_EVENT_OK;

bad:
    free(evs);
    free(events);
    if (efd != -1) {
        close(efd);
    }

    return TC_EVENT_ERROR;
}

int tc_epoll_destroy(tc_event_loop_t *loop)
{
    int                       i;
    tc_event_t               *event;
    tc_select_multiplex_io_t *io;

    io = loop->io;

    for (i = 0; i < io->last; i++) {
        event = io->evs[i];
        if (event->fd > 0) {
            tc_log_info(LOG_NOTICE, 0, "tc_epoll_destroy, close fd:%d", 
                    event->fd);
            tc_socket_close(event->fd);
        }
        event->fd = -1;
        free(event);
    }

    if (io->efd) {
        close(io->efd);
        io->efd = -1;
    }

    free(io->events);
    free(io->evs);
    free(loop->io);

    return TC_EVENT_OK;
}

int tc_epoll_add_event(tc_event_loop_t *loop, tc_event_t *ev, int events)
{
    tc_select_multiplex_io_t *io;
    struct epoll_event        event;

    io = loop->io;

    if (io->last >= loop->size) {
        /* too many */
        return TC_EVENT_ERROR;
    }

    event.data.fd = ev->fd;
    //event.events = EPOLLIN | EPOLLET;
    event.events = EPOLLIN;
    s = epoll_ctl(efd, EPOLL_CTL_ADD, ev->fd, &event);
    if(s == -1)
        return TC_EVENT_ERROR;
    }

    if (events == TC_EVENT_READ && ev->read_handler
            && ev->write_handler == NULL)
    {
        FD_SET(ev->fd, &io->r_set);
    } else if (events == TC_EVENT_WRITE && ev->write_handler
            && ev->read_handler == NULL)
    {
        FD_SET(ev->fd, &io->w_set);
    } else {
        return TC_EVENT_ERROR;
    }

    if (io->max_fd != -1 && ev->fd > io->max_fd) {
        io->max_fd = ev->fd;
    }

    ev->index = io->last;
    io->evs[io->last++] = ev;

    return TC_EVENT_OK;
}

int tc_epoll_del_event(tc_event_loop_t *loop, tc_event_t *ev, int events)
{
    return TC_EVENT_OK;
}

int tc_epoll_polling(tc_event_loop_t *loop, long to)
{
    return TC_EVENT_OK;
}

