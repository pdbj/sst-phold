/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Lawrence Livermore National Laboratory
 * All rights reserved.
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */


#include "Phold.h"
#include "binary-tree.h"

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

#ifndef PHOLD_DEBUG

#  define OPT_LEVEL "optimized"
#  define ASSERT(...)
#  define DEBUG(...)
#  define VERBOSE(...)

#else
   // Optimized or debug build
#  define OPT_LEVEL "debug"

   // Simplify use of sst_assert
   // Extra conditional is to avoid early evaluation of args, so we can do
   //   ASSERT(NULL == p, ..., function(p));
   // when we expect a null pointer, and not call a function on it until necessary
#  define ASSERT(condition, args...)                             \
   if (! (condition))                                            \
   Component::sst_assert(condition, CALL_INFO_LONG, 1, args)

   // Non-asserting assert, for debugging
#  define DEBUG(condition, args...)              \
   if (! (condition)) VERBOSE (3, args)

#  define VERBOSE(l, f, args...)                            \
   m_output.verbosePrefix(VERBOSE_PREFIX.c_str(),           \
                          CALL_INFO, l, 0,                  \
                          "[%u] " f, l, ##args)

#endif

#ifndef PHOLD_FIXED
#  define RNG_MODE "rng"
#else
#  define RNG_MODE "fixed"
#endif


#define OUTPUT(...)                             \
  if (0 == getId()) m_output.output(CALL_INFO, __VA_ARGS__)

// Class static data members
constexpr char Phold::PORT_NAME[];  // constexpr initialized in Phold.h
/* const */ SST::UnitAlgebra Phold::TIMEBASE("1 us");
/* const */ double Phold::TIMEFACTOR;
//  Conversion factor from phold.py script to Phold::TIMEBASE
/* const */ double Phold::PHOLD_PY_TIMEFACTOR{1e6};


double               Phold::m_remote;
SST::SimTime_t       Phold::m_minimum;
SST::UnitAlgebra     Phold::m_average;
unsigned long        Phold::m_number;
unsigned long        Phold::m_events;
bool                 Phold::m_delaysOut;
uint32_t             Phold::m_verbose;
SST::SimTime_t       Phold::m_stop;
SST::TimeConverter * Phold::m_timeConverter;

std::string
Phold::toBestSI(SST::SimTime_t sim) const
{
  auto factor = m_timeConverter->getFactor();
  auto time = m_timeConverter->convertFromCoreTime(sim * factor);
  auto s = TIMEBASE;
  s *= time;
  return s.toStringBestSI();
}


Phold::Phold( SST::ComponentId_t id, SST::Params& params )
  : SST::Component(id)
{
  m_verbose = params.find<long>("pverbose", 0);
#ifndef PHOLD_DEBUG
  m_output.init("@t:Phold: ",
                m_verbose, 0, SST::Output::STDOUT);
#else
  m_output.init("@t:@X:Phold-" + getName() + " [@p()] -> ",
                m_verbose, 0, SST::Output::STDOUT);
  VERBOSE_PREFIX = "@t:@X:Phold-" + getName() + " [@p() (@f:@l)] -> ";
  VERBOSE(2, "Full c'tor() @%p, id: %llu, name: %s\n", this, getId(), getName().c_str());
#endif

  // Default time unit for Component and links
  m_timeConverter = registerTimeBase(TIMEBASE.toString(), true);

  m_remote  = params.find<double> ("remote",   0.9);
  m_minimum = params.find<double> ("minimum",  1.0) * PHOLD_PY_TIMEFACTOR;
  m_average = TIMEBASE;
  m_average *= params.find<double>("average", 10) * PHOLD_PY_TIMEFACTOR;
  m_stop    = params.find<double> ("stop",    10) * PHOLD_PY_TIMEFACTOR;
  m_number  = params.find<unsigned long>   ("number",   2);
  m_events  = params.find<unsigned long>   ("events",   1);
  m_delaysOut = params.find<bool> ("delays",  false);

  if (0 == getId())
    {
      TIMEFACTOR = m_timeConverter->getPeriod().getDoubleValue();
      VERBOSE(3, "  TIMEFACTOR: %f, timeConverter factor: %llu, period: %s (%f s?)\n",
              TIMEFACTOR,
              m_timeConverter->getFactor(),
              m_timeConverter->getPeriod().toStringBestSI().c_str(),
              m_timeConverter->getPeriod().getDoubleValue());

      auto minimum = TIMEBASE;
      minimum *= m_minimum;
      // duty_factor = minimum / (minimum + m_average)
      auto duty = m_average;
      duty += minimum;
      duty.invert();
      duty *= minimum;
      double duty_factor = duty.getDoubleValue();
      VERBOSE(3, "  min: %s, duty: %s, df: %f\n",
              minimum.toStringBestSI().c_str(), duty.toStringBestSI().c_str(), duty_factor);

      double ev_per_win = m_events * duty_factor;
      const double min_ev_per_win = 10;
      unsigned long min_events = min_ev_per_win / duty_factor;
      VERBOSE(3, "  m_ev: %lu, ev_win: %f, min_ev_win: %f, min_ev: %lu\n",
              m_events, ev_per_win, min_ev_per_win, min_events);

      std::stringstream ss;
      ss << "PHOLD Configuration:"
         << "\n    Remote LP fraction:                   " << m_remote
         << "\n    Minimum inter-event delay:            " << toBestSI(m_minimum)
         << "\n    Additional exponential average delay: " << m_average.toStringBestSI()
         << "\n    Stop time:                            " << toBestSI(m_stop)
         << "\n    Number of LPs:                        " << m_number
         << "\n    Number of initial events per LP:      " << m_events

         << "\n    Average events per window:            " << ev_per_win;

      if (ev_per_win < min_ev_per_win)
        {
          ss << "\n      (Too low!  Suggest setting '--events=" << min_events << "')";
        }

      ss << "\n    Output delay histogram:               " << (m_delaysOut ? "yes" : "no")
         << "\n    Sampling:                             " << RNG_MODE
         << "\n    Optimization level:                   " << OPT_LEVEL
         << "\n    Verbosity level:                      " << m_verbose;

#ifndef PHOLD_DEBUG
      if (m_verbose)
        {
          ss << " (ignored in optimized build)";
        }
#endif

      ss << std::endl;

      OUTPUT("%s\n", ss.str().c_str());

    }  // if 0 == getId

  VERBOSE(3, "Initializing RNGs\n");
  m_rng  = new Phold::RNG_t;
  // seed() doesn't check validity of arg, can't be 0
  m_rng->seed(1 + getId());
  VERBOSE(4, "  m_rng      @%p\n", m_rng);
  m_remRng  = m_rng;
  VERBOSE(4, "  m_remRng   @%p\n", m_remRng);
  m_nodeRng = new SST::RNG::SSTUniformDistribution(m_number, m_rng);
  VERBOSE(4, "  m_nodeRng  @%p\n", m_nodeRng);
  auto avgRngRate = m_average;
  avgRngRate /= TIMEFACTOR;
  avgRngRate.invert();
  m_delayRng = new SST::RNG::SSTExponentialDistribution(avgRngRate.getDoubleValue(), m_rng);
  VERBOSE(4, "  m_delayRng @%p, rate: %s (%f)\n",
          m_delayRng, avgRngRate.toString().c_str(), m_delayRng->getLambda());

  // Configure ports/links
  VERBOSE(3, "Configuring links:\n");
  m_links.resize(m_number);

  // Set up the port labels
  auto pre = std::string(PORT_NAME);
  const auto prefix(pre.erase(pre.find('%')));
  std::string port;
  for (uint32_t i = 0; i < m_number; ++i) {
    ASSERT(m_links[i] == nullptr, "Initialized link %u (%p) is not null!\n", i, m_links[i]);
    if (i != getId())
    {
      port = prefix + std::to_string(i);
      ASSERT(isPortConnected(port), "Port %s is not connected\n", port.c_str());
      // Each link needs it's own handler.  SST manages the destruction.
      auto handler = new SST::Event::Handler<Phold, uint32_t>(this, &Phold::handleEvent, i);
      ASSERT(handler, "Failed to create handler\n");
      m_links[i] = configureLink(port, handler);
      ASSERT(m_links[i], "Failed to configure link %u\n", i);
      VERBOSE(4, "    link %u: %s @%p with handler @%p\n", i, port.c_str(), m_links[i], handler);

    } else {

      auto handler = new SST::Event::Handler<Phold, uint32_t>(this, &Phold::handleEvent, i);
      ASSERT(handler, "Failed to create handler\n");
      m_links[i] = configureSelfLink("self", handler);
      ASSERT(m_links[i], "Failed to configure self link\n");
      VERBOSE(4, "    link %u: self   @%p with handler @%p\n", i, m_links[i], handler);
    }
  }

  // Register statistics
  VERBOSE(3, "Initializing statistics\n");
  // Stop stat collection at stop time
  SST::Params p;
  p.insert("stopat", std::to_string(m_stop));

  m_sendCount = dynamic_cast<decltype(m_sendCount)>(registerStatistic<uint64_t>(p, "SendCount"));
  ASSERT(m_sendCount, "Failed to register SendCount statistic");
  m_sendCount->setFlagOutputAtEndOfSim(true);
  ASSERT(m_sendCount->isEnabled(), "SendCount statistic is not enabled!\n");
  ASSERT( ! m_sendCount->isNullStatistic(), "SendCount statistic is Null!\n");
  VERBOSE(4, "  m_sendCount    @%p\n", m_sendCount);

  m_recvCount = dynamic_cast<decltype(m_recvCount)>(registerStatistic<uint64_t>(p, "RecvCount"));
  ASSERT(m_recvCount, "Failed to register RecvCount statistic");
  m_recvCount->setFlagOutputAtEndOfSim(true);
  ASSERT(m_recvCount->isEnabled(), "RecvCount statistic is not enabled!\n");
  ASSERT( ! m_recvCount->isNullStatistic(), "RecvCount statistic is Null!\n");
  VERBOSE(4, "  m_recvCount    @%p\n", m_recvCount);

  // Delay histogram might not be enabled, in which case registerStatistic
  // returns a NullStatistic
  m_delays = registerStatistic<float>(p, "Delays");
  ASSERT(m_delays, "Failed to register Delays statistic\n");
  if (m_delaysOut)
    {
      m_delays->setFlagOutputAtEndOfSim(true);
      ASSERT(m_delays->isEnabled(), "Delays statistic is not enabled!\n");
      ASSERT( ! m_delays->isNullStatistic(), "Delays statistic is Null!\n");
    }
  VERBOSE(4, "  m_delays   @%p\n", m_delays);

  // Initial events created in setup()

  // Tell SST to wait until we authorize it to exit
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();

}  // Phold(...)


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
  m_remRng = m_rng;
  m_nodeRng = new SST::RNG::SSTUniformDistribution(m_number, m_rng);
  m_delayRng = new SST::RNG::SSTExponentialDistribution(m_average.invert().getDoubleValue(), m_rng);
}


