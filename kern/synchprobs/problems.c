/*
 * Copyright (c) 2001, 2002, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Driver code for whale mating problem
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>
#include <wchan.h>

/*
 * 08 Feb 2012 : GWA : Driver code is in kern/synchprobs/driver.c. We will
 * replace that file. This file is yours to modify as you see fit.
 *
 * You should implement your solution to the whalemating problem below.
 */

// 13 Feb 2012 : GWA : Adding at the suggestion of Isaac Elbaz. These
// functions will allow you to do local initialization. They are called at
// the top of the corresponding driver code.

struct semaphore *whl_male;
struct semaphore *whl_female;
struct semaphore *whl_matchmkr;

void whalemating_init() {
	whl_male = sem_create("male_whale", 0);
	whl_female = sem_create("female_whale", 0);
	whl_matchmkr = sem_create("matchmaker_whale", 0);
  return;
}

// 20 Feb 2012 : GWA : Adding at the suggestion of Nikhil Londhe. We don't
// care if your problems leak memory, but if you do, use this to clean up.

void whalemating_cleanup() {
	sem_destroy(whl_male);
	sem_destroy(whl_female);
	sem_destroy(whl_matchmkr);
  return;
}

void
male(void *p, unsigned long which)
{
	struct semaphore * whalematingMenuSemaphore = (struct semaphore *)p;
  (void)which;
  
  male_start();
	// Implement this function 
  P(whl_male);
  male_end();

  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
  V(whalematingMenuSemaphore);
  return;
}

void
female(void *p, unsigned long which)
{
	struct semaphore * whalematingMenuSemaphore = (struct semaphore *)p;
  (void)which;
  
  female_start();
	// Implement this function 
  P(whl_female);
  female_end();
  
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
  V(whalematingMenuSemaphore);
  return;
}

void
matchmaker(void *p, unsigned long which)
{
	struct semaphore * whalematingMenuSemaphore = (struct semaphore *)p;
  (void)which;
  
  matchmaker_start();
	// Implement this function 
  V(whl_male);
  V(whl_female);
  V(whl_matchmkr);
  P(whl_matchmkr);
  matchmaker_end();
  
  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
  V(whalematingMenuSemaphore);
  return;
}

/*
 * You should implement your solution to the stoplight problem below. The
 * quadrant and direction mappings for reference: (although the problem is,
 * of course, stable under rotation)
 *
 *   | 0 |
 * --     --
 *    0 1
 * 3       1
 *    3 2
 * --     --
 *   | 2 | 
 *
 * As way to think about it, assuming cars drive on the right: a car entering
 * the intersection from direction X will enter intersection quadrant X
 * first.
 *
 * You will probably want to write some helper functions to assist
 * with the mappings. Modular arithmetic can help, e.g. a car passing
 * straight through the intersection entering from direction X will leave to
 * direction (X + 2) % 4 and pass through quadrants X and (X + 3) % 4.
 * Boo-yah.
 *
 * Your solutions below should call the inQuadrant() and leaveIntersection()
 * functions in drivers.c.
 */

// 13 Feb 2012 : GWA : Adding at the suggestion of Isaac Elbaz. These
// functions will allow you to do local initialization. They are called at
// the top of the corresponding driver code.

struct lock *sp2_lk;
struct wchan *sp2_wc;
bool locked_quadrants[4] = {false, false, false, false};

void stoplight_init() {

	sp2_lk = lock_create("stoplight lock");
	sp2_wc = wchan_create("stoplight wait channel");
	return;
}

// 20 Feb 2012 : GWA : Adding at the suggestion of Nikhil Londhe. We don't
// care if your problems leak memory, but if you do, use this to clean up.

void stoplight_cleanup() {

	lock_destroy(sp2_lk);
	return;
}

void
gostraight(void *p, unsigned long direction)
{
	struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;

	lock_acquire(sp2_lk);
	while(locked_quadrants[direction]
	                       || locked_quadrants[(direction + 3) % 4]) {
		wchan_lock(sp2_wc);
		lock_release(sp2_lk);
		wchan_sleep(sp2_wc);

		lock_acquire(sp2_lk);
	}

	locked_quadrants[direction] = true;
	locked_quadrants[(direction + 3) % 4] = true;
	lock_release(sp2_lk);

	inQuadrant(direction);
	inQuadrant((direction + 3) % 4);
	leaveIntersection();

	lock_acquire(sp2_lk);
	locked_quadrants[direction] = false;
	locked_quadrants[(direction + 3) % 4] = false;
	lock_release(sp2_lk);

	wchan_wakeall(sp2_wc);
  
	// 08 Feb 2012 : GWA : Please do not change this code. This is so that your
	// stoplight driver can return to the menu cleanly.
	V(stoplightMenuSemaphore);
	return;
}

void
turnleft(void *p, unsigned long direction)
{
	struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;
	int second_quadrant;

	lock_acquire(sp2_lk);
	while(locked_quadrants[direction]
	                   	|| locked_quadrants[(direction + 2) % 4]
	                   	|| (locked_quadrants[(direction + 1) % 4] && locked_quadrants[(direction + 3) % 4])) {
		wchan_lock(sp2_wc);
		lock_release(sp2_lk);
		wchan_sleep(sp2_wc);

		lock_acquire(sp2_lk);
	}

	if(locked_quadrants[(direction + 1) % 4]) {
		second_quadrant = (direction + 3) % 4;
	}
	else {
		second_quadrant = (direction + 1) % 4;
	}

	locked_quadrants[direction] = true;
	locked_quadrants[second_quadrant] = true;
	locked_quadrants[(direction + 2) % 4] = true;

	lock_release(sp2_lk);



	inQuadrant(direction);
	inQuadrant(second_quadrant);
	inQuadrant((direction + 2) % 4);

	leaveIntersection();



	lock_acquire(sp2_lk);

	locked_quadrants[direction] = false;
	locked_quadrants[second_quadrant] = false;
	locked_quadrants[(direction + 2) % 4] = false;

	lock_release(sp2_lk);
  
	wchan_wakeall(sp2_wc);

	// 08 Feb 2012 : GWA : Please do not change this code. This is so that your
	// stoplight driver can return to the menu cleanly.
	V(stoplightMenuSemaphore);
	return;
}

void
turnright(void *p, unsigned long direction)
{
	struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;

	lock_acquire(sp2_lk);
	while(locked_quadrants[direction]) {
		wchan_lock(sp2_wc);
		lock_release(sp2_lk);
		wchan_sleep(sp2_wc);

		lock_acquire(sp2_lk);
	}

	locked_quadrants[direction] = true;
	lock_release(sp2_lk);

	inQuadrant(direction);
	leaveIntersection();

	lock_acquire(sp2_lk);
	locked_quadrants[direction] = false;
	lock_release(sp2_lk);

	wchan_wakeall(sp2_wc);

	// 08 Feb 2012 : GWA : Please do not change this code. This is so that your
	// stoplight driver can return to the menu cleanly.
	V(stoplightMenuSemaphore);
	return;
}
