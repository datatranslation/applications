/* 
 * This is a test of the DT78xx Asynchronous IO file operations. 
 * 
 * This code is based on the example documented in
 *  http://www.fsl.cs.sunysb.edu/~vass/linux-aio.txt 
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>         /* for syscall() */
#include <sys/syscall.h>	/* for __NR_* definitions */
#include <linux/aio_abi.h>	/* for AIO types and constants */

#include "dt78xx_ioctl.h"
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"

#define MAX_CONCURRENT_IO   (8)
#define _MEM_ALIGN          (1)
enum AIO_OP
{
    AIO_READ = 1,
    AIO_WRITE = 2,
};

aio_context_t g_aio_context; //Asynchronous I/O context

extern unsigned int xcrc32 (const unsigned char *buf, int len, 
                                unsigned int init);

static inline int io_setup(unsigned nr, aio_context_t *ctxp)
{
    return syscall(__NR_io_setup, nr, ctxp);
}

static inline int io_destroy(aio_context_t ctx) 
{
    return syscall(__NR_io_destroy, ctx);
}

static inline int io_submit(aio_context_t ctx, long nr,  struct iocb **iocbpp) 
{
    return syscall(__NR_io_submit, ctx, nr, iocbpp);
}

static inline int io_getevents(aio_context_t ctx, long min_nr, long max_nr,
    struct io_event *events, struct timespec *timeout)
{
    return syscall(__NR_io_getevents, ctx, min_nr, max_nr, events, timeout);
}    

static inline int io_cancel(aio_context_t ctx, struct iocb *iocb,
                 struct io_event *result)
{
    return syscall(__NR_io_cancel, ctx, iocb, result);
} 

static int eventfd(int count) 
{
	return syscall(__NR_eventfd, count);
}
/*
 * Command line arguments
 */
static void usage(const char *filename)
{
    fprintf(stderr,"AIO read/write test\n");
    fprintf(stderr,"Usage\n%s -b blocks -l len [-c] [-r|-w] file\n", filename);
    fprintf(stderr,"\tblocks is the number of iocb to submit, default 3\n");
    fprintf(stderr,"\tlen is the number of bytes in each iocb, default 1024\n");
    fprintf(stderr,"\tc will continuously resubmit iocb's which complete\n");
    fprintf(stderr,"\t  otherwise the test will finish after 'blocks' are done.\n");
    fprintf(stderr,"\tr reads from file, w writes to file. Default is rw\n");
    fprintf(stderr,"\t  file is one of /dev/dt78xx-????\n");
}

/*
 * Signal handler for ctrl-C does nothing. It prevents the process from 
 * terminating so that io_getevents() can be interrupted and return
 */
static void intHandler(int i) 
{
}
/**
 * Free array of IO control blocks
 * @param cbs       : pointer to array of pointers to IO control blocks
 * @param num_iocb  : number of IO control blocks
 */
void free_iocbs(struct iocb ** cbs, int num_iocb)
{
    int i;
    for (i=0; i < num_iocb; ++i)
    {
        struct iocb *cb = cbs[i];
        if (cb) 
        {
            if (cb->aio_buf)
            {
                free((void *)(cb->aio_buf));
            }
            free(cb);
        }
    }
    free(cbs);
}
struct iocb ** allocate_iocbs(int file, enum AIO_OP op, int num_iocb, int buff_len)
{
    struct iocb **cbs;
    int i;
    
