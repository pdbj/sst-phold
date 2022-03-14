/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Lawrence Livermore National Laboratory
 * All rights reserved.
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

# pragma once

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
 * are actually valid.
 *
 * \c capacity(depth) returns the maximum number of items which 
 * can be stored in a tree of a given depth.
 *
 * \c depth(index) returns the depth of the item at \c index
 *
 * \c begin(depth), \c end(depth) return the first and one past the last
 * index at \c depth.

 * \c children(parentIdx) returns a std::pair with the indices of the children
 * of the item at \c parentIcx:
 *
 *    2 * parentIdx + 1, 2 * parentIdx + 2
 *
 * \c parent(childIdx) returns the index of the parent of the item at \c childIdx:
 *
 *    parent = (childIdx - 1) / 2  // with integer division truncation
 */
struct BinaryTree
{
  /**
   * Return the total size (maximum number of elements)
   * of a tree with \c depth
   */
  static std::size_t capacity(std::size_t depth)
  {
    std::size_t size = 0;
    std::size_t pow2 = 1;  // 2^k for k = 0
    for (std::size_t k = 0; k <= depth; ++k)
      {
	size += pow2;
	pow2 *= 2;
      }
    return size;
  }

  /**
   * Depth of the item at \c index.
   */
  static std::size_t depth(std::size_t index)
  {
    std::size_t depth = 0;
    // Correct, but slow:
    // while (capacity(depth) <= index) ++depth;
    std::size_t size = 0;
    std::size_t pow2 = 1;  // 2^k for k = 0
    do
      {
	size += pow2;
	pow2 *= 2;
	++depth;
      } while (size <= index);
    return --depth;
  }

  /**
   *  First index at \c depth
   */
  static std::size_t begin(std::size_t depth)
  {
    return (depth > 0 ? capacity(depth -1) : 0);
  }

  /**
   * One past the last index at \c depth
   */
  static std::size_t end(std::size_t depth)
  {
    return capacity(depth);
  }

  /**
   * Return the parent index for \c childIdx
   */
  static std::size_t parent(std::size_t childIdx)
  {
    return (childIdx - 1) / 2;
  }

  /**
   * Return the children indices of \c parentIdx
   */
  static std::pair<std::size_t, std::size_t> children(std::size_t parentIdx)
  {
    std::size_t child1 = 2 * parentIdx + 1;
    return std::make_pair(child1, child1 + 1);
  }

};  // struct BinaryTree

