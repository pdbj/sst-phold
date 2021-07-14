//
// Created by Barnes, Peter D. on 6/29/21.
//

#ifndef PHOLD_PHOLDEVENT_H
#define PHOLD_PHOLDEVENT_H

#include <sst/core/event.h>

namespace Phold {

class PholdEvent : public SST::Event
{
public:
  PholdEvent(SST::SimTime_t sendTime) 
    : SST::Event(),
      m_sendTime (sendTime)
    {};

  // Basic PHOLD has no event data to send
  // \todo Consider adding src tracking, QHOLD hash...

  SST::SimTime_t getSendTime() const
    { 
      return m_sendTime;
    }

public:
  PholdEvent() 
    : SST::Event(),
      m_sendTime(0)
    { }

  void
  serialize_order(SST::Core::Serialization::serializer & ser) override
  {
    Event::serialize_order(ser);
    ser & m_sendTime;
    }

  ImplementSerializable(Phold::PholdEvent);

 private:

  /** Send time of this event. */
  SST::SimTime_t m_sendTime;

};  // class PholdEvent

}  // namespace Phold

#endif //PHOLD_PHOLDEVENT_H
