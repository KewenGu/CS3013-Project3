#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include <pthread.h>
#include <semaphore.h>

#define MAXRATS 5
#define MAXROOMS 8
#define BUFSIZE 6
#define FILENAME "rooms.txt"

typedef struct room {
  int iRoom;    /* room identifier */
  int capacity; /* number of rats can be kept in a room */
  int delay;    /* traversal time in seconds */
  int nRats;    /* number of rats already in the room */
} Room;

typedef struct rat {
  int iRat;     /* rat identifier */
  int nRooms;   /* number of rooms traversed */
} Rat;

typedef struct vbentry {
  int iRat;     /* rat identifier */
  int tEntry;   /* time of entry into room */
  int tDep;     /* time of departure from room */
} VBentry;

/* Function prototypes */
void *EnterRoom(void *ptr);
void *TryEnterRoom(void *ptr);
int RoomConfig(Room rooms[]);
void RatInit(Rat rats[], int numRats);
void DieWithError(char *errorMsg);

/* Global variables */
Room rooms[MAXROOMS];
Rat rats[MAXRATS];
VBentry roomVB[MAXROOMS][MAXRATS];  /* array of room visitors books */
sem_t sems[MAXROOMS];
char *algorithm;
int numRooms, numRats, tStart, tTotal = 0;

int main(int argc, char *argv[]) {
  /* Local variables */
  int i, j, tIdeal = 0;
  pthread_t threads[MAXRATS];
  /* Check for correct number of inputs */
  if (argc != 3)
  {
    fprintf(stderr, "Usage: %s <number of rates> <travesing algorithm>\n", argv[0]);
    exit(1);
  }
  /* The second command line argument indicates the number of rats */
  numRats = atoi(argv[1]);
  /* The third command line argument indicates the traversing algorithm */
  algorithm = argv[2];

  /* Check if the input rat number exceeds the maximum limit */
  if (numRats > MAXRATS)
  {
    fprintf(stderr, "Maximum rats allowed: %d\n", MAXRATS);
    exit(1);
  }

  /* Initialize the rat structures */
  RatInit(rats, numRats);
  /* Read from the room configuration file, initial the room structures and obtain the total number of rooms */
  numRooms = RoomConfig(rooms);
  /* Initialize a semaphore for every room */
  for (i = 0; i < numRooms; i++)
    if (sem_init(&sems[i], 0, rooms[i].capacity) != 0)
      DieWithError("sem_init() failed");
  /* Create a thread for every rat, use different traversing methods according to the input algorithm */
  for (i = 0; i < numRats; i++)
    if (!strcmp(algorithm, "i") || !strcmp(algorithm, "d"))
    {
      if (pthread_create(&threads[i], NULL, &EnterRoom, &rats[i]) != 0)
        DieWithError("pthread_create() failed");
    }
    else if (!strcmp(algorithm, "n"))
    {
      if (pthread_create(&threads[i], NULL, &TryEnterRoom, &rats[i]) != 0)
        DieWithError("pthread_create() failed");
    }
    else
    {
      fprintf(stderr, "%s is not a valid algorithm\n", algorithm);
      exit(1);
    }
  /* Wait until all threads are done */
  for (i = 0; i < numRats; i++)
    if (pthread_join(threads[i], NULL) != 0)
      DieWithError("pthread_join() failed");
  /* Destroy the semaphores */
  for (i = 0; i < numRooms; i++)
    if (sem_destroy(&sems[i]))
      DieWithError("sem_destroy() failed");
  /* Print the results */
  for (i = 0; i < numRooms; i++)
  {
    printf("Room %d [%d %d]:", i, rooms[i].capacity, rooms[i].delay);
    for (j = 0; j < numRats; j++)
    {
      tIdeal += rooms[i].delay;
      printf(" %d %d %d;", roomVB[i][j].iRat, roomVB[i][j].tEntry, roomVB[i][j].tDep);
    }
    printf("\n");
  }
  printf("Total traversal time: %d seconds, compared to ideal time: %d seconds.\n\n", tTotal, tIdeal);

  return 0;
}

