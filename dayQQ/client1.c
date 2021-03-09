#include <func.h>

typedef struct pidnums{//有序退出备用，进程pid集合
    int client1;
    int client2;
    int screen1;
    int screen2;
}pidNums_t,*pPidNums_t;

typedef struct Msgbuf{//(协议)让消息带上类型属性
    long mtype;
    char mtext[64];
}Msgbuf_t,*pMsgbuf_t;

pPidNums_t pidsets;
int fdr=0,fdw=0;
int shmid=0;
int semArrID=0;
int shmidPid=0;
pMsgbuf_t p;

void sigFunc2(int signum,siginfo_t *p,void *p1){//注册信号2的行为为kill -10
    printf("%d is coming\n",signum);
    kill(pidsets->screen1,10);
    kill(pidsets->client2,10);
    kill(pidsets->screen2,10);
    kill(pidsets->client1,10);//不能先给自己发，容易被中断
}

void sigFun10(int signum,siginfo_t *p,void *p1){//自定义退出行为
    printf("%d is coming\n",signum);//检测用
    semctl(semArrID,0,IPC_RMID);
    shmdt(p);
    shmdt(pidsets);
    // sleep(5);
    shmctl(shmid,IPC_RMID,NULL);
    shmctl(shmidPid,IPC_RMID,NULL);
    close(fdr);
    close(fdw);
    exit(0);
}


int main(int argc,char *argv[])
{
    ARGS_CHECK(argc,3);
    struct sigaction act2;//修改ctrl+C信号的注册行为
    bzero(&act2,sizeof(act2));
    act2.sa_flags=SA_SIGINFO;
    act2.sa_sigaction=sigFunc2;
    int ret =sigaction(SIGINT,&act2,NULL);
    ERROR_CHECK(ret,-1,"sigaction 2");

    struct sigaction act10;
    bzero(&act10,sizeof(act10));
    act10.sa_flags=SA_SIGINFO;
    act10.sa_sigaction=sigFun10;
    ret=sigaction(10,&act10,NULL);
    ERROR_CHECK(ret,-1,"sigaction 10");

    //读写管道
    fdr=open(argv[1],O_RDONLY);
    fdw=open(argv[2],O_WRONLY);
    printf("i am client1, fdr=%d fdw=%d\n",fdr,fdw);//测试输出
    char buf[128]={0};
    fd_set rdset;
    fd_set wrset;

    //client1与screen1
    shmid=shmget(1000,4096,IPC_CREAT|0600);
    ERROR_CHECK(shmid,-1,"shmget");
    p=(pMsgbuf_t)shmat(shmid,NULL,0);
    ERROR_CHECK(p,(pMsgbuf_t)-1,"shmat");
    p->mtype=0;
    //屏幕和输入框互斥读写
    semArrID=semget(1000,1,IPC_CREAT|0600);
    ERROR_CHECK(semArrID,-1,"semget");
    ret=semctl(semArrID,0,SETVAL,1);
    ERROR_CHECK(ret,-1,"semctl");
    struct sembuf sopp,sopv;
    sopp.sem_num=0;
    sopp.sem_op=-1;
    sopp.sem_flg=SEM_UNDO;
    sopv.sem_num=0;
    sopv.sem_op=1;
    sopv.sem_flg=SEM_UNDO;
    //pid快捷共享
    int pid=getpid();
    shmidPid=shmget(2000,4096,IPC_CREAT|0600);
    ERROR_CHECK(shmidPid,-1,"shmidPid");
    pidsets=(pPidNums_t)shmat(shmidPid,NULL,0);
    ERROR_CHECK(pidsets,(pPidNums_t)-1,"pid shmat");
    pidsets->client1=pid;


    while(1){
        FD_ZERO(&rdset);
        FD_SET(STDIN_FILENO,&rdset);
        FD_SET(fdr,&rdset);
        ret=select(fdw+1,&rdset,NULL,NULL,NULL);
        ERROR_CHECK(ret,-1,"select");

        if (FD_ISSET(STDIN_FILENO,&rdset)){
            memset(buf,0,sizeof(buf));
            ret=read(STDIN_FILENO,buf,sizeof(buf));

            if(strcmp(buf,"\n")!=0){
                write(fdw,buf,strlen(buf)-1);
                semop(semArrID,&sopp,1);
                p->mtype=2;
                strcpy(p->mtext,buf);
                semop(semArrID,&sopv,1);
            }
        }

        if(FD_ISSET(fdr,&rdset)){
            memset(buf,0,sizeof(buf)-1);
            ret=read(fdr,buf,sizeof(buf));//返回值备用
            semop(semArrID,&sopp,1);
            p->mtype=1;
            strcpy(p->mtext,buf);
            semop(semArrID,&sopv,1);
            //puts(buf);
        }

    }
    return 0;
}

