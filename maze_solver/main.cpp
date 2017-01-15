//
//  main.cpp
//  maze_solver
//
//  Created by user on 2017-01-15.
//  Copyright © 2017 user. All rights reserved.
//

#include <iostream>
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGl/glu.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <random>
#include <unistd.h>


using namespace std;

///CONSTANTS
const unsigned short		\
SCR_X		= 750,	  // screen width
SCR_Y		= 750, 	  // screen height
SOLVE_SPEED = 0x0001,	  // solve animation
MOUSE_DT	= 0x0003;	  // SleepTime(ms)

const unsigned char			\
ROWS		= 0x20,		  // # rows
COLS		= 0x20,		  // # columns
INIT_X		= 0x00,		  // x offset
INIT_Y		= 0x00,		  // y offset
CEL_SIZ		= 0x0F,		  // cell size
LNE_W		= 0x01,		  // wall width
DIRECTIONS	= 0x04,		  // 4 = NESW
FLG_CLR     = 0b0000001,  // color flag
FLG_DIR     = 0b0000010;  // direction flg

///GLOBALS
unsigned char				\
eastWalls[ROWS][COLS],
northWalls[ROWS][COLS],
visited[ROWS][COLS];


///CALCULATIONS FOR POSITIONS IN CELL
#define	LEFT_X(c)			(INIT_X + (CEL_SIZ * c))
#define	RIGHT_X(c)			(INIT_X + (CEL_SIZ * c) + (CEL_SIZ))
#define	BOTTOM_Y(r)			(INIT_Y + (CEL_SIZ * r))
#define	TOP_Y(r)			(INIT_Y + (CEL_SIZ * r) + (CEL_SIZ))
#define MID_X(c)			((LEFT_X(c)+RIGHT_X(c))/2)
#define MID_Y(r)			((BOTTOM_Y(r)+TOP_Y(r))/2)
#define RED					1.0,0.0,0.0
#define BLACK				0.0,0.0,0.0
#define WHITE				1.0,1.0,1.0
#define BLUE				0.0,0.0,1.0
#define GREEN				0.0,1.0,0.0

///simple structure to hold cell position by row (r) and column (c)
typedef struct cell_rc;
struct cell_rc {
    unsigned short		r;
    unsigned short		c;
};

cell_rc						start_rc;
cell_rc						end_rc;

///* Code generates a random 16bit integer on a uniform normal distribution curve
///* Code was taken from a video on C++11 found by following the link below.  The
///* website was last accessed on 10/04/15
///* WS: https://channel9.msdn.com/Events/GoingNative/2013/rand-Considered-Harmful
unsigned short genRand(unsigned short rng_start, unsigned short rng_end) {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<unsigned short> dist(rng_start, rng_end);
    return dist(mt);
}

/// Simple swap function swaps 2 cell_rc values in an array
void swp(cell_rc *a, cell_rc *b) {
    cell_rc		t = *a;
    *a = *b;
    *b = t;
}

/// shuffles cell_rc values in an array with very simple algorithm
void shuf(cell_rc a[]) {
    for (char i = 0; i < DIRECTIONS; i++) {
        short j = genRand(0, i);
        swp(&a[i], &a[j]);
    }
}

/// draws a wall on the east of the cell
void drawWall(short r, short c, char flags) {
    /* flags = FLG_DIR -> 1 = N , 0 = E
     FLG_CLR -> 1 = B , 0 = W */
    if (flags & FLG_CLR) glColor3f(BLACK);
    else glColor3f(WHITE);
    
    glBegin(GL_LINES);
    glVertex2i((flags & FLG_DIR) ? LEFT_X(c) : RIGHT_X(c),
               (flags & FLG_DIR) ? TOP_Y(r) : TOP_Y(r));
    glVertex2i((flags & FLG_DIR) ? RIGHT_X(c) : RIGHT_X(c),
               (flags & FLG_DIR) ? TOP_Y(r) : BOTTOM_Y(r));
    glEnd();
    glFlush();
}

///Draw a dot at cell location (NOTE MUST CALL glColor**() before)
void drawMouse(short r, short c) {
    glPointSize((float)CEL_SIZ/2);
    glBegin(GL_POINTS);
    glVertex2i(MID_X(c), MID_Y(r));
    glEnd();
    glFlush();
}

///erases wall from cell frm to cell to
void makePath(cell_rc frm, cell_rc to) {
    if (frm.c == to.c) {
        northWalls[min(to.r, frm.r)][to.c] = 0;
        ///COMMENT OUT THE FOLLOWING LINE TO REMOVE ANIMATION
        drawWall(min(to.r, frm.r), to.c, FLG_DIR);
    }
    if (frm.r == to.r) {
        eastWalls [to.r][min(to.c, frm.c)] = 0;
        ///COMMENT OUT THE FOLLOWING LINE TO REMOVE ANIMATION
        drawWall(to.r, min(to.c, frm.c),NULL);
    }
}

///Check for a wall between cell from and cell to
///return true if there is no wall and false otherwise.
bool hasPath(cell_rc frm, cell_rc to) {
    if (frm.c == to.c) {
        if (frm.r < to.r) return !northWalls[frm.r][frm.c];
        else return !northWalls[to.r][to.c];
    }
    
    if (frm.r == to.r) {
        if (frm.c < to.c) return !eastWalls[frm.r][frm.c];
        else return !eastWalls[to.r][to.c];
    }
    
    return false;
}

///The name says it all
void drawMaze() {
    glClear(GL_COLOR_BUFFER_BIT);
    for (short r = 0; r < ROWS; r++) {
        for (short c = 0; c < COLS; c++) {
            if (eastWalls[r][c]) drawWall(r, c, FLG_CLR);
            if (northWalls[r][c]) drawWall(r, c, FLG_DIR|FLG_CLR);
        }
        
    }
}

