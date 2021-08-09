//
// Created by Barnes, Peter D. on 6/29/21.
//

#ifndef PHOLD_PHOLDEVENT_H
#define PHOLD_PHOLDEVENT_H

#include <sst/core/event.h>

/**
 * \file
 * Event types for PHOLD benchmark: 
 * Phold::PholdEvent, Phold::InitEvent, Phold::CompleteEvent.
 */


namespace Phold {

/**
 * Event sent by PHOLD LPs during simulation, 
 * just containing the send time, for debugging.
 */
class PholdEvent : public SST::Event
{
public:
  /** 
   * C'tor.
   * @param sendTime The simulation time when the event was sent.
   */
  explicit
  PholdEvent(SST::SimTime_t sendTime)
    : SST::Event(),
      m_sendTime (sendTime)
    {};

  // Basic PHOLD has no event data to send
  // \todo Consider adding src tracking, QHOLD hash...

  /**
   * Extract the send time from the event.
   * @returns The send time.
   */
  SST::SimTime_t getSendTime() const
    { 
      return m_sendTime;
    }

public:
  /** Default c'tor, for serialization. */
  PholdEvent() 
    : SST::Event(),
      m_sendTime(0)
    { }

  // Inherited
  void
  serialize_order(SST::Core::Serialization::serializer & ser) override
  {
    Event::serialize_order(ser);
    ser & m_sendTime;
    }

  /*
    Defined in sst-core/src/sst/core/serializatino/serializeable.h
    Declares public functions: cls_name(), cls_id(), and serialization_name().
    Along with serialize_order() these complete the ABC implementation of
    class SST::Core::Serialization::serializable().
  */
  ImplementSerializable(Phold::PholdEvent);

 private:

  /** Send time of this event. */
  SST::SimTime_t m_sendTime;

};  // class PholdEvent


/**
 * Event sent by PHOLD LPs during initialization, 
 * containing the sender id.
 */
class InitEvent : public SST::Event
{
public:
  /**
   * C'tor
   * @param id The sender component id.
   */
  explicit
  InitEvent(SST::ComponentId_t id)
    : SST::Event(),
    m_sender(id)
    {};

  /**
   * Get the sender id from the event.
   * @returns The sender id.
   */
  SST::ComponentId_t getSenderId() const
    {
      return m_sender;
    }

public:
  /** Default c'tor, for serialization. */
 InitEvent()
   : SST::Event(),
     m_sender(0)
   { }

  ImplementSerializable(Phold::InitEvent);

private:

  /** Sender id. */
  SST::ComponentId_t m_sender;

};  // class InitEvent




}  // namespace Phold

#endif //PHOLD_PHOLDEVENT_H
