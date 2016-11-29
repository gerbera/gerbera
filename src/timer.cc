/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    timer.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
    
    $Id$
*/

/// \file timer.cc

#include "timer.h"

using namespace zmm;

SINGLETON_MUTEX(Timer, true);

template <>
Ref<Array<Timer::TimerSubscriberElement<TimerSubscriberSingleton<Object> > > > Timer::getAppropriateSubscribers<TimerSubscriberSingleton<Object> >()
{
    if (subscribersSingleton == nil)
        throw _Exception(_("timer already inactive!"));
    return subscribersSingleton;
}

template <>
Ref<Array<Timer::TimerSubscriberElement<TimerSubscriberObject> > > Timer::getAppropriateSubscribers<TimerSubscriberObject>()
{
    if (subscribersObject == nil)
        throw _Exception(_("timer already inactive!"));
    return subscribersObject;
}

Timer::Timer() : Singleton<Timer>()
{
    subscribersSingleton = Ref<Array<TimerSubscriberElement<TimerSubscriberSingleton<Object> > > >(new Array<TimerSubscriberElement<TimerSubscriberSingleton<Object> > >);
    subscribersObject = Ref<Array<TimerSubscriberElement<TimerSubscriberObject> > >(new Array<TimerSubscriberElement<TimerSubscriberObject> >);
    cond = Ref<Cond>(new Cond(mutex));
}

void Timer::triggerWait()
{
    log_debug("triggerWait. - %d subscriber(s)\n", subscribersSingleton->size());
    
    AUTOLOCK(mutex);
    if (subscribersSingleton->size() > 0 || subscribersObject->size() > 0)
    {
        struct timespec *timeout = getNextNotifyTime();
        struct timespec now;
        getTimespecNow(&now);
        if (compareTimespecs(timeout, &now) < 0)
        {
            log_debug("sleeping...\n");
            int ret = cond->timedwait(timeout);
            if (ret != 0 && ret != ETIMEDOUT)
            {
                log_debug("pthread_cond_timedwait returned errorcode %d\n", ret);
                throw _Exception(_("pthread_cond_timedwait returned errorcode ") + ret);
            }
            if (ret == ETIMEDOUT)
            {
                notify<TimerSubscriberSingleton<Object> >();
                notify<TimerSubscriberObject>();
            }
        }
        else
        {
            notify<TimerSubscriberSingleton<Object> >();
            notify<TimerSubscriberObject>();
        }
    }
    else
    {
        log_debug("nothing to do, sleeping...\n");
        cond->wait();
    }
}

struct timespec * Timer::getNextNotifyTime()
{
    struct timespec *nextTime = NULL;
    for(int i = 0; i < subscribersSingleton->size(); i++)
    {
        struct timespec *nextNotify = subscribersSingleton->get(i)->getNextNotify();
        if (nextTime == NULL || compareTimespecs(nextNotify, nextTime) > 0)
        {
            nextTime = nextNotify;
        }
    }
    for(int i = 0; i < subscribersObject->size(); i++)
    {
        struct timespec *nextNotify = subscribersObject->get(i)->getNextNotify();
        if (nextTime == NULL || compareTimespecs(nextNotify, nextTime) > 0)
        {
            nextTime = nextNotify;
        }
    }
    return nextTime;
}

void Timer::shutdown()
{
    subscribersSingleton = nil;
    subscribersObject = nil;
    log_debug("finished.\n");
}
