/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    timer.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    $Id$
*/

/// \file timer.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "timer.h"
#include "tools.h"

using namespace zmm;

Timer::Timer() : Object()
{
    subscribers = Ref<Array<TimerSubscriberElement> >(new Array<TimerSubscriberElement>);
    cond = Ref<Cond>(new Cond(mutex));
}

Timer::~Timer()
{
}

Ref<Mutex> Timer::mutex = Ref<Mutex>(new Mutex(true));
Ref<Timer> Timer::instance = nil;

Ref<Timer> Timer::getInstance()
{
    if (instance == nil)
    {
        mutex->lock();
        if (instance == nil) // check again, because there is a very small chance
                             // that 2 threads tried to lock() concurrently
        {
            try
            {
                instance = Ref<Timer>(new Timer());
            }
            catch (Exception e)
            {
                mutex->unlock();
                throw e;
            }
        }
        mutex->unlock();
    }
    return instance;
}

void Timer::addTimerSubscriber(Ref<TimerSubscriber> timerSubscriber, unsigned int notifyInterval, int id)
{
    log_debug("adding subscriber...\n");
    if (notifyInterval <= 0)
        throw _Exception(_("tried to add timer with illegal notifyInterval"));
    mutex->lock();
    //timerSubscriber->timerNotify(id);
    Ref<TimerSubscriberElement> element(new TimerSubscriberElement(timerSubscriber, notifyInterval, id));
    for(int i = 0; i < subscribers->size(); i++)
    {
        if (subscribers->get(i)->equals(element))
        {
            mutex->unlock();
            throw _Exception(_("tried to add same timer twice"));
        }
    }
    subscribers->append(element);
    signal();
    mutex->unlock();
}

void Timer::removeTimerSubscriber(Ref<TimerSubscriber> timerSubscriber, int id)
{
    log_debug("removing subscriber...\n");
    mutex->lock();
    Ref<TimerSubscriberElement> element(new TimerSubscriberElement(timerSubscriber, 0, id));
    bool removed = false;
    for(int i = 0; i < subscribers->size(); i++)
    {
        if (subscribers->get(i)->equals(element))
        {
            subscribers->remove(i);
            removed = true;
            break;
        }
    }
    if (! removed)
    {
        mutex->unlock();
        throw _Exception(_("tried to remove nonexistent timer"));
    }
    signal();
    mutex->unlock();
}

void Timer::triggerWait()
{
    log_debug("triggerWait. - %d subscriber(s)\n", subscribers->size());
    
    mutex->lock();
    if (subscribers->size() > 0)
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
                mutex->unlock();
                throw _Exception(_("pthread_cond_timedwait returned errorcode ") + ret);
            }
            if (ret == ETIMEDOUT)
            {
                notify();
            }
        }
        else
        {
            notify();
        }
    }
    else
    {
        log_debug("nothing to do, sleeping...\n");
        cond->wait();
    }
    mutex->unlock();
}

void Timer::notify()
{
    struct timespec now;
    getTimespecNow(&now);
    log_debug("notifying. - %d subscribers\n", subscribers->size());
    for(int i = 0; i < subscribers->size(); i++)
    {
        Ref<TimerSubscriberElement> element = subscribers->get(i);
        if (compareTimespecs(element->getNextNotify(), &now) >= 0)
        {
            log_debug("notifying %d\n", i);
            zmm::Ref<TimerSubscriber> subscriber = element->getSubscriber();
            subscriber->timerNotify(element->getID());
            element->notified();
        }
    }
}

struct timespec * Timer::getNextNotifyTime()
{
    struct timespec *nextTime = NULL;
    for(int i = 0; i < subscribers->size(); i++)
    {
        struct timespec *nextNotify = subscribers->get(i)->getNextNotify();
        if (nextTime == NULL || compareTimespecs(nextNotify, nextTime) > 0)
        {
            nextTime = nextNotify;
        }
    }
    return nextTime;
}

Timer::TimerSubscriberElement::TimerSubscriberElement(Ref<TimerSubscriber> subscriber, unsigned int notifyInterval, int id)
{
    this->subscriber = subscriber;
    this->notifyInterval = notifyInterval;
    this->id = id;
    notified();
}

void Timer::TimerSubscriberElement::notified()
{
    getTimespecAfterMillis(notifyInterval * 1000, &nextNotify);
}

bool Timer::TimerSubscriberElement::equals(Ref<TimerSubscriberElement> other)
{
    return (subscriber == other->subscriber && id == other->id);
}
