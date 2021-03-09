#include <func.h>

typedef struct pidnums{//有序退出备用，进程pid集合
    int client1;
    int client2;
    int screen1;
    int screen2;
}pidNums_t,*pPidNums_t;

typedef struct Msgbuf{//这次是真·消息队列
    long mtype;
    char mtext[64];
}Msgbuf_t,*pMsgbuf_t;

pPidNums_t pidsets;
int fdr=0,fdw=0;
int shmidPid=0;
int msgid;

void sigFunc2(int signum,siginfo_t *p,void *p1){//注册信号2的行为为kill -10
    printf("%d is coming\n",signum);
    kill(pidsets->client1,10);
    kill(pidsets->client2,10);
    kill(pidsets->screen1,10);
    kill(pidsets->screen2,10);
}

void sigFun10(int signum,siginfo_t *p,void *p1){//自定义退出行为,每个进程不同
    printf("%d is coming\n",signum);//检测用
    shmdt(pidsets);
    exit(0);
}


int main(int argc,char *argv[])
{
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
    //    fdr=open(argv[1],O_RDONLY);
    //    fdw=open(argv[2],O_WRONLY);
    //    printf("i am client2, fdr=%d fdw=%d\n",fdr,fdw);//测试输出
    //    char buf[128]={0};
    //    fd_set rdset;
    //    fd_set wrset;

    //client2与screen2
    msgid=msgget(1000,IPC_CREAT|0600);
    ERROR_CHECK(msgid,-1,"msgget");
    Msgbuf_t msgInfo;

    //pid快捷共享
    int pid=getpid();
    shmidPid=shmget(2000,4096,IPC_CREAT|0600);
    ERROR_CHECK(shmidPid,-1,"shmidPid");
    pidsets=(pPidNums_t)shmat(shmidPid,NULL,0);
    ERROR_CHECK(pidsets,(pPidNums_t)-1,"pid shmat");
    pidsets->screen2=pid;

    while(1){

        bzero(&msgInfo,sizeof(msgInfo));
        ret=msgrcv(msgid,&msgInfo,sizeof(msgInfo),0,0);
        ERROR_CHECK(ret,-1,"msgrcv");
        if(msgInfo.mtype==1){//对方的消息
            printf("%s\n\n",msgInfo.mtext);

        }
        if(msgInfo.mtype==2){//自己的消息
            printf("%*s%s\n",10,"",msgInfo.mtext);

        }

    }
    return 0;
}

