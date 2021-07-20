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
#include <sst/core/eli/statsInfo.h>

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

  // Rest of Rule of 5 are deleted
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

  SST_ELI_REGISTER_COMPONENT
  (
   /* 
      Long macro chain starting in sst-core/sst/core/component.h:
      SST_ELI_REGISTER_COMPONENT(cls,lib,name,ELI_FORWARD_AS_ONE(version),desc)

      Defines class static info functions:

      Return type                     Function name            Macro arg
      ------------------------------  ---------------------    ----------------
      static const std::string &      ELI_getCompileDate();    implicit
      static const std::string        ELI_getCompileFile();    implicit
      static constexpr unsigned       majorVersion();          from version arg
      static constexpr unsigned       minorVersion();          from version arg
      static constexpr unsigned       tertiaryVersion();       from version arg
      static const std::vector<int> & ELI_getVersion();        version
      static const char *             ELI_getLibrary();        lib
      static const char *             ELI_getName();           cls
      static const char *             ELI_getDescription();    desc
      static uint32_t                 ELI_getCategory();       category tag

      (Implicit args are derived during compilation.)

      As if there was a struct (but the information isn't stored that way):

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
   */

   Phold,
   "phold",
   "Phold",
   SST_ELI_ELEMENT_VERSION( 1, 0, 0 ),
   "PHOLD benchmark component for SST",
   COMPONENT_CATEGORY_UNCATEGORIZED
   );

  SST_ELI_DOCUMENT_PARAMS
  ( 
   /* 
      Macro defined in sst-core/src/sst/core/eli/paramsInfo.h:
      static const std::vector<SST::ElementInfoParam> & ELI_getParams();

      struct SST::ElementInfoParam from sst-core/src/sst/core/eli/elibase.h:
      {
        const char * name;          // !< Name, as used in calls to Params::find().
        const char * description;   // !< Description as reported by sst-info.
        const char * defaultValue;  // !< Default value if not set explicitly.
      }
     */

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
     "10"  // TODO - make this a time, such as seconds
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

  SST_ELI_DOCUMENT_STATISTICS
  (
   /*
     Macro defined in sst-core/src/sst/core/eli/statsInfo.h:
     static const std::vector<SST::ElementInfoStatistic>& ELI_getStatistics();

     struct SST::ElementInfoStatistic from sst-core/src/sst/core/eli/elibase.h
     {
       const char* name;               //!< Name of the Statistic to be Enabled
       const char* description;        //!< Brief description of the Statistic
       const char* units;              //!< Units associated with this Statistic value
       const uint8_t enableLevel;      //!< Level to meet to enable statistic 0 = disabled 
     }
   */

   { "Count",
     "Count of events handled before stop time.",
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
   * Format for dynamic ports 'port_x'. The number of ports created
   * will be determined from the "number" argument.
   */
  static constexpr char PORT_NAME[]   = "port_%(number)d";

  SST_ELI_DOCUMENT_PORTS
  (
   /*
     Macro defined in sst-core/src/sst/core/eli/portsInfo.h:
     static const std::vector<SST::ElementInfoPort2>& ELI_getPorts();
     
     struct SST::ElementInfoPort2 from sst-core/src/sst/core/eli/elibase.h:
     { 
       /// Name of the port. Can contain d for a dynamic port, 
       /// also %(xxx)d for dynamic port with xxx being the 
       /// controlling component parameter
       const char* name;
       /// Brief description of the port (ie, what it is used for)
       const char* description;
       /// List of fully-qualified event types that this Port expects 
       /// to send or receive
       const std::vector<std::string> validEvents;
       };
   */

   { PORT_NAME,                  
     "Representative port",      
     {"phold.PholdEvent", ""}    
    }
  );
  
private:
  /** Default c'tor for serialization only. */
  Phold();

  /** Send a new event to a random LP. */
  bool SendEvent ();

  /**
   * Incoming event handler
   * @param ev The incoming event.
   * @param from The sending LP id.
   */
  void handleEvent(SST::Event *ev, uint32_t from);

  // Class static data members

  /** Default time base for component and associated links */
  static constexpr char    TIMEBASE[] = "1ms";
  static constexpr double  TIMEFACTOR = 1e-3;


  static double            m_remote;     /**< Remote event fraction */
  static double            m_minimum;    /**< Minimum event delay */
  static double            m_average;    /**< Mean event delay, added to m_minimum */
  static SST::SimTime_t    m_stop;       /**< Stop time */
  static long              m_number;     /**< Total number of LPs */
  static long              m_events;     /**< Initial number of events per LP */
  static bool              m_delaysOut;  /**< Output delays histogram */
  static uint32_t          m_verbose;    /**< Verbose output flag */

  static SST::TimeConverter * m_timeConverter;


  // Class instance data members
  /** Output stream for verbose output */
  SST::Output              m_output;
  /** 
   * Verbose output prefix, 
   * per instance since it includes the component name. 
   */
  std::string VERBOSE_PREFIX;
  /** The list of links to other LPs */
  std::vector<SST::Link *> m_links;     
 
  /** 
   * Choice of underlying RNG: 
   * * SST::RNG::MersenneRNG or 
   * * SST::RNG::MarsagliaRNG 
   */
  typedef SST::RNG::MarsagliaRNG RNG_t;

  /**< Base RNG instance */
  RNG_t                            * m_rng;
  /**< Uniform RNG for deciding if event should be remote */
  SST::RNG::SSTUniformDistribution * m_remRng;
  /**< Uniform RNG for picking remote LPs */
  SST::RNG::SSTUniformDistribution * m_nodeRng;
  /**< Poisson RNG for picking delay times */
  SST::RNG::SSTExponentialDistribution * m_delayRng;

  // Class instance statistics
  /** Count of events handled. */
  Statistic<uint64_t> * m_count;
  /** Histogram of delay times. */
  Statistic<float>   * m_delays;
};

}  // namespace Phold


#endif  // PHOLD_PHOLD_H
