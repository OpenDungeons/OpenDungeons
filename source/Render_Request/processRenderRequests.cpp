/*! \brief Loop through the render requests in the queue and process them
*/
void RenderManager::processRenderRequests
()
{
    /* If the renderQueue now contains 0 objects we should process this object and then
    * release any of the other threads which were waiting on a renderQueue flush.
    * FIXME: Noting is actually being done based on this, this should be used to implement
    * a function making it easy to allow functions to wait on this.
    */
    sem_wait ( &renderQueueSemaphore );
    while (!renderQueue.empty())
    {
        // Remove the first item from the render queue


        RenderRequest *curReq = renderQueue.front();
        renderQueue.pop_front();
        sem_post ( &renderQueueSemaphore );

        // Handle the request
        handleRenderRequest ( *curReq );

        /* Decrement the number of outstanding references to things from the turn number the event was queued on.
        * (Locked in queueRenderRequest)
        */
        gameMap->threadUnlockForTurn ( curReq->turnNumber );

        delete curReq;
        curReq = NULL;

        /* If we have finished processing the last renderRequest that was in the queue we
        * can release all of the threads that were waiting for the queue to be flushed.
        */
        // FIXME - should this be here, or outside the while loop?
        for (unsigned int i = 0, numThreadsWaiting = numThreadsWaitingOnRenderQueueEmpty.get();
                i < numThreadsWaiting; ++i)
        {
            sem_post(&renderQueueEmptySemaphore);
        }

        sem_wait ( &renderQueueSemaphore );
    }
    sem_post ( &renderQueueSemaphore );
