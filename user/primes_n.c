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
int buf2[1];
array(array_35);
int pid =fork();

if (pid ==0)  //子进程
{

        read(pd[0],buf2,sizeof(buf2)); 

        read(fd[0],buf,sizeof(int)*buf2[0]); 
        printf("prime %d\n",buf[1]);
           

    pipeline(buf);
    int pi=fork();
    if (pi==0)
    {
        read(fd[0],buf,sizeof(int)*buf2[0]);
        printf("prime %d\n",buf[1]);
    }

}   

else  //父进程
{ 
   pipeline(array_35);
    // for(int i=0;i<34;i++)
    // {  
    //     if (array_35[i]%array_35[0]==0&&array_35[i]!=2)
    //     {
    //        account++;
    //         continue;
    //     }
    //     else
    //     { 
    //         count++;
    //          write(fd[1],&array_35[i],sizeof(int));
             
            
    //         if (count==1)
    //         printf("prime %d\n",array_35[count+account-1]);
           
    //     }
      
    // }
     write(pd[1],&count,sizeof(int));
    close(pd[1]);
      close(fd[1]);
        wait(0);

   
}
exit(0);

}