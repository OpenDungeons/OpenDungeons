
//NOTE: This function has not yet been tested.
void RenderManager::waitOnRenderQueueFlush()
{
    numThreadsWaitingOnRenderQueueEmpty.lock();
    unsigned int tempUnsigned = numThreadsWaitingOnRenderQueueEmpty.rawGet();
    ++tempUnsigned;
    numThreadsWaitingOnRenderQueueEmpty.rawSet(tempUnsigned);
    numThreadsWaitingOnRenderQueueEmpty.unlock();

    sem_wait(&renderQueueEmptySemaphore);
}
