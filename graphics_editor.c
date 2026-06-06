/*
 * ============================================================================
 * 2D GRAPHICS EDITOR – College Mini Project
 * ============================================================================
 * Features:
 *  - Draw: point, line, circle (midpoint), ellipse (midpoint), rectangle,
 *    filled rectangle, filled circle, polygon (max 8 vertices)
 *  - Flood fill (boundary fill)
 *  - Freehand drawing (arrow keys + 'd' to draw / move)
 *  - Undo / Redo (stores last 10 canvas states)
 *  - Change drawing character (any ASCII printable)
 *  - Save canvas to file / Load canvas from file
 *  - Clear canvas, display canvas, exit
 *
 * Compile: gcc graphics_editor.c -o graphics_editor -lncurses -lm
 * Run: ./graphics_editor
 * ============================================================================
 */

#include <math.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------ Constants ------------------------------ */
#define HEIGHT 20
#define WIDTH 40
#define DEFAULT_EMPTY '_'
#define DEFAULT_FILL '*'

#define MAX_UNDO 10 /* maximum number of undo states */
#define MAX_POLY 8  /* maximum polygon vertices */

/* ------------------------------ Global Data ------------------------------ */
char canvas[HEIGHT][WIDTH];     /* current drawing canvas */
char fillChar = DEFAULT_FILL;   /* current drawing character */
char emptyChar = DEFAULT_EMPTY; /* background character */

void floodFill(int sx, int sy);

/* Undo / Redo stacks */
char undoStack[MAX_UNDO][HEIGHT][WIDTH];
int undoTop = -1;
int redoTop = -1;

/* ------------------------------ Helper Functions
 * ------------------------------ */
/* Save current canvas to undo stack (before making changes) */
void pushUndo(void) {
  if (undoTop == MAX_UNDO - 1) {
    /* shift all entries left to make room (drop oldest) */
    for (int i = 1; i < MAX_UNDO; i++)
      memcpy(undoStack[i - 1], undoStack[i], sizeof(undoStack[i]));
    undoTop--;
  }
  undoTop++;
  for (int i = 0; i < HEIGHT; i++)
    for (int j = 0; j < WIDTH; j++)
      undoStack[undoTop][i][j] = canvas[i][j];
  /* clear redo stack after a new action */
  redoTop = -1;
}

/* Undo: restore previous canvas state */
void undo(void) {
  if (undoTop > 0) {
    /* move current canvas to redo stack */
    redoTop++;
    for (int i = 0; i < HEIGHT; i++)
      for (int j = 0; j < WIDTH; j++)
        undoStack[redoTop][i][j] =
            canvas[i][j]; /* reuse undoStack as redo buffer */
    /* restore previous state */
    undoTop--;
    for (int i = 0; i < HEIGHT; i++)
      for (int j = 0; j < WIDTH; j++)
        canvas[i][j] = undoStack[undoTop][i][j];
  }
}

/* Redo: reapply a previously undone action */
void redo(void) {
  if (redoTop >= 0) {
    /* push current to undo */
    pushUndo(); /* this will clear redo stack, so we need a special redo buffer
                 */
                /* simpler: swap instead */
                /* We'll implement a proper redo buffer separately */
  }
}

/* Clean redo implementation (separate stack) */
char redoStack[MAX_UNDO][HEIGHT][WIDTH];
int redoCount = 0;

void pushUndoRedo(void) {
  if (undoTop == MAX_UNDO - 1) {
    for (int i = 1; i < MAX_UNDO; i++)
      memcpy(undoStack[i - 1], undoStack[i], sizeof(undoStack[i]));
    undoTop--;
  }
  undoTop++;
  memcpy(undoStack[undoTop], canvas, sizeof(canvas));
  redoCount = 0; /* new action invalidates redo */
}

