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
  PholdEvent() : SST::Event() {};

  // Basic PHOLD has no event data to send
  // \todo Consider adding src tracking, QHOLD hash...

public:
  void
  serialize_order(SST::Core::Serialization::serializer & ser) override
  {
    Event::serialize_order(ser);
    // ser & ...
    }

  ImplementSerializable(Phold::PholdEvent);

};  // class PholdEvent

}  // namespace Phold

#endif //PHOLD_PHOLDEVENT_H
