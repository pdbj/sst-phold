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

#include <cinttypes>  // PRIxxx
#include <cstdint>    // UINT32_MAX
#include <iostream>
#include <string>     // to_string()
#include <utility>    // swap()

/**
 * \file
 * Phold::Phold class implementation.
 */

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
#  define DEBUG(condition, args...)             \
   if (! (condition)) VERBOSE (3, args)

#  define VERBOSE(l, f, args...)                        \
  do {                                                  \
    m_output.verbosePrefix(VERBOSE_PREFIX.c_str(),      \
                           CALL_INFO, l, 0,             \
                           "[%u] " f, l, ##args);       \
    m_output.flush();                                   \
  } while (0)

#endif

#define OUTPUT(...)                             \
  do {                                          \
    m_output.output(CALL_INFO, __VA_ARGS__);    \
    m_output.flush();                           \
  } while (0)

#define OUTPUT0(...)                            \
  if (0 == getId()) m_output.output(CALL_INFO, __VA_ARGS__)


namespace Phold {

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
bool                 Phold::m_initLive {false};

std::string
Phold::toBestSI(SST::SimTime_t sim) const
{
  auto s = TIMEBASE * sim;
  return s.toStringBestSI();
}


Phold::Phold( SST::ComponentId_t id, SST::Params& params )
  : SST::Component(id)
{
  m_verbose = params.find<long>("pverbose", 0);
#ifndef PHOLD_DEBUG
  // Prefix with virtual time
  m_output.init("@t:Phold: ",
                m_verbose, 0, SST::Output::STDOUT);
#else
  // Prefix with "<time>:[<rank>:<thread>]Phold-<id> [<function>] -> "
  m_output.init("@t:@X:Phold-" + getName() + " [@p()] -> ",
                m_verbose, 0, SST::Output::STDOUT);
  // Prefix with "<time>:[<rank>:<thread>]Phold-<id> [<function> (<file>:<lineL)] -> "
  VERBOSE_PREFIX = "@t:@X:Phold-" + getName() + " [@p() (@f:@l)] -> ";
  VERBOSE(2, "Full c'tor() @%p, id: %" PRIu64 ", name: %s\n",
          this, getId(), getName().c_str());
#endif

  m_remote    = params.find<double>("remote",   0.9);
  m_minimum   = params.find<double>("minimum",  1.0) * PHOLD_PY_TIMEFACTOR;
  m_average   = TIMEBASE;
  m_average  *= params.find<double>("average", 9.0) * PHOLD_PY_TIMEFACTOR;
  m_stop      = params.find<double>("stop",    10) * PHOLD_PY_TIMEFACTOR;
  m_number    = params.find<unsigned long>("number",   2);
  m_events    = params.find<unsigned long>("events",   1);
  m_delaysOut = params.find<bool>("delays",  false);

  m_initLive = false;

#ifdef PHOLD_DEBUG
  // Register a clock
  auto clock = new SST::Clock::Handler<Phold>(this, &Phold::clockTick);
  ASSERT(clock, "Failed to create clock handler\n");
  auto clockRate = m_average;
  clockRate += TIMEBASE * (m_minimum + getId());
  VERBOSE(2, "  clock period %s\n", clockRate.toStringBestSI().c_str());
  clockRate.invert();
  m_clockTimeConverter = registerClock(clockRate, clock);
  ASSERT(m_clockTimeConverter, "Failed to register clock\n");
  auto cycles = clockRate;
  cycles *= (TIMEBASE * m_stop);

  m_clockPrintInterval = std::max(1.0, cycles.getDoubleValue() / 10);
  VERBOSE(2, "Configured clock on Phold %" PRIu64 " with rate %s\n",
          getId(), clockRate.toStringBestSI().c_str());
  VERBOSE(2,"  expect %s cycles, print interval %" PRIu64 "\n",
          cycles.toStringBestSI().c_str(), m_clockPrintInterval);

  // Register with the free clock
  auto freeTimePeriod = m_clockTimeConverter->getPeriod();
  freeTimePeriod *= 2;
  freeTimePeriod.invert();
  FreeClock::Register(freeTimePeriod, this, &Phold::freeClockTick);

#endif

  // Default time unit for Component and links
  m_timeConverter = registerTimeBase(TIMEBASE.toString(), true);
  TIMEFACTOR = m_timeConverter->getPeriod().getDoubleValue();

  if (0 == getId())
    {
      ShowConfiguration();
      ShowSizes();
    }

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
    ASSERT(m_links[i] == nullptr,
           "Initialized link %u (%p) is not null!\n", i, m_links[i]);
    // Each link needs it's own handler.  SST manages the destruction in ~Link
    auto handler = new SST::Event::Handler<Phold, uint32_t>(this, &Phold::handleEvent, i);
    ASSERT(handler, "Failed to create event handler %u\n", i);
    if (i != getId())
    {
      port = prefix + std::to_string(i);
      ASSERT(isPortConnected(port),
             "Port %s is not connected\n", port.c_str());
      m_links[i] = configureLink(port, handler);
      ASSERT(m_links[i], "Failed to create link\n");
      VERBOSE(4, "    link %u: %s @%p with handler @%p\n",
              i, port.c_str(), m_links[i], handler);

    } else {

      m_links[i] = configureSelfLink("self", handler);
      VERBOSE(4, "    link %u: self   @%p with handler @%p\n",
              i, m_links[i], handler);
    }
    ASSERT(m_links[i], "Failed to configure link %u\n", i);
  }

  // Register statistics
  VERBOSE(3, "Initializing statistics\n");
  // Stop stat collection at stop time
  SST::Params p;
  std::string stopat {toBestSI(m_stop)};
  VERBOSE(3, "  Setting stopat to %s\n", stopat.c_str());
  p.insert("stopat", stopat);

  m_sendCount = dynamic_cast<decltype(m_sendCount)>(registerStatistic<uint64_t>(p, "SendCount"));
  ASSERT(m_sendCount,
         "Failed to register SendCount statistic");
  m_sendCount->setFlagOutputAtEndOfSim(false);
  ASSERT(m_sendCount->isEnabled(),
         "SendCount statistic is not enabled!\n");
  ASSERT( ! m_sendCount->isNullStatistic(),
          "SendCount statistic is Null!\n");
  VERBOSE(4, "  m_sendCount    @%p\n", m_sendCount);

  m_recvCount = dynamic_cast<decltype(m_recvCount)>(registerStatistic<uint64_t>(p, "RecvCount"));
  ASSERT(m_recvCount,
         "Failed to register RecvCount statistic");
  m_recvCount->setFlagOutputAtEndOfSim(false);
  ASSERT(m_recvCount->isEnabled(),
         "RecvCount statistic is not enabled!\n");
  ASSERT( ! m_recvCount->isNullStatistic(),
          "RecvCount statistic is Null!\n");
  VERBOSE(4, "  m_recvCount    @%p\n", m_recvCount);

  // Delay histogram might not be enabled, in which case registerStatistic
  // returns a NullStatistic
  m_delays = registerStatistic<float>(p, "Delays");
  ASSERT(m_delays,
         "Failed to register Delays statistic\n");
  if (m_delaysOut)
    {
      m_delays->setFlagOutputAtEndOfSim(true);
      ASSERT(m_delays->isEnabled(),
             "Delays statistic is not enabled!\n");
      ASSERT( ! m_delays->isNullStatistic(),
              "Delays statistic is Null!\n");
      ASSERT(dynamic_cast<SST::Statistics::HistogramStatistic<float> *>(m_delays),
             "m_delays is not a Histogram!\n");
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

}  // Phold()


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

}  // ~Phold()


void
Phold::ShowConfiguration() const
{
  VERBOSE(2, "\n");

  // Show TIMEFACTOR and TimeConverter values only from LP 0
  VERBOSE(3, "  TIMEFACTOR: %f, timeConverter factor: %" PRIu64 ", period: %s (%f s?)\n",
          TIMEFACTOR,
          m_timeConverter->getFactor(),
          m_timeConverter->getPeriod().toStringBestSI().c_str(),
          m_timeConverter->getPeriod().getDoubleValue());

  auto minimum = TIMEBASE * m_minimum;
  // duty_factor = m_average / (minimum + m_average)
  auto duty = m_average;
  duty += TIMEBASE * m_minimum;
  auto period = duty;  // minimum + m_average
  duty.invert();
  duty *= m_average;
  double duty_factor = duty.getDoubleValue();
  VERBOSE(3, "  min: %s, duty: %s, df: %f\n",
          minimum.toStringBestSI().c_str(), 
          duty.toStringBestSI().c_str(), 
          duty_factor);

  double ev_per_win = m_events * duty_factor;
  const double min_ev_per_win = 10;
  unsigned long min_events = min_ev_per_win / duty_factor;
  VERBOSE(3, "  m_ev: %lu, ev_win: %f, min_ev_win: %f, min_ev: %lu\n",
          m_events, ev_per_win, min_ev_per_win, min_events);

  // Convert period to rate, then expected total number of events
  auto tEvents = (TIMEBASE * m_number * m_events * m_stop) / period;
  double totalEvents = tEvents.getDoubleValue();

  std::stringstream ss;
  ss << "PHOLD Configuration:"

#ifndef PHOLD_FIXED
     << "\n    Remote LP fraction:                   " << m_remote
#else
     << "\n    Remote LP fraction:                   " << "1 (fixed)"
#endif

     << "\n    Minimum inter-event delay:            " << toBestSI(m_minimum)

#ifndef PHOLD_FIXED
     << "\n    Additional exponential average delay: " << m_average.toStringBestSI()
#else
     << "\n    Additional fixed delay:               " << m_average.toStringBestSI()
#endif
     << "\n    Average period:                       " << period.toStringBestSI()
     << "\n    Stop time:                            " << toBestSI(m_stop)
     << "\n    Number of LPs:                        " << m_number
     << "\n    Number of initial events per LP:      " << m_events

     << "\n    Average events per window:            " << ev_per_win;

  if (ev_per_win < min_ev_per_win)
    {
      ss << "\n      (Too low!  Suggest setting '--events=" << min_events << "')";
    }
  ss << "\n    Expected total number of events:      " << totalEvents;

#ifdef PHOLD_DEBUG
  ss << "\n    Clock print interval:                 " << m_clockPrintInterval
     << " cycles";
#endif

  ss << "\n    Output delay histogram:               " << (m_delaysOut ? "yes" : "no")

#ifndef PHOLD_FIXED
     << "\n    Sampling:                             " << "rng"
#else
     << "\n    Sampling:                             " << "fixed"
#endif

     << "\n    Optimization level:                   " << OPT_LEVEL
     << "\n    Verbosity level:                      " << m_verbose;

#ifndef PHOLD_DEBUG
  if (m_verbose)
    {
      ss << " (ignored in optimized build)";
    }
#endif


  ss << std::endl;
  OUTPUT0("%s\n", ss.str().c_str());

  // SST config
  auto myRank = getRank();
  auto ranks = getNumRanks();

  std::string runMode;
  switch (getSimulation()->getSimulationMode())
    {
    case SST::Simulation::INIT:     runMode = "INIT";        break;
    case SST::Simulation::RUN:      runMode = "RUN";         break;
    case SST::Simulation::BOTH:     runMode = "BOTH";        break;
    case SST::Simulation::UNKNOWN:  runMode = "UNKNOWN";     break;
    default:                        runMode = "undefined)";
    };

  ss.str("");
  ss.clear();

  ss << "SST Configuration:"
     << "\n    Rank, thread:                         " << myRank.rank << ", " << myRank.thread
     << "\n    Total ranks, threads:                 " << ranks.rank << ", " << ranks.thread
     << "\n    Run mode:                             " << runMode
     << std::endl;
  OUTPUT0("%s\n", ss.str().c_str());

}  // ShowConfiguration()


void
Phold::ShowSizes() const
{
  if (!m_verbose) return;

  VERBOSE(2, "\n");

# define TABLE(label, value)                    \
  ss << "\n    "  << std::left << std::setw(50) \
     << (std::string( label ) + ":")            \
     << std::right << std::setw(3) << value

# define SIZEOF(object, member)    \
  TABLE( #object, sizeof(object) ) \
    << " (" << member << ")";      \
  pholdTotal += sizeof(object)

  std::stringstream ss;
  std::size_t pholdTotal{0};

  ss << "Sizes of objects:";
  SIZEOF(Phold, "class instance");                 pholdTotal = 0;
  ss << "\n\n    Plus heap allocated:";
  SIZEOF(SST::RNG::MersenneRNG, "m_rng");
  SIZEOF(SST::RNG::MarsagliaRNG, "m_remRng");
  SIZEOF(SST::RNG::SSTUniformDistribution, "m_nodeRng");
  SIZEOF(SST::RNG::SSTExponentialDistribution, "m_delayRNg");
  SIZEOF(SST::Statistics::AccumulatorStatistic<uint64_t>, "m_sendCount");
  SIZEOF(SST::Statistics::AccumulatorStatistic<uint64_t>, "m_recvCount");
  SIZEOF(SST::Statistics::HistogramStatistic<uint64_t>, "m_delays");
  ss << "\n      (Bins are stored in a map, so additional 3 * "
     << sizeof(uint64_t) << " bytes per bin.)";
  TABLE("Subtotal heap allocated: ", pholdTotal);
  SIZEOF(SST::Link, "N * (N - 1) links total");
  ss << "\n\n    Other components:";

  SIZEOF(SST::UnitAlgebra, "statics TIMEBASE, m_average");
  SIZEOF(SST::TimeConverter, "static m_timeConverter");
  SIZEOF(SST::Output, "m_output, included in Phold");
  SIZEOF(SST::Core::ThreadSafe::Barrier, "many instances in Simulator_impl");
  SIZEOF(std::atomic<bool>, "used by Barrier");
  SIZEOF(std::atomic<std::size_t>, "used by Barrier");
#ifdef PHOLD_DEBUG
  SIZEOF(std::string, "VERBOSE_PREFIX, included in Phold");
#endif

  ss << std::endl;

#undef SIZEOF
#undef TABLE

  OUTPUT0("%s\n", ss.str().c_str());

}  // ShowSizes()


void
Phold::SendEvent(bool mustLive /* = false */)
{
  VERBOSE(3, "\n");

  // Remote or local?
  SST::ComponentId_t nextId = getId();

#ifndef PHOLD_FIXED
  const auto rem = m_remRng->nextUniform();
#else
  const auto rem = 1.0;
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

      VERBOSE(3, "  next rng: %f, remote (%u tries) %" PRIu64 "\n",
              rem, reps, nextId);
  }
  else
  {
    local = true;
    VERBOSE(3, "  next rng: %f, self             %" PRIu64 "\n", rem, nextId);
  }
  ASSERT(nextId < m_number && nextId >= 0, "invalid nextId: %" PRIu64 "\n", nextId);

  // When?
  auto now = getCurrentSimTime();
#ifndef PHOLD_FIXED
  auto delay = static_cast<SST::SimTime_t>(m_delayRng->getNextDouble());
#else
  static const
  auto delayAvg = static_cast<SST::SimTime_t>(m_average.getDoubleValue() / TIMEFACTOR);
  auto delay = delayAvg;
#endif
  auto delayTotal = delay + m_minimum;
  auto nextEventTime = delayTotal + now;

  // Clean up delay
  if ( ! local)
    {
      // For remotes m_minimum is added by the link
      VERBOSE(3, "  delay: %" PRIu64 ", total: %" PRIu64 " => %" PRIu64 "\n",
              delay,
              delayTotal,
              nextEventTime);

    } else {
      VERBOSE(3, "  delay: %" PRIu64 " + %" PRIu64 " = %" PRIu64 " => %" PRIu64 "\n",
              delay,
              m_minimum,
              delayTotal,
              nextEventTime);
      // Self links don't have a min latency configured,
      // so use the total delay in the send
      delay = delayTotal;
    }


  // Send a new event.  This is deleted at the reciever in handleEvent()
  auto event = new PholdEvent(getCurrentSimTime());
  m_links[nextId]->send(delay, event);
  VERBOSE(2, "from %" PRIu64 " @ %" PRIu64 ", delay: %" PRIu64 " -> %" PRIu64 " @ %" PRIu64 "%s, @%p\n",
          getId(), now, delay, nextId, nextEventTime,
          (nextEventTime < m_stop ? "" : " (too late)"),
          event
          );
  // Record only sends which will be *received* before stop time.
  if (nextEventTime < m_stop)
    {
      m_sendCount->addData(1);
      VERBOSE(3, "  histogramming %f\n", delayTotal * TIMEFACTOR);
      m_delays->addData(delayTotal * TIMEFACTOR);

#ifdef PHOLD_DEBUG
      if (mustLive && !m_initLive)
        {
          VERBOSE(3, "  recording live event\n");
          m_initLive = true;
        }
#endif
    }

  VERBOSE(3, "  done\n");

}  // SendEvent()


void
Phold::handleEvent(SST::Event *ev, uint32_t from)
{
  auto event = dynamic_cast<PholdEvent*>(ev);
  ASSERT(event, "Failed to cast SST::Event * to PholdEvent *");
  // Extract any useful data, then clean it up
  auto sendTime = event->getSendTime();
  VERBOSE(3, "  deleting event @%p\n", event);
  delete event;

  auto now = getCurrentSimTime();

  // Record the receive.  Configured (in .py) not to record after m_stop,
  // but doesn't seem to work
  if (now < m_stop)
    {
      m_recvCount->addData(1);
    }

  // Check the stopping condition
  if (now < m_stop)
  {
    VERBOSE(2, "now: %" PRIu64 ", from %" PRIu32 " @ %" PRIu64 "\n",
            now, from, sendTime);
    SendEvent();
  } else
  {
    VERBOSE(2, "now: %" PRIu64 ", from %u @ %" PRIu64
            ", stopping due to late event\n",
            now, from, sendTime);
    primaryComponentOKToEndSim();
  }
  VERBOSE(3, "  done\n");
}  // handleEvent()


bool
Phold::clockTick(SST::Cycle_t cycle)
{
  auto nextCore  = m_clockTimeConverter->convertToCoreTime(cycle + 1);
  auto next = m_timeConverter->convertFromCoreTime(nextCore);

  // Print periodically
  if (cycle % m_clockPrintInterval == 0)
    {
      auto nextCycle = getNextClockCycle(m_clockTimeConverter);
      OUTPUT0("Clock tick %" PRIu64 ", next: %" PRIu64 "%s\n",
             cycle, nextCycle,
             (next <= m_stop ? "" : " stopping clock"));
    }

  // To signal stop from a clock return true
  // To continue return false

  return next > m_stop;

}  // clockTick()


template <typename E>
E *
Phold::getEvent(SST::ComponentId_t id)
{
  VERBOSE(3, "    getting event from link %" PRIu64 "\n", id);
  auto event = m_links[id]->recvUntimedData();
  VERBOSE(3, "    got %p\n", (void *)(event));
  return dynamic_cast<E*>(event);

}  // getEvent()


template <typename E>
void
Phold::checkForEvents(const std::string msg)
{
  for (SST::ComponentId_t id = 0; id < m_number; ++id)
    {
      VERBOSE(3, "  checking link %" PRIu64 "\n", id);
      auto event = getEvent<E>(id);
      ASSERT(NULL == event,
             "    got %s event from %" PRIu64 "\n", msg.c_str(), id);
      // This won't run because of the assert above
      if (event)
        {
          VERBOSE(3, "    deleting event @%p\n", event);
          delete event;
        }
    }

}  // checkForEvents()


void
Phold::sendToChild(SST::ComponentId_t child)
{
  if (child < m_number)
    {
      // This is deleted in init()
      auto event = new InitEvent(getId());
      VERBOSE(3, "    sending to child %" PRIu64 ", @%p\n", child, (void*)(event));
      m_links[child]->sendUntimedData(event);
    }
  else
    {
      VERBOSE(3, "    skipping overflow child %" PRIu64 "\n", child);
    }
}  // sendToChild()


void
Phold::init(unsigned int phase)
{
  // Use binary tree indexing to form a tree of Phold components
  using bt = BinaryTree;

  // phase is the level in the tree we're working now,
  // which includes all components with getId() < bt::capacity(phase)
  if (0 == phase) OUTPUT0("First init phase\n");
  if (bt::depth(m_number - 1)  == phase) OUTPUT0("Last init phase\n");


  VERBOSE(2, "depth: %zu, phase: %u, begin: %zu, end: %zu\n",
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
          VERBOSE(3, "    checking for expected event from parent %zu\n", parent);
          auto event = getEvent<InitEvent>(parent);

          ASSERT(event,
                 "    failed to recv expected event from parent %zu\n", parent);
          auto src = event->getSenderId();
          VERBOSE(3, "    received from %" PRIu64 ", @%p\n", src, event);
          ASSERT(parent == src,
                 "    event from %" PRIu64 ", expected parent %zu\n", src, parent);

          VERBOSE(3, "  deleting event @%p\n", event);
          delete event;
        }  // 0 != getId(),  non-root receive from parent
      else
        {
          // 0 == getId()
          VERBOSE(3, "    initiating tree: child %" PRIu64 "\n", getId());
        }

      // Send to two children
      auto children = bt::children(getId());
      VERBOSE(3, "    sending to my children %zu and %zu\n",
              children.first, children.second);
      sendToChild(children.first);
      sendToChild(children.second);

      // Check for any other events
      VERBOSE(3, "  checking for other events\n");
      checkForEvents<InitEvent>("OTHER");
    }

  else

    {
      VERBOSE(3, "  checking for late events\n");
      ASSERT(phase > bt::depth(getId()),
             "  expected to be late in this phase, but not\n");
      // Check for late events
      checkForEvents<InitEvent>("LATE");
    }

}  // init()


