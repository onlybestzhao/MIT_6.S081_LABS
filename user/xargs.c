#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"



void fork_again(char *again_args[])
{
    int pid =fork();

    if (pid==0)
    {
       
        exec(again_args[0],again_args);
        
      
    }
    else
    {
        wait(0);
        
    }
   
    
}

int main(int argc,char *xargs[])
{
char buf[100];
char *input[100];
char *new_args[50];
int i;
int flag=0;
int a=0;
int o=0;
int b=0;
int count;
int type=0;
    
    if(strcmp(xargs[1],"-n")==0)
    {
        for (i=0;i<argc-3;i++)
        {
        new_args[i]=xargs[i+3];
        o++;
        b++;
        }
    }
    else
    {
        for (i=0;i<argc-1;i++)
        {
        new_args[i]=xargs[i+1];
        o++;
        b++;
        }
    }



while((count=read(0,buf,sizeof(buf)))>0)
{
    for(i=0;i<count;i++)
    {
    if (buf[i]==' ')
    {
        buf[i]=0;

        flag=0;
       
    }
    else if (buf[i]=='"')
    {
        buf[i]=0;
        if (flag)
        { 
            input[a]=&buf[i];
            a++;
            for (int j=0;j<a;j++)
            {
                new_args[o]=input[j];
                o++;
            }
            new_args[o]=0; 
            fork_again(new_args);
            
        }
        flag=0;
        type=1;
    }
     else if(buf[i]=='\\'&&buf[i+1]=='n')
    {
        buf[i]=0;
        buf[i+1]=0;
        i++;
        for (int j=0;j<a;j++) //把else if(!flag)里面的input数组里的参数放到new_args数组里
        {
            new_args[o]=input[j];
            o++;
        }
        new_args[o]=0; 
        fork_again(new_args);
        a=0;
      // i++;
        o=b;
        flag=0;
        type=1;
        
    }
    else if (buf[i]=='\n')
    {
        buf[i]=0;
        for (int j=0;j<a;j++) //把else if(!flag)里面的input数组里的参数放到new_args数组里
        {
            new_args[o]=input[j];
            o++;
        }
        new_args[o]=0; 
        fork_again(new_args);
        a=0;
      // i++;
        o=b;
        flag=0;
        type=1;
        
    }
    else if(!flag)  
    {
        input[a]=&buf[i];
        a++;
        flag =1 ;
    }
    }
}

if(!type)
    {
     for (int j=0;j<a;j++) 
        {
            new_args[o]=input[j];
            o++;
        }
        new_args[o]=0; 
    fork_again(new_args);
    }
// printf("buf[0]=%d\n",buf[0]);
// printf("buf[1]=%d\n",buf[1]);
// printf("buf[2]=%d\n",buf[2]);
// printf("buf[3]=%d\n",buf[3]);
// printf("buf[4]=%d\n",buf[4]);
// printf("buf[5]=%d\n",buf[5]);

// //最后一个参数必须是0，表示参数结束，否则exec会报错
// printf("new_args[0]=%s\n",new_args[0]);
// printf("new_args[1]=%s\n",new_args[1]);
// printf("new_args[2]=%s\n",new_args[2]);
// printf("new_args[3]=%s\n",new_args[3]);
// printf("new_args[4]=%s\n",new_args[4]);
// printf("new_args[5]=%s\n",new_args[5]);

// printf("xargs[0]=%s\n",xargs[0]);
// printf("xargs[1]=%s\n",xargs[1]);
// printf("xargs[2]=%s\n",xargs[2]);



 exit(0);
}