/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Lawrence Livermore National Laboratory
 * All rights reserved.
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */


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
   * @param bytes The number of additional data bytes to include
   *        as payload in the event.
   */
  explicit
    PholdEvent(SST::SimTime_t sendTime, std::size_t bytes = 0)
    : SST::Event(),
      m_sendTime{sendTime},
      m_bytes{bytes},
      m_buffer{nullptr}
  {
    if (m_bytes > 0) m_buffer = new char[m_bytes];
  };

  ~PholdEvent()
    {
      delete [] m_buffer;
    };

  // Rule of 5
  PholdEvent(const PholdEvent &) = delete;
  PholdEvent & operator= (const PholdEvent &) = delete;
  PholdEvent(PholdEvent &&) = delete;
  PholdEvent & operator= (const PholdEvent &&) = delete;
  

  // Basic PHOLD has no event data to send
  // \todo Consider adding src tracking, QHOLD hash...

  /**
   * Extract the send time from the event.
   * @returns The send time.
   */
  SST::SimTime_t getSendTime() const
  {
    return m_sendTime;
  };

  /** 
   * Get the buffer size. 
   * @returns The event data buffer size, in bytes.
   */
  std::size_t getBufferSize() const
    {
      return m_bytes;
    }

  /** Default c'tor, for serialization. */
  PholdEvent()
    : SST::Event(),
    m_sendTime(0),
    m_bytes(0),
    m_buffer{nullptr}
  {};

  // Inherited
  void
  serialize_order(SST::Core::Serialization::serializer & ser) override
  {
    Event::serialize_order(ser);
    ser & m_sendTime;
    // Serializes m_bytes then m_buffer
    ser.binary(m_buffer, m_bytes);
  };

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

  /** Byte buffer size. */
  std::size_t m_bytes;
  /** Bytes buffer. */
  char * m_buffer;

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
  };

  /** Default c'tor, for serialization. */
  InitEvent()
   : SST::Event(),
    m_sender(0)
  {};

  // Inherited
  void
  serialize_order(SST::Core::Serialization::serializer & ser) override
  {
    Event::serialize_order(ser);
    ser & m_sender;
  };

  ImplementSerializable(Phold::InitEvent);

private:

  /** Sender id. */
  SST::ComponentId_t m_sender;

};  // class InitEvent



/**
 * Event sent by PHOLD LPs during completion,
 * containing the total number of events sent and received
 * by the LP sending this event, and all it's children.
 */
class CompleteEvent : public SST::Event
{
public:
  /**
   * C'tor
   * @param sendCount The total number of events sent.
   * @param recvCount The total number of events received.
   */
  CompleteEvent(std::size_t sendCount, std::size_t recvCount)
    : m_sendCount(sendCount),
    m_recvCount(recvCount)
  {};

  /**
   * Get the send count from the event.
   * @returns The total number of events sent.
   */
  std::size_t getSendCount() const
  {
    return m_sendCount;
  };

  /**
   * Get the receive count from the event.
   * @returns The total number of events received.
   */
  std::size_t getRecvCount() const
  {
    return m_recvCount;
  };

  /** Default c'tor, for serialization. */
  CompleteEvent()
   : SST::Event(),
    m_sendCount(0),
    m_recvCount(0)
  {};

  // Inherited
  void
  serialize_order(SST::Core::Serialization::serializer & ser) override
  {
    Event::serialize_order(ser);
    ser & m_sendCount;
    ser & m_recvCount;
  };

  ImplementSerializable(Phold::CompleteEvent);

private:

  std::size_t m_sendCount;  /** The send count. */
  std::size_t m_recvCount;  /** The receive count. */

};  // class CompleteEvent

}  // namespace Phold

#endif //PHOLD_PHOLDEVENT_H
