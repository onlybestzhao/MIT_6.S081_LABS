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
int count = 0;
int account = 0;
int fd[2];
int pd[2];
int ad[2];
void pipeline(int arra[])
{
     for(int i=0;i<34;i++)
    {  
        if (arra[i]%arra[0]==0&&arra[i]!=2)
        {
           account++;
            continue;
        }
        else
        { 
            count++;
             write(fd[1],&arra[i],sizeof(int));
             
            
            if (count==1)
            printf("prime %d\n",arra[count+account-1]);
           
        }
      
    }
    count=0;
    account=0;
}


int main ()
{
    // volatile int flag =1;


pipe (pd);
pipe(fd);
pipe(ad);
int array_35[34] ;
int buf[20];
array(array_35);
int pid =fork();

if (pid ==0)  //子进程
{


}   

else  //父进程
{ 


   
}
exit(0);

}