/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Lawrence Livermore National Laboratory
 * All rights reserved.
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */


#include "Phold.h"

#include <sst/core/timeConverter.h>
namespace Phold {

// Class static member
double Phold::m_remote;
double Phold::m_minimum;
double Phold::m_average;
long   Phold::m_number;
long   Phold::m_events;
bool   Phold::m_verbose;
SST::SimTime_t Phold::m_stop;
SST::TimeConverter * Phold::m_timeConverter;

// Simplify use of sst_assert
#define ASSERT(condition, ...) \
    Component::sst_assert(condition, CALL_INFO_LONG, 1, __VA_ARGS__)


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
  m_stop = static_cast<SST::SimTime_t>(1e9 * stop);
  m_timeConverter = getTimeConverter("1ns");

  if (m_verbose) {
    std::stringstream ss;
    ss << "  Config: "
       << "remote=" << m_remote
       << ", min=" << m_minimum
       << ", avg=" << m_average
       << ", num=" << m_number
       << ", ev=" << m_events
       << ", st="   << m_stop
       << (m_verbose ? " VERBOSE" : "");

    m_output.verbose(CALL_INFO, 1, 0, "%s\n", ss.str().c_str());
  }

  m_rng  = new SST::RNG::MersenneRNG();
  m_uni  = new SST::RNG::SSTUniformDistribution(m_number, m_rng);
  m_pois = new SST::RNG::SSTPoissonDistribution(m_average, m_rng);

  // Initial events created in setup()


  // Tell SST to wait until we authorize it to exit
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();
}


Phold::Phold(void) : SST::Component(-1)
{
  m_output.verbose(CALL_INFO, 1, 0, "Default c'tor()\n");
  /*
   * \todo How to initialize a Component after deserialization?
   * Here we need m_number, m_average
   * These are class static, so available in this case,
   * but what to do in the general case of instance data?
   */
  m_rng  = new SST::RNG::MersenneRNG();
  m_uni  = new SST::RNG::SSTUniformDistribution(m_number, m_rng);
  m_pois = new SST::RNG::SSTPoissonDistribution(m_average, m_rng);
}

Phold::~Phold() noexcept
{
  m_output.verbose(CALL_INFO, 1, 0, "Destructor()\n");
  delete m_pois;
  delete m_uni;
  delete m_rng;
}

void
Phold::SendEvent ()
{
  m_output.verbose(CALL_INFO, 1, 0, "sendEvent():\n");

  // Remote or local?
  SST::ComponentId_t nextId = getId();
  auto rem = m_rem->getNextDouble() / UINT32_MAX;
  m_output.verbose(CALL_INFO, 1, 0, "  next rng: %f\n", rem);
  if (m_remote < rem)
  {
    nextId = static_cast<SST::ComponentId_t>(m_uni->getNextDouble());
    m_output.verbose(CALL_INFO, 1, 0, "  remote %lld\n", nextId);
  }
  else
  {
    m_output.verbose(CALL_INFO, 1, 0, "  self   %lld\n", nextId);
  }
  ASSERT(nextId < m_number && nextId >= 0, "invalid nextId: %lld\n", nextId);

  // Time to event, in s
  auto delayS = m_pois->getNextDouble();

  // Need to convert to SST::SimTime_t
  auto delaySt = static_cast<SST::SimTime_t>(1e9 * delayS);

  // m_minimum is added by the link

  m_output.verbose(CALL_INFO, 1, 0,
                   "  delay: %f, total: %f\n", delayS, delayS + m_minimum);

  // Send a new event
  auto ev = new PholdEvent();
  m_links[nextId]->send(delaySt, m_timeConverter, ev);
}


void
Phold::handleEvent(SST::Event *ev)
{
  m_output.verbose(CALL_INFO, 1, 0, "handlEvent()\n");
  auto event = dynamic_cast<PholdEvent*>(ev);
  ASSERT(event, "Failed to cast SST::Event * to PholdEvent *");

  // Check the stopping condition
  auto now = getCurrentSimTime(m_timeConverter);
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
