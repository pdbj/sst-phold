/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Lawrence Livermore National Laboratory
 * All rights reserved.
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */


#include "Rng.h"
#include "PholdEvent.h"

#include <sst/core/timeConverter.h>
#include <sst/core/sst_types.h>

#include <cstdint>  // UINT32_MAX
#include <iostream>
#include <string>  // to_string()
#include <utility> // swap()

/**
 * \file
 * Phold::Rng class implementation.
 */

namespace Phold {

#ifdef PHOLD_DEBUG
// Simplify use of sst_assert
#define ASSERT(condition, args...) \
    Component::sst_assert(condition, CALL_INFO_LONG, 1, args)

#define VERBOSE(l, f, args...)                          \
  m_output.verbosePrefix(VERBOSE_PREFIX.c_str(),        \
                         CALL_INFO, l, 0,               \
                         "[%d] " f, l, ##args)
#else
#define ASSERT(...)
#define VERBOSE(...)
#endif


#define OUTPUT(...)                             \
  if (0 == getId()) m_output.output(CALL_INFO, __VA_ARGS__)
  
// Class static data members
constexpr char Rng::PORT_NAME[];  // constexpr initialized in Rng.h
long           Rng::m_number;
long           Rng::m_samples;
uint32_t       Rng::m_verbose;


Rng::Rng( SST::ComponentId_t id, SST::Params& params )
  : SST::Component(id)
{
  m_verbose = params.find<long>("pverbose", 0);
  m_output.init("@t:@X:Rng-" + getName() + " [@p()] -> ", 
                m_verbose, 0, SST::Output::STDOUT);
#ifdef PHOLD_DEBUG
  VERBOSE_PREFIX = "@t:@X:Rng-" + getName() + " [@p() (@f:@l)] -> ";
  VERBOSE(2, "Full c'tor() @0x%p, id: %u, name: %s\n", this, getId(), getName().c_str());
#endif
  
  m_number  = params.find<long>   ("number",   2);
  m_samples = params.find<long>   ("samples",   1);

  registerTimeBase("1 us", true);

  if (0 == getId()) {
    std::stringstream ss;

    ss << "\nRng Configuration:"
       << "\n    Number of components:             " << m_number
       << "\n    Number of samples per component:  " << m_samples
       << "\n    Verbosity level:                  " << m_verbose
       << "\n    Optimization level:               "
#ifdef PHOLD_DEBUG
       << "debug"
#else
       << "optimized"
#endif
       << std::endl;
#undef BEST

    OUTPUT("%s\n", ss.str().c_str());
  }

  VERBOSE(3, "Initializing RNGs\n");
  m_rng  = new Rng::RNG_t;
  // seed() doesn't check validity of arg, can't be 0
  m_rng->seed(1 + getId());
  VERBOSE(4, "  m_rng      @0x%p\n", m_rng);

  // Configure ports/links
  auto handler = new SST::Event::Handler<Rng, uint32_t>(this, &Rng::handleEvent, getId());
  ASSERT(handler, "Failed to create handler\n");
  m_self = configureSelfLink("self", handler);
  ASSERT(m_self, "Failed to configure self link\n");
  VERBOSE(4, "    self link  @0x%p with handler @0x%p\n", m_self, handler);

  // Register statistics

  // Rng samples acquired in the event handler()

  // Tell SST to wait until we authorize it to exit
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();
}


Rng::Rng() : SST::Component(-1)
{
  VERBOSE(2, "Default c'tor()\n");
  /*
   * \todo How to initialize a Component after deserialization?
   * Here we need m_number, m_average
   * These are class static, so available in this case,
   * but what to do in the general case of instance data?
   */
  m_rng  = new Rng::RNG_t;
}


Rng::~Rng() noexcept
{
  VERBOSE(2, "Destructor()\n");
#define DELETE(p) \
  VERBOSE(4, "  deleting %s @0x%p\n", #p, (p));  \
  p = 0

  DELETE(m_rng);

#undef DELETE
}


void
Rng::handleEvent(SST::Event *ev, uint32_t from)
{
  auto event = dynamic_cast<PholdEvent*>(ev);
  ASSERT(event, "Failed to cast SST::Event * to PholdEvent *");
  // Extract any useful data, then clean it up
  delete event;

  // Do the iterations
  double sum = 0;
  for (uint64_t i = 0; i < m_samples; ++i)
    {
      sum += m_rng->nextUniform();
    }

  // Check the stopping condition
  primaryComponentOKToEndSim();
}


void
Rng::setup() 
{
  VERBOSE(2, "sending initial event\n");

  auto ev = new PholdEvent(0);
  m_self->send(0, ev);
}


void
Rng::finish() 
{
  VERBOSE(2, "\n");
}

}  // namespace Phold
