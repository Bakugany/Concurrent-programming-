#include "future_combinators.h"
#include <stdlib.h>

#include "future.h"
#include "waker.h"

static FutureState return_state(Future* future, Mio* mio, Waker waker) {
    return future->progress(future, mio, waker);
}

static FutureState then_future_progress(Future* base, Mio* mio, Waker waker)
{
    ThenFuture* self = (ThenFuture*)base;

    if (!self->fut1_completed) {
        FutureState state = return_state(self->fut1, mio, waker);

        if (state == FUTURE_COMPLETED) {
            self->fut1_completed = true;
            self->fut2->arg = self->fut1->ok;
        }

        if (state == FUTURE_FAILURE) {
            self->base.errcode = THEN_FUTURE_ERR_FUT1_FAILED;
            return FUTURE_FAILURE;
        }

        if (state == FUTURE_PENDING)
            return FUTURE_PENDING;
    }

    FutureState state = return_state(self->fut2, mio, waker);
    if (state == FUTURE_COMPLETED) {
        self->base.ok = self->fut2->ok;
        return FUTURE_COMPLETED;
    }

    if (state == FUTURE_FAILURE) {
        self->base.errcode = THEN_FUTURE_ERR_FUT2_FAILED;
        return FUTURE_FAILURE;
    }

    if (state == FUTURE_PENDING)
        return FUTURE_PENDING;
}

ThenFuture future_then(Future* fut1, Future* fut2)
{
    return (ThenFuture) {
        .base = future_create(then_future_progress),
        .fut1 = fut1,
        .fut2 = fut2,
        .fut1_completed = false,
    };
}

static FutureState join_future_progress(Future* base, Mio* mio, Waker waker)
{
    JoinFuture* self = (JoinFuture*)base;

    if (self->fut1_completed == FUTURE_PENDING) {
        self->fut1_completed = return_state(self->fut1, mio, waker);

        if (self->fut1_completed == FUTURE_COMPLETED)
            self->result.fut1.ok = self->fut1->ok;

        if (self->fut1_completed == FUTURE_FAILURE)
            self->result.fut1.errcode = JOIN_FUTURE_ERR_FUT1_FAILED;
    }

    if (self->fut2_completed == FUTURE_PENDING) {
        self->fut2_completed = return_state(self->fut2, mio, waker);

        if (self->fut2_completed == FUTURE_COMPLETED)
            self->result.fut2.ok = self->fut2->ok;

         if (self->fut2_completed == FUTURE_FAILURE)
            self->result.fut2.errcode = JOIN_FUTURE_ERR_FUT2_FAILED;

    }

    if (self->fut1_completed != FUTURE_PENDING && self->fut2_completed != FUTURE_PENDING) {
        if (self->fut1_completed == FUTURE_FAILURE || self->fut2_completed == FUTURE_FAILURE) {
            self->base.errcode = (self->fut1_completed == FUTURE_FAILURE && self->fut2_completed == FUTURE_FAILURE)
                ? JOIN_FUTURE_ERR_BOTH_FUTS_FAILED
                : (self->fut1_completed == FUTURE_FAILURE ? JOIN_FUTURE_ERR_FUT1_FAILED : JOIN_FUTURE_ERR_FUT2_FAILED);
            return FUTURE_FAILURE;
        }
        self->base.ok = &self->result;
        return FUTURE_COMPLETED;
    }

    return FUTURE_PENDING;
}

JoinFuture future_join(Future* fut1, Future* fut2)
{
    return (JoinFuture) {
        .base = future_create(join_future_progress),
        .fut1 = fut1,
        .fut2 = fut2,
        .fut1_completed = FUTURE_PENDING,
        .fut2_completed = FUTURE_PENDING,
    };
}

static FutureState select_future_progress(Future* base, Mio* mio, Waker waker)
{
    SelectFuture* self = (SelectFuture*)base;

    if (self->which_completed == SELECT_COMPLETED_NONE) {
        FutureState fut1_state = return_state(self->fut1, mio, waker);
        FutureState fut2_state = return_state(self->fut2, mio, waker);

        if (fut1_state == FUTURE_COMPLETED) {
            self->which_completed = SELECT_COMPLETED_FUT1;
            self->base.ok = self->fut1->ok;
            return FUTURE_COMPLETED;
        }

        if (fut2_state == FUTURE_COMPLETED) {
            self->which_completed = SELECT_COMPLETED_FUT2;
            self->base.ok = self->fut2->ok;
            return FUTURE_COMPLETED;
        }

        if (fut1_state == FUTURE_FAILURE && fut2_state == FUTURE_FAILURE) {
            self->which_completed = SELECT_FAILED_BOTH;
            self->base.errcode = JOIN_FUTURE_ERR_BOTH_FUTS_FAILED;
            return FUTURE_FAILURE;
        }

        if (fut1_state == FUTURE_FAILURE)
            self->which_completed = SELECT_FAILED_FUT1;

        if (fut2_state == FUTURE_FAILURE)
            self->which_completed = SELECT_FAILED_FUT2;
    }

    return FUTURE_PENDING;
}

SelectFuture future_select(Future* fut1, Future* fut2)
{
    return (SelectFuture) {
        .base = future_create(select_future_progress),
        .fut1 = fut1,
        .fut2 = fut2,
        .which_completed = SELECT_COMPLETED_NONE,
    };
}