Phold::~Phold() noexcept
{
  VERBOSE(2, "Destructor()\n");
#define DELETE(p) \
  VERBOSE(4, "  deleting %s @%p\n", #p, (p));  \
  p = 0

  DELETE(m_rng);
  DELETE(m_nodeRng);
  DELETE(m_delayRng);

#undef DELETE
}


void
Phold::SendEvent()
{
  VERBOSE(2, "\n");

  // Remote or local?
  SST::ComponentId_t nextId = getId();

#ifndef PHOLD_FIXED
  auto rem = m_remRng->nextUniform();
#else
  auto rem = 0.0;
#endif

  // Whether the event is local or remote
  bool local = false;
  if (rem < m_remote)
  {
    auto reps = 0;
    do
      {
#ifndef PHOLD_FIXED
        nextId = static_cast<SST::ComponentId_t>(m_nodeRng->getNextDouble());
#else
        nextId = (nextId + 1) % m_number;
#endif
        ++reps;
      } while (nextId == getId());

      VERBOSE(3, "  next rng: %f, remote (%u tries) %llu\n", rem, reps, nextId);
  }
  else
  {
    local = true;
    VERBOSE(3, "  next rng: %f, self             %llu\n", rem, nextId);
  }
  ASSERT(nextId < m_number && nextId >= 0, "invalid nextId: %llu\n", nextId);

  // When?
  auto now = getCurrentSimTime();
#ifndef PHOLD_FIXED
  auto delay = static_cast<SST::SimTime_t>(m_delayRng->getNextDouble());
#else
  auto delay = static_cast<SST::SimTime_t>(m_average.getDoubleValue() / TIMEFACTOR);
#endif
  auto delayTotal = delay + m_minimum;
  auto nextEventTime = delayTotal + now;

  // Clean up delay
  if ( ! local)
    {
      // For remotes m_minimum is added by the link
      VERBOSE(3, "  delay: %llu, total: %llu => %llu\n",
              delay,
              delayTotal,
              nextEventTime);

    } else {
      VERBOSE(3, "  delay: %llu + %llu = %llu => %llu\n",
              delay,
              m_minimum,
              delayTotal,
              nextEventTime);
      // Self links don't have a min latency configured, 
      // so use the total delay in the send
      delay = delayTotal;
    }


  // Send a new event.  This is deleted in handleEvent
  auto ev = new PholdEvent(getCurrentSimTime());
  m_links[nextId]->send(delay, ev);
  VERBOSE(2, "from %llu @ %llu, delay: %llu -> %llu @ %llu\n",
         getId(), now, delay, nextId, nextEventTime);
  m_delays->addData(delay);

  // Record only sends which will be *received* before stop time.
  if (nextEventTime < m_stop)
    {
      m_sendCount->addData(1);
    }

}  // SendEvent()


void
Phold::handleEvent(SST::Event *ev, uint32_t from)
{
  auto event = dynamic_cast<PholdEvent*>(ev);
  ASSERT(event, "Failed to cast SST::Event * to PholdEvent *");
  // Extract any useful data, then clean it up
  auto sendTime = event->getSendTime();
  delete event;
  // Record the receive.  Configured (in .py) not to record after m_stop,
  // so no additional logic needed here.
  m_recvCount->addData(1);

  // Check the stopping condition
  auto now = getCurrentSimTime();
  if (now < m_stop)
  {
    VERBOSE(2, "now: %llu, from %u @ %llu\n", now, from, sendTime);
    SendEvent();
  } else
  {
    VERBOSE(2, "now: %llu, from %u @ %llu, stopping\n", now, from, sendTime);
    primaryComponentOKToEndSim();
  }
}

template <typename E>
E *
Phold::getEvent(SST::ComponentId_t id)
{
  return dynamic_cast<E*>(m_links[id]->recvUntimedData());
}

InitEvent *
Phold::getInitEvent(SST::ComponentId_t id)
{
  return getEvent<InitEvent>(id);
}


template <typename E>
void
Phold::checkForEvents(const std::string msg)
{
  for (SST::ComponentId_t i = 0; i < m_number; ++i)
    {
      auto event = getEvent<E>(i);
      ASSERT(NULL == event, "    got %s event from %llu\n", msg, i);
    }
}


void
Phold::sendToChild(SST::ComponentId_t child)
{
  if (child < m_number)
    {
      VERBOSE(3, "    sending to child %llu\n", child);
      auto event = new InitEvent(getId());
      m_links[child]->sendUntimedData(event);
    }
  else
    {
      VERBOSE(3, "    skipping overflow child %llu\n", child);
    }
};


void
Phold::init(unsigned int phase)
{
  // Use binary tree indexing to form a tree of Phold components
  using bt = BinaryTree;

  // phase is the level in the tree we're working now,
  // which includes all components with getId() < bt::size(phase)
  if (0 == phase) OUTPUT("First init phase\n");
  if (bt::depth(m_number - 1)  == phase) OUTPUT("Last init phase\n");


  VERBOSE(2, "depth: %llu, phase: %u, begin: %llu, end: %llu\n",
          bt::depth(getId()), phase, bt::begin(phase), bt::end(phase));

  // First check for early init event
  if (phase < bt::depth(getId()))
    {
      VERBOSE(3, "  checking for early events\n");
      checkForEvents<InitEvent>("EARLY");
    }

  else if (phase == bt::depth(getId()))

    {
      VERBOSE(3, "  our phase\n");
      // Get the expected event from parent
      // Root id 0 does not have a parent, so skip it
      if (0 != getId())
        {
          auto parent = bt::parent(getId());
          VERBOSE(3, "    checking for expected event from parent %llu\n", parent);
          auto event = getInitEvent(parent);

          ASSERT(event, "    failed to recv expected event from parent %llu\n", parent);
          auto src = event->getSenderId();
          VERBOSE(3, "    received from %llu\n", src);
          ASSERT(parent == src, "    event from %llu, expected parent %llu\n", src, parent);
        }  // 0 != getId(),  non-root receive from parent
      else
        {
          // 0 == getId()
          VERBOSE(3, "    initiating tree: child %llu\n", getId());
        }

      // Send to two children
      auto children = bt::children(getId());
      sendToChild(children.first);
      sendToChild(children.second);

      // Check for any other events
      VERBOSE(3, "  checking for other events\n");
      checkForEvents<InitEvent>("OTHER");
    }

  else

    {
      VERBOSE(3, "  checking for late events\n");
      ASSERT(phase > bt::depth(getId()), "  expected to be late in this phase, but not\n");
      // Check for late events
      checkForEvents<InitEvent>("LATE");
    }

}  // init()


void
Phold::setup()
{
  VERBOSE(2, "initial events: %lu\n", m_events);

  // Generate initial event set
  for (auto i = 0; i < m_events; ++i)
    {
      SendEvent();
    }
  OUTPUT("Setup complete\n");
}


CompleteEvent *
Phold::getCompleteEvent(SST::ComponentId_t id)
{
  return getEvent<CompleteEvent>(id);
}


std::pair<std::size_t, std::size_t>
Phold::getChildCounts(SST::ComponentId_t child)
{
  std::pair<std::size_t, std::size_t> counts{0, 0};

  if (child < m_number)
    {
      VERBOSE(3, "    getting expected event from child %llu\n", child);
      auto event = getCompleteEvent(child);
      ASSERT(event, "   failed to receive expected event from child %llu\n", child);
      counts.first  = event->getSendCount();
      counts.second = event->getRecvCount();
      VERBOSE(4, "      child %llu reports %llu sends, %llu recvs\n",
              child,counts.first, counts.second);
    }
  else {
    VERBOSE(3, "    skipping overflow child %llu\n", child);
  }
  return counts;

}  // getChildCounts


void
Phold::sendToParent(SST::ComponentId_t parent,
                    std::size_t sendCount, std::size_t recvCount)
{
  VERBOSE(3, "    sending to parent %llu with sends: %llu, recvs: %llu\n",
          parent, sendCount, recvCount);
  auto event = new CompleteEvent(sendCount, recvCount);
  m_links[parent]->sendUntimedData(event);
}


void
Phold::complete(unsigned int phase)
{
  using bt = BinaryTree;

  // Similar pattern to init(), but starting from the leaves
  if (0 == phase) OUTPUT("First complete phase\n");

  // depth containing the last Component
  std::size_t maxDepth = bt::depth(m_number - 1);
  // effective phase, starting up from leaves, to parallel init()
  std::size_t ephase = maxDepth - phase;

  VERBOSE(2, "complete phase: %lu, max depth %llu, ephase: %llu\n",
          phase, maxDepth, ephase);

  // First check for early events
  if (ephase > bt::depth(getId()))
    {
      VERBOSE(3, "  checking for early events\n");
      checkForEvents<CompleteEvent>("EARLY");
    }

  else if (ephase == bt::depth(getId()))

    {
      VERBOSE(3, "  our phase\n");
      // Get the send counts from children
      auto children = bt::children(getId());
      auto left = getChildCounts(children.first);
      auto right = getChildCounts(children.second);

      // Accumulate the send counts
      auto sendCount = m_sendCount->getCount() + left.first + right.first;
      auto recvCount = m_recvCount->getCount() + left.second + right.second;

      VERBOSE(3, "    accumulating sends: me: %llu, left: %llu, right: %llu, total: %llu\n",
              m_sendCount->getCount(), left.first, right.first, sendCount);
      VERBOSE(3, "    accumulating recvs: me: %llu, left: %llu, right: %llu, total: %llu\n",
              m_recvCount->getCount(), left.second, right.second, recvCount);

      // Send the totals to our parent, unless we're at the root
      if (0 < getId())
      {
        sendToParent(bt::parent(getId()), sendCount, recvCount);
      }
      // Log the total, from root
      OUTPUT("Last complete phase\n");
      OUTPUT("Grand total sends: %llu, receives: %llu, error: %llu\n",
             sendCount, recvCount, sendCount - recvCount);

      // Finally, check for any other events
      VERBOSE(3, "  checking for other events\n");
      checkForEvents<CompleteEvent>("OTHER");
    }

  else

    {
      VERBOSE(3, "  checking for late events\n");
      ASSERT(ephase < bt::depth(getId()), "  expected to be late in this phase, but not\n");
      // Check for late eents
      checkForEvents<CompleteEvent>("LATE");
    }

}  // complete()


void
Phold::finish()
{
  VERBOSE(2, "\n");
  OUTPUT("Finish complete\n");
}

}  // namespace Phold
