#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
const char *argv;
// int fd;
//  char buf[DIRSIZ+1];
// char *p;
// struct dirent de;
//   struct stat st; // 这些需要递归的变量，必须在find函数里面去定义，不能作为全局变量，一旦作为全局变量递归时就会被共享，直接导致错误
int i; //从父层开始遍历
void find ( char *path,char *target)
{int fd;
 char buf[512];
char *p;
struct dirent de;
  struct stat st;
    if(( fd = open(path, O_RDONLY)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return ;
  }
strcpy(buf,path); //把argv[1]的字符串复制到buf字符串数组，第一次运行
p=buf+strlen(buf);  
*p= '/';
p=p+1;
while(read(fd,&de,sizeof(de))==sizeof(de))
{
    if(de.inum==0)
    {
      continue;
    }
    if(strcmp(de.name,".")==0 || strcmp(de.name,"..")==0)
    {
      continue; 
    }

  memmove(p,de.name,DIRSIZ); //p和buf是指向同一块内存的指针，buf是字符数组，其中p指向buf的最后一个字符的下一个位置，de.name是目录项的名字，DIRSIZ是目录项名字的最大长度
  p[DIRSIZ]=0;
  if(stat(buf,&st)<0)//循环查看buf路径下的文件或目录的状态信息，存储在st结构体中
  {
    fprintf(2,"find: cannot stat %s\n",buf);
    continue;
  }
  if(st.type==1)
  {
    find(buf,target);
    // if(strcmp(de.name,target)==0) 
    // {
     // printf("%s\n",buf);
    // }
  }
  else if(st.type==2&&strcmp(de.name,target)==0) //字符串之前的比较要用strcmp，不能直接==，如果用==，比较的是指针的地址，而不是字符串的内容
  {
    printf("%s\n",buf);
  }
  // else if(st.type==3)
  // {
  //   printf("find: %s is a device file\n",buf);
  // }
  // else if(st.type!=1&&st.type!=2&&st.type!=3)
  // {
  //   fprintf(2,"find: %s is not a file or directory\n",buf);
  // }


}



}



// void search (char *argv[],char target)
// {
//    if(stat(buf, &st) < 0){
//         printf("ls: cannot stat %s\n", buf);
      
//       }
//     while(st.type==1)
//     {
      
//       find(argv);
      
//       continue;
   
//     }

//   printf("%s\n",de.name);
// }


  
int main(int argc,char *argv[]) //argc_min=2
{ 
    i=argc-1;
    if(argc<2) //没有目标文件，argc=1 说明shell里只输入了find
    {
        fprintf(2,"find: no target\n");//用fprintf而不用printf是因为fprintf可以指定输出到标准错误流2，而printf默认输出到标准输出流1
        exit(1);
    }
    find(argv[1],argv[i]);

    return 0;
    exit(0);
}