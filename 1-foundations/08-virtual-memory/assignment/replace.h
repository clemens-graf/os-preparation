/*
 * replace.h - Module 08 assignment, part C: page-replacement algorithms.
 *
 * Each function simulates `nframes` frames, initially empty, for the page
 * references refs[0 .. n-1] and returns the number of page faults. A
 * reference to a page that is not in a frame is a fault; while a frame is
 * still empty the page goes there, otherwise the algorithm picks a victim.
 *
 *   FIFO   evict the page that was loaded longest ago.
 *   LRU    evict the page whose last use lies furthest back.
 *   OPT    (Belady) evict the page whose NEXT use lies furthest in the
 *          future; a page never used again is the best victim of all.
 *   CLOCK  (second chance) one reference bit R per frame and a hand that
 *          starts at frame 0. A hit sets R = 1. On a fault: while the frame
 *          under the hand is occupied and has R = 1, clear R and advance;
 *          then put the page into that frame with R = 1 and advance the
 *          hand once more. (Empty frames count as R = 0, so the hand fills
 *          them in order 0, 1, 2, ...)
 *
 * Page numbers are >= 0; 1 <= nframes <= REPL_MAX_FRAMES.
 */
#pragma once

#define REPL_MAX_FRAMES 64

int repl_fifo(const int *refs, int n, int nframes);
int repl_lru(const int *refs, int n, int nframes);
int repl_opt(const int *refs, int n, int nframes);
int repl_clock(const int *refs, int n, int nframes);
