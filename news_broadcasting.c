#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>

#define MAX_MESSAGE_LEN 100
int producersNum = 0;

// Unbounded Buffer
typedef struct
{
    char **buffer;
    int size;
    int count;
    int out;
    int in;
    pthread_mutex_t mutex;
    sem_t full;
} UnboundedBuffer;

UnboundedBuffer *createUnboundedBuffer()
{
    UnboundedBuffer *buffer = (UnboundedBuffer *)malloc(sizeof(UnboundedBuffer));
    buffer->buffer = (char **)malloc(sizeof(char *));
    buffer->size = 1;
    buffer->count = 0;
    buffer->out = 0;
    buffer->in = -1;
    pthread_mutex_init(&buffer->mutex, NULL);
    sem_init(&buffer->full, 0, 0);
    return buffer;
}

void insertUnbounded(UnboundedBuffer *buffer, char *message)
{
    pthread_mutex_lock(&buffer->mutex); // the dispathcer can always add new items
    buffer->in++;
    buffer->buffer = (char **)realloc(buffer->buffer, sizeof(char *) * (buffer->in + 1));
    buffer->buffer[buffer->in] = strdup(message);
    buffer->count++;
    pthread_mutex_unlock(&buffer->mutex);
    sem_post(&buffer->full);
}

char *removeUnbounded(UnboundedBuffer *buffer)
{
    sem_wait(&buffer->full); // the co-editor has to check there's item to read
    pthread_mutex_lock(&buffer->mutex);
    char *message = buffer->buffer[buffer->out];
    buffer->out++;
    buffer->count--;
    pthread_mutex_unlock(&buffer->mutex);
    return message;
}

// Bounded Buffer
typedef struct
{
    char **buffer;
    int size;
    int count;
    int out;
    int in;
    pthread_mutex_t mutex;
    sem_t empty;
    sem_t full;
} BoundedBuffer;

BoundedBuffer *createBoundedBuffer(int size)
{
    BoundedBuffer *buffer = (BoundedBuffer *)malloc(sizeof(BoundedBuffer));
    buffer->buffer = (char **)malloc(sizeof(char *) * size);
    buffer->size = size;
    buffer->count = 0;
    buffer->out = 0;
    buffer->in = -1;
    pthread_mutex_init(&buffer->mutex, NULL);
    sem_init(&buffer->empty, 0, size);
    sem_init(&buffer->full, 0, 0);
    return buffer;
}

void insertBounded(BoundedBuffer *buffer, char *message)
{
    sem_wait(&buffer->empty);
    pthread_mutex_lock(&buffer->mutex);
    buffer->in = (buffer->in + 1) % buffer->size;
    buffer->buffer[buffer->in] = strdup(message);
    buffer->count++;
    pthread_mutex_unlock(&buffer->mutex);
    sem_post(&buffer->full);
}

char *removeBounded(BoundedBuffer *buffer)
{
    sem_wait(&buffer->full);
    pthread_mutex_lock(&buffer->mutex);
    char *message = buffer->buffer[buffer->out];
    buffer->out = (buffer->out + 1) % buffer->size;
    buffer->count--;
    pthread_mutex_unlock(&buffer->mutex);
    sem_post(&buffer->empty);
    return message;
}

// Producer
typedef struct
{
    int id;
    int numProducts;
    int queueSize;
    BoundedBuffer *producerQueue;
} ProducerArgs;

void *runProducerThread(void *arg)
{
    ProducerArgs *args = (ProducerArgs *)arg;

    for (int i = 0; i < args->numProducts; i++)
    {
        char message[MAX_MESSAGE_LEN];
        snprintf(message, MAX_MESSAGE_LEN, "Producer %d %s %d", args->id, (i % 3 == 0) ? "SPORTS" : ((i % 3 == 1) ? "NEWS" : "WEATHER"), (i / 3));
        insertBounded(args->producerQueue, message);
    }

    insertBounded(args->producerQueue, "DONE");
    pthread_exit(NULL);
}

// Dispatcher
typedef struct
{
    BoundedBuffer **producerQueues;
    UnboundedBuffer *dispatcherQueues[3];

} DispatcherArgs;

void *runDispatcherThread(void *arg)
{
    DispatcherArgs *args = (DispatcherArgs *)arg;

    int currentProducer = 0;
    int doneCount = 0;

    while (doneCount < producersNum)
    {
        if (args->producerQueues[currentProducer]->count > 0)
        { // if the current producer has items to read - read one and continue to the next producer
            char *message = removeBounded(args->producerQueues[currentProducer]);
            if (strcmp(message, "DONE") == 0)
            {
                doneCount++;
            }
            else
            {
                if (strstr(message, "SPORTS") != NULL)
                    insertUnbounded(args->dispatcherQueues[0], message);
                else if (strstr(message, "NEWS") != NULL)
                    insertUnbounded(args->dispatcherQueues[1], message);
                else if (strstr(message, "WEATHER") != NULL)
                    insertUnbounded(args->dispatcherQueues[2], message);
            }
        }

        currentProducer = (currentProducer + 1) % producersNum;
    }

    for (int i = 0; i < 3; i++)
    {
        insertUnbounded(args->dispatcherQueues[i], "DONE");
    }

    pthread_exit(NULL);
}

// Co-Editor
typedef struct
{
    UnboundedBuffer *dispatcherQueue;
    BoundedBuffer *coEditorQueue;
} CoEditorArgs;

