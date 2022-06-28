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
main (int argc, char ** argv)
{
  using bt = BinaryTree;

  // numeric field width
  constexpr std::size_t w {5};

  // last index at depth
  std::size_t last {0};

  std::cout << "Depth  Cap(d)  |  Indices" << std::endl;  
  for (std::size_t d = 0; 
       d < 10 /* std::numeric_limits<std::size_t>::digits */;
       ++d)
    {
      std::size_t c {bt::capacity (d)};
      std::cout << std::setw (w) << d << "   "
		<< std::setw (w) << c << "  |";

      // Print indices explicitly for shallow depths
      if (d < 4)
	{
	  for (std::size_t j = last + (d > 0 ? 1 : 0); j < c; ++j)
	    {
	      std::cout << std::setw (w) << j;
	      // Flag depth errors, for debugging
	      if (bt::depth (j) != d)
		{
		  std::cout << "[" << bt::depth (j) << "]";
		}
	      else
		{
		  std::cout << "  ";
		}
	      last = j;
	    }
	}
      else
	{
	  // Just print first and last index, for deep trees
	  std::cout << std::setw (w) << last + 1 << "  ...  ";
	  last = c - 1;
	  std::cout << last;
	}
      std::cout << std::endl;
    }

  return 0;
}
