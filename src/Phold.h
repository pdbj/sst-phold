/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Lawrence Livermore National Laboratory
 * All rights reserved.
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#pragma once


// Work around a Clang12-MacPorts MPI configuration issue
#include <climits>
#ifndef ULLONG_MAX
# pragma message "Supplying fallback ULLONG_MAX"
# define ULLONG_MAX 0xffffffffffffffffULL 
#endif

#include "PholdEvent.h"

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/eli/elementinfo.h>
#include <sst/core/rng/mersenne.h>
#include <sst/core/rng/marsaglia.h>
#include <sst/core/rng/xorshift.h>
#include <sst/core/rng/uniform.h>
#include <sst/core/rng/expon.h>
#include <sst/core/eli/statsInfo.h>
#include <sst/core/statapi/stataccumulator.h>
#include <sst/core/statapi/stathistogram.h>

#include <vector>

/**
 * @file
 * Phold::Phold class declaration.
 */

/** Namespace for Phold benchmark. */
namespace Phold {

/**
 * Classic PDES PHOLD Benchmark.
 *
 * See R. M. Fujimoto. Performance of time warp under synthetic workloads, January 1990.
 * In Proceedings of the SCS Multiconference on Distributed Simulation 22, 1 (January 1990), pp. 23-28.
 *
 * In the literature each Phold instance is considered a "logical process" (LP).
 * Since this also serves as an SST example, we'll mostly use the SST terminology
 * and refer to the Phold LPs as "components".
 */
class Phold : public SST::Component
{

public:

  /**
   * @copydoc  component.h#SST_ELI_REGISTER_COMPONENT
   *
   * Register this component with SST.
   */
   /**
    * 
    * Long macro chain starting in `sst-core/sst/core/component.h`:
    *
    * `SST_ELI_REGISTER_COMPONENT(cls,lib,name,ELI_FORWARD_AS_ONE(version),desc)`
    *
    * Defines class static info functions:
    *
    * Return type                     |  Function name         |  Macro arg
    * ------------------------------- | ---------------------- | ----------------
    * static const std::string &      | ELI_getCompileDate();  | implicit
    * static const std::string        | ELI_getCompileFile();  | implicit
    * static constexpr unsigned       | majorVersion();        | from version arg
    * static constexpr unsigned       | minorVersion();        | from version arg
    * static constexpr unsigned       | tertiaryVersion();     | from version arg
    * static const std::vector<int> & | ELI_getVersion();      | version
    * static const char *             | ELI_getLibrary();      | lib
    * static const char *             | ELI_getName();         | cls
    * static const char *             | ELI_getDescription();  | desc
    * static uint32_t                 | ELI_getCategory();     | category tag
    *
    * (Implicit args are derived during compilation.)
    *
    * As if there was a struct (but the information isn't stored that way):
    *
    * \code{cpp}
      template<class T> struct
      {
        <T>  cls;                   // !< Component class type, used in InstantiateBuilder templates
        const char* lib;            // !< Module (library) name
        const char* name;           // !< Class name, as a string (could be different from '# cls'
        const unsigned major;       // !< Major version number
        const unsigned minor;       // !< Minor version number
        const unsigned tertiary;    // !< Version patch level
        const char* desc;           // !< Descriptive string
        const CATEGORY_T category;  // !< Component category type
        const std::string compDate; // !< Compile date
        const std::string file;     // !< File (compilation unit)
      },
      \endcode
    *
    */
  SST_ELI_REGISTER_COMPONENT
  (
   Phold,
   "phold",
   "Phold",
   SST_ELI_ELEMENT_VERSION( 1, 0, 0 ),
   "PHOLD benchmark LP component for SST",
   COMPONENT_CATEGORY_UNCATEGORIZED
   );

