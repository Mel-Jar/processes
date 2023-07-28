//
// Created by Mason Landis on 3/4/22.
//
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <signal.h>

#define BUFFER_SIZE 32
int done;
int going;
int finished;
void handler1(int signum) {
    printf("parent: got a signal %d\n", signum);
    done = 1;
}
void handler2(int signum) {
    printf("child: got a signal %d\n", signum);
    finished = 1;
}
int main(int argc, char *argv[]) {
    going  = 1;
    char array[5][BUFFER_SIZE] = {"hello from me", "cake is amazing", "Ava is best cat", "Ava is A+", "done"};
    int result;
    int pid;
    struct sigaction action;
    int memid;
    int key = IPC_PRIVATE;
    char *ptr;
    int count = 0;
    char buffer[BUFFER_SIZE];
    strcpy(buffer, (const char *) array);

    memid = shmget(key, BUFFER_SIZE, IPC_EXCL | 0666);
    if (memid < 0) {
        printf("shmget() failed\n");
        return(8);
    }
    memset(&action, 0, sizeof(struct sigaction));
    pid = fork();
    if (pid < 0) {
        printf("fork failed\n");
        return(8);
    }

    if (pid > 0) {
        action.sa_handler = handler1;
        sigaction(SIGUSR1, &action, NULL);
        printf("I am the parent, and my pid is %d\n", getpid());
        // this is the parent
        while(going==1) {
            while ( ! done );
            done  = 0;
            ptr = (char *) shmat(memid, 0, 0);
            if (ptr == NULL) {
                printf("shmat() failed\n");
                return (8);
            }
            strcpy(buffer, (const char *) (array + count));
            printf("Parent is writing '%s' to the shared memory\n", buffer);
            strcpy(ptr, buffer);
            count ++;
            kill(pid, SIGUSR2);
            result  = strcmp(ptr, "done");
            if (result == 0)
                going = 0;
        }

        wait(NULL);

    } else {
        pid = getpid();
        action.sa_handler = handler2;
        printf("I am the child, and my pid is %d\n", pid);
        sigaction(SIGUSR2, &action, NULL);
        kill(getppid(), SIGUSR1);
        // this is the child
        while(going) {
            while ( ! finished );
            ptr = (char *) shmat(memid, 0, 0);
            if (ptr == NULL) {
                printf("shmat() in child failed\n");
                return(8);
            }
            printf("I am the child, and I read this from the shared memory: '%s'\n", ptr);
            finished = 0;
            result  = strcmp(ptr, "done");
            if (result == 0) {
                going = 0;
                printf("lets be done\n");
                shmdt(ptr);
                going = 0;
                return 0;
            } else {
                kill(getppid(), SIGUSR1);
                shmdt(ptr);
            }
        }
    }
    shmdt(ptr);
    shmctl(memid, IPC_RMID, NULL);

    return 0;
}
