
#ifndef _TRAFFICSYNCH_H_
#define _TRAFFICSYNCH_H_

#include <types.h>
#include <synchprobs.h>

typedef struct
{
  Direction origin;
  struct cv *vehiclesDispatchCv;
  volatile int numVehicles;
} WaitGroup;

extern const unsigned int NUM_DIRECTIONS;

void intersection_sync_init(void);
void intersection_sync_cleanup(void);

struct cv *add_to_waitgroup(Direction);
void dispatch_next_waitgroup(void);
void assertNotNull(void);
void assertWaitListProperties(void);
bool waitListContains(Direction);
WaitGroup *waitGroupCreate(Direction);
void waitGroupDestroy(WaitGroup*);
WaitGroup *getWaitGroupFromIdx(size_t);
WaitGroup *getWaitGroupFromOrigin(Direction);
struct cv *getOriginCv(Direction);
WaitGroup *removeWaitListFront(void);
bool originInIntersection(Direction);

struct array *volatile waitList;  // array of WaitGroup, acts as a queue
struct lock *operationsLock;

struct cv *cvOriginNorth;
struct cv *cvOriginSouth;
struct cv *cvOriginEast;
struct cv *cvOriginWest;

#endif /* _TRAFFICSYNCH_H_ */