///check if a cell has previously been on a path taken
///return true if it has and false otherwise
bool isVisited(cell_rc m) {
    return visited[m.r][m.c];
}

///check if current cell is a "dead end": no adjoining cells
/// have a possible path that have not already been visited
/// return true if "dead end" conditions met, false otherwise
bool isDeadEnd(cell_rc m) {
    cell_rc		north_rc = m,
				east_rc = m,
				south_rc = m,
				west_rc = m;
    
    north_rc.r++; south_rc.r--; east_rc.c++; west_rc.c--;
    
    return !((hasPath(m, north_rc) & !isVisited(north_rc)) || (hasPath(m, east_rc) & !isVisited(east_rc))
             || (hasPath(m, south_rc) & !isVisited(south_rc)) || (hasPath(m, west_rc) & !isVisited(west_rc)));
}

///reset paths taken (this way the maze can be run multiple times)
void clearVisited() {
    for (char r = 1; r < ROWS; r++) {
        for (char c = 1; c < COLS; c++)
            visited[r][c] = 0;
    }
}

///recursive function to create the maze. See beginning of comments
///for a detailed outline of the algorithm.
void eatWalls(cell_rc m) {
    cell_rc		north_rc = m,
				east_rc = m,
				south_rc = m,
				west_rc = m;
    
    north_rc.r++; south_rc.r--; east_rc.c++; west_rc.c--;
    
    visited[m.r][m.c] = 1;
    
    cell_rc dirs[] = { north_rc,south_rc,east_rc,west_rc }, n;
    shuf(dirs);
    
    usleep(MOUSE_DT);
    
    for (char i = 0; i < DIRECTIONS; i++) {
        n = dirs[i];
        if (n.r <= 0 || n.r >= ROWS || n.c <= 0 || n.c >= COLS) continue;
        if (!hasPath(m, n) && !visited[n.r][n.c]) {
            makePath(m, n);
            eatWalls(n);
        }
    }
}

///recursive function to solve the maze. See beginning of comments
///for a detailed outline of the algorithm.
bool solveMaze(cell_rc m) {
    cell_rc		north_rc = m,
				east_rc = m,
				south_rc = m,
				west_rc = m;
    
    north_rc.r++; south_rc.r--; east_rc.c++; west_rc.c--;
    
    visited[m.r][m.c] = 1;
    usleep(SOLVE_SPEED);
    
    if (m.r <= 0 || m.r >= ROWS || m.c <= 0 || m.c >= COLS) return false;
    else if (isDeadEnd(m)) return false;
    else if (m.r == end_rc.r && m.c == end_rc.c) {
        glColor3f(RED);
        drawMouse(m.r, m.c);
        return true;
    }
    else {
        cell_rc dirs[] = { east_rc,north_rc,west_rc,south_rc };
        shuf(dirs);
        glColor3f(BLUE);
        drawMouse(m.r, m.c);
        for (char i = 0; i < DIRECTIONS; i++) {
            if (hasPath(m,dirs[i]) && solveMaze(dirs[i])) return true;
        }
        
    }
    glColor3f(WHITE);
    drawMouse(m.r, m.c);
    return false;
}
/// Initialize variables and set openGL environment variables
void myInit(void) {
    glClearColor(WHITE, 0.0);
    glColor3f(BLACK);
    glPointSize(CEL_SIZ/2);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, (GLdouble)SCR_X, 0.0, (GLdouble)SCR_Y);
    glLineWidth(LNE_W);
    glClear(GL_COLOR_BUFFER_BIT);
    
    //initialize arrays for the north and east walls
    for (short r = 0; r < ROWS; r++) {
        for (short c = 0; c < COLS; c++) {
            if (r != 0) eastWalls[r][c] = 1;
            else { eastWalls[r][c] = 0; visited[r][c] = 1; }
            
            if (c != 0) northWalls[r][c] = 1;
            else { northWalls[r][c] = 0; visited[r][c] = 1; }
            
            visited[r][c] = 0;
        }
    }
}
/// render function for gl_loop
void myDisplay(void)
{
    drawMaze();
    cout << "Lets Make The Maze...";
    char _ = getchar();
    
    cell_rc	m = { genRand(1,ROWS), genRand(1,COLS) };
    start_rc = { genRand(1,ROWS), 0 };
    end_rc = { genRand(1,ROWS), COLS - 1 };
    
    eatWalls(m);
    
    drawWall(start_rc.r, start_rc.c, NULL);
    drawWall(end_rc.r,end_rc.c, NULL);
    
    start_rc.c++;
    glColor3f(GREEN);
    drawMouse(start_rc.r, start_rc.c);
    
    cout << "Lets Solve The Maze...";
    _ = getchar();
    
    clearVisited();
    solveMaze(start_rc);
    
    glColor3f(GREEN);
    drawMouse(start_rc.r, start_rc.c);
    cout << " THIS IS THE END!!!!!!!!!!! =)) " << endl;
    _ = getchar();
    exit(0);
    
}
//<<<<<<<<<<<<<<<<<<<<<<<< main >>>>>>>>>>>>>>>>>>>>>>
int main(int argc, char** argv)
{
    glutInit(&argc, argv); // initialize the toolkit
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB); // set display mode
    glutInitWindowSize(SCR_X, SCR_Y); // set window size
    glutInitWindowPosition(100, 50); // set window position on screen
    glutCreateWindow("Assignment 1 :: moro1422 :: Robert Morouney"); // open the screen window
    glutDisplayFunc(myDisplay); // register redraw function
    myInit();	// initialization function
    glutMainLoop(); // go into a perpetual loop
}
