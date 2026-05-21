#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main ()
{ 
//     int pid = fork();
   
//     int pid_value = pipe(&pid);

//     write(0,&pid_value,pid);
    
//    int a= read(0,&pid_value,sizeof(pid_value));
//  printf("%d:received ping\n",a);
   
//     int pi = getpid();
//     int pi_value = pipe(&pi);

//     write(1,&pi_value,pi);

//     int b = read (1,&pi_value,sizeof(pi_value));

//     printf("%d:received pong\n",b);

int fd[2];
int sd[2]; // 文件描述符需要用内核去操作，不能直接在user里面去操作
// int buf[4];
// char bu[2];
pipe(fd);
pipe(sd);

int pid = fork();
int a =2;
char buf_2[1];
char buf[1];
if(pid == 0) //子进程 
{
   int pi=getpid();
  read(sd[0],buf,1);
    if (buf[0]==2)
    {
        printf("%d:received ping\n",pi);
        write(fd[1],&a,sizeof(a));
    }

}
else //父进程
{
    int p =getpid();
    write(sd[1],&a,sizeof(a));
    read(fd[0],buf_2,1);
    if (buf_2[0]==2)
    {
     printf("%d:received pong\n",p);

    }


}

// if(pid==0) //两个进程，同步运行，会出现乱码
// {
//     int pi=getpid();
   
//     printf("%d:received ping\n",pi);
// }
// else 
// {
//    int p =fetpid();
//  printf("%d:received pong\n",fd[1]);
    
// }

exit(0);    
}
// write(pid,fd,1);
 // int pi =getpid();
    // if (pi==fd[1]){
    // }