//
// Created by Barnes, Peter D. on 6/29/21.
//

#ifndef PHOLD_PHOLDEVENT_H
#define PHOLD_PHOLDEVENT_H

namespace Phold {

class PholdEvent : public SST::Event
{
public:
  PholdEvent() : SSTEvent() {};

  // Basic PHOLD has no event data to send

public:
  void serialize_order(SST::CORE::Serialization)

};

}  // namespace Phold

#endif //PHOLD_PHOLDEVENT_H
