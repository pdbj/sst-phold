/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Lawrence Livermore National Laboratory
 * All rights reserved.
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#ifndef _PHOLD_H
#define _PHOLD_H

#include <sst/core/component.h>
//#include <sst/core/eli/elementinfo.h>
#include <sst/core/link.h>
#include <sst/core/rng/mersenne.h>
#include <sst/core/rng/uniform.h>
#include <sst/core/rng/poisson.h>

class Phold : public SST::Component 
{

public:
  Phold( SST::ComponentId_t id, SST::Params& params );
  ~Phold();
  
  void setup();
  void finish();
  
  SST_ELI_REGISTER_COMPONENT
  (
   Phold,
   "phold",
   "Phold",
   SST_ELI_ELEMENT_VERSION( 1, 0, 0 ),
   "PHOLD benchmark component for SST",
   COMPONENT_CATEGORY_PROCESSOR
   )
  
  SST_ELI_DOCUMENT_PARAMS
  (
   { "remote",
     "Fraction of events which should be remote",
     "0.9"
   },
   { "minimum",
     "Minimum delay when sending events.",
     "0.1"  // TODO - make this a time, such as seconds
   },
   { "average",
     "Mean delay to be added to min when sending events",
     "0.9"  // TODO - make this a time, such as seconds
   },
   { "number",
     "Total number of LPs. Must be at least 2.",
     "2"
   },
   { "verbose",
     "Verbose output",
     "false"
   }
   )
  
private:

  void handleEvent(SST::Event *ev);
  bool clockTick( SST::Cycle_t currentCycle );
  
  SST::Output m_output;    /**< Output stream for verbose output */
  double m_remote;         /**< Remote event fraction */
  double m_minimum;        /**< Minimum event delay */
  double m_average;        /**< Mean event delay, added to m_minimum */
  long   m_number;         /**< Total number of LPs */
  bool m_verbose;          /**< Verbose output flag */

  SST::RNG::MersenneRNG * m_rng;    /**< Base RNG instance */
  SST::RNG::SSTUniformDistribution * m_uni;  /**< Uniform RNG for picking remotes */
  SST::RNG::SSTPoissonDistribution * m_pois; /**< Poisson RNG for picking delay times */
  
};


#endif  // _PHOLD_H
