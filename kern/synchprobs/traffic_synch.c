#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>
#include <opt-A1.h>
#include <array.h>
#include <trafficsynch.h>

/* 
 * This simple default synchronization mechanism allows only vehicle at a time
 * into the intersection.   The intersectionSem is used as a a lock.
 * We use a semaphore rather than a lock so that this code will work even
 * before locks are implemented.
 */

/* 
 * Replace this default synchronization mechanism with your own (better) mechanism
 * needed for your solution.   Your mechanism may use any of the available synchronzation
 * primitives, e.g., semaphores, locks, condition variables.   You are also free to 
 * declare other global variables if your solution requires them.
 */

/*
 * replace this with declarations of any synchronization and other variables you need here
 */

const unsigned int NUM_DIRECTIONS = 4;

void assertWaitListProperties()
{
  KASSERT(waitList != NULL);
  KASSERT(array_num(waitList) <= NUM_DIRECTIONS);

  // assert uniqueness of elements
  for (size_t i = 0; i < array_num(waitList) - 1; ++i)
  {
    for (size_t j = i + 1; j < array_num(waitList); ++j)
    {
      KASSERT(getWaitGroupFromIdx(i)->origin != getWaitGroupFromIdx(j)->origin);
    }
  }
}

void assertNotNull()
{
  KASSERT(waitList);
  KASSERT(operationsLock);
  KASSERT(cvOriginNorth);
  KASSERT(cvOriginSouth);
  KASSERT(cvOriginEast);
  KASSERT(cvOriginWest);
}

struct cv *getOriginCv(Direction origin)
{
  switch (origin)
  {
  case north:
    return cvOriginNorth;
  case south:
    return cvOriginSouth;
  case east:
    return cvOriginEast;
  case west:
    return cvOriginWest;
  }
  panic("invalid origin passed to getOriginCv");
  return NULL;
}

WaitGroup *waitGroupCreate(Direction origin)
{
  WaitGroup *wg = kmalloc(sizeof(WaitGroup));
  wg->origin = origin;
  wg->vehiclesDispatchCv = getOriginCv(origin);
  wg->numVehicles = 0;
  return wg;
}

WaitGroup *removeWaitListFront()
{
  WaitGroup *front = getWaitGroupFromIdx(0);
  array_remove(waitList, 0);
  return front;
}

void waitGroupDestroy(WaitGroup *wg)
{
  kfree(wg);
}

WaitGroup *getWaitGroupFromIdx(size_t i)
{
  return (WaitGroup *)array_get(waitList, i);
}

WaitGroup *getWaitGroupFromOrigin(Direction origin)
{
  for (size_t i = 0; i < array_num(waitList); ++i)
  {
    WaitGroup *wg = getWaitGroupFromIdx(i);
    if (wg->origin == origin) {
      return wg;
    }
  }
  return NULL;
}

bool waitListContains(Direction origin)
{
  WaitGroup *wg = getWaitGroupFromOrigin(origin);
  return wg != NULL;
}

/* 
 * The simulation driver will call this function once before starting
 * the simulation
 *
 * You can use it to initialize synchronization and other variables.
 * 
 */
void intersection_sync_init(void)
{
  waitList = array_create();
  if (waitList == NULL)
  {
    panic("could not create waitList array");
  }

  operationsLock = kmalloc(sizeof(struct lock));
  if (operationsLock == NULL) {
    panic("could not create operationsLock lock");
  }
  operationsLock = lock_create("operations_lock");

  cvOriginNorth = cv_create("cv_for_origin_north_vehicles");
  cvOriginSouth = cv_create("cv_for_origin_south_vehicles");
  cvOriginEast = cv_create("cv_for_origin_east_vehicles");
  cvOriginWest = cv_create("cv_for_origin_west_vehicles");
  if (cvOriginNorth == NULL)
    panic("could not create north cv");
  if (cvOriginSouth == NULL)
    panic("could not create south cv");
  if (cvOriginEast == NULL)
    panic("could not create east cv");
  if (cvOriginWest == NULL)
    panic("could not create west cv");
}

/* 
 * The simulation driver will call this function once after
 * the simulation has finished
 *
 * You can use it to clean up any synchronization and other variables.
 *
 */
void intersection_sync_cleanup(void)
{
  assertNotNull();
  KASSERT(array_num(waitList) == 0);
  array_destroy(waitList);
  lock_destroy(operationsLock);
  cv_destroy(cvOriginNorth);
  cv_destroy(cvOriginSouth);
  cv_destroy(cvOriginEast);
  cv_destroy(cvOriginWest);
}

/*
 * The simulation driver will call this function each time a vehicle
 * tries to enter the intersection, before it enters.
 * This function should cause the calling simulation thread 
 * to block until it is OK for the vehicle to enter the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle is arriving
 *    * destination: the Direction in which the vehicle is trying to go
 *
 * return value: none
 */

struct cv *add_to_waitgroup(Direction origin)
{
  WaitGroup *wg;
  if (!waitListContains(origin))
  {
    wg = waitGroupCreate(origin);
    array_add(waitList, wg, NULL);
    assertWaitListProperties();
  }
  else
  {
    wg = getWaitGroupFromOrigin(origin);
  }
  wg->numVehicles += 1;
  return wg->vehiclesDispatchCv;
}

bool originInIntersection(Direction origin) {
  return array_num(waitList) != 0 && getWaitGroupFromIdx(0)->origin == origin;
}

void intersection_before_entry(Direction origin, Direction destination)
{
  (void)destination; /* avoid compiler complaint about unused parameter */
  assertNotNull();

  lock_acquire(operationsLock);
  struct cv *originCv = add_to_waitgroup(origin);
  // If there's another WaitGroup in the intersection
  // that isn't from this origin
  if (array_num(waitList) > 1 && !originInIntersection(origin)) {
    cv_wait(originCv, operationsLock);
  }
  lock_release(operationsLock);
}

void dispatch_next_waitgroup()
{
  WaitGroup *wg = removeWaitListFront();
  waitGroupDestroy(wg);
  if (array_num(waitList) == 0) {
    return;
  }
  WaitGroup *nextUp = getWaitGroupFromIdx(0);
  KASSERT(nextUp != NULL);
  cv_broadcast(nextUp->vehiclesDispatchCv, operationsLock);
}

/*
 * The simulation driver will call this function each time a vehicle
 * leaves the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle arrived
 *    * destination: the Direction in which the vehicle is going
 *
 * return value: none
 */

void intersection_after_exit(Direction origin, Direction destination)
{
  (void)destination; /* avoid compiler complaint about unused parameter */
  assertNotNull();

  lock_acquire(operationsLock);
  WaitGroup *justExited = getWaitGroupFromOrigin(origin);
  KASSERT(justExited);
  KASSERT(justExited == getWaitGroupFromIdx(0));
  justExited->numVehicles -= 1;
  KASSERT(justExited->numVehicles >= 0);
  if (justExited->numVehicles == 0) {
    dispatch_next_waitgroup();
  }
  lock_release(operationsLock);
}