    cbs = (struct iocb **)malloc(sizeof(struct iocb *) * num_iocb);
    if (!cbs)
    {
        fprintf(stderr, "ERROR malloc cbs\n");
        return (NULL);
    }
    memset(cbs, 0, (sizeof(struct iocb *) * num_iocb));
    // setup I/O control blocks
#ifdef _MEM_ALIGN
    int page = getpagesize();  
#endif
    for (i=0; i < num_iocb; ++i)
    {
        struct iocb *cb = (struct iocb *)malloc(sizeof(struct iocb));
        if (!cb)
        {
            fprintf(stderr, "ERROR malloc cb\n");
            break;
        }
        memset(cb, 0, sizeof(struct iocb));
        cbs[i] = cb;
#ifdef _MEM_ALIGN
        uint8_t *buff = (uint8_t *)memalign(page,buff_len);
#else        
        uint8_t *buff = (uint8_t *)malloc(buff_len);
#endif        
        if (!buff)
        {
            fprintf(stderr, "ERROR malloc buff of len %d\n",buff_len);
            break;
        }
        cb->aio_fildes = file;
        cb->aio_lio_opcode = (op==AIO_READ)?IOCB_CMD_PREAD:IOCB_CMD_PWRITE;
        cb->aio_buf = (__u64)(void *)buff;
        cb->aio_offset = 0;
        cb->aio_nbytes = buff_len;
        cb->aio_data = 1; //user identifier
        
        fprintf(stdout, "iocb%d %p buff=%llx\n", i, cb, cb->aio_buf);
    }
    
    //Handle memory allocation failures
    if (i < num_iocb)
    {
        free_iocbs(cbs, i);
        return NULL;
    }
    
    return cbs;
}
/*
 * Command line arguments see usage above
 */