   /**
    * Macro defined in `sst-core/src/sst/core/eli/paramsInfo.h`:
    *
    * `static const std::vector<SST::ElementInfoParam> & ELI_getParams();`
    *
    * \code{cpp}
      struct SST::ElementInfoParam  // From sst-core/src/sst/core/eli/elibase.h:
      {
        const char * name;          // !< Name, as used in calls to Params::find().
        const char * description;   // !< Description as reported by sst-info.
        const char * defaultValue;  // !< Default value if not set explicitly.
      }
      \endcode
    *
    * Special values for `defaultValue`:
    * - NULL: no default, required (must be in the c'tor params)
    * - "":   no default, optional
    */
  SST_ELI_DOCUMENT_PARAMS
  (
   { "remote",
     "Fraction of events which should be remote",
     "0.9"
   },
   { "minimum",
     "Minimum delay when sending events, in seconds. Must be >0.",
     "1"  // TODO - make this a time, such as seconds
   },
   { "average",
     "Mean delay to be added to min when sending events, in seconds. Must be >0.",
     "9"  // TODO - make this a time, such as seconds
   },
   { "stop",
     "Maximum simulation time, in seconds. Must be >0",
     "10"
   },
   { "number",
     "Total number of LPs. Must be at least >1.",
     "2"
   },
   { "events",
     "Initial number of events per LP. Must be > 0.",
     "1"
   },
   { "delays",
     "Output delay histogram.",
     "false"
   },
   { "pverbose",
     "Verbose output",
     "false"
   }
  );

   /**
    * Macro defined in `sst-core/src/sst/core/eli/statsInfo.h`:
    *
    * `static const std::vector<SST::ElementInfoStatistic>& ELI_getStatistics();`
    *
    * \code{cpp}
      struct SST::ElementInfoStatistic  // From sst-core/src/sst/core/eli/elibase.h
      {
        const char* name;               //!< Name of the Statistic to be Enabled
        const char* description;        //!< Brief description of the Statistic
        const char* units;              //!< Units associated with this Statistic value
        const uint8_t enableLevel;      //!< Level to meet to enable statistic 0 = disabled
      }
      \endcode
    *
    */
  SST_ELI_DOCUMENT_STATISTICS
  (
   { "SendCount",
     "Count of events sent to execute before stop time.",
     "events",
     1
   },
   { "RecvCount",
     "Count of events received before stop time.",
     "events",
     1
   },
   { "Delays",
     "Histogram of sampled delay times.",
     "s",
     2
   }
   );

  /**
   * Format for dynamic ports `port_x`. The number of ports created
   * will be determined from the `number` argument.
   */
  static constexpr char PORT_NAME[]   = "port_%(number)d";

   /**
    * Macro defined in `sst-core/src/sst/core/eli/portsInfo.h`:
    *
    * `static const std::vector<SST::ElementInfoPort2>& ELI_getPorts();`
    *
    * \code{cpp}
      struct SST::ElementInfoPort2  // From sst-core/src/sst/core/eli/elibase.h:
      {
        /// Name of the port. Can contain `d` for a dynamic port, 
        /// also `%(xxx)d` for dynamic port with `xxx` being 
        ///the controlling component parameter    
        const char* name;

        /// Brief description of the port (ie, what it is used for).
        /// List of fully-qualified event types that this Port
        /// expects to send or receive
        const char* description;

        /// Valid event type names
        const std::vector<std::string> validEvents;
      };
      \endcode
   * 
   */
  SST_ELI_DOCUMENT_PORTS
  (
   { PORT_NAME,
     "Representative port",
     {"phold.PholdEvent", ""}
    }
  );


  // **** Rule of 5 ****

  /**
   * Constructor
   * @param id     Component instance unique id
   * @param params Configuration parameters
   */
  Phold( SST::ComponentId_t id, SST::Params& params );
  /** D'tor */
  ~Phold() noexcept override;

  // Rest of Rule of 5 are deleted
  /** Copy c'tor, deleted. */
  Phold(const Phold &) = delete;
  /** Copy assignment, deleted. */
  Phold & operator= (const Phold &) = delete;
  /** Move c'tor, deleted. */
  Phold(Phold &&) = delete;
  /** Move assignment, deleted. */
  Phold & operator= (Phold &&) = delete;


  // **** Inherited from SST::BaseComponent ****

