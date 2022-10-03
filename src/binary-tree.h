/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Lawrence Livermore National Laboratory
 * All rights reserved.
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

# pragma once

#include <array>
#include <limits>
#include <utility>

/**
 * Convenience functions for working with a binary tree stored 
 * in an indexed container.
 *
 * Note these functions know nothing about the container,
 * these just work with notional indices.  They also know nothing
 * about how many actual items are stored in the container, 
 * just how many _could_ be stored in a tree of a given depth.
 *
 * In general the caller should check that any indices
 * returned by `begin()`, `end()`, `children()`, `parent()` 
 * are actually valid with respect to the number of items which
 * have actually been stored.
 *
 * \c capacity(depth) returns the maximum number of items which 
 * can be stored in a tree of a given depth.
 *
 * \c depth(index) returns the depth of the item at \c index
 *
 * \c begin(depth), \c end(depth) return the first and one past the last
 * index at \c depth.

 * \c children(parentIdx) returns a std::pair with the indices of the children
 * of the item at \c parentIdx:
 *
 *    {2 * parentIdx + 1, 2 * parentIdx + 2}
 *
 * \c parent(childIdx) returns the index of the parent of the item at \c childIdx:
 *
 *    parent = (childIdx - 1) / 2  // with integer division truncation
 */
struct BinaryTree
{
  /** The maximum depth (number of bits in size_t).*/
  static constexpr std::size_t max_depth {std::numeric_limits<std::size_t>::digits};
  /**
   * Return the total size (maximum number of elements)
   * of a tree with \c depth
   *
   * The capacity is given by the expression `(1 << (depth + 1)) - 1`.
   */
  static inline std::size_t
  capacity(std::size_t depth)
  {
    /*
      Since the number of bits available is rather small,
      just memoize the result.
     */
    typedef std::array<std::size_t, max_depth + 1> memo_t;  // +1 for 0-based
    static auto cap_memo = []() -> memo_t
      {
        memo_t cap;
        cap[0] = 0;
        std::size_t s {1};  // 2^(d + 1) - 1 for d == 0
        for (std::size_t d = 1; d <= max_depth; ++d)
          {
            cap[d] = cap[d - 1] + s;
            s <<= 1;
          }
        return cap;
      } ();
    return cap_memo[depth + 1];
  }

  /**
   * Depth of the item at \c index.
   */
  static inline std::size_t
  depth(std::size_t index)
  {
    std::size_t depth = 0;
    while (capacity (depth++) <= index);
    return --depth;
  }

  /**
   *  First index at \c depth
   */
  static inline std::size_t
  begin(std::size_t depth)
  {
    if (depth == 0) return 0;
    return capacity(depth - 1);
  }

  /**
   * One past the last index at \c depth
   */
  static inline std::size_t
  end(std::size_t depth)
  {
    return capacity(depth);
  }

  /**
   * Return the parent index for \c childIdx
   */
  static inline std::size_t
  parent(std::size_t childIdx)
  {
    return (childIdx - 1) / 2;
  }

  /**
   * Return the children indices of \c parentIdx
   */
  static inline std::pair<std::size_t, std::size_t>
  children(std::size_t parentIdx)
  {
    std::size_t child1 = 2 * parentIdx + 1;
    return std::make_pair(child1, child1 + 1);
  }

};  // struct BinaryTree

