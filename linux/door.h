# if !defined(DOOR_H)
# define	DOOR_H

/* Basic door type information */
typedef unsigned long long door_ptr_t;  /* Handle 64 bit pointers */
typedef unsigned long long door_id_t;   /* Unique door identifier */
typedef unsigned int       door_attr_t; /* Door attributes */

/*
 * Structure used to return info from door_info
 */
typedef struct door_info {
        pid_t           di_target;      /* Server process */
        door_ptr_t      di_proc;        /* Server procedure */
        door_ptr_t      di_data;        /* Data cookie */
        door_attr_t     di_attributes;  /* Attributes associated with door */
        door_id_t       di_uniquifier;  /* Unique number */
        int             di_resv[4];     /* Future use */
} door_info_t;

/*
 * System call subcodes
 */
#define DOOR_CREATE     0
#define DOOR_REVOKE     1
#define DOOR_INFO       2
#define DOOR_CALL       3
#define DOOR_BIND       6
#define DOOR_UNBIND     7
#define DOOR_UNREFSYS   8
#define DOOR_UCRED      9
#define DOOR_RETURN     10
#define DOOR_GETPARAM   11
#define DOOR_SETPARAM   12

# endif
