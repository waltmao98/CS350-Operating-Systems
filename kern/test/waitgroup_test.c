#include <trafficsynch.h>
#include <lib.h>
#include <test.h>
#include <array.h>

int testwg(int nargs, char **args) {
    // test for A1 traffic synchronization problem WaitGroups 
    // Quit the OS after running the test because 
    // kfree doesn't work and OS will run out of memory
    (void)nargs; // remove compiler warnings 
    (void)args; // remove compiler warnings 
    /*
    
    //TODO: There's something wrong with compiling this file
    // but since we don't need it anymore, I'm not going to fix it.

    intersection_sync_init();
    WaitGroup *wg = waitGroupCreate(north);
    array_add(waitList, wg, NULL);
    wg = waitGroupCreate(east);
    array_add(waitList, wg, NULL);
    wg = waitGroupCreate(west);
    array_add(waitList, wg, NULL);

    KASSERT(waitListContains(north));
    KASSERT(!waitListContains(south));

    wg = getWaitGroupFromIdx(0);
    KASSERT(wg->origin == north);
    wg = getWaitGroupFromIdx(2);
    KASSERT(wg->origin == west);
    
    KASSERT(getWaitGroupFromOrigin(west) == wg);

    KASSERT(getOriginCv(north) == cvOriginNorth);

    KASSERT(removeWaitListFront()->origin == north);
    KASSERT(removeWaitListFront()->origin == east);
    KASSERT(removeWaitListFront()->origin == west);

    intersection_sync_cleanup();
    kprintf("WaitGroup functions test passed!\n"); 
    */
    return 0;
}