void *runCoEditorThread(void *arg)
{
    CoEditorArgs *args = (CoEditorArgs *)arg;

    while (1)
    {
        char *message = removeUnbounded(args->dispatcherQueue);
        if (strcmp(message, "DONE") == 0)
        {
            insertBounded(args->coEditorQueue, message);
            break;
        }

        sleep(0.1); // Simulate editing process

        insertBounded(args->coEditorQueue, message);
    }

    pthread_exit(NULL);
}

// Screen-Manager
typedef struct
{
    BoundedBuffer *coEditorQueue;
} ScreenManagerArgs;

void *runScreenManagerThread(void *arg)
{
    ScreenManagerArgs *args = (ScreenManagerArgs *)arg;
    int doneCount = 0;

    while (doneCount < 3)
    {
        char *message = removeBounded(args->coEditorQueue);
        if (strcmp(message, "DONE") == 0)
        {
            doneCount++;
        }
        else
        {
            printf("%s\n", message);
        }
    }

    printf("DONE\n");
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <config_file>\n", argv[0]);
        return 1;
    }

    // Open and read configuration file
    FILE *configFile = fopen(argv[1], "r");
    if (configFile == NULL)
    {
        printf("Failed to open config file: %s\n", argv[1]);
        return 1;
    }

    // Count the number of repetitive blocks (producers) in the file
    char buffer[128];
    while (fgets(buffer, MAX_MESSAGE_LEN, configFile) != NULL)
    {
        if (strcmp(buffer, "\n") == 0)
        {
            producersNum++;
        }
    }

    // Reset the file pointer to the beginning of the file
    fseek(configFile, 0, SEEK_SET);

    ProducerArgs producersArg[producersNum]; // to contain the file input:id,#products,#queue
    int coEditorQueueSize;                   // to contain the file input for coEditors queue size
    BoundedBuffer *producerQueues[producersNum];
    UnboundedBuffer *dispatcherQueues[3];
    BoundedBuffer *coEditorQueue;
    pthread_t producerThreads[producersNum];
    pthread_t dispatcherThread;
    pthread_t coEditorThread[3];
    pthread_t screenManagerThread;

    // Read producersArg information
    for (int i = 0; i < producersNum; i++)
    {
        if (fscanf(configFile, "%d\n%d\n%d\n", &producersArg[i].id, &producersArg[i].numProducts, &producersArg[i].queueSize) != 3)
        {
            printf("Error reading producer information from config file.\n");
            fclose(configFile);
            return 1;
        }
        fscanf(configFile, "\n");
    }

    // Read co-editor queue size
    if (fscanf(configFile, "%d", &coEditorQueueSize) != 1)
    {
        printf("Error reading co-editor queue size from config file.\n");
        fclose(configFile);
        return 1;
    }

    fclose(configFile);

    //decreased the id with 1
    for (int i = 0; i < producersNum; i++)
    {
        producersArg[i].id-- ;
    }

    // Initialize producer queues

    for (int i = 0; i < producersNum; i++)
    {
        producerQueues[i] = createBoundedBuffer(producersArg[i].queueSize);
        producersArg[i].producerQueue = producerQueues[i];
    }

    // Initialize dispatcher queues as UnboundedBuffer
    for (int i = 0; i < 3; i++)
    {
        dispatcherQueues[i] = createUnboundedBuffer();
    }

    // Initialize co-editor queue
    coEditorQueue = createBoundedBuffer(coEditorQueueSize);

    // Create producer threads
    for (int i = 0; i < producersNum; i++)
    {
        pthread_create(&producerThreads[i], NULL, runProducerThread, &producersArg[i]);
    }

    // Create dispatcher thread
    DispatcherArgs dispatcherArgs;

    dispatcherArgs.producerQueues = producerQueues;

    for (int i = 0; i < 3; i++)
    {
        dispatcherArgs.dispatcherQueues[i] = dispatcherQueues[i];
    }
    pthread_create(&dispatcherThread, NULL, runDispatcherThread, &dispatcherArgs);

    // Create co-editor thread
    CoEditorArgs coEditorArgs[3];
    for (int i = 0; i < 3; i++)
    {
        coEditorArgs[i].dispatcherQueue = dispatcherQueues[i];
        coEditorArgs[i].coEditorQueue = coEditorQueue;
        pthread_create(&coEditorThread[i], NULL, runCoEditorThread, &coEditorArgs[i]);
    }

    // Create screen manager thread
    ScreenManagerArgs screenManagerArgs;
    screenManagerArgs.coEditorQueue = coEditorQueue;
    pthread_create(&screenManagerThread, NULL, runScreenManagerThread, &screenManagerArgs);

    // Wait for producer threads to finish
    for (int i = 0; i < producersNum; i++)
    {
        pthread_join(producerThreads[i], NULL);
    }

    // Wait for dispatcher thread to finish
    pthread_join(dispatcherThread, NULL);

    // Wait for co-editor thread to finish
    for (int i = 0; i < 3; i++)
    {
        pthread_join(coEditorThread[i], NULL);
    }

    // Wait for screen manager thread to finish
    pthread_join(screenManagerThread, NULL);

    // Cleanup
    for (int i = 0; i < producersNum; i++)
    {
        free(producerQueues[i]->buffer);
    }
    for (int i = 0; i < 3; i++)
    {
        free(dispatcherQueues[i]->buffer);
    }
    free(coEditorQueue->buffer);

    return 0;
}
