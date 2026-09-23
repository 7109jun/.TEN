#include "ten/thread_pool.h"
#include "ten/sync.h"
#include "ten/memory.h"
#include "internal/error_internal.h"

typedef struct ten_task {
    ten_thread_fn function;
    void *userdata;
    struct ten_task *next;
} ten_task_t;

struct ten_thread_pool {
    ten_thread_t **workers;
    size_t worker_count;
    ten_mutex_t *lock;
    ten_cond_t *cond;
    ten_task_t *queue_head;
    ten_task_t *queue_tail;
    int shutting_down;
};

static void *ten__thread_pool_worker(void *arg)
{
    ten_thread_pool_t *pool = (ten_thread_pool_t *)arg;

    for (;;)
    {
        ten_task_t *task;

        ten_mutex_lock(pool->lock);
        while (pool->queue_head == NULL && !pool->shutting_down)
        {
            ten_cond_wait(pool->cond, pool->lock);
        }

        if (pool->queue_head == NULL && pool->shutting_down)
        {
            ten_mutex_unlock(pool->lock);
            break;
        }

        task = pool->queue_head;
        pool->queue_head = task->next;
        if (pool->queue_head == NULL)
        {
            pool->queue_tail = NULL;
        }
        ten_mutex_unlock(pool->lock);

        task->function(task->userdata);
        ten_free(task);
    }

    return NULL;
}

/* 부분적으로 생성되다 실패한 pool을 정리한다. created_workers는 이미
   성공적으로 만들어진 워커 스레드 수(join까지 마친다). */
static void ten__thread_pool_cleanup_partial(ten_thread_pool_t *pool, size_t created_workers)
{
    size_t i;

    if (pool == NULL)
    {
        return;
    }

    if (pool->lock != NULL && pool->cond != NULL && created_workers > 0)
    {
        ten_mutex_lock(pool->lock);
        pool->shutting_down = 1;
        ten_mutex_unlock(pool->lock);
        ten_cond_broadcast(pool->cond);

        for (i = 0; i < created_workers; i++)
        {
            ten_thread_join(pool->workers[i], NULL);
        }
    }

    ten_free(pool->workers);
    ten_mutex_destroy(pool->lock);
    ten_cond_destroy(pool->cond);
    ten_free(pool);
}

ten_thread_pool_t *ten_thread_pool_create(size_t worker_count)
{
    ten_thread_pool_t *pool;
    size_t i;

    if (worker_count == 0)
    {
        ten__set_error(TEN_ERROR_INVALID_ARGUMENT);
        return NULL;
    }

    pool = (ten_thread_pool_t *)ten_malloc(sizeof(ten_thread_pool_t));
    if (pool == NULL)
    {
        return NULL;
    }

    pool->workers = NULL;
    pool->worker_count = worker_count;
    pool->queue_head = NULL;
    pool->queue_tail = NULL;
    pool->shutting_down = 0;

    pool->lock = ten_mutex_create();
    pool->cond = ten_cond_create();
    if (pool->lock == NULL || pool->cond == NULL)
    {
        ten__thread_pool_cleanup_partial(pool, 0);
        return NULL;
    }

    pool->workers = (ten_thread_t **)ten_malloc(sizeof(ten_thread_t *) * worker_count);
    if (pool->workers == NULL)
    {
        ten__thread_pool_cleanup_partial(pool, 0);
        return NULL;
    }

    for (i = 0; i < worker_count; i++)
    {
        pool->workers[i] = ten_thread_create(ten__thread_pool_worker, pool);
        if (pool->workers[i] == NULL)
        {
            ten_error_code_t err = ten_last_error();
            ten__thread_pool_cleanup_partial(pool, i);
            ten__set_error(err);
            return NULL;
        }
    }

    ten__set_error(TEN_OK);
    return pool;
}

int ten_thread_pool_submit(ten_thread_pool_t *pool, ten_thread_fn function, void *userdata)
{
    ten_task_t *task;

    if (pool == NULL || function == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return (int)TEN_ERROR_NULL_ARGUMENT;
    }

    task = (ten_task_t *)ten_malloc(sizeof(ten_task_t));
    if (task == NULL)
    {
        return (int)TEN_ERROR_OUT_OF_MEMORY;
    }

    task->function = function;
    task->userdata = userdata;
    task->next = NULL;

    ten_mutex_lock(pool->lock);

    if (pool->shutting_down)
    {
        ten_mutex_unlock(pool->lock);
        ten_free(task);
        ten__set_error(TEN_ERROR_INVALID_STATE);
        return (int)TEN_ERROR_INVALID_STATE;
    }

    if (pool->queue_tail == NULL)
    {
        pool->queue_head = task;
        pool->queue_tail = task;
    }
    else
    {
        pool->queue_tail->next = task;
        pool->queue_tail = task;
    }

    ten_mutex_unlock(pool->lock);
    ten_cond_signal(pool->cond);

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

void ten_thread_pool_destroy(ten_thread_pool_t *pool)
{
    size_t i;
    ten_task_t *task;

    if (pool == NULL)
    {
        return;
    }

    ten_mutex_lock(pool->lock);
    pool->shutting_down = 1;
    ten_mutex_unlock(pool->lock);
    ten_cond_broadcast(pool->cond);

    for (i = 0; i < pool->worker_count; i++)
    {
        ten_thread_join(pool->workers[i], NULL);
    }

    task = pool->queue_head;
    while (task != NULL)
    {
        ten_task_t *next = task->next;
        ten_free(task);
        task = next;
    }

    ten_free(pool->workers);
    ten_mutex_destroy(pool->lock);
    ten_cond_destroy(pool->cond);
    ten_free(pool);
}
