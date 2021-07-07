/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Lawrence Livermore National Laboratory
 * All rights reserved.
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */


#include "Phold.h"

namespace Phold {

// Class static member
double Phold::m_remote;
double Phold::m_minimum;
double Phold::m_average;
long   Phold::m_number;
long   Phold::m_events;
bool   Phold::m_verbose;

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

  if (m_verbose) {
    std::stringstream ss;
    ss << "  Config: "
       << "remote=" << m_remote
       << ", min=" << m_minimum
       << ", avg=" << m_average
       << ", num=" << m_number
       << ", ev=" << m_events
       << (m_verbose ? " VERBOSE" : "");

    m_output.verbose(CALL_INFO, 1, 0, "%s\n", ss.str().c_str());
  }

  m_rng  = new SST::RNG::MersenneRNG();
  m_uni  = new SST::RNG::SSTUniformDistribution(m_number, m_rng);
  m_pois = new SST::RNG::SSTPoissonDistribution(m_average, m_rng);

  // Initial events created in setup()

  /*
  // Just register a plain clock for this simple example
  registerClock("100MHz", new SST::Clock::Handler<Phold>(this, &Phold::clockTick));
  */

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
Phold::setup() 
{
  m_output.verbose(CALL_INFO, 1, 0, "setup()\n");

  // Generate initial event set
  m_output.verbose (CALL_INFO, 1, 0, "  Initial samples:\n");

  for (auto i = 0; i <m_events; ++i)
  {
    auto remoteId = static_cast<uint64_t>(m_uni->getNextDouble());
    double delay  = m_pois->getNextDouble();
    // m_minimum is added by the link

    /// \todo Schedule the event

    if (m_verbose) {
      std::stringstream ss;
      ss.clear();
      ss.str("");
      ss << "    remote: " << remoteId
         << ", delay: " << delay
         << ", total: " << delay + m_minimum;
      m_output.verbose(CALL_INFO, 1, 0, "%s\n", ss.str().c_str());
    }
  }
}

void
Phold::finish() 
{
  m_output.verbose(CALL_INFO, 1, 0, "finish()\n");
}

bool
Phold::clockTick( SST::Cycle_t currentCycle ) 
{
  m_output.verbose(CALL_INFO, 1, 0, "clockTick()\n");
  uint64_t remoteId = m_uni->getNextDouble();
  double   delay    = m_pois->getNextDouble();
  
  if (m_verbose) {
    std::stringstream ss;
    ss << "Tick: " << currentCycle
       << ", remote: " << remoteId
       << ", delay: " << delay
       << ", total: " << delay + m_minimum;

    m_output.verbose(CALL_INFO, 1, 0, "%s\n", ss.str().c_str());
  }

  primaryComponentOKToEndSim();

  return true;
}

void
Phold::handleEvent(SST::Event *ev)
{
  m_output.verbose(CALL_INFO, 1, 0, "handlEvent()\n");
  auto event = dynamic_cast<PholdEvent*>(ev);
  ASSERT(event, "Failed to cast SST::Event * to PholdEvent *");

  // Schedule and send new event
  uint64_t remoteId = m_uni->getNextDouble();
  double   delay    = m_pois->getNextDouble();
  // m_minimum is added by the link

  // Send the event
  PholdEvent * e = new PholdEvent();
  // link N->send(e)

  if (m_verbose) {
    std::stringstream ss;
    ss.clear();
    ss.str("");
    ss << "  New event: remote: " << remoteId
       << ", delay: " << delay
       << ", total: " << delay + m_minimum;

    m_output.verbose(CALL_INFO, 1, 0, "%s\n", ss.str().c_str());
  }

}

}  // namespace Phold