void performUndo(void) {
  if (undoTop > 0) {
    /* save current to redo */
    memcpy(redoStack[redoCount], canvas, sizeof(canvas));
    redoCount++;
    /* restore previous undo */
    undoTop--;
    memcpy(canvas, undoStack[undoTop], sizeof(canvas));
  }
}

void performRedo(void) {
  if (redoCount > 0) {
    /* save current to undo */
    if (undoTop == MAX_UNDO - 1) {
      for (int i = 1; i < MAX_UNDO; i++)
        memcpy(undoStack[i - 1], undoStack[i], sizeof(undoStack[i]));
      undoTop--;
    }
    undoTop++;
    memcpy(undoStack[undoTop], canvas, sizeof(canvas));
    /* restore from redo */
    redoCount--;
    memcpy(canvas, redoStack[redoCount], sizeof(canvas));
  }
}

/* ------------------------------ Canvas Initialization
 * ------------------------------ */
void initCanvas(void) {
  for (int i = 0; i < HEIGHT; i++)
    for (int j = 0; j < WIDTH; j++)
      canvas[i][j] = emptyChar;
  pushUndoRedo(); /* initial state saved */
}

void clearCanvas(void) {
  for (int i = 0; i < HEIGHT; i++)
    for (int j = 0; j < WIDTH; j++)
      canvas[i][j] = emptyChar;
  pushUndoRedo();
}

/* ------------------------------ Drawing Primitives
 * ------------------------------ */
void drawPoint(int x, int y) {
  if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
    canvas[y][x] = fillChar;
}