  /**
   * Components can send/receive events, negotiate configuration...
   * Called repeatedly until no more events sent.
   *
   * Here we send events down a binary spanning tree over the component (LP) ids,
   * just for illustration.  See complete() for a more useful example, to
   * roll up the total number of events to LP 0.
   *
   * For example,
   *
   *     $  sst tests/phold.py -- -n 5 -vvvv | grep "\(init\|operator\)()"
   *
   * will show detailed logging of the init process with the following checks
   * for each component at each interation (@c phase):
   *
   * * If @c phase is less than the expected depth for this component, 
   *   check for "early" events."
   * * If @c phase is the expected depth:
   *   * Check for the expected event from the parent,
   *   * Check for any "other" unexpected events.
   * * If @c phase is deeper than the expected depth, 
   *   check for "late" events.
   *
   * This command:
   *
   *     $  sst tests/phold.py -- -n 5 -vvvv | grep child | sort -nk 9
   *
   * will print just the messages relating to sending to children, sorted
   * by child index, which is another way to show that each child receives
   * exactly one message.
   *
   *     0:[0:0]:Phold-0 [operator()() (Phold.cc:407)] -> [3]     sending to child 1
   *     0:[0:0]:Phold-0 [operator()() (Phold.cc:407)] -> [3]     sending to child 2
   *     0:[0:0]:Phold-1 [operator()() (Phold.cc:407)] -> [3]     sending to child 3
   *     0:[0:0]:Phold-1 [operator()() (Phold.cc:407)] -> [3]     sending to child 4
   *     0:[0:0]:Phold-2 [operator()() (Phold.cc:413)] -> [3]     skipping overflow child 5
   *     0:[0:0]:Phold-2 [operator()() (Phold.cc:413)] -> [3]     skipping overflow child 6
   *     0:[0:0]:Phold-3 [operator()() (Phold.cc:413)] -> [3]     skipping overflow child 7
   *     0:[0:0]:Phold-3 [operator()() (Phold.cc:413)] -> [3]     skipping overflow child 8
   *     0:[0:0]:Phold-4 [operator()() (Phold.cc:413)] -> [3]     skipping overflow child 9
   *     0:[0:0]:Phold-4 [operator()() (Phold.cc:413)] -> [3]     skipping overflow child 10
   *
   */
  virtual void init(unsigned int phase) override;

  /**
   *  Complete configuration, no send/rcv, single invocation.
   * 
   * This follows the `init(phase)`
   */
  virtual void setup() override;

  /**
   * Pass number of executed events back to the root LP at id 0.
   * Operates similarly to init().
   * Here we start at the leaves and pass counts up to the parents.
   */
  virtual void complete(unsigned int phase) override;

  /**
   *  Similar to setup()
   *
   * This follows the `complete(phase)`
   */
  virtual void finish() override;


private:

  /** Default c'tor for serialization only. */
  Phold();

  /** 
   * Send a new event to a random LP. 
   * @param [in] mustLive If @c true record (in m_initLive) 
   *     if the scheduled event will be executed before the stop time.
   */
  void SendEvent(bool mustLive = false);

  /**
   * Incoming event handler.
   * @param ev The incoming event.
   * @param from The sending LP id.
   */
  void handleEvent(SST::Event *ev, uint32_t from);

  /**
   * Clock handler, only enabled in debug builds.
   * @param cycle The current time when this is called.
   * @return \c true if this clock should be disabled.
   */
  bool clockTick(SST::Cycle_t cycle);

  /** Helper functions for init(), complete() */
  /** @{ */

  /**
   * Get a possible event from the link at @c id.
   * @tparam E The Phold event type to return
   * @param id The link/remote to query.
   * @returns The event pointer, NULL of no event from the @c id.
   */
  template <typename E>
  E * getEvent(SST::ComponentId_t id);

  /**
   * Check for unexpected messages during init() or complete().
   * This iterates through all links checking for messages.
   * Check for expected messages before calling this function.
   * Asserts if any messages are found.
   * @tparam E The Phold event type to check for.
   */
  template <typename E>
  void checkForEvents(const std::string msg);

  /**
   * Send an init event to a child by index.
   * This skips children greater than @c m_number, so it's ok to call this
   * on both pair members returned by BinaryTree::children().
   * @param child The child index to send to
   */
  void sendToChild(SST::ComponentId_t child);

