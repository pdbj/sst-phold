/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Lawrence Livermore National Laboratory
 * All rights reserved.
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

# pragma once

/**
 * Work with a binary tree stored in an indexed container.
 *
 * \c size(depth) returns the maximum number of items stored in a tree
 * of a given depth.
 *
 * \c depth(index) returns the depth of the item at \c index
 *
 * \c begin(depth), \c end(depth) return the first and one past the last
 * index at \c depth.

 * \c children(index) returns a std::pair with the indices of the children
 * of the item at \c index:
 *
 *    2 * index + 1, 2 * index + 2
 *
 * \c parent(index) returns the index of the parent of the item at \c child:
 *
 *    parent = (child - 1) / 2  // with integer division truncation
 */
struct BinaryTree
{
  /**
   * Return the total size (maximum number of elements)
   * of a tree with \c depth
   */
  static std::size_t size(std::size_t depth)
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
    // while (size(depth) <= index) ++depth;
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
    return (depth > 0 ? size(depth -1) : 0);
  }

  /**
   * One past the last index at \c depth
   */
  static std::size_t end(std::size_t depth)
  {
    return size(depth);
  }

  /**
   * Return the parent indes for \c child
   */
  static std::size_t parent(std::size_t child)
  {
    return (child - 1) / 2;
  }

  /**
   * Return the children indices of \c index
   */
  static std::pair<std::size_t, std::size_t> children(std::size_t index)
  {
    std::size_t child1 = 2 * index + 1;
    return std::make_pair(child1, child1 + 1);
  }

};  // struct BinaryTree