void drawLine(int x0, int y0, int x1, int y1) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, e2;
  while (1) {
    drawPoint(x0, y0);
    if (x0 == x1 && y0 == y1)
      break;
    e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

/* Midpoint circle */
void drawCircle(int cx, int cy, int r) {
  int x = 0, y = r;
  int d = 1 - r;
  while (x <= y) {
    drawPoint(cx + x, cy + y);
    drawPoint(cx - x, cy + y);
    drawPoint(cx + x, cy - y);
    drawPoint(cx - x, cy - y);
    drawPoint(cx + y, cy + x);
    drawPoint(cx - y, cy + x);
    drawPoint(cx + y, cy - x);
    drawPoint(cx - y, cy - x);
    if (d < 0)
      d += 2 * x + 3;
    else {
      d += 2 * (x - y) + 5;
      y--;
    }
    x++;
  }
}

/* Filled circle using flood fill after drawing boundary */
void drawFilledCircle(int cx, int cy, int r) {
  /* draw the circle boundary first */
  pushUndoRedo(); /* save state before filling */
  drawCircle(cx, cy, r);
  /* choose a seed point inside */
  int seedX = cx, seedY = cy;
  if (seedX >= 0 && seedX < WIDTH && seedY >= 0 && seedY < HEIGHT)
    floodFill(seedX, seedY);
}

/* Midpoint ellipse */
void drawEllipse(int cx, int cy, int rx, int ry) {
  float dx, dy, d1, d2;
  int x = 0, y = ry;
  d1 = (ry * ry) - (rx * rx * ry) + (0.25 * rx * rx);
  dx = 2 * ry * ry * x;
  dy = 2 * rx * rx * y;
  while (dx < dy) {
    drawPoint(cx + x, cy + y);
    drawPoint(cx - x, cy + y);
    drawPoint(cx + x, cy - y);
    drawPoint(cx - x, cy - y);
    if (d1 < 0) {
      x++;
      dx += 2 * ry * ry;
      d1 += dx + (ry * ry);
    } else {
      x++;
      y--;
      dx += 2 * ry * ry;
      dy -= 2 * rx * rx;
      d1 += dx - dy + (ry * ry);
    }
  }
  d2 = ((ry * ry) * ((x + 0.5) * (x + 0.5))) +
       ((rx * rx) * ((y - 1) * (y - 1))) - (rx * rx * ry * ry);
  while (y >= 0) {
    drawPoint(cx + x, cy + y);
    drawPoint(cx - x, cy + y);
    drawPoint(cx + x, cy - y);
    drawPoint(cx - x, cy - y);
    if (d2 > 0) {
      y--;
      dy -= 2 * rx * rx;
      d2 += (rx * rx) - dy;
    } else {
      y--;
      x++;
      dx += 2 * ry * ry;
      dy -= 2 * rx * rx;
      d2 += dx - dy + (rx * rx);
    }
  }
}

/* Rectangle (outline) */
void drawRectangle(int x1, int y1, int x2, int y2) {
  drawLine(x1, y1, x2, y1);
  drawLine(x1, y2, x2, y2);
  drawLine(x1, y1, x1, y2);
  drawLine(x2, y1, x2, y2);
}

/* Filled rectangle (scan‑line like – simple row fill) */
void drawFilledRectangle(int x1, int y1, int x2, int y2) {
  int left = (x1 < x2) ? x1 : x2;
  int right = (x1 > x2) ? x1 : x2;
  int top = (y1 < y2) ? y1 : y2;
  int bottom = (y1 > y2) ? y1 : y2;
  for (int y = top; y <= bottom; y++)
    for (int x = left; x <= right; x++)
      drawPoint(x, y);
}

/* Polygon – connect list of vertices */
void drawPolygon(int points[][2], int n) {
  for (int i = 0; i < n; i++) {
    int j = (i + 1) % n;
    drawLine(points[i][0], points[i][1], points[j][0], points[j][1]);
  }
}

/* Flood fill (boundary fill) using iterative stack */
void floodFill(int sx, int sy) {
  if (sx < 0 || sx >= WIDTH || sy < 0 || sy >= HEIGHT)
    return;
  char target = canvas[sy][sx];
  if (target == fillChar)
    return;
  if (target == emptyChar)
    target = emptyChar; // boundary is anything not empty? Actually boundary is
                        // fillChar? No, boundary is fillChar.
// We fill all empty cells connected to seed, until we hit fillChar (boundary).
// But in typical flood fill, boundary is a different color. Here we treat
// fillChar as boundary.
#define MAX_STACK (HEIGHT * WIDTH)
  int stackX[MAX_STACK], stackY[MAX_STACK];
  int top = 0;
  stackX[top] = sx;
  stackY[top] = sy;
  top++;
  while (top > 0) {
    top--;
    int x = stackX[top], y = stackY[top];
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
      continue;
    if (canvas[y][x] == fillChar)
      continue;
    if (canvas[y][x] == fillChar)
      continue;
    canvas[y][x] = fillChar;
    stackX[top] = x + 1;
    stackY[top] = y;
    top++;
    stackX[top] = x - 1;
    stackY[top] = y;
    top++;
    stackX[top] = x;
    stackY[top] = y + 1;
    top++;
    stackX[top] = x;
    stackY[top] = y - 1;
    top++;
  }
}

/* ------------------------------ Freehand Drawing
 * ------------------------------ */
void freehandDraw(void) {
  int x = WIDTH / 2, y = HEIGHT / 2;
  int ch;
  curs_set(1);
  while (1) {
    mvprintw(HEIGHT + 2, 0,
             "Freehand mode: arrow keys to move, 'd' to draw, 'q' to quit");
    mvprintw(HEIGHT + 3, 0, "Current pos: (%2d,%2d)  ", x, y);
    refresh();
    ch = getch();
    if (ch == 'q')
      break;
    if (ch == KEY_UP && y > 0)
      y--;
    if (ch == KEY_DOWN && y < HEIGHT - 1)
      y++;
    if (ch == KEY_LEFT && x > 0)
      x--;
    if (ch == KEY_RIGHT && x < WIDTH - 1)
      x++;
    if (ch == 'd') {
      pushUndoRedo();
      drawPoint(x, y);
    }
    // redraw canvas each step for feedback
    clear();
    for (int i = 0; i < HEIGHT; i++) {
      for (int j = 0; j < WIDTH; j++)
        printw("%c", canvas[i][j]);
      printw("\n");
    }
  }
  curs_set(0);
}

/* ------------------------------ Display & File I/O
 * ------------------------------ */
void displayCanvas(void) {
  clear();
  for (int i = 0; i < HEIGHT; i++) {
    for (int j = 0; j < WIDTH; j++)
      printw("%c", canvas[i][j]);
    printw("\n");
  }
  printw("\nPress any key to continue...");
  refresh();
  getch();
}

void saveCanvasToFile(const char *fname) {
  FILE *fp = fopen(fname, "w");
  if (!fp) {
    mvprintw(HEIGHT + 2, 0, "Error opening file for writing!");
    refresh();
    getch();
    return;
  }
  for (int i = 0; i < HEIGHT; i++) {
    for (int j = 0; j < WIDTH; j++)
      fputc(canvas[i][j], fp);
    fputc('\n', fp);
  }
  fclose(fp);
  mvprintw(HEIGHT + 2, 0, "Canvas saved to %s", fname);
  refresh();
  getch();
}

void loadCanvasFromFile(const char *fname) {
  FILE *fp = fopen(fname, "r");
  if (!fp) {
    mvprintw(HEIGHT + 2, 0, "Error opening file for reading!");
    refresh();
    getch();
    return;
  }
  char newCanvas[HEIGHT][WIDTH];
  int row = 0;
  char line[WIDTH + 2];
  while (row < HEIGHT && fgets(line, sizeof(line), fp)) {
    int len = strlen(line);
    for (int col = 0; col < WIDTH && col < len; col++)
      newCanvas[row][col] = line[col];
    row++;
  }
  fclose(fp);
  if (row == HEIGHT) {
    pushUndoRedo();
    memcpy(canvas, newCanvas, sizeof(canvas));
    mvprintw(HEIGHT + 2, 0, "Canvas loaded from %s", fname);
  } else {
    mvprintw(HEIGHT + 2, 0, "Invalid canvas file (wrong dimensions)");
  }
  refresh();
  getch();
}

/* ------------------------------ User Input Helpers
 * ------------------------------ */
int getInt(const char *prompt) {
  int val;
  echo();
  curs_set(1);
  printw("%s", prompt);
  refresh();
  scanw("%d", &val);
  noecho();
  curs_set(0);
  return val;
}

void getCoords(const char *prompt, int *x, int *y) {
  *x = getInt("X coordinate: ");
  *y = getInt("Y coordinate: ");
}

/* ------------------------------ Main Menu ------------------------------ */
int main(void) {
  initCanvas();

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  int choice;
  do {
    clear();
    mvprintw(0, 0, "========== 2D GRAPHICS EDITOR ==========");
    mvprintw(1, 0, "1. Draw point         2. Draw line");
    mvprintw(2, 0, "3. Draw circle        4. Draw filled circle");
    mvprintw(3, 0, "5. Draw ellipse       6. Draw rectangle");
    mvprintw(4, 0, "7. Draw filled rect   8. Draw polygon");
    mvprintw(5, 0, "9. Flood fill        10. Freehand draw");
    mvprintw(6, 0, "11. Undo             12. Redo");
    mvprintw(7, 0, "13. Change fill char 14. Clear canvas");
    mvprintw(8, 0, "15. Save to file     16. Load from file");
    mvprintw(9, 0, "17. Display canvas   18. Exit");
    mvprintw(11, 0, "Current fill char: '%c'", fillChar);
    mvprintw(12, 0, "Choice: ");
    refresh();

    echo();
    scanw("%d", &choice);
    noecho();

    switch (choice) {
    case 1: { // point
      int x, y;
      clear();
      getCoords("", &x, &y);
      pushUndoRedo();
      drawPoint(x, y);
      break;
    }
    case 2: { // line
      int x0, y0, x1, y1;
      clear();
      x0 = getInt("x0: ");
      y0 = getInt("y0: ");
      x1 = getInt("x1: ");
      y1 = getInt("y1: ");
      pushUndoRedo();
      drawLine(x0, y0, x1, y1);
      break;
    }
    case 3: { // circle
      int cx, cy, r;
      clear();
      cx = getInt("Center x: ");
      cy = getInt("Center y: ");
      r = getInt("Radius: ");
      pushUndoRedo();
      drawCircle(cx, cy, r);
      break;
    }
    case 4: { // filled circle
      int cx, cy, r;
      clear();
      cx = getInt("Center x: ");
      cy = getInt("Center y: ");
      r = getInt("Radius: ");
      pushUndoRedo();
      drawCircle(cx, cy, r); // boundary
      floodFill(cx, cy);     // fill from center
      break;
    }
    case 5: { // ellipse
      int cx, cy, rx, ry;
      clear();
      cx = getInt("Center x: ");
      cy = getInt("Center y: ");
      rx = getInt("Radius x: ");
      ry = getInt("Radius y: ");
      pushUndoRedo();
      drawEllipse(cx, cy, rx, ry);
      break;
    }
    case 6: { // rectangle
      int x1, y1, x2, y2;
      clear();
      x1 = getInt("Left top x: ");
      y1 = getInt("Left top y: ");
      x2 = getInt("Right bottom x: ");
      y2 = getInt("Right bottom y: ");
      pushUndoRedo();
      drawRectangle(x1, y1, x2, y2);
      break;
    }
    case 7: { // filled rectangle
      int x1, y1, x2, y2;
      clear();
      x1 = getInt("Left top x: ");
      y1 = getInt("Left top y: ");
      x2 = getInt("Right bottom x: ");
      y2 = getInt("Right bottom y: ");
      pushUndoRedo();
      drawFilledRectangle(x1, y1, x2, y2);
      break;
    }
    case 8: { // polygon
      int n = getInt("Number of vertices (3-8): ");
      if (n < 3)
        n = 3;
      if (n > MAX_POLY)
        n = MAX_POLY;
      int points[MAX_POLY][2];
      clear();
      for (int i = 0; i < n; i++) {
        mvprintw(i + HEIGHT / 2, 0, "Vertex %d: ", i + 1);
        points[i][0] = getInt(" x: ");
        points[i][1] = getInt(" y: ");
      }
      pushUndoRedo();
      drawPolygon(points, n);
      break;
    }
    case 9: { // flood fill
      int x, y;
      clear();
      getCoords("Seed point", &x, &y);
      pushUndoRedo();
      floodFill(x, y);
      break;
    }
    case 10: { // freehand
      freehandDraw();
      break;
    }
    case 11: // undo
      performUndo();
      break;
    case 12: // redo
      performRedo();
      break;
    case 13: { // change fill char
      clear();
      echo();
      curs_set(1);
      printw("Enter new drawing character: ");
      refresh();
      fillChar = getch();
      noecho();
      curs_set(0);
      break;
    }
    case 14: // clear canvas
      pushUndoRedo();
      clearCanvas();
      break;
    case 15: { // save
      clear();
      echo();
      curs_set(1);
      char fname[256];
      printw("File name: ");
      refresh();
      scanw("%s", fname);
      noecho();
      curs_set(0);
      saveCanvasToFile(fname);
      break;
    }
    case 16: { // load
      clear();
      echo();
      curs_set(1);
      char fname[256];
      printw("File name: ");
      refresh();
      scanw("%s", fname);
      noecho();
      curs_set(0);
      loadCanvasFromFile(fname);
      break;
    }
    case 17: // display
      displayCanvas();
      break;
    case 18: // exit
      break;
    default:
      mvprintw(14, 0, "Invalid choice. Press any key.");
      refresh();
      getch();
    }
  } while (choice != 18);

  endwin();
  return 0;
}