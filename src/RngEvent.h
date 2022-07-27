/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Lawrence Livermore National Laboratory
 * All rights reserved.
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */


#pragma once

#include <sst/core/event.h>

/**
 * \file
 * Event type Rng::RngEvent for RNG benchmark
 */

namespace Phold {

/**
 * Nuisance event to enable Rng components to link to each other.
 */
class RngEvent : public SST::Event
{
public:
  /** Default c'tor. */
  RngEvent()
    : SST::Event()
  {};

  // Inherited
  void
  serialize_order(SST::Core::Serialization::serializer & ser) override
  {
    Event::serialize_order(ser);
  };

  /*
    Defined in sst-core/src/sst/core/serializatino/serializeable.h
    Declares public functions: cls_name(), cls_id(), and serialization_name().
    Along with serialize_order() these complete the ABC implementation of
    class SST::Core::Serialization::serializable().
  */
  ImplementSerializable(Phold::RngEvent);

private:

};  // class RngEvent


}  // namespace Phold