void
Phold::setup()
{
  VERBOSE(2, "initial events: %lu\n", m_events);

  // Generate initial event set
  for (auto i = 0ul; i < m_events; ++i)
    {
#ifdef PHOLD_DEBUG
      SendEvent(true);  // record if any events are scheduled before stop
#else
      SendEvent();
#endif
    }

#ifdef PHOLD_DEBUG
  // Make sure at least one event will actually run
  std::size_t extras{0};
  while (!m_initLive)
    {
      ++extras;
      SendEvent(true);
    }
  if (extras)
    {
      VERBOSE(3, "    used %zu extra SendEvent calls to ensure at least one live event\n", extras);
    }
#endif

  OUTPUT0("Setup complete\n");

}  // setup()


std::pair<std::size_t, std::size_t>
Phold::getChildCounts(SST::ComponentId_t child)
{
  std::pair<std::size_t, std::size_t> counts{0, 0};

  if (child < m_number)
    {
      VERBOSE(3, "    getting expected event from child %" PRIu64 "\n", child);
      auto event = getEvent<CompleteEvent>(child);
      ASSERT(event,
             "   failed to receive expected event from child %" PRIu64 "\n", child);
      counts.first  = event->getSendCount();
      counts.second = event->getRecvCount();
      VERBOSE(4, "      child %" PRIu64 " reports %zu sends, %zu recvs, @%p\n",
              child, counts.first, counts.second, event);
      VERBOSE(3, "  deleting event @%p\n", event);
      delete event;
    }
  else {
    VERBOSE(3, "    skipping overflow child %" PRIu64 "\n", child);
  }
  return counts;

}  // getChildCounts