  /**
   * Get the send and receive counts from a child.
   * @param child The child to receive from
   * @returns A pair of the send count and the receive count.
  */
  std::pair<std::size_t, std::size_t>
  getChildCounts(SST::ComponentId_t child);

  /**
   * Send a complete event to a parent by index, containing the total
   * number of events sent by me an my children.
   * @param parent The parent index.
   * @param sendCount The total number of events sent by me and my children
   * @param recvCount The total number of events received by me and my children
   */
  void sendToParent(SST::ComponentId_t parent,
                    std::size_t sendCount, std::size_t recvCount);

  /** @} */  // init(), complete() helpers

  /**
   * Generate the best SI representation of the time.
   * @param sim The time value to conver to a string.
   */
  std::string    toBestSI(SST::SimTime_t sim) const;


  /**
   *  Show the configuration. 
   *  @param thread_latency Latency between LPs on the same thread, in seconds.
   */
  void ShowConfiguration(double thread_latency) const;

  /** Show sizes of objects. */
  void ShowSizes() const;

  // **** Class static data members ****

  /** Default time base for component and associated links */
  static /* const */ SST::UnitAlgebra TIMEBASE;
  /** Conversion factor for TIMEBASE, used in toSimTime() and toSeconds() */
  static /* const */ double TIMEFACTOR;
  /** Conversion factor between python timebase and PHOLD component. */
  static /* const */ double PHOLD_PY_TIMEFACTOR;

  static double            m_remote;     /**< Remote event fraction */
  static SST::SimTime_t    m_minimum;    /**< Minimum event delay */
  static SST::UnitAlgebra  m_average;    /**< Mean event delay, added to m_minimum */
  static SST::SimTime_t    m_stop;       /**< Stop time */
  static uint64_t          m_number;     /**< Total number of LPs */
  static uint64_t          m_events;     /**< Initial number of events per LP */
  static std::size_t       m_bufferSize; /**< Event buffer size, bytes */
  static bool              m_statsOut;   /**< Output statistics */
  static bool              m_delaysOut;  /**< Include delays histogram in stats output*/
  static uint32_t          m_verbose;    /**< Verbose output flag */

  static SST::TimeConverter * m_timeConverter;

  /** Flag to record that at least one initial event is scheduled
   *  before the stop time.
   *  This is set by SendEvent(true), called by Setup()
   */
  static bool m_initLive;

  /** Number of cycles between print statements in mainTick */
  SST::Cycle_t m_clockPrintInterval;


  // **** Class instance data members ****

  /** Output stream for verbose output */
  mutable SST::Output              m_output;
#ifdef PHOLD_DEBUG
  /**
   * Verbose output prefix,
   * per instance since it includes the component name.
   */
  std::string VERBOSE_PREFIX;
#endif

  /** The list of links to other LPs */
  std::vector<SST::Link *> m_links;

  /** The clock. */
  SST::TimeConverter *     m_clockTimeConverter;

  /**
   * Choice of underlying RNG:
   * * SST::RNG::MersenneRNG
   * * SST::RNG::MarsagliaRNG
   * * SST::RNG::XORShiftRNG
   */
  typedef SST::RNG::XORShiftRNG RNG_t;

  /** Base RNG instance */
  RNG_t                                * m_rng;
  /** Uniform RNG for deciding if event should be remote */
  RNG_t                                * m_remRng;
  /** Uniform RNG for picking remote LPs */
  SST::RNG::SSTUniformDistribution     * m_nodeRng;
  /** Exponential RNG for picking delay times */
  SST::RNG::SSTExponentialDistribution * m_delayRng;

  // Class instance statistics
  /** Count of events sent. */
  SST::Statistics::AccumulatorStatistic<uint64_t> * m_sendCount;
  /** Count of events received. */
  SST::Statistics::AccumulatorStatistic<uint64_t> * m_recvCount;
  /**
   * Histogram of delay times.
   * This has to be generic, instead of explicitly
   * `SST::Statistics::HistogramStatistic<float>`,
   * because it might not be enabled, in which case it will be
   * a `SST::Statistics::NullStatistic< T >`.
   */
  Statistic<float> * m_delays;

};  // class Phold

}  // namespace Phold

