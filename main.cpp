//
//  main.cpp
//  GL threads
//
//  Created by Jean-Yves Hervé on 2017-04-24, revised 2019-11-19
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

#include <cstdio>
#include <stdlib.h>
#include <time.h>
//
#include "gl_frontEnd.h"

//==================================================================================
//	Function prototypes
//==================================================================================
void displayGridPane(void);
void displayStatePane(void);
void initializeApplication(void);


//==================================================================================
//	Application-level global variables
//==================================================================================

//	Don't touch
extern int	GRID_PANE, STATE_PANE;
extern int	gMainWindow, gSubwindow[2];

//	The state grid and its dimensions
int** grid;
const int NUM_ROWS = 20, NUM_COLS = 20;

//	the number of live threads (that haven't terminated yet)
int MAX_NUM_TRAVELER_THREADS = 10;
int numLiveThreads = 0;

//	the ink levels
int MAX_LEVEL = 50;
int MAX_ADD_INK = 10;
int redLevel = 20, greenLevel = 10, blueLevel = 40;

//	ink producer sleep time (in microseconds)
const int MIN_SLEEP_TIME = 1000;
int producerSleepTime = 100000;

//	Enable this declaration if you want to do the traveler information
//	maintaining extra credit section
//TravelerInfo *travelList;



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
	drawGrid(grid, NUM_ROWS, NUM_COLS);
	//
	//	Use this drawing call instead if you do the extra credits for
	//	maintaining traveler information
//	drawGridAndTravelers(grid, NUM_ROWS, NUM_COLS, travelList);
	
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
int acquireRedInk(int theRed)
{
	int ok = 0;
	if (redLevel >= theRed)
	{
		redLevel -= theRed;
		ok = 1;
	}
	return ok;
}

int acquireGreenInk(int theGreen)
{
	int ok = 0;
	if (greenLevel >= theGreen)
	{
		greenLevel -= theGreen;
		ok = 1;
	}
	return ok;
}

int acquireBlueInk(int theBlue)
{
	int ok = 0;
	if (blueLevel >= theBlue)
	{
		blueLevel -= theBlue;
		ok = 1;
	}
	return ok;
}

//------------------------------------------------------------------------
//	These are the functions that would be called by a producer thread in
//	order to refill the red/green/blue ink tanks.
//	You *must* synchronized access to the ink levels
//------------------------------------------------------------------------
//
int refillRedInk(int theRed)
{
	int ok = 0;
	if (redLevel + theRed <= MAX_LEVEL)
	{
		redLevel += theRed;
		ok = 1;
	}
	return ok;
}

int refillGreenInk(int theGreen)
{
	int ok = 0;
	if (greenLevel + theGreen <= MAX_LEVEL)
	{
		greenLevel += theGreen;
		ok = 1;
	}
	return ok;
}

int refillBlueInk(int theBlue)
{
	int ok = 0;
	if (blueLevel + theBlue <= MAX_LEVEL)
	{
		blueLevel += theBlue;
		ok = 1;
	}
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
//	free(travelList);
	
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
	const unsigned char minVal = (unsigned char) 40;
	const unsigned char range = (unsigned char)(255-minVal);
	
	//	create RGB values (and alpha  = 255) for each pixel
	for (int i=0; i<NUM_ROWS; i++)
	{
		for (int j=0; j<NUM_COLS; j++)
		{
			//	temp code to get some color initially
			unsigned char red = (unsigned char) (rand() % range + minVal);
			unsigned char green = (unsigned char) (rand() % range + minVal);
			unsigned char blue = (unsigned char) (rand() % range + minVal);
			grid[i][j] = 0xFF000000 | (blue << 16) | (green << 8) | red;
			
			//	the intialization you should use
//			grid[i][j] = 0xFF000000;
		}
	}
	
//	//	Enable this code if you want to do the traveler information
//	//	maintaining extra credit section
//	travelList = (TravelerInfo*) malloc(MAX_NUM_TRAVELER_THREADS * sizeof(TravelerInfo));
//	for (int k=0; k< MAX_NUM_TRAVELER_THREADS; k++)
//	{
//		travelList[k].type = rand() % NUM_TRAV_TYPES;
//		travelList[k].row = rand() % NUM_ROWS;
//		travelList[k].col = rand() % NUM_COLS;
//		travelList[k].dir = rand() % NUM_TRAVEL_DIRECTIONS;
//		travelList[k].isLive = 1;
//	}
}


