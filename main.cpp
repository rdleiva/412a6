//
//  main.cpp
//  GL threads
//
//  Created by Jean-Yves Hervé on 2017-04-24, revised 2019-11-19
//  Edited by Rotman Daniel Leiva on 2019-12-06
//

 /*-------------------------------------------------------------------------+
 |	A graphic front end for a grid+state simulation.						|
 |																			|
 |	This application simply creates a glut window with a pane to display	|
 |	a colored grid and the other to display some state information.			|
 |	Sets up callback functions to handle menu, mouse and keyboard events.	|
 |	Normally, you shouldn't have to touch anything in this code, unless		|
 |	you want to change some of the things displayed, add menus, etc.		|
 |	Only mess with this after everything else works and making a backup		|
 |	copy of your project.  OpenGL & glut are tricky and it's really easy	|
 |	to break everything with a single line of code.							|
 |																			|
 |	Current GUI:															|
 |		- 'ESC' --> exit the application									|
 |		- 'r' --> add red ink												|
 |		- 'g' --> add green ink												|
 |		- 'b' --> add blue ink												|
 +-------------------------------------------------------------------------*/

#include <iostream>
#include <thread>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <time.h>
#include <unistd.h>

//
#include "gl_frontEnd.h"

using namespace std;

//==================================================================================
//	Function prototypes
//==================================================================================
void displayGridPane(void);
void displayStatePane(void);
void initializeApplication(void);

// TravelDirection newDirection(TravelerInfo* tt, int distance);
void* runTravelerThread(void* data);
// int getAcceptableDirections(unsigned int x, unsigned int y, TravelDirection dirs[NUM_TRAVEL_DIRECTIONS]);
// void moveTravelerToPosition(TravelDirection dir, unsigned int x, unsigned int y, TravelerInfo* tt);

TravelDirection generateDirection(int col, int row, TravelDirection dir);
bool checkDirection(unsigned int x, unsigned int y, unsigned int dir);
void moveTraveler(TravelerInfo* tt);
unsigned colorCell(TravelerInfo *tt);
unsigned newDistance(int col, int row, TravelDirection dir);
bool getInk(TravelerType type);

void* produceInkThread(void* producer);

//==================================================================================
//	Application-level global variables
//==================================================================================

//	Don't touch
extern int	GRID_PANE, STATE_PANE;
extern int	gMainWindow, gSubwindow[2];

//	The state grid and its dimensions
int** grid;
const int NUM_ROWS = 30, NUM_COLS = 20;

//	the number of live threads (that haven't terminated yet)
int MAX_NUM_TRAVELER_THREADS = 15;
int numLiveThreads = 0;

//	the ink levels
const int NUM_PRODUCER_THREADS = 9;
int MAX_LEVEL = 50;
int MAX_ADD_INK = 10;
int redLevel = 20, greenLevel = 10, blueLevel = 40;
const int TRAV_INK_INCR = 16;

//	ink producer sleep time (in microseconds)
const int MIN_SLEEP_TIME = 1000;
int producerSleepTime = 100000;

//	Enable this declaration if you want to do the traveler information
//	maintaining extra credit section
TravelerInfo *travelList = NULL;
Producer *producerList = NULL;

pthread_mutex_t p_mutex;
pthread_mutex_t grid_lock;
pthread_mutex_t ink_lock;

const unsigned int TRAV_COLOR[NUM_TRAV_TYPES] = {0xFF0000FF, 0xFF00FF00, 0xFFFF0000};


//==================================================================================
//	These are the functions that tie the simulation with the rendering.
//	Some parts are "don't touch."  Other parts need your intervention
//	to make sure that access to critical section is properly synchronized
//==================================================================================


