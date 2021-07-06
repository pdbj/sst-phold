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
#include <sst/core/rng/uniform.h>
#include <sst/core/rng/expon.h>

namespace Phold {

class Phold : public SST::Component
{

public:
  Phold( SST::ComponentId_t id, SST::Params& params );
  ~Phold() noexcept override;
  Phold(const Phold &) = delete;
  Phold operator= (const Phold &) = delete;

  void setup() override;
  void finish() override;
  
  SST_ELI_REGISTER_COMPONENT
  (
   Phold,
   "phold",
   "Phold",
   SST_ELI_ELEMENT_VERSION( 1, 0, 0 ),
   "PHOLD benchmark component for SST",
   COMPONENT_CATEGORY_PROCESSOR
   );
  
  SST_ELI_DOCUMENT_PARAMS
  (
   { "remote",
     "Fraction of events which should be remote",
     "0.9"
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

  SST_ELI_DOCUMENT_PORTS
  (
      {"port",
       "Representative port",
        {"phold.PholdEvent", ""}
      }
  );
  
private:
  Phold();  /**< Default c'tor for serialization only. */

  void SendEvent (void);
  void handleEvent(SST::Event *ev);

  SST::Output m_output;    /**< Output stream for verbose output */

  static double m_remote;     /**< Remote event fraction */
  static double m_minimum;    /**< Minimum event delay */
  static double m_average;    /**< Mean event delay, added to m_minimum */
  static SST::SimTime_t m_stop;  /**< Stop time */
  static long   m_number;     /**< Total number of LPs */
  static long   m_events;     /**< Initial number of events per LP */
  static bool   m_verbose;    /**< Verbose output flag */

  static SST::TimeConverter * m_timeConverter;  /**< Convert between Component time and simulator. */

  SST::RNG::MersenneRNG * m_rng;    /**< Base RNG instance */
  SST::RNG::SSTUniformDistribution * m_remRng;   /**< Uniform RNG for deciding if event should be remote */
  SST::RNG::SSTUniformDistribution * m_nodeRng;  /**< Uniform RNG for picking remote LPs */
  SST::RNG::SSTExponentialDistribution * m_delayRng; /**< Poisson RNG for picking delay times */

  typedef std::vector<SST::Link *> LinkVector;
  LinkVector m_links;
};

}  // namespace Phold


#endif  // PHOLD_PHOLD_H
