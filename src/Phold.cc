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
bool   Phold::m_verbose;

Phold::Phold( SST::ComponentId_t id, SST::Params& params )
  : SST::Component(id)
{
  m_output.init("Phold-" + getName() + "-> ", 1, 0, SST::Output::STDOUT);

  m_remote  = params.find<double>("remote", 0.9);
  m_minimum = params.find<double>("minimum", 0.1);
  m_average = params.find<double>("average", 0.9);
  m_number  = params.find<long>  ("number", 2);
  m_verbose = params.find<bool>  ("verbose", false);

  std::stringstream ss;
  ss << "Config: "
     << "remote=" << m_remote
     << ", min=" << m_minimum
     << ", avg=" << m_average
     << ", num=" << m_number
     << (m_verbose ? " VERBOSE" : "");

  m_output.verbose(CALL_INFO, 1, 0, "%s\n", ss.str().c_str());


  m_rng  = new SST::RNG::MersenneRNG();
  m_uni  = new SST::RNG::SSTUniformDistribution(m_number, m_rng);
  m_pois = new SST::RNG::SSTPoissonDistribution(m_average, m_rng);

  uint64_t remoteId = m_uni->getNextDouble();
  double   delay    = m_pois->getNextDouble();
  
  ss.clear();
  ss.str("");
  ss << "Initial samples: "
     << "remote: " << remoteId
     << ", delay: " << delay
     << ", total: " << delay + m_minimum;

  m_output.verbose(CALL_INFO, 1, 0, "%s\n", ss.str().c_str());

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
  m_output.verbose(CALL_INFO, 1, 0, "Component is being setup.\n");
}

void
Phold::finish() 
{
  m_output.verbose(CALL_INFO, 1, 0, "Component is being finished.\n");
}

bool
Phold::clockTick( SST::Cycle_t currentCycle ) 
{
  uint64_t remoteId = m_uni->getNextDouble();
  double   delay    = m_pois->getNextDouble();
  
  std::stringstream ss;
  ss << "Tick: " << currentCycle
     << ", remote: " << remoteId
     << ", delay: " << delay
     << ", total: " << delay + m_minimum;

  m_output.verbose(CALL_INFO, 1, 0, "%s\n", ss.str().c_str());

  primaryComponentOKToEndSim();

  return true;
}

void
Phold::handleEvent(SST::Event *ev)
{
  //PholdEvent * event = dynamic_cast<PholdEvent*>(ev);
  // ASSERT (event)

  // Schedule and send new event
  uint64_t remoteId = m_uni->getNextDouble();
  double   delay    = m_pois->getNextDouble();

  // Normally PHOLD adds the min delay here:
  // double   total    = m_minimum + delay;
  // But we use the link latency feature, configured in the Python script

//  PholdEvent * e = new PholdEvent();
  // add hash to e?
  // link N->send(e)

}

}  // namespace Phold
