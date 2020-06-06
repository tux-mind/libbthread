#include <pthread.h>
#include <errno.h>
#include <signal.h>


// bthred stuff

# define PTHREAD_CANCEL_ENABLE    	 0x00000010
# define PTHREAD_CANCEL_DISABLE   	 0x00000000

# define PTHREAD_CANCEL_ASYNCHRONOUS 0x00000020
# define PTHREAD_CANCEL_DEFERRED     0x00000000

#define PTHREAD_CANCELED ((void *) -1)

int pthread_setcancelstate (int , int *);
int pthread_setcanceltype (int , int *);
void pthread_testcancel (void);
int pthread_cancel (pthread_t t);


// p internal stuff

struct pthread_internal_t {
  struct pthread_internal_t* next;
  struct pthread_internal_t* prev;

  pid_t tid;

  void** tls;

  volatile pthread_attr_t attr;

  __pthread_cleanup_t* cleanup_stack;

  void* (*start_routine)(void*);
  void* start_routine_arg;
  void* return_value;

  void* alternate_signal_stack;

  /*
   * The dynamic linker implements dlerror(3), which makes it hard for us to implement this
   * per-thread buffer by simply using malloc(3) and free(3).
   */
#define __BIONIC_DLERROR_BUFFER_SIZE 508
  char dlerror_buffer[__BIONIC_DLERROR_BUFFER_SIZE];
	
	//  ugly hack: use last 4 bytes of dlerror_buffer as cancel_lock
	pthread_mutex_t cancel_lock; 
};

/* Has the thread a cancellation request? */
#define PTHREAD_ATTR_FLAG_CANCEL_PENDING 0x00000008

/* Has the thread enabled cancellation? */
#define PTHREAD_ATTR_FLAG_CANCEL_ENABLE 0x00000010

/* Has the thread asyncronous cancellation? */
#define PTHREAD_ATTR_FLAG_CANCEL_ASYNCRONOUS 0x00000020

/* Has the thread a cancellation handler? */
#define PTHREAD_ATTR_FLAG_CANCEL_HANDLER 0x00000040

struct pthread_internal_t *__pthread_getid ( pthread_t );

// int __pthread_do_cancel (struct pthread_internal_t *);

// void pthread_init(void);

void pthread_cancel_handler(int signum) {
	pthread_exit(0);
}

void pthread_init(void) {
	struct sigaction sa;
	struct pthread_internal_t * p = (struct pthread_internal_t *)pthread_self();
	
	if(p->attr.flags & PTHREAD_ATTR_FLAG_CANCEL_HANDLER)
		return;
	
	// set thread status as pthread_create should do.
	// ASYNCROUNOUS is not set, see pthread_setcancelstate(3)
	p->attr.flags |= PTHREAD_ATTR_FLAG_CANCEL_HANDLER|PTHREAD_ATTR_FLAG_CANCEL_ENABLE;
	
	sa.sa_handler = pthread_cancel_handler;
	sigemptyset(&(sa.sa_mask));
	sa.sa_flags = 0;
	
	sigaction(SIGRTMIN, &sa, NULL);
}




static void call_exit (void)
{
  pthread_exit (0);
}

int __pthread_do_cancel (struct pthread_internal_t *p)
{
	
	if(p == (struct pthread_internal_t *)pthread_self())
    call_exit ();
  else if(p->attr.flags & PTHREAD_ATTR_FLAG_CANCEL_HANDLER)
    pthread_kill((pthread_t)p, SIGRTMIN);
	else
		pthread_kill((pthread_t)p, SIGTERM);

  return 0;
}






// source files

