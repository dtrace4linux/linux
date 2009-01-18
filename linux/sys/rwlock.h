#if !defined(RWLOCK_H_INCLUDE)
# define RWLOCK_H_INCLUDE

typedef enum {
        RW_WRITER,
        RW_READER
} krw_t;

void    rw_enter(krwlock_t *, krw_t);
void	rw_exit(krwlock_t *);

#endif