void
Phold::sendToParent(SST::ComponentId_t parent,
                    std::size_t sendCount, std::size_t recvCount)
{
  // This is deleted in getChildCounts()
  auto event = new CompleteEvent(sendCount, recvCount);
  VERBOSE(3, "    sending to parent %" PRIu64 " with sends: %zu, recvs: %zu, @%p\n",
          parent, sendCount, recvCount, event);
  m_links[parent]->sendUntimedData(event);

}  // sendToParents()


void
Phold::complete(unsigned int phase)
{
  using bt = BinaryTree;

  // Similar pattern to init(), but starting from the leaves
  if (0 == phase) OUTPUT0("First complete phase\n");

  // depth containing the last Component
  std::size_t maxDepth = bt::depth(m_number - 1);
  // effective phase, starting up from leaves, to parallel init()
  std::size_t ephase = maxDepth - phase;

  VERBOSE(2, "complete phase: %u, max depth %zu, ephase: %zu\n",
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

      VERBOSE(3, "    accumulating sends: me: %" PRIu64 ", left: %zu, right: %zu, total: %zu\n",
              m_sendCount->getCount(), left.first, right.first, sendCount);
      VERBOSE(3, "    accumulating recvs: me: %" PRIu64 ", left: %zu, right: %zu, total: %" PRIu64 "\n",
              m_recvCount->getCount(), left.second, right.second, recvCount);

      // Send the totals to our parent, unless we're at the root
      if (0 < getId())
      {
        sendToParent(bt::parent(getId()), sendCount, recvCount);
      }
      // Log the total, from root
      OUTPUT0("Last complete phase\n");
      OUTPUT0("Grand total sends: %" PRIu64 ", receives: %" PRIu64 ", error: %lld\n",
             sendCount, recvCount, (long long)sendCount - recvCount);

      // Finally, check for any other events
      VERBOSE(3, "  checking for other events\n");
      checkForEvents<CompleteEvent>("OTHER");
    }

  else

    {
      VERBOSE(3, "  checking for late events\n");
      ASSERT(ephase < bt::depth(getId()),
             "  expected to be late in this phase, but not\n");
      // Check for late eents
      checkForEvents<CompleteEvent>("LATE");
    }

}  // complete()


void
Phold::finish()
{
  VERBOSE(2, "\n");
  OUTPUT0("Finish complete\n");
}


}  // namespace Phold