int pthread_setcancelstate (int state, int *oldstate)
{
  struct pthread_internal_t *p = (struct pthread_internal_t*)pthread_self();
	int newflags;

	pthread_init();
	
  switch (state)
    {
    default:
      return EINVAL;
    case PTHREAD_CANCEL_ENABLE:
    case PTHREAD_CANCEL_DISABLE:
      break;
    }

  pthread_mutex_lock (&p->cancel_lock);
  if (oldstate)
    *oldstate = p->attr.flags & PTHREAD_ATTR_FLAG_CANCEL_ENABLE;
  
	if(state == PTHREAD_ATTR_FLAG_CANCEL_ENABLE)
		p->attr.flags |= PTHREAD_ATTR_FLAG_CANCEL_ENABLE;
	else
		p->attr.flags &= ~PTHREAD_ATTR_FLAG_CANCEL_ENABLE;
	newflags=p->attr.flags;
  pthread_mutex_unlock (&p->cancel_lock);

	if((newflags & PTHREAD_ATTR_FLAG_CANCEL_PENDING) && (newflags & PTHREAD_ATTR_FLAG_CANCEL_ENABLE) && (newflags & PTHREAD_ATTR_FLAG_CANCEL_ASYNCRONOUS))
		__pthread_do_cancel(p);
	
  return 0;
}

int pthread_setcanceltype (int type, int *oldtype)
{
  struct pthread_internal_t *p = (struct pthread_internal_t*)pthread_self();
	int newflags;
	
	pthread_init();
	
  switch (type)
    {
    default:
      return EINVAL;
    case PTHREAD_CANCEL_DEFERRED:
    case PTHREAD_CANCEL_ASYNCHRONOUS:
      break;
    }

  pthread_mutex_lock (&p->cancel_lock);
  if (oldtype)
    *oldtype = p->attr.flags & PTHREAD_ATTR_FLAG_CANCEL_ASYNCRONOUS;
	
	if(type == PTHREAD_CANCEL_ASYNCHRONOUS)
		p->attr.flags |= PTHREAD_ATTR_FLAG_CANCEL_ASYNCRONOUS;
	else
		p->attr.flags &= ~PTHREAD_ATTR_FLAG_CANCEL_ASYNCRONOUS;
	newflags=p->attr.flags;
  pthread_mutex_unlock (&p->cancel_lock);

	if((newflags & PTHREAD_ATTR_FLAG_CANCEL_PENDING) && (newflags & PTHREAD_ATTR_FLAG_CANCEL_ENABLE) && (newflags & PTHREAD_ATTR_FLAG_CANCEL_ASYNCRONOUS))
		__pthread_do_cancel(p);
	
  return 0;
}

void pthread_testcancel (void)
{
  struct pthread_internal_t *p = (struct pthread_internal_t*)pthread_self();
  int cancelled;
	
	pthread_init();

  pthread_mutex_lock (&p->cancel_lock);
  cancelled = (p->attr.flags & PTHREAD_ATTR_FLAG_CANCEL_ENABLE) && (p->attr.flags & PTHREAD_ATTR_FLAG_CANCEL_PENDING);
  pthread_mutex_unlock (&p->cancel_lock);

  if (cancelled)
    pthread_exit (PTHREAD_CANCELED);
		
}

int pthread_cancel (pthread_t t)
{
  int err = 0;
  struct pthread_internal_t *p = (struct pthread_internal_t*) t;
	
	pthread_init();
	
  pthread_mutex_lock (&p->cancel_lock);
  if (p->attr.flags & PTHREAD_ATTR_FLAG_CANCEL_PENDING)
    {
      pthread_mutex_unlock (&p->cancel_lock);
      return 0;
    }
    
  p->attr.flags |= PTHREAD_ATTR_FLAG_CANCEL_PENDING;

  if (!(p->attr.flags & PTHREAD_ATTR_FLAG_CANCEL_ENABLE))
    {
      pthread_mutex_unlock (&p->cancel_lock);
      return 0;
    }

  if (p->attr.flags & PTHREAD_ATTR_FLAG_CANCEL_ASYNCRONOUS) {
		pthread_mutex_unlock (&p->cancel_lock);
    err = __pthread_do_cancel (p);
	} else {
		// DEFERRED CANCEL NOT IMPLEMENTED YET
		pthread_mutex_unlock (&p->cancel_lock);
	}

  return err;
}