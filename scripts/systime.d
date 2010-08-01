/* Script to time how long non-boring syscalls take. */

syscall:::entry                                                                          
{                                                                                        
        x[pid, probefunc] = timestamp;                                                   
}                                                                                        
syscall:::return                                                                         
/probefunc != "epoll_wait" && timestamp - x[pid, probefunc] > 100000/                    
{                                                                                        
        printf("%-20s %d", execname, timestamp - x[pid, probefunc]);                     
}     
