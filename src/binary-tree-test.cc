/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2022 Lawrence Livermore National Laboratory
 * All rights reserved.
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */


#include "binary-tree.h"

#include <iostream>
#include <iomanip>

int
main (int argc, char** argv [[maybe_unused]])
{
  using bt = BinaryTree;

  bool all {argc > 1};

  // numeric field width
  constexpr std::size_t ww {5};
  constexpr std::size_t wn {4};

  // max depth to show
  const std::size_t m { all ? bt::max_depth : 9};

  std::cout << "Depth  Cap(d)  [begin -   end) |  Indices" << std::endl;  
  for (std::size_t d = 0; d <= m; ++d)
    {
      std::size_t b {bt::begin (d)};
      std::size_t e {bt::end (d)};
      std::size_t c {bt::capacity (d)};
      std::cout << std::setw (ww) << d << "   "
		<< std::setw (ww) << c << "   "
		<< std::setw (ww) << b << " -  "
		<< std::setw (ww) << e
		<< " | ";

      // Print indices explicitly for shallow depths
      if (d < 4)
	{
	  for (std::size_t j = b; j < e; ++j)
	    {
	      std::cout << std::setw (wn) << j;
	      // Flag depth errors, for debugging
	      if (bt::depth (j) != d)
		{
		  std::cout << "[" << bt::depth (j) << "]?";
		}
	      else if (j < e - 1)
		{
		  std::cout << "  ";
		}
	    }
	}
      else
	{
	  // Just print first and last index, for deep trees
	  std::cout << std::setw (wn) << b << "..." << e - 1;
	}
      std::cout << std::endl;
    }

  if (all)
    {
      std::cout << "\nstd::numeric_limits<std::size_t>::max() = "
		<< std::numeric_limits<std::size_t>::max()
		<< std::endl;
    }

  return 0;
}
