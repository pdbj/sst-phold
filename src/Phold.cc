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
#include <string>  // to_string()
#include <utility> // swap()

/**
 * \file
 * Phold::Phold class implementation.
 */

namespace Phold {

// Simplify use of sst_assert
#define ASSERT(condition, ...) \
    Component::sst_assert(condition, CALL_INFO_LONG, 1, __VA_ARGS__)

#define VERBOSE(l, ...)                                                 \
  m_output.verbosePrefix(VERBOSE_PREFIX.c_str(), \
                         CALL_INFO, l, 0, __VA_ARGS__)

#define OUTPUT(...)                             \
  if (m_verbose > 0) m_output.output(CALL_INFO, __VA_ARGS__)
  
// Class static data members
const char     Phold::PORT_NAME[];  // constexpr initialized in Phold.h
const char     Phold::TIMEBASE[];

double         Phold::m_remote;
double         Phold::m_minimum;
double         Phold::m_average;
long           Phold::m_number;
long           Phold::m_events;
bool           Phold::m_delaysOut;
uint32_t       Phold::m_verbose;
SST::SimTime_t Phold::m_stop;


Phold::Phold( SST::ComponentId_t id, SST::Params& params )
  : SST::Component(id)
{
  // SST::Params doesn't understand Python bools
  m_verbose = params.find<long>  ("pverbose", 0);
  m_output.init("@t:@X:Phold-" + getName() + " [@p()] -> ", 
                m_verbose, 0, SST::Output::STDOUT);
  VERBOSE_PREFIX = "@t:@X:Phold-" + getName() + " [@p() (@f:@l)] -> ";
  VERBOSE(2, "Full c'tor() @0x%p\n", this);

  m_remote  = params.find<double>("remote",   0.9);
  m_minimum = params.find<double>("minimum",  1.0);
  m_average = params.find<double>("average", 10);
  auto stop = params.find<double>("stop",    10)  / TIMEFACTOR;
  m_stop    = static_cast<SST::SimTime_t>(stop);
  m_number  = params.find<long>  ("number",   2);
  m_events  = params.find<long>  ("events",   1);
  m_delaysOut = params.find<bool> ("delays", false);

  // Default time unit for Component and links
  registerTimeBase(TIMEBASE, true);

  if (m_verbose) {
    std::stringstream ss;  // Declare here so we can reuse it below
    ss << "  Config: "
       << "remote=" << m_remote
       << ", min="  << m_minimum
       << ", avg="  << m_average
       << ", num="  << m_number
       << ", ev="   << m_events
       << ", st="   << m_stop
       << ", dly="  << m_delaysOut
       << ", v="    << m_verbose;

    VERBOSE(2, "%s\n", ss.str().c_str());
  }

  VERBOSE(2, "Initializing RNGs\n");
  m_rng  = new Phold::RNG_t;
  // seed() doesn't check validity of arg, can't be 0
  m_rng->seed(1 + getId());
  VERBOSE(4, "  m_rng      @0x%p\n", m_rng);
  m_remRng  = new SST::RNG::SSTUniformDistribution(UINT32_MAX, m_rng);
  VERBOSE(4, "  m_remRng   @0x%p\n", m_remRng);
  m_nodeRng = new SST::RNG::SSTUniformDistribution(m_number, m_rng);
  VERBOSE(4, "  m_nodeRng  @0x%p\n", m_nodeRng);
  m_delayRng = new SST::RNG::SSTExponentialDistribution(1.0 / m_average, m_rng);
  VERBOSE(4, "  m_delayRng @0x%p\n", m_delayRng);

  // Configure ports/links
  VERBOSE(2, "Configuring links:\n");
  m_links.resize(m_number);

  // Set up the port labels
  auto pre = std::string(PORT_NAME);
  const auto prefix(pre.erase(pre.find('%')));
  std::string port;
  for (uint32_t i = 0; i < m_number; ++i) {
    ASSERT(m_links[i] == nullptr, "Initialized link %d (0x%p) is not null!\n", i, m_links[i]);
    if (i != getId())
    {
      port = prefix + std::to_string(i);
      ASSERT(isPortConnected(port), "Port %s is not connected\n", port.c_str());
      // Each link needs it's own handler.  SST manages the destruction.
      auto handler = new SST::Event::Handler<Phold, uint32_t>(this, &Phold::handleEvent, i);
      ASSERT(handler, "Failed to create handler\n");
      m_links[i] = configureLink(port, handler);
      ASSERT(m_links[i], "Failed to configure link %d\n", i);
      VERBOSE(4, "    link %d: %s @0x%p with handler @0x%p\n", i, port.c_str(), m_links[i], handler);
    } else {
      auto handler = new SST::Event::Handler<Phold, uint32_t>(this, &Phold::handleEvent, i);
      ASSERT(handler, "Failed to create handler\n");
      m_links[i] = configureSelfLink("self", handler);
      ASSERT(m_links[i], "Failed to configure self link\n");
      VERBOSE(4, "    link %d: self   @0x%p with handler @0x%p\n", i, m_links[i], handler);
    }
  }

  // Register statistics
  VERBOSE(2, "Initializing statistics\n");
  m_count = registerStatistic<uint64_t>("Count");
  ASSERT(m_count, "Failed to register Count statistic");
  m_count->setFlagOutputAtEndOfSim(true);
  ASSERT(m_count->isEnabled(), "Count statistic is not enabled!\n");
  ASSERT( ! m_count->isNullStatistic(), "Count statistic is Null!\n");
  VERBOSE(4, "  m_count    @0x%p\n", m_count);

  m_delays = registerStatistic<float>("Delays");
  ASSERT(m_delays, "Failed to register Delays statistic\n");
  m_delays->setFlagOutputAtEndOfSim(true);
  if (m_delaysOut)
    {
      ASSERT(m_delays->isEnabled(), "Delays statistic is not enabled!\n");
      ASSERT( ! m_count->isNullStatistic(), "Delays statistic is Null!\n");
    }
  VERBOSE(4, "  m_delays   @0x%p\n", m_delays);

  // Initial events created in setup()

  // Tell SST to wait until we authorize it to exit
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();
}


Phold::Phold() : SST::Component(-1)
{
  VERBOSE(2, "Default c'tor()\n");
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
  VERBOSE(2, "Destructor()\n");
#define DELETE(p) \
  VERBOSE(4, "  deleting %s @0x%p\n", #p, (p));  \
  p = 0

  DELETE(m_rng);
  DELETE(m_remRng);
  DELETE(m_nodeRng);
  DELETE(m_delayRng);

#undef DELETE
}

bool
Phold::SendEvent ()
{
  VERBOSE(2, "\n");

  // Time to event, in s
  auto now = getCurrentSimTime();
  auto delayS = m_delayRng->getNextDouble();
  auto delayTotS = delayS + m_minimum;
  auto delayTb = static_cast<SST::SimTime_t>(delayS / TIMEFACTOR);
  auto delayTotTb = static_cast<SST::SimTime_t>(delayTotS / TIMEFACTOR);
  if (now + delayTotTb > m_stop)
    {
      // Event would be beyond end time, so don't generate it and stop this LP
      VERBOSE(2, "now: %llu, next delay %f beyond stop\n", now, delayS);
      primaryComponentOKToEndSim();
      return false;
    }

  // Remote or local?
  SST::ComponentId_t nextId = getId();
  auto rem = m_remRng->getNextDouble() / UINT32_MAX;
  // Whether the event is local or remote
  bool local = false;
  if (rem < m_remote)
  {
    auto reps = 0;
    do
      {
        nextId = static_cast<SST::ComponentId_t>(m_nodeRng->getNextDouble());
        ++reps;
      } while (nextId == getId());

      VERBOSE(3, "  next rng: %f, remote (%d tries) %lld\n", rem, reps, nextId);
  }
  else
  {
    local = true;
    VERBOSE(3, "  next rng: %f, self             %lld\n", rem, nextId);
  }
  ASSERT(nextId < m_number && nextId >= 0, "invalid nextId: %lld\n", nextId);

  // Log and clean up delay
  m_delays->addData(delayS);
  if ( ! local)
    {
      // For remotes m_minimum is added by the link
      VERBOSE(3, "  delay: %f (%llu tb), total: %f => %f\n",
              delayS, 
              delayTb,
              delayS + m_minimum,
              delayS + m_minimum + now * TIMEFACTOR);

    } else {
      // self links don't have a min latency configured, so add it now
      auto totDelayS = delayS + m_minimum;
      delayTb = static_cast<SST::SimTime_t>(totDelayS / TIMEFACTOR);
      VERBOSE(3, "  delay: %f + %f = %f (%llu tb) => %f\n",
              delayS, 
              m_minimum,
              totDelayS,
              delayTb,
              totDelayS + now * TIMEFACTOR);
    }

  // Log the event
  delayS += m_minimum + static_cast<double>(now) * TIMEFACTOR;
  OUTPUT("from %u @ %llu, delay: %llu tb -> %lld @ %f\n", 
         from, sendTime, delayTb, nextId, delayS);

  // Send a new event.  This is deleted in handleEvent
  auto ev = new PholdEvent(getCurrentSimTime());
  m_links[nextId]->send(delayTb, ev);
  return true;
}


void
Phold::handleEvent(SST::Event *ev, uint32_t from)
{
  auto event = dynamic_cast<PholdEvent*>(ev);
  ASSERT(event, "Failed to cast SST::Event * to PholdEvent *");
  // Extract any useful data, then clean it up
  auto sendTime = event->getSendTime();
  delete event;

  // Check the stopping condition
  auto now = getCurrentSimTime();
  if (now < m_stop)
  {
    VERBOSE(2, "now: %llu, from %u @ %llu\n", now, from, sendTime);
    m_count->addData(1);
    SendEvent();
  } else
  {
    VERBOSE(2, "now: %llu, from %u @ %llu, stopping\n", now, from, sendTime);
    primaryComponentOKToEndSim();
    ASSERT(0, "Shouldn't be here handling a post-stop event\n");
  }
}


void
Phold::setup() 
{
  VERBOSE(2, "initial events: %ld\n", m_events);

  // Generate initial event set
  for (auto i = 0; i < m_events; ++i)
  {
    SendEvent();
  }
}


void
Phold::finish() 
{
  VERBOSE(2, "\n");
}

}  // namespace Phold
