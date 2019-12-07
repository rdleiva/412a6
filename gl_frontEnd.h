//
//  gl_frontEnd.h
//  GL threads
//
//  Created by Jean-Yves Hervé on 2017-04-24.
//  Copyright © 2017 Jean-Yves Hervé. All rights reserved.
//

#ifndef GL_FRONT_END_H
#define GL_FRONT_END_H
#include <pthread.h>


//------------------------------------------------------------------------------
//	Find out whether we are on Linux or macOS (sorry, Windows people)
//	and load the OpenGL & glut headers.
//	For the macOS, lets us choose which glut to use
//------------------------------------------------------------------------------
#if (defined(__dest_os) && (__dest_os == __mac_os )) || \
	defined(__APPLE_CPP__) || defined(__APPLE_CC__)
	//	Either use the Apple-provided---but deprecated---glut
	//	or the third-party freeglut install
	#if 1
		#include <GLUT/GLUT.h>
	#else
		#include <GL/freeglut.h>
		#include <GL/gl.h>
	#endif
#elif defined(linux)
	#include <GL/glut.h>
#else
	#error unknown OS
#endif


//-----------------------------------------------------------------------------
//	Data types
//-----------------------------------------------------------------------------

//	Travel direction data type
//	Note that if you define a variable
//	TravelDirection dir = whatever;
//	you get the opposite directions from dir as (NUM_TRAVEL_DIRECTIONS - dir)
//	you get left turn from dir as (dir + 1) % NUM_TRAVEL_DIRECTIONS
typedef enum TravelDirection {
								SOUTH = 0,
								WEST,
								NORTH,
								EAST,
								//
								NUM_TRAVEL_DIRECTIONS
} TravelDirection;

//	The 
using TravelerType = enum {
								RED_TRAV = 0,
								GREEN_TRAV,
								BLUE_TRAV,
								//
								NUM_TRAV_TYPES
};
using ProducerType = TravelerType;

//	Traveler info data type
/** Traveler info struct
 *  @var type       type of traveler
 *  @var row        row location of traveler
 *  @var col        col location of traveler
 *  @var isLive     thread is live bool
 *  @var distance   distance travel for traveler
 *  @var index      index of traveler
 *  @var threadID   thread id of traveler
 */
typedef struct TravelerInfo {
								TravelerType type;
								//	location of the traveler
								int row;
								int col;
								//	in which direciton is the traveler going
								TravelDirection dir;
								// initialized to 1, set to 0 if terminates
								int isLive;
								int distance;

								unsigned int index;
								pthread_t threadID;
//                                pthread_mutex_t* thread_lock;
} TravelerInfo;


/** Producer struct
 *  @var type       type of producer
 *  @var threadID   pthread_t thread id
 */
typedef struct Producer {
    ProducerType type;
    pthread_t threadID;
}Producer;

//-----------------------------------------------------------------------------
//	Function prototypes
//-----------------------------------------------------------------------------

void drawGrid(int**grid, int numRows, int numCols);
void drawGridAndTravelers(int**grid, int numRows, int numCols, TravelerInfo *travelList);
void drawState(int numLiveThreads, int redLevel, int greenLevel, int blueLevel);
void initializeFrontEnd(int argc, char** argv, void (*gridCB)(void), void (*stateCB)(void));
void speedupProducers(void);
void slowdownProducers(void);

#endif // GL_FRONT_END_H

