/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Lawrence Livermore National Laboratory
 * All rights reserved.
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#ifndef PHOLD_PHOLD_H
#define PHOLD_PHOLD_H

#include "PholdEvent.h"

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/eli/elementinfo.h>
#include <sst/core/rng/mersenne.h>
#include <sst/core/rng/marsaglia.h>
#include <sst/core/rng/uniform.h>
#include <sst/core/rng/expon.h>

#include <vector>

/**
 * \file
 * Phold::Phold class declaration.
 */

/** Namespace for Phold benchmark. */
namespace Phold {

/**
 * Classic PDES PHOLD Benchmark.
 * See R. M. Fujimoto. Performance of time warp under synthetic workloads, January 1990.
 * In Proceedings of the SCS Multiconference on Distributed Simulation 22, 1 (January 1990), pp. 23-28.
 */
class Phold : public SST::Component
{

public:
  /**
   * Constructor
   * @param id     Component instance unique id
   * @param params Configuration parameters
   */
  Phold( SST::ComponentId_t id, SST::Params& params );
  /** D'tor */
  ~Phold() noexcept override;
  /** Copy c'tor, deleted. */
  Phold(const Phold &) = delete;
  /** Copy assignment, deleted. */
  Phold & operator= (const Phold &) = delete;
  /** Move c'tor, deleted. */
  Phold(Phold &&) = delete;
  /** Move assignment, deleted. */
  Phold & operator= (Phold &&) = delete;

  // Inherited from SST::BaseComponent
  void setup() override;
  void finish() override;

  // Macro defined in sst-core/sst/core/component.h:
  SST_ELI_REGISTER_COMPONENT
  (
   Phold,                      // !< class type (class Identifier)
   "phold",                // !<  Module name (const char *)
   "Phold",              // !< class name (const char *)
   SST_ELI_ELEMENT_VERSION( 1, 0, 0 ),  // !< version number (Major, minor, patch)
   "PHOLD benchmark component for SST",   // !< description (const char *)
   COMPONENT_CATEGORY_PROCESSOR
   );

  // Macro defined in sst-core/src/core/eli/paramsInfo.h:
  // std::vector<SST::ElementInfoParam>
  SST_ELI_DOCUMENT_PARAMS
  ( // struct SST::ElementInfoParam from sst-core/src/core/eli/elibase.h:
   { "remote",                                     // !< name (const char *)
     "Fraction of events which should be remote",  // !< description (const char *)
     "0.9"                                         // !< defaultValue (const char *)
   },
   { "minimum",
     "Minimum delay when sending events, in seconds.",
     "1"  // TODO - make this a time, such as seconds
   },
   { "average",
     "Mean delay to be added to min when sending events, in seconds.",
     "10"  // TODO - make this a time, such as seconds
   },
   { "stop",
     "Maximum simulation time, in seconds.",
     "10"
   },
   { "number",
     "Total number of LPs. Must be at least 2.",
     "2"
   },
   { "events",
     "Initial number of events per LP.",
     "1"
   },
   { "pverbose",
     "Verbose output",
     "false"
   }
  );

  // SST_ELI_DOCUMENT_STATISTICS();

  // Macro defined in sst-core/src/core/eli/portsInfo.h:
  // std::vector<SST::ElementInfoPort2>
  // A fixed list of port declarations would look like this:
  SST_ELI_DOCUMENT_PORTS
  (
   // struct SST::ElementInfoPort2 from sst-core/src/core/eli/elibase.h:
   { "port",                      //!< name (const char *)
     "Representative port",       //!< description (const char *)
     {"phold.PholdEvent", ""}    //!< validEvents (std::vector<std::string>)
    }
  );
  
private:
  /**< Default c'tor for serialization only. */
  Phold();

  /** Send a new event to a random LP. */
  void SendEvent ();
  /**
   * Incoming event handler
   * @param ev The incoming event.
   */
  void handleEvent(SST::Event *ev);

  // class static data members
  static const char * TIMEBASE;


  static double m_remote;        /**< Remote event fraction */
  static double m_minimum;       /**< Minimum event delay */
  static double m_average;       /**< Mean event delay, added to m_minimum */
  static SST::SimTime_t m_stop;  /**< Stop time */
  static long   m_number;        /**< Total number of LPs */
  static long   m_events;        /**< Initial number of events per LP */
  static bool   m_verbose;       /**< Verbose output flag */

  // class instance data members
  SST::Output m_output;              /**< Output stream for verbose output */
  std::vector<SST::Link *> m_links;  /**< The list of links for this LP. */

  /** Choice of underlying RNG: SST::RNG::MersenneRNG or SST::RNG::MarsagliaRNG */
  typedef SST::RNG::MarsagliaRNG RNG_t;

  RNG_t * m_rng;    /**< Base RNG instance */
  SST::RNG::SSTUniformDistribution * m_remRng;       /**< Uniform RNG for deciding if event should be remote */
  SST::RNG::SSTUniformDistribution * m_nodeRng;      /**< Uniform RNG for picking remote LPs */
  SST::RNG::SSTExponentialDistribution * m_delayRng; /**< Poisson RNG for picking delay times */

};

}  // namespace Phold


#endif  // PHOLD_PHOLD_H
