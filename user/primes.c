#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


void array(int number[])
{

for(int i=0;i<34;i++) 
{
    number[i]=i+2;
}
}

int b;



int prime (int single_prime)
{
  
    if(single_prime%2==0&&single_prime!=2) 
    {
        return 0 ;
    }
       else
    {
        for(int i=3;i*i<=single_prime;i+=2)
        {
            if(single_prime%i==0)
            return 0;
        }
        return single_prime;
    }
}

int main ()
{
    // volatile int flag =1;
int fd[2];
pipe(fd);
int array_35[34] ;
int buf[1];
array(array_35);
int pid =fork();
if (pid ==0)  //子进程
{
    for(int n=0;n<35;n++)
    {

   
        read(fd[0],buf,sizeof(buf)); //return 4 Int的字节数
        close(fd[0]);

    }


    

}   

else  //父进程
{
    for(int n=0;n<34;n++)
{
   
    int output_prime =  prime(array_35[n]);
        if (output_prime!=0)
        {
            //  write(fd[1],&array_35[n],sizeof(array_35));
            //  close(fd[1]);
            printf("prime %d\n",output_prime);
        }
}
   
}
exit(0);

}