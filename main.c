#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <unistd.h>

#define MAX_TABLE_NUMBER            10
#define MAX_PHILOSOPHER_PER_TABLE   8
#define PORTION_PER_CYCLE           100
#define RICE_BOWL_WEIGHT            500
#define TABLE_OPENING_PRICE         99.90
#define TABLE_REFRESH_PRICE         19.90
#define RICE_PRICE_PER_BOWL         5.90

#define GET_LEFT_FORK(philosopherID) philosopherID % MAX_PHILOSOPHER_PER_TABLE
#define GET_RIGHT_FORK(philosopherID) (philosopherID + 1) % MAX_PHILOSOPHER_PER_TABLE

typedef struct Table
{
    int iTableID;
    int iTableRefreshCount;
    int iRemainingRice;
    int aiEatState[MAX_PHILOSOPHER_PER_TABLE];
} TableStruct;

typedef struct Group
{
    pthread_t philosophers[MAX_PHILOSOPHER_PER_TABLE];
    pthread_mutex_t groupForkMutex[MAX_PHILOSOPHER_PER_TABLE];
    pthread_mutex_t tableLock;
    int forks[MAX_PHILOSOPHER_PER_TABLE];
    TableStruct table;
} GroupStruct;

GroupStruct *s_pGroupStatus;
pthread_mutex_t s_tables[MAX_TABLE_NUMBER];

int getRandomNumber(int maxNumber)
{
    return (rand() % maxNumber) + 1;
}

int isEveryoneEat(int iGroupID)
{
    int iIndex;
    int iStatus = 1;

    for(iIndex=0; iIndex < MAX_PHILOSOPHER_PER_TABLE; iIndex++)
    {
        iStatus &= s_pGroupStatus[iGroupID].table.aiEatState[iIndex];
    }

    return iStatus;
}

void *keepCalmAndEatRice(int iPhilosopherID)
{
    int iGroupID = iPhilosopherID / MAX_PHILOSOPHER_PER_TABLE;
    int iPhiID = iPhilosopherID % MAX_PHILOSOPHER_PER_TABLE;

    while(isEveryoneEat(iGroupID) == 0)
    {
        pthread_mutex_lock(&s_pGroupStatus[iGroupID].tableLock);
        if (s_pGroupStatus[iGroupID].table.iRemainingRice < PORTION_PER_CYCLE)
        {
            s_pGroupStatus[iGroupID].table.iRemainingRice = RICE_BOWL_WEIGHT;
            s_pGroupStatus[iGroupID].table.iTableRefreshCount += 1;
        }
        pthread_mutex_unlock(&s_pGroupStatus[iGroupID].tableLock);

        // Think
        int thinkTime = getRandomNumber(5);
//        printf("Philosopher %d is thinking for %d seconds.\n", iPhilosopherID, thinkTime);
        usleep(thinkTime);

        // Get forks
        if(iPhiID != MAX_PHILOSOPHER_PER_TABLE - 1){
            pthread_mutex_lock(&s_pGroupStatus[iGroupID].groupForkMutex[GET_LEFT_FORK(iPhiID)]);
            pthread_mutex_lock(&s_pGroupStatus[iGroupID].groupForkMutex[GET_RIGHT_FORK(iPhiID)]);
        } else {
            pthread_mutex_lock(&s_pGroupStatus[iGroupID].groupForkMutex[GET_RIGHT_FORK(iPhiID)]);
            pthread_mutex_lock(&s_pGroupStatus[iGroupID].groupForkMutex[GET_LEFT_FORK(iPhiID)]);
        }
        s_pGroupStatus[iGroupID].forks[GET_LEFT_FORK(iPhiID)] = iPhiID+1;
        s_pGroupStatus[iGroupID].forks[GET_RIGHT_FORK(iPhiID)] = iPhiID+1;

        pthread_mutex_lock(&s_pGroupStatus[iGroupID].tableLock);
        s_pGroupStatus[iGroupID].table.iRemainingRice -= PORTION_PER_CYCLE;
        s_pGroupStatus[iGroupID].table.aiEatState[iPhiID] = 1;
        pthread_mutex_unlock(&s_pGroupStatus[iGroupID].tableLock);

        // Eat
        int eatTime = getRandomNumber(5);
//        printf("\t\tPhilosopher %d is eating for %d seconds.\n", iPhilosopherID, eatTime);
        usleep(eatTime);

        // Return Forks
        s_pGroupStatus[iGroupID].forks[GET_LEFT_FORK(iPhiID)] = 0;
        s_pGroupStatus[iGroupID].forks[GET_RIGHT_FORK(iPhiID)] = 0;
        pthread_mutex_unlock(&s_pGroupStatus[iGroupID].groupForkMutex[GET_LEFT_FORK(iPhiID)]);
        pthread_mutex_unlock(&s_pGroupStatus[iGroupID].groupForkMutex[GET_RIGHT_FORK(iPhiID)]);
    }

    pthread_exit(NULL);
}

