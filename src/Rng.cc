/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Lawrence Livermore National Laboratory
 * All rights reserved.
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */


#include "Rng.h"
#include "RngEvent.h"

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

#ifdef RNG_DEBUG
// Simplify use of sst_assert
#define ASSERT(condition, ...)                                          \
  Component::sst_assert(condition, CALL_INFO_LONG, 1, __VA_ARGS__)

#define VERBOSE(l, f, ...)                              \
  m_output.verbosePrefix(VERBOSE_PREFIX.c_str(),        \
                         CALL_INFO, l, 0,               \
                         "[%d] " f, l, ## __VA_ARGS__)
#else
#define ASSERT(...)
#define VERBOSE(...)
#endif


#define OUTPUT(...)                             \
  if (0 == getId()) m_output.output(CALL_INFO, __VA_ARGS__)
  
// Class static data members
constexpr char Rng::PORT_NAME[];  // constexpr initialized in Rng.h
uint32_t       Rng::m_number;
uint64_t       Rng::m_samples;
uint32_t       Rng::m_verbose;


Rng::Rng( SST::ComponentId_t id, SST::Params& params )
  : SST::Component(id)
{
  m_verbose = params.find<long>("pverbose", 0);
  m_output.init("@t:@X:Rng-" + getName() + " [@p()] -> ", 
                m_verbose, 0, SST::Output::STDOUT);
#ifdef RNG_DEBUG
  VERBOSE_PREFIX = "@t:@X:Rng-" + getName() + " [@p() (@f:@l)] -> ";
  VERBOSE(2, "Full c'tor() @0x%p, id: %" PRIu64 ", name: %s\n", 
          (void*)this, getId(), getName().c_str());
#endif
  
  m_number  = params.find<long>   ("number",   2);
  m_samples = params.find<long>   ("samples",   1);

  registerTimeBase("1 us", true);

  if (0 == getId()) {
    std::stringstream ss;
    double totalSamples = m_number * m_samples;

    ss << "\nRng Configuration:"
       << "\n    Number of components:             " << m_number
       << "\n    Number of samples per component:  " << m_samples
       << "\n    Total rng samples:                " << totalSamples
       << "\n    Verbosity level:                  " << m_verbose
       << "\n    Optimization level:               "
#ifdef RNG_DEBUG
       << "debug"
#else
       << "optimized"
#endif
       << std::endl;

    OUTPUT("%s\n", ss.str().c_str());
  }

  VERBOSE(3, "%s", "Initializing RNGs\n");
  m_rng  = new Rng::RNG_t;
  // seed() doesn't check validity of arg, can't be 0
  m_rng->seed(1 + getId());
  VERBOSE(4, "  m_rng      @0x%p\n", (void*)m_rng);

  // Configure ports/links
  VERBOSE(3, "%s", "Configuring links:\n");

  auto linkup = [this](std::string port, uint32_t id) -> void
    {
      ASSERT(isPortConnected(port),
             "%s is not connected\n", port.c_str());
      auto handler = new SST::Event::Handler<Rng, uint32_t>(this, &Rng::handleEvent, id);
      ASSERT(handler, "Failed to create %s handler\n", port.c_str());
      SST::Link * link [[maybe_unused]] {nullptr};
      if (port == "self")
        {
          link = configureSelfLink(port, handler);
        } 
      else {
        link = configureLink(port, handler);
      }
      ASSERT(link, "Failed to configure %s link", port.c_str());
      VERBOSE(4, "    %s link @%p with handler @%p\n",
              port.c_str(), (void*)link, (void*)handler);
    };

  uint32_t left  = (getId() > 0 ? getId() : m_number) - 1;
  uint32_t right = (getId() + 1) % m_number;

  linkup("portL", left);
  linkup("portR", right);
  linkup("self", getId());

  // Register statistics

  // Rng samples acquired in the event handler()

  // Tell SST to wait until we authorize it to exit
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();
}


Rng::Rng() : SST::Component(-1)
{
  VERBOSE(2, "%s", "Default c'tor()\n");
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
  VERBOSE(2, "%s", "Destructor()\n");
#define DELETE(p) \
  VERBOSE(4, "  deleting %s @0x%p\n", #p, (void*)(p));  \
  p = 0

  DELETE(m_rng);

#undef DELETE
}


void
Rng::handleEvent(SST::Event *ev, uint32_t from [[maybe_unused]])
{
  auto event = dynamic_cast<RngEvent*>(ev);
  ASSERT(event, "Failed to cast SST::Event * to PholdEvent *");
  // Extract any useful data, then clean it up
  delete event;

  // Do the iterations
  double sum = 0;
  for (decltype(m_samples) i = 0; i < m_samples; ++i)
    {
      sum += m_rng->nextUniform();
    }

  // Check the stopping condition
  primaryComponentOKToEndSim();
}


void
Rng::setup() 
{
  VERBOSE(2, "%s", "sending initial event\n");

  auto ev = new RngEvent();
  m_self->send(0, ev);
}


void
Rng::finish() 
{
  VERBOSE(2, "%s", "\n");
}

}  // namespace Phold