void displayGridPane(void)
{
	//	This is OpenGL/glut magic.  Don't touch
	glutSetWindow(gSubwindow[GRID_PANE]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//---------------------------------------------------------
	//	This is the call that makes OpenGL render the grid.
	//
	//	You *must* synchronize this call.
	//
	//---------------------------------------------------------
	// drawGrid(grid, NUM_ROWS, NUM_COLS);
	//
	//	Use this drawing call instead if you do the extra credits for
	//	maintaining traveler information
	drawGridAndTravelers(grid, NUM_ROWS, NUM_COLS, travelList);
	
	//	This is OpenGL/glut magic.  Don't touch
	glutSwapBuffers();
	
	glutSetWindow(gMainWindow);
}

void displayStatePane(void)
{
	//	This is OpenGL/glut magic.  Don't touch
	glutSetWindow(gSubwindow[STATE_PANE]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//---------------------------------------------------------
	//	This is the call that makes OpenGL render information
	//	about the state of the simulation.
	//
	//	You *must* synchronize this call.
	//
	//---------------------------------------------------------
	drawState(numLiveThreads, redLevel, greenLevel, blueLevel);
	
	
	//	This is OpenGL/glut magic.  Don't touch
	glutSwapBuffers();
	
	glutSetWindow(gMainWindow);
}

//------------------------------------------------------------------------
//	These are the functions that would be called by a traveler thread in
//	order to acquire red/green/blue ink to trace its trail.
//	You *must* synchronized access to the ink levels
//------------------------------------------------------------------------
//
bool acquireRedInk(int theRed)
{
	int ok = false;
	pthread_mutex_lock(&ink_lock);
	if (redLevel >= theRed)
	{
		redLevel -= theRed;
		ok = true;
	}
	pthread_mutex_unlock(&ink_lock);
	return ok;
}

bool acquireGreenInk(int theGreen)
{
	bool ok = false;
	pthread_mutex_lock(&ink_lock);
	if (greenLevel >= theGreen)
	{
		greenLevel -= theGreen;
		ok = true;
	}
	pthread_mutex_unlock(&ink_lock);
	return ok;
}

bool acquireBlueInk(int theBlue)
{
	bool ok = false;
	pthread_mutex_lock(&ink_lock);
	if (blueLevel >= theBlue)
	{
		blueLevel -= theBlue;
		ok = true;
	}
	pthread_mutex_unlock(&ink_lock);
	return ok;
}

//------------------------------------------------------------------------
//	These are the functions that would be called by a producer thread in
//	order to refill the red/green/blue ink tanks.
//	You *must* synchronized access to the ink levels
//------------------------------------------------------------------------
//
bool refillRedInk(int theRed)
{
	bool ok = false;
	pthread_mutex_lock(&ink_lock);
	if (redLevel + theRed <= MAX_LEVEL)
	{
		redLevel += theRed;
		ok = true;
	}
	pthread_mutex_unlock(&ink_lock);
	return ok;
}

bool refillGreenInk(int theGreen)
{
	bool ok = false;
	pthread_mutex_lock(&ink_lock);
	if (greenLevel + theGreen <= MAX_LEVEL)
	{
		greenLevel += theGreen;
		ok = true;
	}
	pthread_mutex_unlock(&ink_lock);
	return ok;
}

bool refillBlueInk(int theBlue)
{
	bool ok = false;
	pthread_mutex_lock(&ink_lock);
	if (blueLevel + theBlue <= MAX_LEVEL)
	{
		blueLevel += theBlue;
		ok = true;
	}
	pthread_mutex_unlock(&ink_lock);
	return ok;
}

//------------------------------------------------------------------------
//	You shouldn't have to touch this one.  Definitely if you don't
//	add the "producer" threads, and probably not even if you do.
//------------------------------------------------------------------------
void speedupProducers(void)
{
	//	decrease sleep time by 20%, but don't get too small
	int newSleepTime = (8 * producerSleepTime) / 10;
	
	if (newSleepTime > MIN_SLEEP_TIME)
	{
		producerSleepTime = newSleepTime;
	}
}

void slowdownProducers(void)
{
	//	increase sleep time by 20%
	producerSleepTime = (12 * producerSleepTime) / 10;
}

//------------------------------------------------------------------------
//	You shouldn't have to change anything in the main function
//------------------------------------------------------------------------
int main(int argc, char** argv)
{
	initializeFrontEnd(argc, argv, displayGridPane, displayStatePane);

	pthread_mutex_init(&grid_lock, NULL);
	pthread_mutex_init(&ink_lock, NULL);
	
	//	Now we can do application-level
	initializeApplication();

	//	Now we enter the main loop of the program and to a large extend
	//	"lose control" over its execution.  The callback functions that 
	//	we set up earlier will be called when the corresponding event
	//	occurs
	glutMainLoop();
	
	//	Free allocated resource before leaving (not absolutely needed, but
	//	just nicer.  Also, if you crash there, you know something is wrong
	//	in your code.
	for (int i=0; i< NUM_ROWS; i++)
		free(grid[i]);
	free(grid);
	//
	free(travelList);
	
	//	This will never be executed (the exit point will be in one of the
	//	call back functions).
	return 0;
}


//==================================================================================
//
//	This is a part that you have to edit and add to.
//
//==================================================================================


void initializeApplication(void)
{
	//	Allocate the grid
	grid = (int**) malloc(NUM_ROWS * sizeof(int*));
	for (int i=0; i<NUM_ROWS; i++)
		grid[i] = (int*) malloc(NUM_COLS * sizeof(int));
	
    
	//---------------------------------------------------------------
	//	All the code below to be replaced/removed
	//	I initialize the grid's pixels to have something to look at
	//---------------------------------------------------------------
	//	Yes, I am using the C random generator after ranting in class that the C random
	//	generator was junk.  Here I am not using it to produce "serious" data (as in a
	//	simulation), only some color, in meant-to-be-thrown-away code
	
	//	seed the pseudo-random generator
	srand((unsigned int) time(NULL));
	
	//	create RGB values (and alpha  = 255) for each pixel
	for (int i=0; i<NUM_ROWS; i++)
	{
		for (int j=0; j<NUM_COLS; j++)
		{
			//	temp code to get some color initially
			// unsigned char red = (unsigned char) (rand() % range + minVal);
			// unsigned char green = (unsigned char) (rand() % range + minVal);
			// unsigned char blue = (unsigned char) (rand() % range + minVal);
			// grid[i][j] = 0xFF000000 | (blue << 16) | (green << 8) | red;
			
			//	the intialization you should use
			grid[i][j] = 0xFF000000;
		}
	}
	
//	//	Enable this code if you want to do the traveler information
//	//	maintaining extra credit section
	travelList = (TravelerInfo*) malloc(MAX_NUM_TRAVELER_THREADS * sizeof(TravelerInfo));
	for (int k=0; k< MAX_NUM_TRAVELER_THREADS; k++){
		travelList[k].type = TravelerType(rand() % NUM_TRAV_TYPES);
		travelList[k].row = 1 + rand() % (NUM_ROWS-1);
		travelList[k].col = 1 + rand() % (NUM_COLS-1);
		travelList[k].dir = TravelDirection(rand() % NUM_TRAVEL_DIRECTIONS);
		travelList[k].isLive = 1;
		travelList[k].index=k;
        travelList[k].threadID=0;
        travelList[k].distance = newDistance(travelList[k].col, travelList[k].row, travelList[k].dir);
		numLiveThreads++;
//        travelList[k].thread_lock=&p_mutex;
	}

	for (unsigned int k = 0; k<MAX_NUM_TRAVELER_THREADS; k++){
		int errorCode = pthread_create(&travelList[k].threadID, nullptr, runTravelerThread, travelList+k);
		if (errorCode != 0){
            cerr << "could not pthread_create thread " << k <<
					 ", Error code " << errorCode << ": " << strerror(errorCode) << endl;
            exit (EXIT_FAILURE);
        }

    
    }
    
    producerList = (Producer*) malloc(NUM_PRODUCER_THREADS * sizeof(Producer));
    for (unsigned int k=0; k<NUM_PRODUCER_THREADS; k++){
        producerList[k].type = ProducerType(rand() % NUM_TRAV_TYPES);
        producerList[k].threadID=0;
    }

    for (unsigned int k = 0; k<NUM_PRODUCER_THREADS; k++){
        int errorCode = pthread_create(&producerList[k].threadID, nullptr, produceInkThread, producerList+k);
        if (errorCode != 0){
            cerr << "could not pthread_create thread " << k <<
            ", Error code " << errorCode << ": " << strerror(errorCode) << endl;
            exit (EXIT_FAILURE);
        }
     }


}

/** runs traveler thread
 * @param data      traveler info data
 * @return NULL     null pointer
 */
void* runTravelerThread(void* data){
    TravelerInfo* tt = static_cast<TravelerInfo*>(data);
						//dynamic, const, reinterpret
	printf("here 1, and isLive=%d\n", tt->isLive);
	fflush(stdout);
	tt->dir = generateDirection(tt->col, tt->row, tt->dir);
    while (tt->isLive){
		// tt->dir = generateDirection(tt->dir);
		// printf("here 2\n");
		// fflush(stdout);
		
	//	TravelDirection dirs[NUM_TRAVEL_DIRECTIONS];
		unsigned int x = tt->col, y = tt->row;
		// printf("x = %d, y = %d\n", x, y);
		
		// change color in grid
		// directions_allowed = getAcceptableDirections(x,y,dirs);
		// pthread_mutex_unlock(tt->thread_lock);
		if((x == 0 && y == 0) || (x == NUM_COLS-1 && y == 0) ||
			(x == 0 && y == NUM_ROWS-1) || (x == NUM_COLS-1 && y == NUM_ROWS-1)){
				tt->isLive = false;
				//	must be synchronized
				numLiveThreads--;
				// kill thread
			}
		else{
			// goodDir = checkDirection(x, y, tt->dir);
			// printf("goodDir = %d\n", goodDir);
			// if(goodDir == false) tt->dir = generateDirection(tt->dir);
			tt->distance = newDistance(x, y, tt->dir);
			moveTraveler(tt);
			// tt->dir = generateDirection(tt->dir);
		}
    }

    return NULL;
}

/** runs traveler thread
 * @param col           traveler col location
 * @param row           traveler row location
 * @param dir           direction of traveler
 * @return dir          direction of traveler
 */
TravelDirection generateDirection(int col, int row, TravelDirection dir){
	if(dir == NORTH || dir == SOUTH){
		if (col == 0)
			dir = EAST;
		else if (col == NUM_COLS-1)
			dir = WEST;
		//west or east if 2 or 3
		else
			dir = static_cast<TravelDirection>((rand() % 2)*2 + 1);
	}// south or north if 0 or 1
	else {
		if (row == 0)
			dir = SOUTH;
		else if (row == NUM_ROWS-1)
			dir = NORTH;
		else
			dir = static_cast<TravelDirection>((rand() % 2)*2);
	}
	return dir;
}

/** runs traveler thread
 * @param tt            traveler info pointer
 */
void moveTraveler(TravelerInfo* tt){
	// printf("here move traveler\n");
	
	int dir = tt->dir;
	int distance = tt->distance;

	for(int i = 0; i < distance; i++) {
        bool moveNotCompleted = true;
        while (moveNotCompleted) {
            if(getInk(tt->type)) {

                switch(dir) {
                    case NORTH:
                        tt->row -= 1;
                    break;
                    case SOUTH:
                        tt->row += 1;
                        break;
                    case WEST:
                        tt->col -= 1;
                        break;
                    case EAST:
                        tt->col += 1;
                        break;
                    default:
                        break;
                }
                pthread_mutex_lock(&grid_lock);
                switch (tt->type) {
                    case RED_TRAV: {
                            unsigned char red = (grid[tt->row][tt->col] & 0x000000FF);
                            if (red < 0xFF) {
                                red += TRAV_INK_INCR;
                                grid[tt->row][tt->col] |= red;
                            }
                        }
                        break;
                    case GREEN_TRAV: {
                            unsigned char green = (grid[tt->row][tt->col] & 0x0000FF00) >> 8;
                            if (green < 0xFF) {
                                green += TRAV_INK_INCR;
                                grid[tt->row][tt->col] |= (green << 8);
                            }
                        }
                        break;
                    case BLUE_TRAV: {
                            unsigned char blue = (grid[tt->row][tt->col] & 0x00FF0000) >> 16;
                            if (blue < 0xFF) {
                                blue += TRAV_INK_INCR;
                                grid[tt->row][tt->col] |= (blue << 16);
                            }
                        }
                        break;
                    default:
                        break;
                }
                pthread_mutex_unlock(&grid_lock);
                moveNotCompleted = false;
            }
            else
                usleep(100000);
        }

		usleep(100000);
	}
	tt->dir = generateDirection(tt->col, tt->row, tt->dir);	
}

/** runs traveler thread
 * @param type          traveler color type
 * @return okMove       bool okay to move
 */
bool getInk(TravelerType type) {
	bool okMove = false;
	switch(type) {
		case RED_TRAV:
			okMove = acquireRedInk(1);
			break;
		case GREEN_TRAV:
			okMove = acquireGreenInk(1);
			break;
		case BLUE_TRAV:
			okMove = acquireBlueInk(1);
			break;
		default:
			break;
	}
	return okMove;
}

/** runs traveler thread
 * @param col           traveler col location
 * @param row           traveler row location
 * @param dir           direction of traveler
 * @return dist         distance to move traveler
 */
unsigned newDistance(int col, int row, TravelDirection dir){
	int dist=NORTH;
		switch(dir) {
		case NORTH:
			dist = max(1, rand() % row);
			break;
		case SOUTH:
			dist = max(1, rand() % (NUM_ROWS - row));
			break;
		case WEST:
			dist = max(1, rand() % col);
			break;
		case EAST:
			dist = max(1, rand() % (NUM_COLS - col));
			break;
		default:
			break;
	}
	return dist;
}

/** runs traveler thread
 * @param ptr       Producer thread pointer
 * @return NULL     pointer
 */
void* produceInkThread(void* ptr){
    Producer* producer = (Producer*) ptr;
    
    while (true) {
        
        usleep(producerSleepTime);
        switch(producer->type) {
            case RED_TRAV:
                refillRedInk(1);
                break;
            case GREEN_TRAV:
                refillGreenInk(1);
                break;
            case BLUE_TRAV:
                refillBlueInk(1);
                break;
            default:
                break;
        }
    }
    return NULL;
}


// unsigned colorCell(TravelerInfo *tt) {
//     printf("color\n");
//     unsigned color;
//     switch(tt->type) {
//         case RED_TRAV:
//             if(acquireRedInk(tt->distance)!=0) color = 0xFF0000FF;
//             else {
//                 color = 0xFF000000;
//             }
//             break;
//         case GREEN_TRAV:
//             if(acquireGreenInk(tt->distance)!=0) color = 0xFF00FF00;
//             else {
//                 color = 0xFF000000;
//             }
//             break;
//         case BLUE_TRAV:
//             if(acquireBlueInk(tt->distance)!=0) color = 0xFFFF0000;
//             else {
//                 color = 0xFF000000;
//             }
//             break;
//         default:
//             break;
//     }
//     return color;
// }