int main (int argc, char** argv)
{
    int ret = EXIT_SUCCESS;
    int fd = 0;  //DEVICE FILE HANDLE
    int i;
    struct iocb **iocbs = NULL;
    struct iocb **iocbs_done = NULL;
    struct io_event *events = NULL;
    
    int opt;
    int block_len = 1024;
    int blocks = 3;
    uint32_t total_len = 0;
    int mode = O_RDWR;
    const char *filename = NULL;
    int forever = 0;
    while ((opt = getopt(argc, argv, "l:b:rwc")) != -1) 
    {
        switch (opt) 
        {
            case 'c':
                forever = 1;
                break;
            case 'l':
                block_len = strtoul(optarg, NULL, 10);
                break;
            
            case 'b':
                blocks = strtoul(optarg, NULL, 10);
                break;
            
            case 'r':
                mode = O_RDONLY;
                break;
                
            case 'w':
                mode = O_WRONLY;
             break;
        }
    }  
    if ((block_len <= 0) || (blocks <=0) || (optind==argc))
    {
        usage(argv[0]);
        return (EXIT_FAILURE);
    }
    total_len = block_len * blocks;
    
    filename = argv[optind];
    fd = open(filename, mode);
    if (fd < 0)
    {
        perror("open ");
        return (EXIT_FAILURE);
    }
    fprintf(stdout, "%s opened for %s\n", filename, 
            (mode==O_WRONLY)?"write":(mode==O_RDONLY)?"read":"rw");
    
    //Set up ctrl-C handler to terminate infinite wait in io_getevents()
    struct sigaction act;
    act.sa_handler = intHandler;
    sigaction(SIGINT, &act, NULL);
#ifdef _MEM_ALIGN  
    int page = getpagesize();
    fprintf(stdout, "Each %d byte request allocated on a %d page size\n",
                     block_len, page);
#endif
    //allocation events to read asynchronous completion
    events = malloc(sizeof(struct io_event) * blocks);
    if (!events)
    {
        fprintf(stderr, "ERROR io_submit returned %d\n", ret);
        ret = EXIT_FAILURE;
        goto finish;
    }
    
    //Create the asynchronous IO context
    g_aio_context = 0;
    ret = io_setup(MAX_CONCURRENT_IO, &g_aio_context); 
    if (ret < 0)
    {
        perror("ERROR io_setup");
        goto finish;
    }
    //allocate an array of IO control blocks for AIO operation
    iocbs = allocate_iocbs(fd, AIO_WRITE, blocks, block_len);
    if (!iocbs)
    {
        fprintf(stderr, "ERROR malloc cbs\n");
        ret = EXIT_FAILURE;
        goto finish;
    }
    iocbs_done = (struct iocb **)malloc(sizeof(struct iocb *) * blocks);
    if (!iocbs_done)
    {
        fprintf(stderr, "ERROR malloc cbs\n");
        ret = EXIT_FAILURE;
        goto finish;
    }    
    //submit a bunch of requests
    uint32_t blocks_submitted=0;;
    uint32_t blocks_done = 0;
    uint32_t len_done = 0;
    
    ret = io_submit(g_aio_context, blocks, iocbs);
    if (ret != blocks) 
    {
        if (ret < 0)
        {
            perror("io_submit error");
        }
        else
        {
            fprintf(stderr, "ERROR io_submit returned %d\n", ret);
            ret = EXIT_FAILURE;
        }
        goto finish;
    }
    blocks_submitted = blocks;
    fprintf(stdout,"Queued %d blocks each of %d bytes\n", blocks, block_len);
    
    //Wait for user input to start or abort
    fprintf(stdout,"Press s to start, a to abort\n");
    ret = 0;
    while (1)
    {
        int c = getchar();
        if (c == 's')
        {
            ret = ioctl(fd, IOCTL_START_SUBSYS, 0);    
            break;
        }
        if ((c == 'a') || (c == 'q'))
        {
            goto cleanup;
        }
    }
    if (ret)
    {
        fprintf(stderr, "ERROR %d \"%s\"\n", errno, strerror(errno));
        goto cleanup;
    }
    uint32_t resubmit =0;
    
    //wait for completion of AIO requests
    fprintf(stdout, "Press CTRL-c to abort\n");   
    while (forever || (blocks_done != blocks))
    {
        ret = io_getevents(g_aio_context, 1, blocks, events, NULL);
        if (ret <= 0) //if ctrl-C is hit and the wait interrupted
        {
            fprintf(stderr, "ERROR io_getevents returned %d\n", ret);
            break;
        }
        blocks_done += ret;
        for (i=0; i < ret; ++i)
        {
            struct iocb *p = (struct iocb *)events[i].obj;
            iocbs_done[i] = p;
            fprintf(stdout, "\nWrote iocb %p buff %llx data %u length %lld", 
                             p, p->aio_buf, events[i].data, 
                             events[i].res);  
            if (events[i].res2 != 0) //errors
            {
                fprintf(stderr, "\nERROR %lld\n", events[i].res2);
            }
            if (p->aio_data != events[i].data)
            {
                fprintf(stderr, "\nERROR aio_data %llx evt_data %llx\n", 
                        p->aio_data, events[i].data);
            }
            
            p->aio_nbytes = forever ? block_len : 0;
            ++p->aio_data;
            len_done += (uint32_t)events[i].res;
        }
            
        //Resubmit the iocb that was completed
        if (forever)
        {
            io_submit(g_aio_context, ret, iocbs_done);
            blocks_submitted += ret;
            fprintf(stdout, " resubmit %d", ret);
            if (ret > resubmit)
            {
                resubmit = ret;
            }
        }
    }
loop_exit:    
    ioctl(fd, IOCTL_STOP_SUBSYS, 0);    

cleanup:   
    //Cancel iocb's that were aborted due to CTRL-c or user abort
    for (i=0; i< blocks; ++i)
    {
        struct iocb *p = iocbs[i];
        if (p->aio_nbytes)
        {
            io_cancel(g_aio_context, p, events);                
            fprintf(stdout, "Cancelled iocb %p buff %llx data %u length %lld\n", 
                             p, p->aio_buf, p->aio_data, p->aio_nbytes);  
        }
    }
    
    if (len_done == 0)
    {
        goto finish;
    }

    fprintf(stdout, "\nsubmitted %u done %u\n",blocks_submitted, blocks_done); 
    if (forever)
    {
        fprintf(stdout, "Completed %d bytes Max resubmit %u\n",len_done, resubmit);
    }
    else
    {
        fprintf(stdout, "Completed %d of %d bytes\n", len_done, total_len);
    }

finish : 
    if (events)
    {
        free(events);
    }
    if (iocbs_done)
    {
        free(iocbs_done);
    }
    if (iocbs)
    {
        free_iocbs(iocbs, blocks);
    }
    io_destroy(g_aio_context);
    if (fd > 0)
    {
        close(fd);
        fprintf(stdout, "%s closed\n", filename);
    }
    return (ret);
}

