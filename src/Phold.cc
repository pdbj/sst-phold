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

// Class static data members
const char Phold::PORT_NAME[];
const char Phold::TIMEBASE[];

double Phold::m_remote;
double Phold::m_minimum;
double Phold::m_average;
long   Phold::m_number;
long   Phold::m_events;
bool   Phold::m_verbose;
SST::SimTime_t Phold::m_stop;


Phold::Phold( SST::ComponentId_t id, SST::Params& params )
  : SST::Component(id)
{
  // SST::Params doesn't understand Python bools
  auto pverb = params.find<std::string>  ("pverbose", "False");
  m_verbose = ( "True" == pverb);
  m_output.init("Phold-" + getName() + "-> ", m_verbose, 0, SST::Output::STDOUT);
  m_output.verbose(CALL_INFO, 1, 0, "Full c'tor()\n");

  m_remote  = params.find<double>("remote", 0.9);
  m_minimum = params.find<double>("minimum", 0.1);
  m_average = params.find<double>("average", 0.9);
  m_number  = params.find<long>  ("number", 2);
  m_events  = params.find<long>  ("events", 2);
  auto stop = params.find<double>("stop", 100);
  // \todo Use TIMEBASE for this conversion
  m_stop = static_cast<SST::SimTime_t>(1e9 * stop);
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

    m_output.verbose(CALL_INFO, 1, 0, "%s\n", ss.str().c_str());
  }

  m_output.verbose(CALL_INFO, 1, 0, "Initializing RNGs\n");
  m_rng  = new Phold::RNG_t;
  m_remRng  = new SST::RNG::SSTUniformDistribution(UINT32_MAX, m_rng);
  m_nodeRng = new SST::RNG::SSTUniformDistribution(m_number, m_rng);
  m_delayRng = new SST::RNG::SSTExponentialDistribution(m_average, m_rng);

  // Configure ports/links
  m_output.verbose(CALL_INFO, 1, 0,"Configuring links:\n");

  m_output.verbose(CALL_INFO, 1, 0,"  Creating handler\n");
  m_handler = new SST::Event::Handler<Phold>(this, &Phold::handleEvent);
  ASSERT(m_handler, "Failed to create handler\n");

  m_output.verbose(CALL_INFO, 1, 0, "  Port links\n");
  m_links.reserve(m_number);

  // Set up the ss with the prefix  
  auto pre = std::string(PORT_NAME);
  pre.erase(pre.find('%'));
  const auto prefix(pre);
  const auto end = prefix.size();
  ss.clear();
  for (uint32_t i = 0; i < m_number; ++i)
    {
      ss.str(prefix);
      ss.seekp(end) << i;

      if (i  != getId())
        {
          m_output.verbose(CALL_INFO, 1, 0, "    link %d: %s\n", 
                           i, ss.str().c_str());
          m_links[i] = configureLink(ss.str(), m_handler);
          ASSERT(m_links[i], "Failed to configure link %d\n", i);
        }
      else
        {
          m_output.verbose(CALL_INFO, 1, 0, "    link %d: %s (%s)\n", 
                           i, ss.str().c_str(), "self");
          m_links[i] = configureSelfLink(ss.str(), m_handler);
          ASSERT(m_links[i], "Failed to configure self link %d\n", i);
        }
    }

  // Initial events created in setup()

  // Tell SST to wait until we authorize it to exit
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();
}


Phold::Phold() : SST::Component(-1)
{
  m_output.verbose(CALL_INFO, 1, 0, "Default c'tor()\n");
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
  m_handler = new SST::Event::Handler<Phold>(this, &Phold::handleEvent);
}

Phold::~Phold() noexcept
{
  m_output.verbose(CALL_INFO, 1, 0, "Destructor()\n");
  delete m_delayRng;
  delete m_nodeRng;
  delete m_remRng;
  delete m_rng;

  delete m_handler;
}

void
Phold::SendEvent ()
{
  m_output.verbose(CALL_INFO, 1, 0, "sendEvent():\n");

  // Remote or local?
  SST::ComponentId_t nextId = getId();
  auto rem = m_remRng->getNextDouble() / UINT32_MAX;
  m_output.verbose(CALL_INFO, 1, 0, "  next rng: %f\n", rem);
  if (m_remote < rem)
  {
    nextId = static_cast<SST::ComponentId_t>(m_nodeRng->getNextDouble());
    m_output.verbose(CALL_INFO, 1, 0, "  remote %lld\n", nextId);
  }
  else
  {
    m_output.verbose(CALL_INFO, 1, 0, "  self   %lld\n", nextId);
  }
  ASSERT(nextId < m_number && nextId >= 0, "invalid nextId: %lld\n", nextId);

  // Time to event, in s
  auto delayS = m_delayRng->getNextDouble();
  auto delayNs = getTimeConverterNano()->convertToCoreTime(delayS);

  // m_minimum is added by the link

  m_output.verbose(CALL_INFO, 1, 0,
                   "  delay: %f, total: %f => %f\n",
                   delayS, delayS + m_minimum,
                   delayS + m_minimum +
                   static_cast<double>(getCurrentSimTime("1s")));

  // Send a new event.  This is deleted in handleEvent
  auto ev = new PholdEvent();
  m_links[nextId]->send(delayNs, ev);
}


void
Phold::handleEvent(SST::Event *ev)
{
  m_output.verbose(CALL_INFO, 1, 0,
                   "handleEvent(): %lld ms\n",
                   getCurrentSimTime("1ms"));
  auto event = dynamic_cast<PholdEvent*>(ev);
  ASSERT(event, "Failed to cast SST::Event * to PholdEvent *");
  // Extract any useful data, then clean it up
  delete event;

  // Check the stopping condition
  auto now = getCurrentSimTime(TIMEBASE);
  if (now < m_stop)
  {
    SendEvent();
  } else
  {
    primaryComponentOKToEndSim();
  }
}


void
Phold::setup() 
{
  m_output.verbose(CALL_INFO, 1, 0, "setup(): initial events: %ld\n", m_events);

  // Generate initial event set
  for (auto i = 0; i < m_events; ++i)
  {
    SendEvent();
  }
}


void
Phold::finish() 
{
  m_output.verbose(CALL_INFO, 1, 0, "finish()\n");
}

}  // namespace Phold
