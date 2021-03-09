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
    kill(pidsets->screen1,10);
    kill(pidsets->screen2,10);
    kill(pidsets->client2,10);

}

void sigFun10(int signum,siginfo_t *p,void *p1){//自定义退出行为,每个进程不同
    printf("%d is coming\n",signum);//检测用
    shmdt(pidsets);
    msgctl(msgid,IPC_RMID,NULL);
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
    //printf("退出信号注册成功\n");
    //读写管道

    fdw=open(argv[1],O_WRONLY);
    fdr=open(argv[2],O_RDONLY);
    printf("i am client2, fdr=%d fdw=%d\n",fdr,fdw);//测试输出
    char buf[128]={0};
    fd_set rdset;
    fd_set wrset;

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
    pidsets->client2=pid;
    //printf("共享pid client2=%d\n",pidsets->client2);
    //printf("共享pid client1=%d\n",pidsets->client1);
    //printf("共享pid screen1=%d\n",pidsets->screen1);
    //printf("共享pid screen2=%d\n",pidsets->screen2);


    fdw=open(argv[1],O_WRONLY);
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
                msgInfo.mtype=2;//自己的消息
                strcpy(msgInfo.mtext,buf);
                ret=msgsnd(msgid,&msgInfo,strlen(msgInfo.mtext),0);
                ERROR_CHECK(ret,-1,"msgsnd");
            }
        }

        if(FD_ISSET(fdr,&rdset)){
            memset(buf,0,sizeof(buf));
            ret=read(fdr,buf,sizeof(buf));//返回值备用
            ERROR_CHECK(ret,-1,"read");

            if(strlen(buf)!=0){
                //puts(buf);
                msgInfo.mtype=1;//对方的消息
                strcpy(msgInfo.mtext,buf);
                // printf("test,buf=%s\n",buf);
                ret=msgsnd(msgid,&msgInfo,strlen(msgInfo.mtext),0);
                ERROR_CHECK(ret,-1,"msgsnd");
            }

        }

    }
    close(fdr);
    close(fdw);
    return 0;
}

