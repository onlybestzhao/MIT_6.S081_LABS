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
// int pd[2]; //这bug不是问AI我真想不到，tmd文件描述符随着递归不够用了
// int ad[2]; //由于之前尝试了很多种写法，这边就没管
              //现在看来任何资源都不能浪费
              //我现在是只关了写端，没有关读端，关读端应该是可以更省资源，递归更多次
void pipeline(int bd) //fd[0]
{  
   int buf;
   

   int flag=read(bd,&buf,sizeof(int));
     if(flag==0) //这边flag报-1了，其实我应该注意到的，说明fd不够用了
   {
    exit(0);
   }
   printf("prime %d\n",buf);
 
   int cd[2];
    pipe(cd);

    if(fork()==0)
    {    close(cd[1]);
        pipeline(cd[0]);
    }
    else
    {   
        int buf2;
        while(read(bd,&buf2,sizeof(int))>0)
        {      
            if(buf2%buf!=0) 
                {
                write(cd[1],&buf2,sizeof(int));
                }
                
        }
        close(cd[1]);
        
        wait(0);
   
    

   }
    
}


int main ()
{
    // volatile int flag =1;


// pipe (pd);
pipe(fd);
// pipe(ad);
int array_35[34] ;

array(array_35);
int pid =fork();

if (pid ==0)  //子进程
{   close(fd[1]);
    pipeline(fd[0]);
  
}   

else  //父进程
{ 

for(int i=0;i<34;i++)
{
    write(fd[1],&array_35[i],sizeof(int));
}
close(fd[1]);
wait(0);


}
exit(0);

}