/* The in-order method and the distributed method of traversing the rooms, employs semaphores and blocks the thread (rat) when the room is already full */
void *EnterRoom(void *ptr)
{
  int index, nextRoom;
  struct timeval tv;
  index = ((Rat *)ptr)->iRat;

  if (!strcmp(algorithm, "i"))
    nextRoom = 0;
  else if (!strcmp(algorithm, "d"))
    nextRoom = index % numRooms;

  gettimeofday(&tv, NULL);
  tStart = tv.tv_sec;
  while (rats[index].nRooms < numRooms)
  {
      roomVB[nextRoom][index].iRat = index;
      /* Attempting to enter the room */
//    printf("Rat %d attempts to enter room %d\n", index, nextRoom);
      sem_wait(&sems[nextRoom]);
//    printf("Rat %d enters room %d\n", index, nextRoom);
      rooms[nextRoom].nRats += 1;
      gettimeofday(&tv, NULL);
      roomVB[nextRoom][index].tEntry = tv.tv_sec - tStart;
      sleep(rooms[nextRoom].delay);
      gettimeofday(&tv, NULL);
      roomVB[nextRoom][index].tDep = tv.tv_sec - tStart;
      /* Leaving the room */
//    printf("Rat %d leaves room %d\n", index, nextRoom);
      sem_post(&sems[nextRoom]);
      rooms[nextRoom].nRats -= 1;
      rats[index].nRooms += 1;

      if (nextRoom >= (numRooms - 1))
        nextRoom = 0;
      else
        nextRoom++;
  }
  gettimeofday(&tv, NULL);
  tTotal += tv.tv_sec - tStart;
  printf("Rat %d completed maze in %d seconds.\n", index, tv.tv_sec - tStart);
}

/* The non-blocking method of traversing the rooms, does not block the thread (rat) when the room is already full */
void *TryEnterRoom(void *ptr)
{
  int index, nextRoom;
  struct timeval tv;
  index = ((Rat *)ptr)->iRat;
  /* Choose to use the same traversing order as the distributed method */
  nextRoom = index % numRooms;

  gettimeofday(&tv, NULL);
  tStart = tv.tv_sec;
  while (rats[index].nRooms < numRooms)
  {
      roomVB[nextRoom][index].iRat = index;
      /* Attempting to enter the room */
//    printf("Rat %d attempts to enter room %d\n", index, nextRoom);
      while (rooms[nextRoom].nRats >= rooms[nextRoom].capacity) {};
//    printf("Rat %d enters room %d\n", index, nextRoom);
      rooms[nextRoom].nRats += 1;
      gettimeofday(&tv, NULL);
      roomVB[nextRoom][index].tEntry = tv.tv_sec - tStart;
      sleep(rooms[nextRoom].delay);
      gettimeofday(&tv, NULL);
      roomVB[nextRoom][index].tDep = tv.tv_sec - tStart;
      /* Leaving the room */
//    printf("Rat %d leaves room %d\n", index, nextRoom);
      rooms[nextRoom].nRats -= 1;
      rats[index].nRooms += 1;

      if (nextRoom >= (numRooms - 1))
        nextRoom = 0;
      else
        nextRoom++;
  }
  gettimeofday(&tv, NULL);
  tTotal += tv.tv_sec - tStart;
  printf("Rat %d completed maze in %d seconds.\n", index, tv.tv_sec - tStart);
}

/* Configure the room structs from the room configuration file, given an array of struct room, return the number of rooms configured */
int RoomConfig(Room rooms[])
{
  FILE *roomFile;
  char *ret, *token, readBuf[BUFSIZE];
  int i = 0;

  if ((roomFile = fopen(FILENAME, "r")) == NULL)
    DieWithError("fopen() failed");
  /* Read up to MAXROOMS of rooms */
  while ((ret = fgets(readBuf, BUFSIZE, roomFile)) != NULL && i < MAXROOMS) 
  {
    rooms[i].iRoom = i;
    token = strtok(readBuf, " ");
    rooms[i].capacity = atoi(token);
    token = strtok(NULL, " ");
    rooms[i].delay = atoi(token);
    rooms[i].nRats = 0;
    i++;
  }

  return i;
}

/* Initial an array of struct rat, given the array and the number of rat struct need to be initialized */
void RatInit(Rat rats[], int numRats)
{
  int i;
  for (i = 0; i < numRats; i++)
  {
    rats[i].iRat = i;
    rats[i].nRooms = 0;
  }
}

/* Print the error message and exit from the program when there'r error generated */
void DieWithError(char *errorMsg)
{
  perror(errorMsg);
  exit(1);
}