int main(int argc, char** argv)
{
    int iGroupNumber;
    int iGroupIndex;
    int iPhilosopherIndex;
    int iCurrentPhilosopher;
    int iTotalBill;
    pthread_attr_t tattr;
    pthread_t tid;
    struct sched_param param;

    srand(time(NULL));

    if (argc > 0)
    {
        iGroupNumber = atoi(argv[1]);
    }
    else
    {
        printf("Incorrect Usage! Group count must be provided\n");
    }
    if (iGroupNumber > 0)
    {
        s_pGroupStatus = (GroupStruct *)malloc(iGroupNumber * sizeof(GroupStruct));

        for (iGroupIndex = 0; iGroupIndex < iGroupNumber; iGroupIndex++)
        {
            memset(&s_pGroupStatus[iGroupIndex].forks, 1, sizeof(int) * MAX_PHILOSOPHER_PER_TABLE);
            s_pGroupStatus[iGroupIndex].tableLock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
        }

        for(iPhilosopherIndex=0; iPhilosopherIndex < MAX_PHILOSOPHER_PER_TABLE * iGroupNumber; iPhilosopherIndex++)
        {
            iGroupIndex = (iPhilosopherIndex / MAX_PHILOSOPHER_PER_TABLE);
            iCurrentPhilosopher = (iPhilosopherIndex % MAX_PHILOSOPHER_PER_TABLE);

            pthread_mutex_init(&s_pGroupStatus[iGroupIndex].groupForkMutex[iCurrentPhilosopher], NULL);
        }

        for(iPhilosopherIndex=0; iPhilosopherIndex < MAX_PHILOSOPHER_PER_TABLE * iGroupNumber; iPhilosopherIndex++)
        {
            iGroupIndex = (iPhilosopherIndex / MAX_PHILOSOPHER_PER_TABLE);
            iCurrentPhilosopher = (iPhilosopherIndex % MAX_PHILOSOPHER_PER_TABLE);

            pthread_attr_init (&tattr);
            pthread_attr_getschedparam (&tattr, &param);
            param.sched_priority = getRandomNumber(5) * 10; // get random priority between 10-50
            pthread_attr_setschedparam (&tattr, &param);

            pthread_create(&s_pGroupStatus[iGroupIndex].philosophers[iCurrentPhilosopher], &tattr, (void *) keepCalmAndEatRice, (void*)iPhilosopherIndex);
        }

        for(iPhilosopherIndex=0; iPhilosopherIndex < MAX_PHILOSOPHER_PER_TABLE * iGroupNumber; iPhilosopherIndex++)
        {
            iGroupIndex = (iPhilosopherIndex / MAX_PHILOSOPHER_PER_TABLE);
            iCurrentPhilosopher = (iPhilosopherIndex % MAX_PHILOSOPHER_PER_TABLE);

            pthread_join(s_pGroupStatus[iGroupIndex].philosophers[iCurrentPhilosopher], NULL);
        }

        for(iPhilosopherIndex=0; iPhilosopherIndex < MAX_PHILOSOPHER_PER_TABLE * iGroupNumber; iPhilosopherIndex++)
        {
            iGroupIndex = (iPhilosopherIndex / MAX_PHILOSOPHER_PER_TABLE);
            iCurrentPhilosopher = (iPhilosopherIndex % MAX_PHILOSOPHER_PER_TABLE);

            pthread_mutex_destroy(&s_pGroupStatus[iGroupIndex].groupForkMutex[iCurrentPhilosopher]);
        }

        for (iGroupIndex = 0; iGroupIndex < iGroupNumber; iGroupIndex++)
        {
            iTotalBill = TABLE_OPENING_PRICE;
            iTotalBill += TABLE_REFRESH_PRICE * s_pGroupStatus[iGroupIndex].table.iTableRefreshCount;
            iTotalBill += RICE_PRICE_PER_BOWL * s_pGroupStatus[iGroupIndex].table.iTableRefreshCount;

            printf("Group #%d sit on Table #%d and they paid %d TRY\n", iGroupIndex, s_pGroupStatus[iGroupIndex].table.iTableID, iTotalBill);
        }

        free(s_pGroupStatus);
    }

    return 0;
}