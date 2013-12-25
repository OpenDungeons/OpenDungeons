/*! \brief Put a render request in the queue (implementation)
*/
void RenderManager::queueRenderRequest_priv(RenderRequest* renderRequest)
{
    renderRequest->turnNumber = GameMap::turnNumber.get();
    //Unlocked in processRenderRequests
    gameMap->threadLockForTurn(renderRequest->turnNumber);

    sem_wait(&renderQueueSemaphore);
    renderQueue.push_back(renderRequest);
    sem_post(&renderQueueSemaphore);
}