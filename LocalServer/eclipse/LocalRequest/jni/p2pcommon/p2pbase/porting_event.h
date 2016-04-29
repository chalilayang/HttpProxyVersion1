#ifndef WINDOWS_EVENT_H_
#define  WINDOWS_EVENT_H_

#include "../base/common.h"

namespace porting
{
    class Event;
    typedef boost::shared_ptr<Event> EventHandle;
    static const unsigned kINFINITE = 0xFFFFFFFF;

    class Event
    {
        friend EventHandle CreateEvent( void );
        friend void CloseHandle( EventHandle evt );
        friend void SetEvent( EventHandle evt );
        friend bool WaitForSingleObject( EventHandle evt, unsigned timeout );

        Event( void ) : m_bool(false) { }

        bool m_bool;
        boost::mutex m_mutex;
        boost::condition m_cond;
    };

    inline EventHandle CreateEvent( void )
    { return boost::shared_ptr<Event>(new Event); }

    inline void CloseHandle( EventHandle evt )
    { evt.reset(); }

    inline void SetEvent( EventHandle evt )
    {
        evt->m_bool = true;
        evt->m_cond.notify_all();
    }

    inline bool WaitForSingleObject( EventHandle evt, unsigned timeout )
    {
        boost::mutex::scoped_lock lock( evt->m_mutex );

        //slightly more complex code for timeouts
        return evt->m_cond.timed_wait(lock, boost::posix_time::milliseconds(timeout));
    }

}// porting

#endif