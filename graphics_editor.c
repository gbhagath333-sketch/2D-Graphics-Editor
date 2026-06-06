#include <ncurses.h> // For the menu system
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define the canvas size. We'll use a 20x40 grid.
#define HEIGHT 20
#define WIDTH 40
#define EMPTY '_' // Character for empty canvas
#define FILL '*'  // Character for drawing
// Global canvas array
char canvas[HEIGHT][WIDTH];

// Function to clear the canvas
void clearCanvas() {
  for (int i = 0; i < HEIGHT; i++) {
    for (int j = 0; j < WIDTH; j++) {
      canvas[i][j] = EMPTY;
    }
  }
}

// Function to display the canvas using ncurses
void displayCanvas() {
  clear();

  for (int i = 0; i < HEIGHT; i++) {
    for (int j = 0; j < WIDTH; j++) {
      printw("%c ", canvas[i][j]);
    }
    printw("\n");
  }

  refresh();
  getch();
}
// Function to draw a point if it is within canvas bounds
void drawPoint(int x, int y) {
  if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
    canvas[y][x] = FILL;
  }
}
int main() {
  clearCanvas();

  for (int x = 5; x <= 25; x++) {
    drawPoint(x, 10);
  }

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);

  displayCanvas();

  endwin();
  return 0;
}