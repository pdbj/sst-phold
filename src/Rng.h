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
 * Phold::Rng class declaration.
 */

/** Namespace for Phold (and related) benchmark. */
namespace Phold {

/**
 * SST RNG performance benchmark.
 */
class Rng : public SST::Component
{

public:
  /**
   * Constructor
   * @param id     Component instance unique id
   * @param params Configuration parameters
   */
  Rng( SST::ComponentId_t id, SST::Params& params );
  /** D'tor */
  ~Rng() noexcept override;

  // Rest of Rule of 5 are deleted
  /** Copy c'tor, deleted. */
  Rng(const Rng &) = delete;
  /** Copy assignment, deleted. */
  Rng & operator= (const Rng &) = delete;
  /** Move c'tor, deleted. */
  Rng(Rng &&) = delete;
  /** Move assignment, deleted. */
  Rng & operator= (Rng &&) = delete;

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

   Rng,
   "phold",
   "Rng",
   SST_ELI_ELEMENT_VERSION( 1, 0, 0 ),
   "RNG benchmark component",
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

   { "number",
     "Total number of Rng componentss. Must be at least 1.",
     "1"
   },
   { "samples",
     "Number of rng samples per component. Must be > 0.",
     "1"
   },
   { "pverbose",
     "Verbose output",
     "false"
   }
  );

  //  SST_ELI_DOCUMENT_STATISTICS
  //  (
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

  //   );

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

   { "portL", "Nuisance port", {"phold.RngEvent", ""}},
   { "portR", "Nuisance port", {"phold.RngEvent", ""}},

  );
  
private:
  /** Default c'tor for serialization only. */
  Rng();

  /**
   * Timing event handler.
   * We just borrow Phold::PholdEvent for this
   * @param ev The incoming event.
   * @param from The sending component id.
   */
  void handleEvent(SST::Event *ev, uint32_t from);
  
  // Class static data members

  static uint32_t          m_number;     /**< Total number of components */
  static uint64_t          m_samples;    /**< Number of samples per component */
  static uint32_t          m_verbose;    /**< Verbose output flag */

  // Class instance data members
  /** Output stream for verbose output */
  SST::Output              m_output;
#ifdef RNG_DEBUG
  /** 
   * Verbose output prefix, 
   * per instance since it includes the component name. 
   */
  std::string VERBOSE_PREFIX;
#endif

  /** Self link. */
  SST::Link * m_self;

  /** 
   * Choice of underlying RNG: 
   * * SST::RNG::MersenneRNG or 
   * * SST::RNG::MarsagliaRNG 
   */
  typedef SST::RNG::MarsagliaRNG RNG_t;

  /**< Base RNG instance */
  RNG_t                            * m_rng;

  // Class instance statistics

};  // class Rng

}  // namespace Phold


