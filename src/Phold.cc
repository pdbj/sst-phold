/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Lawrence Livermore National Laboratory
 * All rights reserved.
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */


#include "Phold.h"

#include <sst/core/timeConverter.h>
#include <sst/core/sst_types.h>

#include <cstdint>  // UINT32_MAX
#include <iostream>
#include <iomanip>
#include <string>  // to_string()
#include <utility> // swap()
#include <vector>

/**
 * \file
 * Phold::Phold class implementation.
 */

namespace Phold {

// Simplify use of sst_assert
#define ASSERT(condition, ...) \
    Component::sst_assert(condition, CALL_INFO_LONG, 1, __VA_ARGS__)

// Simplify logging
#define VERBOSE(...) \
  m_output.verbose(CALL_INFO, 1, 0, __VA_ARGS__)

// Class static data members
const char     Phold::PORT_NAME[];
const char     Phold::TIMEBASE[];

double         Phold::m_remote;
double         Phold::m_minimum;
double         Phold::m_average;
long           Phold::m_number;
long           Phold::m_events;
uint32_t       Phold::m_verbose;
SST::SimTime_t Phold::m_stop;


Phold::Phold( SST::ComponentId_t id, SST::Params& params )
  : SST::Component(id)
{
  // SST::Params doesn't understand Python bools
  auto pverb = params.find<std::string>  ("pverbose", "False");
  m_verbose = ( "True" == pverb);
  m_output.init("@t:Phold-" + getName() + " [@p (@f:@l)] -> ", m_verbose, 0, SST::Output::STDOUT);
  VERBOSE("Full c'tor() @0x%p\n", this);

  m_remote  = params.find<double>("remote",   0.9);
  m_minimum = params.find<double>("minimum",  1.0);
  m_average = params.find<double>("average", 10);
  auto stop = params.find<double>("stop",    10)  / TIMEFACTOR;
  m_stop    = static_cast<SST::SimTime_t>(stop);
  m_number  = params.find<long>  ("number",   2);
  m_events  = params.find<long>  ("events",   1);

  // Default time unit for Component and links
  registerTimeBase(TIMEBASE, true);

  std::stringstream ss;  // Declare here so we can reuse it below
  if (m_verbose) {
    ss << "  Config: "
       << "remote=" << m_remote
       << ", min="  << m_minimum
       << ", avg="  << m_average
       << ", num="  << m_number
       << ", ev="   << m_events
       << ", st="   << m_stop
       << (m_verbose ? " VERBOSE" : "");

    VERBOSE("%s\n", ss.str().c_str());
  }

  VERBOSE("Initializing RNGs\n");
  m_rng  = new Phold::RNG_t;
  VERBOSE("  m_rng      @0x%p\n", m_rng);
  m_remRng  = new SST::RNG::SSTUniformDistribution(UINT32_MAX, m_rng);
  VERBOSE("  m_remRng   @0x%p\n", m_remRng);
  m_nodeRng = new SST::RNG::SSTUniformDistribution(m_number, m_rng);
  VERBOSE("  m_nodeRng  @0x%p\n", m_nodeRng);
  m_delayRng = new SST::RNG::SSTExponentialDistribution(m_average, m_rng);
  VERBOSE("  m_delayRng @0x%p\n", m_delayRng);

  // Configure ports/links
  VERBOSE("Configuring links:\n");
  VERBOSE("  Port links\n");
  m_links.resize(m_number);

  // Set up the port labels
  auto pre = std::string(PORT_NAME);
  const auto prefix(pre.erase(pre.find('%')));
  std::string port;
  for (uint32_t i = 0; i < m_number; ++i) {
    ASSERT(m_links[i] == nullptr, "Initialized link %d (0x%p) is not null!\n", i, m_links[i]);
    port = prefix + std::to_string(i);
    if (i != getId())
    {
      ASSERT(isPortConnected(port), "Port %s is not connected\n", port.c_str());
      // Each link needs it's own handler.  SST manages the destruction.
      auto handler = new SST::Event::Handler<Phold>(this, &Phold::handleEvent);
      ASSERT(handler, "Failed to create handler\n");
      m_links[i] = configureLink(port, handler);
      ASSERT(m_links[i], "Failed to configure link %d\n", i);
      VERBOSE("    link %d: %s @0x%p with handler @0x%p\n", i, port.c_str(), m_links[i], handler);
    } else {
      auto handler = new SST::Event::Handler<Phold>(this, &Phold::handleEvent);
      ASSERT(handler, "Failed to create handler\n");
      m_links[i] = configureSelfLink("self", handler);
      ASSERT(m_links[i], "Failed to configure self link\n");
      VERBOSE("    link %d: self   @0x%p with handler @0x%p\n", i, m_links[i], handler);
    }
  }

  // Initial events created in setup()

  // Tell SST to wait until we authorize it to exit
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();
}


Phold::Phold() : SST::Component(-1)
{
  VERBOSE("Default c'tor()\n");
  /*
   * \todo How to initialize a Component after deserialization?
   * Here we need m_number, m_average
   * These are class static, so available in this case,
   * but what to do in the general case of instance data?
   */
  m_rng  = new Phold::RNG_t;
  m_remRng  = new SST::RNG::SSTUniformDistribution(UINT32_MAX, m_rng);
  m_nodeRng = new SST::RNG::SSTUniformDistribution(m_number, m_rng);
  m_delayRng = new SST::RNG::SSTExponentialDistribution(m_average, m_rng);
}

Phold::~Phold() noexcept
{
  VERBOSE("Destructor()\n");
#define DELETE(p) \
  VERBOSE("  deleting %s @0x%p\n", #p, (p)); \
  p = 0

  DELETE(m_rng);
  DELETE(m_remRng);
  DELETE(m_nodeRng);
  DELETE(m_delayRng);

#undef DELETE
}

void
Phold::SendEvent ()
{
  VERBOSE("\n");

  // Remote or local?
  SST::ComponentId_t nextId = getId();
  auto rem = m_remRng->getNextDouble() / UINT32_MAX;
  if (rem < m_remote)
  {
    nextId = static_cast<SST::ComponentId_t>(m_nodeRng->getNextDouble());
    VERBOSE("  next rng: %f, remote %lld\n", rem, nextId);
  }
  else
  {
    VERBOSE("  next rng: %f, self   %lld\n", rem, nextId);
  }
  ASSERT(nextId < m_number && nextId >= 0, "invalid nextId: %lld\n", nextId);

  // Time to event, in s
  auto delayS = m_delayRng->getNextDouble();
  auto delayTb = delayS / TIMEFACTOR;
  // m_minimum is added by the link
  auto now = getCurrentSimTime("1s");
  VERBOSE("  delay: %f (%f tb), total: %f => %f\n",
          delayS, 
          delayTb,
          delayS + m_minimum,
          delayS + m_minimum + now);

  // Send a new event.  This is deleted in handleEvent
  auto ev = new PholdEvent();
  m_links[nextId]->send(delayTb, ev);
}


void
Phold::handleEvent(SST::Event *ev)
{
  auto event = dynamic_cast<PholdEvent*>(ev);
  ASSERT(event, "Failed to cast SST::Event * to PholdEvent *");
  // Extract any useful data, then clean it up
  delete event;

  // Check the stopping condition
  auto now = TIMEFACTOR * getCurrentSimTime();
  if (now < m_stop)
  {
    VERBOSE("now: %f\n", now);
    SendEvent();
  } else
  {
    VERBOSE("now: %f, stopping\n", now);
    primaryComponentOKToEndSim();
  }
}


void
Phold::setup() 
{
  VERBOSE("initial events: %ld\n", m_events);

  // Generate initial event set
  for (auto i = 0; i < m_events; ++i)
  {
    SendEvent();
  }
}


void
Phold::finish() 
{
  VERBOSE("\n");
}

}  // namespace Phold
