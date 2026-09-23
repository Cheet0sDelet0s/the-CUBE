#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string.h>
#include <math.h>

/* ===================== CONFIG ===================== */

#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

#define LEDS_PER_FACE 25
#define NUM_FACES 6
#define FACE_SIZE 5

/*
         ___
        | 5 |
 ___ ___|_v_|___
| 6 > 2>< 1 < 3 |
|___|___|_^_|___|
        | 4 |
        |___|

*/

// Face pins
#define FACE1_PIN 33
#define FACE2_PIN 25
#define FACE3_PIN 2
#define FACE4_PIN 26 
#define FACE5_PIN 13
#define FACE6_PIN 32

// Dock detect (top face removed = HIGH via pullup)
#define FACE6_DOCK_PIN 15

// battery voltage divider pin
#define BATTERY_PIN 39

// voltage divider values
#define R1 100000
#define R2 100000

#define MIC_PIN 27

bool dockState = false;
bool previousDockState = false;

// Buttons (external pulldown, HIGH when pressed)
#define LEFT_BUTTON 37
#define RIGHT_BUTTON 38
#define SELECT_BUTTON 36

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

#define MPU_ADDR 0x68
#define PWR_MGMT_1 0x6B
#define ACCEL_XOUT_H 0x3B
#define GYRO_XOUT_H 0x43
#define WHO_AM_I 0x75

// Raw accelerometer values
int16_t ax = 0;
int16_t ay = 0;
int16_t az = 0;

// Raw gyroscope values
int16_t gx = 0;
int16_t gy = 0;
int16_t gz = 0;

float batteryVoltage = 3.70;
int batteryTimer = 0;

const uint8_t dockDebounce = 10;
bool dockOpen = true;

/* ===================== LED ARRAYS ===================== */

CRGB face1[LEDS_PER_FACE];
CRGB face2[LEDS_PER_FACE];
CRGB face3[LEDS_PER_FACE];
CRGB face4[LEDS_PER_FACE];
CRGB face5[LEDS_PER_FACE];
CRGB face6[LEDS_PER_FACE];

CRGB *faces[NUM_FACES] = {
    face1, face2, face3, face4, face5, face6};

CRGB glider[5][5] = {
    {CRGB::Black, CRGB::White, CRGB::Black, CRGB::Black, CRGB::Black},
    {CRGB::Black, CRGB::Black, CRGB::White, CRGB::Black, CRGB::Black},
    {CRGB::White, CRGB::White, CRGB::White, CRGB::Black, CRGB::Black},
    {CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black},
    {CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black}};

CRGB smiley[5][5] = {
    {CRGB::Yellow, CRGB::Yellow, CRGB::Yellow, CRGB::Yellow, CRGB::Yellow},
    {CRGB::Yellow, CRGB::Black, CRGB::Yellow, CRGB::Black, CRGB::Yellow},
    {CRGB::Yellow, CRGB::Yellow, CRGB::Yellow, CRGB::Yellow, CRGB::Yellow},
    {CRGB::Yellow, CRGB::Black, CRGB::Yellow, CRGB::Black, CRGB::Yellow},
    {CRGB::Yellow, CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Yellow}};

CRGB num_one[5][5] = {
    {CRGB::Black, CRGB::Black, CRGB::White, CRGB::Black, CRGB::Black},
    {CRGB::Black, CRGB::White, CRGB::White, CRGB::Black, CRGB::Black},
    {CRGB::White, CRGB::Black, CRGB::White, CRGB::Black, CRGB::Black},
    {CRGB::Black, CRGB::Black, CRGB::White, CRGB::Black, CRGB::Black},
    {CRGB::White, CRGB::White, CRGB::White, CRGB::White, CRGB::White}};

CRGB num_two[5][5] = {
    {CRGB::White, CRGB::White, CRGB::White, CRGB::Black, CRGB::Black},
    {CRGB::Black, CRGB::Black, CRGB::Black, CRGB::White, CRGB::Black},
    {CRGB::White, CRGB::White, CRGB::White, CRGB::Black, CRGB::Black},
    {CRGB::White, CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black},
    {CRGB::White, CRGB::White, CRGB::White, CRGB::White, CRGB::White}};

CRGB num_three[5][5] = {
    {CRGB::White, CRGB::White, CRGB::White, CRGB::Black, CRGB::Black},
    {CRGB::Black, CRGB::Black, CRGB::Black, CRGB::White, CRGB::Black},
    {CRGB::White, CRGB::White, CRGB::White, CRGB::Black, CRGB::Black},
    {CRGB::Black, CRGB::Black, CRGB::Black, CRGB::White, CRGB::Black},
    {CRGB::White, CRGB::White, CRGB::White, CRGB::Black, CRGB::Black}};

CRGB num_four[5][5] = {
    {CRGB::White, CRGB::Black, CRGB::Black, CRGB::White, CRGB::Black},
    {CRGB::White, CRGB::Black, CRGB::Black, CRGB::White, CRGB::Black},
    {CRGB::White, CRGB::White, CRGB::White, CRGB::White, CRGB::White},
    {CRGB::Black, CRGB::Black, CRGB::Black, CRGB::White, CRGB::Black},
    {CRGB::Black, CRGB::Black, CRGB::Black, CRGB::White, CRGB::Black}};

CRGB num_five[5][5] = {
    {CRGB::White, CRGB::White, CRGB::White, CRGB::White, CRGB::White},
    {CRGB::White, CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black},
    {CRGB::White, CRGB::White, CRGB::White, CRGB::Black, CRGB::Black},
    {CRGB::Black, CRGB::Black, CRGB::Black, CRGB::White, CRGB::Black},
    {CRGB::White, CRGB::White, CRGB::White, CRGB::Black, CRGB::Black}};

CRGB num_six[5][5] = {
    {CRGB::Black, CRGB::White, CRGB::White, CRGB::White, CRGB::Black},
    {CRGB::White, CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black},
    {CRGB::White, CRGB::White, CRGB::White, CRGB::Black, CRGB::Black},
    {CRGB::White, CRGB::Black, CRGB::Black, CRGB::White, CRGB::Black},
    {CRGB::Black, CRGB::White, CRGB::White, CRGB::Black, CRGB::Black}};

// A 3D integer vector.
// All vectors here are axis-aligned and have components -1, 0, or 1.
struct Vec3
{
  float x;
  float y;
  float z;
};

struct VoxelData
{
  CRGB colour;
  bool empty;
};


// -----------------------------------------------------------------------------
// Cube geometry
// -----------------------------------------------------------------------------

// Face normals.
// These determine which side of the cube each face occupies.
//
//             +Y = face 5
//                  |
//       -X face 2 | +X face 3
//                  |
//             +Z = face 1
//             -Z = face 6
//             -Y = face 4
//
static const Vec3 faceNormal[NUM_FACES] = {
    {0, 0, 1},   // face 1: bottom
    {-1, 0, 0},  // face 2
    {1, 0, 0},   // face 3
    {0, -1, 0},  // face 4
    {0, 1, 0},   // face 5
    {0, 0, -1}   // face 6: top
};

static const Vec3 faceRow[NUM_FACES] = {
    {1, 0, 0},   // face 1
    {0, 0, -1},  // face 2: first row is at top
    {0, 0, -1},  // face 3: first row is at top
    {0, 0, -1},  // face 4: first row is at top
    {0, 0, -1},  // face 5: first row is at top
    {-1, 0, 0}   // face 6: first row points towards face 3
};

static const Vec3 faceCol[NUM_FACES] = {
    {0, -1, 0},  // face 1
    {0, -1, 0},  // face 2
    {0, 1, 0},   // face 3
    {1, 0, 0},   // face 4
    {-1, 0, 0},  // face 5
    {0, 1, 0}    // face 6
};

// -----------------------------------------------------------------------------
// Small vector helpers
// -----------------------------------------------------------------------------

bool vecEqual(Vec3 a, Vec3 b)
{
  return a.x == b.x &&
         a.y == b.y &&
         a.z == b.z;
}

Vec3 vecNeg(Vec3 a)
{
  return {-a.x, -a.y, -a.z};
}

// Find which face has a particular normal.
int findFace(Vec3 normal)
{
  for (int f = 0; f < NUM_FACES; f++)
  {
    if (vecEqual(faceNormal[f], normal))
      return f;
  }

  return -1;
}

// Find whether direction corresponds to +row, -row, +col or -col.
//
// Returns:
//   0 = +row
//   1 = -row
//   2 = +col
//   3 = -col
//
int findLocalDirection(int face, Vec3 direction)
{
  if (vecEqual(direction, faceRow[face]))
    return 0;

  if (vecEqual(direction, vecNeg(faceRow[face])))
    return 1;

  if (vecEqual(direction, faceCol[face]))
    return 2;

  if (vecEqual(direction, vecNeg(faceCol[face])))
    return 3;

  return -1;
}

// -----------------------------------------------------------------------------
// Move one cell in a single direction, including wrapping across cube faces.
//
// dr must be -1, 0, or +1
// dc must be -1, 0, or +1
//
// Exactly one of dr/dc should be non-zero.
// -----------------------------------------------------------------------------

void moveOneCell(
    int face,
    int row,
    int col,
    int dr,
    int dc,
    int &newFace,
    int &newRow,
    int &newCol)
{
  // Normal movement within the current face.
  int nr = row + dr;
  int nc = col + dc;

  if (nr >= 0 && nr < FACE_SIZE &&
      nc >= 0 && nc < FACE_SIZE)
  {

    newFace = face;
    newRow = nr;
    newCol = nc;
    return;
  }

  // ---------------------------------------------------------------------------
  // We crossed an edge.
  // ---------------------------------------------------------------------------

  Vec3 crossingDirection;

  if (dr == 1)
  {
    crossingDirection = faceRow[face];
  }
  else if (dr == -1)
  {
    crossingDirection = vecNeg(faceRow[face]);
  }
  else if (dc == 1)
  {
    crossingDirection = faceCol[face];
  }
  else
  {
    crossingDirection = vecNeg(faceCol[face]);
  }

  // The direction across the edge determines the neighbouring face.
  int targetFace = findFace(crossingDirection);

  if (targetFace < 0)
  {
    // Should never happen if the cube geometry is correct.
    newFace = face;
    newRow = row;
    newCol = col;
    return;
  }

  // ---------------------------------------------------------------------------
  // Determine which edge of the target face touches the old face.
  // ---------------------------------------------------------------------------

  Vec3 towardsOldFace = faceNormal[face];

  int edgeDirection =
      findLocalDirection(targetFace, towardsOldFace);

  // ---------------------------------------------------------------------------
  // Determine the coordinate running along the shared edge.
  // ---------------------------------------------------------------------------

  Vec3 sourceEdgeDirection;

  int edgeCoordinate;

  if (dr != 0)
  {
    // Crossing a top/bottom edge.
    sourceEdgeDirection = faceCol[face];
    edgeCoordinate = col;
  }
  else
  {
    // Crossing a left/right edge.
    sourceEdgeDirection = faceRow[face];
    edgeCoordinate = row;
  }

  // Is the edge coordinate a row or column on the new face?
  int targetEdgeAxis;

  if (vecEqual(sourceEdgeDirection, faceRow[targetFace]) ||
      vecEqual(sourceEdgeDirection, vecNeg(faceRow[targetFace])))
  {

    targetEdgeAxis = 0; // row
  }
  else
  {
    targetEdgeAxis = 1; // col
  }

  // ---------------------------------------------------------------------------
  // The cube may rotate the coordinate when we cross the edge.
  // ---------------------------------------------------------------------------

  Vec3 targetEdgeDirection =
      (targetEdgeAxis == 0)
          ? faceRow[targetFace]
          : faceCol[targetFace];

  if (!vecEqual(sourceEdgeDirection, targetEdgeDirection))
  {
    edgeCoordinate = FACE_SIZE - 1 - edgeCoordinate;
  }

  // ---------------------------------------------------------------------------
  // Put the cell onto the correct edge of the new face.
  // ---------------------------------------------------------------------------

  int targetRow = 0;
  int targetCol = 0;

  switch (edgeDirection)
  {

  case 0: // +row points towards old face
    targetRow = 0;
    break;

  case 1: // -row points towards old face
    targetRow = FACE_SIZE - 1;
    break;

  case 2: // +col points towards old face
    targetCol = 0;
    break;

  case 3: // -col points towards old face
    targetCol = FACE_SIZE - 1;
    break;
  }

  // Fill the coordinate along the edge.
  if (targetEdgeAxis == 0)
    targetRow = edgeCoordinate;
  else
    targetCol = edgeCoordinate;

  newFace = targetFace;
  newRow = targetRow;
  newCol = targetCol;
}

// -----------------------------------------------------------------------------
// Get a neighbouring cell, including wrapping across faces.
//
// For diagonal neighbours we perform the movement as two individual
// movements. This is important at cube corners, where a diagonal can
// cross two face boundaries.
//
// dr, dc = -1, 0, or +1
// -----------------------------------------------------------------------------

void getNeighbour(
    int face,
    int row,
    int col,
    int dr,
    int dc,
    int &neighbourFace,
    int &neighbourRow,
    int &neighbourCol)
{
  int f = face;
  int r = row;
  int c = col;

  // ---------------------------------------------------------------------------
  // Move in the row direction first.
  // ---------------------------------------------------------------------------

  if (dr != 0)
  {
    moveOneCell(
        f, r, c,
        dr, 0,
        f, r, c);
  }

  // ---------------------------------------------------------------------------
  // Then move in the column direction.
  //
  // This second movement can itself cross another face boundary.
  // ---------------------------------------------------------------------------

  if (dc != 0)
  {
    moveOneCell(
        f, r, c,
        0, dc,
        f, r, c);
  }

  neighbourFace = f;
  neighbourRow = r;
  neighbourCol = c;
}

void gameOfLifeTick()
{

  // ===========================================================================
  // COLOUR PALETTE
  // ===========================================================================

  const CRGB colourPalette[] = {
      CRGB::Red,
      CRGB::Green,
      CRGB::Blue,
      CRGB::Yellow,
      CRGB::Cyan,
      CRGB::Magenta,
      CRGB::Orange,
      CRGB::Purple};

  const int NUM_COLOURS =
      sizeof(colourPalette) / sizeof(colourPalette[0]);

  // ===========================================================================
  // PERSISTENT SIMULATION DATA
  // ===========================================================================

  static bool initialised = false;

  static bool alive[NUM_FACES][LEDS_PER_FACE];
  static bool nextAlive[NUM_FACES][LEDS_PER_FACE];

  // Instead of storing RGB values for every cell, store a colour ID.
  //
  // For example:
  //   0 = Red
  //   1 = Green
  //   2 = Blue
  //   etc.
  //
  // This means colours can be compared exactly.
  static uint8_t cellColourID[NUM_FACES][LEDS_PER_FACE];
  static uint8_t nextColourID[NUM_FACES][LEDS_PER_FACE];

  static int unchangedTicks = 0;

  // Restart after this many ticks with no changes.
  const int restartAfterTicks = 10;

  // ===========================================================================
  // FIRST CALL: CREATE RANDOM STARTING POPULATION
  // ===========================================================================

  if (!initialised || (!dockState && previousDockState))
  {

    clearCube();

    // -------------------------------------------------------------------------
    // How full the cube starts.
    //
    // 0.25 = approximately 25% of all cells start alive.
    // -------------------------------------------------------------------------

    const float initialDensity = 0.25;

    // -------------------------------------------------------------------------
    // Start with everything dead.
    // -------------------------------------------------------------------------

    for (int f = 0; f < NUM_FACES; f++)
    {

      for (int i = 0; i < LEDS_PER_FACE; i++)
      {

        alive[f][i] = false;
        nextAlive[f][i] = false;

        cellColourID[f][i] = 0;
        nextColourID[f][i] = 0;
      }
    }

    // -------------------------------------------------------------------------
    // Create a list of all cells and randomly shuffle it.
    //
    // This prevents the same cell being selected twice.
    // -------------------------------------------------------------------------

    const int totalCells =
        NUM_FACES * LEDS_PER_FACE;

    int cellOrder[totalCells];

    for (int i = 0; i < totalCells; i++)
    {
      cellOrder[i] = i;
    }

    // Fisher-Yates shuffle.
    for (int i = totalCells - 1; i > 0; i--)
    {

      int j = random(i + 1);

      int temp = cellOrder[i];
      cellOrder[i] = cellOrder[j];
      cellOrder[j] = temp;
    }

    // Number of cells that will initially be alive.
    int cellsToAdd =
        totalCells * initialDensity;

    // =========================================================================
    // REVEAL STARTING CELLS ONE AT A TIME
    // =========================================================================

    for (int n = 0; n < cellsToAdd; n++)
    {

      int cell = cellOrder[n];

      int f =
          cell / LEDS_PER_FACE;

      int index =
          cell % LEDS_PER_FACE;

      // -----------------------------------------------------------------------
      // Make this cell alive.
      // -----------------------------------------------------------------------

      alive[f][index] = true;

      // -----------------------------------------------------------------------
      // Give it a random colour.
      // -----------------------------------------------------------------------

      cellColourID[f][index] =
          random(NUM_COLOURS);

      // -----------------------------------------------------------------------
      // Draw the entire cube.
      // -----------------------------------------------------------------------

      for (int drawFace = 0;
           drawFace < NUM_FACES;
           drawFace++)
      {

        for (int row = 0;
             row < FACE_SIZE;
             row++)
        {

          for (int col = 0;
               col < FACE_SIZE;
               col++)
          {

            int physicalRow = row;
            int physicalCol = col;

            // Face 1 is physically mounted 180 degrees rotated.
            if (drawFace == 0)
            {

              physicalRow =
                  FACE_SIZE - 1 - row;

              physicalCol =
                  FACE_SIZE - 1 - col;
            }

            int physicalIndex =
                physicalRow * FACE_SIZE + physicalCol;

            int logicalIndex =
                row * FACE_SIZE + col;

            if (alive[drawFace][logicalIndex])
            {

              faces[drawFace][physicalIndex] =
                  colourPalette[cellColourID[drawFace][logicalIndex]];
            }
            else
            {

              faces[drawFace][physicalIndex] =
                  CRGB::Black;
            }
          }
        }
      }

      FastLED.show();

      // Delay between each new starting cell.
      delay(40);
    }

    initialised = true;
  }

  // ===========================================================================
  // CALCULATE NEXT GENERATION
  // ===========================================================================

  for (int f = 0; f < NUM_FACES; f++)
  {

    for (int row = 0; row < FACE_SIZE; row++)
    {

      for (int col = 0; col < FACE_SIZE; col++)
      {

        int index =
            row * FACE_SIZE + col;

        // ---------------------------------------------------------------------
        // Count live neighbours.
        // ---------------------------------------------------------------------

        int neighbours = 0;

        // ---------------------------------------------------------------------
        // Count how many times each colour occurs amongst the neighbours.
        //
        // For example:
        //
        //   Red   Red   Blue
        //
        // produces:
        //
        //   Red  = 2
        //   Blue = 1
        //
        // so Red wins.
        // ---------------------------------------------------------------------

        int colourCounts[NUM_COLOURS];

        for (int colour = 0;
             colour < NUM_COLOURS;
             colour++)
        {

          colourCounts[colour] = 0;
        }

        // ---------------------------------------------------------------------
        // Examine all eight neighbouring cells.
        // ---------------------------------------------------------------------

        for (int dr = -1; dr <= 1; dr++)
        {

          for (int dc = -1; dc <= 1; dc++)
          {

            // Don't count the cell itself.
            if (dr == 0 && dc == 0)
              continue;

            int nf;
            int nr;
            int nc;

            getNeighbour(
                f,
                row,
                col,
                dr,
                dc,
                nf,
                nr,
                nc);

            int neighbourIndex =
                nr * FACE_SIZE + nc;

            if (alive[nf][neighbourIndex])
            {

              neighbours++;

              // Record the colour of this neighbour.
              uint8_t neighbourColour =
                  cellColourID[nf][neighbourIndex];

              colourCounts[neighbourColour]++;
            }
          }
        }

        // ---------------------------------------------------------------------
        // Conway's Game of Life rules.
        // ---------------------------------------------------------------------

        bool currentlyAlive =
            alive[f][index];

        bool willLive =
            (currentlyAlive &&
             (neighbours == 2 || neighbours == 3)) ||

            (!currentlyAlive &&
             neighbours == 3);

        nextAlive[f][index] =
            willLive;

        // =====================================================================
        // DETERMINE COLOUR
        // =====================================================================

        if (willLive)
        {

          // -------------------------------------------------------------------
          // EXISTING CELL SURVIVED
          //
          // It keeps its original colour.
          // -------------------------------------------------------------------

          if (currentlyAlive)
          {

            nextColourID[f][index] =
                cellColourID[f][index];
          }

          // -------------------------------------------------------------------
          // NEW CELL WAS BORN
          //
          // It gets the colour used by the majority of its three neighbours.
          // -------------------------------------------------------------------

          else
          {

            int mostCommonColour = 0;
            int highestCount = 0;

            // Find the highest number of occurrences.
            for (int colour = 0;
                 colour < NUM_COLOURS;
                 colour++)
            {

              if (colourCounts[colour] > highestCount)
              {

                highestCount =
                    colourCounts[colour];

                mostCommonColour =
                    colour;
              }
            }

            // -----------------------------------------------------------------
            // If all three parents have different colours, there is no
            // majority.
            //
            // Example:
            //
            //   Red + Blue + Green
            //
            // Randomly select one of the three.
            // -----------------------------------------------------------------

            if (highestCount == 1)
            {

              int choices[3];
              int numChoices = 0;

              for (int colour = 0;
                   colour < NUM_COLOURS;
                   colour++)
              {

                if (colourCounts[colour] == 1)
                {

                  choices[numChoices] =
                      colour;

                  numChoices++;
                }
              }

              mostCommonColour =
                  choices[random(numChoices)];
            }

            nextColourID[f][index] =
                mostCommonColour;
          }
        }

        // ---------------------------------------------------------------------
        // CELL DIED
        //
        // Its colour no longer matters.
        // ---------------------------------------------------------------------

        else
        {

          nextColourID[f][index] = 0;
        }
      }
    }
  }

  // ===========================================================================
  // CHECK WHETHER THE GENERATION CHANGED
  // ===========================================================================

  bool changed = false;

  for (int f = 0;
       f < NUM_FACES && !changed;
       f++)
  {

    for (int i = 0;
         i < LEDS_PER_FACE;
         i++)
    {

      if (alive[f][i] != nextAlive[f][i])
      {

        changed = true;
        break;
      }
    }
  }

  if (changed)
  {

    unchangedTicks = 0;
  }

  else
  {

    unchangedTicks++;

    if (unchangedTicks >= restartAfterTicks)
    {

      // Force the initialisation code to run again.
      initialised = false;

      unchangedTicks = 0;

      return;
    }
  }

  // ===========================================================================
  // COPY NEXT GENERATION INTO CURRENT GENERATION
  // ===========================================================================

  for (int f = 0;
       f < NUM_FACES;
       f++)
  {

    for (int i = 0;
         i < LEDS_PER_FACE;
         i++)
    {

      alive[f][i] =
          nextAlive[f][i];

      cellColourID[f][i] =
          nextColourID[f][i];
    }
  }

  // ===========================================================================
  // DRAW THE RESULT
  // ===========================================================================

  for (int f = 0;
       f < NUM_FACES;
       f++)
  {

    for (int row = 0;
         row < FACE_SIZE;
         row++)
    {

      for (int col = 0;
           col < FACE_SIZE;
           col++)
      {

        int physicalRow = row;
        int physicalCol = col;

        // Face 1 is physically mounted 180 degrees rotated.
        if (f == 0)
        {

          physicalRow =
              FACE_SIZE - 1 - row;

          physicalCol =
              FACE_SIZE - 1 - col;
        }

        int physicalIndex =
            physicalRow * FACE_SIZE + physicalCol;

        int logicalIndex =
            row * FACE_SIZE + col;

        if (alive[f][logicalIndex])
        {

          faces[f][physicalIndex] =
              colourPalette[cellColourID[f][logicalIndex]];
        }

        else
        {

          faces[f][physicalIndex] =
              CRGB::Black;
        }
      }
    }
  }

  // Delay between generations.
  delay(100);
}

// ============================================================
// CONFIGURATION
// ============================================================

const int SIZE = 7;

const int TOTAL_PARTICLES = 1470;

const int CELL_COUNT = SIZE * SIZE * SIZE;

const int CELL_CAPACITY = 10; // the maximum amount of particles that can enter one cell. if a particle tries to enter a cell that is full, it will bounce off.

Vec3 gravity = { -1.0f, 0.0f, 0.0f };

struct WaterParticle {
  // particle coordinates
  uint8_t x;
  uint8_t y;
  uint8_t z;

  // particle vector/velocity
  Vec3 vel;
};

unsigned long previousMillis = 0; 
const long interval = 30;

// ============================================================
// FLUID GRID
// ============================================================

int8_t water[SIZE][SIZE][SIZE];

WaterParticle particles[TOTAL_PARTICLES];

void printSlice(int x) {
  Serial.print("Cube slice X: ");
  Serial.println(x);
  for (int y = 0; y < SIZE; y++) {
    for (int z = 0; z < SIZE; z++) {    
      Serial.print(water[x][y][z]);
      Serial.print('\t');
    }
    Serial.println();
  }
}

Vec3 normalize(Vec3 v) {

    float length = sqrt(
        v.x * v.x +
        v.y * v.y +
        v.z * v.z
    );

    if (length < 0.000001f) {
        return { 0.0f, 0.0f, 0.0f };
    }

    return {
        v.x / length,
        v.y / length,
        v.z / length
    };
}

Vec3 add(Vec3 a, Vec3 b) {
    return {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z
    };
}

Vec3 subtract(Vec3 a, Vec3 b) {
    return {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z
    };
}

bool vectorsAreEqual(Vec3 a, Vec3 b) {
  if (a.x == b.x && a.y == b.y && a.z == b.z) {
    return true;
  }
  return false;
}

void initialiseWater() {
  for (int index = 0; index < TOTAL_PARTICLES; index++) {
    WaterParticle newParticle = WaterParticle(random(1, SIZE), random(0, SIZE), random(0, SIZE), Vec3(-1.0f, 0.0f, 0.0f));
    particles[index] = newParticle;
  }

  updateCells();
} 

void updateCells() {
  for (int x = 0; x < SIZE; x++) {
    for (int y = 0; y < SIZE; y++) {
      for (int z = 0; z < SIZE; z++) {
        water[x][y][z] = 0;
      }
    }
  }

  for (int index = 0; index < TOTAL_PARTICLES; index++) {
    WaterParticle cPart = particles[index]; // current particle
    
    if (cPart.x != 200) {
      water[cPart.x][cPart.y][cPart.z] += 1;
    }
  }
}

void updateParticles() {

  // All possible neighbouring voxel directions.
  //
  // This includes:
  //   6 cardinal directions
  //   12 edge diagonals
  //   8 corner diagonals
  //
  // This is what allows the water to follow arbitrary gravity
  // directions instead of being restricted to X/Y/Z.
  const int NUM_DIRECTIONS = 26;

  const int directions[NUM_DIRECTIONS][3] = {
    // Cardinal
    { 1,  0,  0},
    {-1,  0,  0},
    { 0,  1,  0},
    { 0, -1,  0},
    { 0,  0,  1},
    { 0,  0, -1},

    // XY diagonals
    { 1,  1,  0},
    { 1, -1,  0},
    {-1,  1,  0},
    {-1, -1,  0},

    // XZ diagonals
    { 1,  0,  1},
    { 1,  0, -1},
    {-1,  0,  1},
    {-1,  0, -1},

    // YZ diagonals
    { 0,  1,  1},
    { 0,  1, -1},
    { 0, -1,  1},
    { 0, -1, -1},

    // XYZ diagonals
    { 1,  1,  1},
    { 1,  1, -1},
    { 1, -1,  1},
    { 1, -1, -1},
    {-1,  1,  1},
    {-1,  1, -1},
    {-1, -1,  1},
    {-1, -1, -1}
  };


  // Calculate how well a direction agrees with gravity.
  //
  // Because gravity is normalized, this is essentially the cosine
  // of the angle between the direction and gravity.
  //
  //  1.0 = exactly downhill
  //  0.0 = perpendicular to gravity
  // -1.0 = directly uphill
  auto gravityScore = [&](int direction) -> float {

    float x = directions[direction][0];
    float y = directions[direction][1];
    float z = directions[direction][2];

    float length = sqrt(x * x + y * y + z * z);

    x /= length;
    y /= length;
    z /= length;

    return x * gravity.x +
           y * gravity.y +
           z * gravity.z;
  };


  for (int index = 0; index < TOTAL_PARTICLES; index++) {
    WaterParticle* cPart = &particles[index];

    if (cPart->x != 200) {
      // ------------------------------------------------------------
      // Update velocity.
      // ------------------------------------------------------------

      cPart->vel = normalize(add(cPart->vel, gravity));


      // ------------------------------------------------------------
      // Store old position.
      // ------------------------------------------------------------

      int oldX = cPart->x;
      int oldY = cPart->y;
      int oldZ = cPart->z;

      Vec3 oldPos = Vec3(oldX, oldY, oldZ);


      // ------------------------------------------------------------
      // Calculate desired new position.
      //
      // Unlike the old gravity-axis system, this uses the complete
      // continuous velocity vector.
      // ------------------------------------------------------------

      int newX = round(oldX + cPart->vel.x);
      int newY = round(oldY + cPart->vel.y);
      int newZ = round(oldZ + cPart->vel.z);


      // Keep inside the cube.
      newX = constrain(newX, 0, SIZE - 1);
      newY = constrain(newY, 0, SIZE - 1);
      newZ = constrain(newZ, 0, SIZE - 1);

      Vec3 newPos = Vec3(newX, newY, newZ);


      // ------------------------------------------------------------
      // If the particle didn't move, nothing else needs to happen.
      // ------------------------------------------------------------

      if (vectorsAreEqual(newPos, oldPos)) {
        continue;
      }


      int searchStatus = 0;
      // 0 = searching
      // 1 = found a cell
      // 2 = cannot move


      // ------------------------------------------------------------
      // Check the initially requested cell.
      // ------------------------------------------------------------

      int8_t newCell = water[newX][newY][newZ];

      if (newCell < CELL_CAPACITY) {

        searchStatus = 1;

      } else {

        // ----------------------------------------------------------
        // The desired cell is full.
        //
        // Find the direction which most closely follows gravity.
        // ----------------------------------------------------------

        int bestDirection = 0;
        float bestScore = -1000.0f;

        for (int i = 0; i < NUM_DIRECTIONS; i++) {

          float score = gravityScore(i);

          if (score > bestScore) {
            bestScore = score;
            bestDirection = i;
          }
        }


        // ----------------------------------------------------------
        // Search progressively farther in the downhill direction.
        //
        // This retains the useful behaviour from your original code:
        // if a cell below is full, look farther down.
        //
        // Because bestDirection can now be diagonal, this can also
        // search diagonally.
        // ----------------------------------------------------------

        int belowX = newX;
        int belowY = newY;
        int belowZ = newZ;

        bool foundBelow = false;

        while (true) {

          belowX += directions[bestDirection][0];
          belowY += directions[bestDirection][1];
          belowZ += directions[bestDirection][2];


          // Stop when we leave the cube.
          if (belowX < 0 || belowX >= SIZE ||
              belowY < 0 || belowY >= SIZE ||
              belowZ < 0 || belowZ >= SIZE) {

            break;
          }


          // Found an available cell.
          if (water[belowX][belowY][belowZ] < CELL_CAPACITY) {

            newX = belowX;
            newY = belowY;
            newZ = belowZ;

            newPos = Vec3(newX, newY, newZ);

            foundBelow = true;
            break;
          }
        }


        if (foundBelow) {

          searchStatus = 1;

        } else {

          // --------------------------------------------------------
          // Direct downhill path is blocked.
          //
          // Try all 26 surrounding cells.
          //
          // Rather than choosing a dominant axis, rank them according
          // to how closely their direction follows the actual
          // continuous gravity vector.
          // --------------------------------------------------------

          int candidateDirections[NUM_DIRECTIONS];
          float candidateScores[NUM_DIRECTIONS];

          int candidateCount = 0;


          for (int i = 0; i < NUM_DIRECTIONS; i++) {

            int testX = oldX + directions[i][0];
            int testY = oldY + directions[i][1];
            int testZ = oldZ + directions[i][2];


            // Ignore cells outside the cube.
            if (testX < 0 || testX >= SIZE ||
                testY < 0 || testY >= SIZE ||
                testZ < 0 || testZ >= SIZE) {

              continue;
            }


            // Ignore full cells.
            if (water[testX][testY][testZ] >= CELL_CAPACITY) {
              continue;
            }


            candidateDirections[candidateCount] = i;
            candidateScores[candidateCount] = gravityScore(i);

            candidateCount++;
          }


          // --------------------------------------------------------
          // No neighbouring cells are available.
          // --------------------------------------------------------

          if (candidateCount == 0) {

            searchStatus = 2;

          } else {

            // ------------------------------------------------------
            // Find the best score.
            // ------------------------------------------------------

            float highestScore = -1000.0f;

            for (int i = 0; i < candidateCount; i++) {

              if (candidateScores[i] > highestScore) {
                highestScore = candidateScores[i];
              }
            }


            // ------------------------------------------------------
            // There may be several directions with almost identical
            // gravity alignment.
            //
            // For example, at exactly 45 degrees:
            //
            //     gravity = (0.707, 0.707, 0)
            //
            // +X and +Y are equally good.
            //
            // Allow a little randomness between similarly good
            // directions rather than always choosing whichever one
            // happens to appear first in the array.
            // ------------------------------------------------------

            const float SIMILARITY_THRESHOLD = 0.10f;

            int goodDirections[NUM_DIRECTIONS];
            int goodCount = 0;

            for (int i = 0; i < candidateCount; i++) {

              if (candidateScores[i] >=
                  highestScore - SIMILARITY_THRESHOLD) {

                goodDirections[goodCount] =
                  candidateDirections[i];

                goodCount++;
              }
            }


            // Pick one of the similarly good directions.
            int selectedIndex = random(0, goodCount);

            int selectedDirection =
              goodDirections[selectedIndex];


            // ------------------------------------------------------
            // Move into the selected cell.
            // ------------------------------------------------------

            newX = oldX + directions[selectedDirection][0];
            newY = oldY + directions[selectedDirection][1];
            newZ = oldZ + directions[selectedDirection][2];

            newPos = Vec3(newX, newY, newZ);

            searchStatus = 1;
          }
        }
      }


      // ------------------------------------------------------------
      // Move the particle if a destination was found.
      // ------------------------------------------------------------

      if (searchStatus == 1) {

        cPart->x = newX;
        cPart->y = newY;
        cPart->z = newZ;


        // Remove particle from old cell.
        water[oldX][oldY][oldZ] -= 1;

        if (dockOpen && newX == 0) {
          cPart->x = 200;
        } else {
          // Add particle to new cell.
          water[newX][newY][newZ] += 1;
        }
      }
    }
  }
}

// void updateParticles() {
//   for (int index = 0; index < TOTAL_PARTICLES; index++) {
//     // find particle
//     WaterParticle* cPart = &particles[index]; // current particle

//     // add gravity to the particles vector
//     cPart->vel = normalize(add(cPart->vel, gravity));
    
//     // store current position
//     int oldX = cPart->x;
//     int oldY = cPart->y;
//     int oldZ = cPart->z;
    
//     // store as vector for ezness
//     Vec3 oldPos = Vec3(oldX, oldY, oldZ);

//     // apply vector to position
//     int newX = round(oldX + cPart->vel.x);
//     int newY = round(oldY + cPart->vel.y);
//     int newZ = round(oldZ + cPart->vel.z);

//     // constrain position within the cube
//     newX = constrain(newX, 0, SIZE - 1);
//     newY = constrain(newY, 0, SIZE - 1);
//     newZ = constrain(newZ, 0, SIZE - 1);

//     // store as vector for ezness
//     Vec3 newPos = Vec3(newX, newY, newZ);

//     // if the particle hasn't moved, don't continue
//     if (!vectorsAreEqual(newPos, oldPos)) {
//       int searchStatus = 0;  // 0 = searching, 1 = found open cell, 2 = nowhere to go, must remain stationary.

//       while (searchStatus == 0) {
//         // calculate how far the particle has moved
//         Vec3 distance = subtract(newPos, oldPos);

//         // get the cell that this particle is attempting to enter
//         int8_t newCell = water[newX][newY][newZ];

//         // if this cell is full, the particle cannot enter and must instead move around it
//         if (newCell >= CELL_CAPACITY) {
//           // get the direction the particle has moved most in
//           int directionType = 1; // 1 = x, 2 = y, 3 = z
//           float maxValue = abs(distance.x);
          
//           if (abs(distance.y) > maxValue) {
//             directionType = 2;
//             maxValue = abs(distance.y);
//           }

//           if (abs(distance.z) > maxValue) {
//             directionType = 3;
//             maxValue = abs(distance.z);
//           }

//           int movement = 0; // prepare movement variable

//           // step backwards in the largest direction to attempt to find an empty cell around the current full one
//           switch (directionType) {
//             case 1: // if the largest movement was X...
//               // turn the X movement into the lowest form of itself, while retaining its sign
//               // eg if the movement in X was -2, it would become -1
//               movement = distance.x / abs(distance.x);
//               // then add this to the new position
//               newX -= movement;
//               newX = constrain(newX, 0, SIZE - 1);
//               break;
//             case 2: // if the largest movement was Y...
//               // same thing we did for X
//               movement = distance.y / abs(distance.y);
//               newY -= movement;
//               newY = constrain(newY, 0, SIZE - 1);
//               break;
//             case 3: // if the largest movement was Z...
//               // and again
//               movement = distance.z / abs(distance.z);
//               newZ -= movement;
//               newZ = constrain(newZ, 0, SIZE - 1);
//               break;
//           }
//           // update the newPos vector
//           newPos = Vec3(newX, newY, newZ);
//           // check if we have now returned to where we started
//           if (vectorsAreEqual(newPos, oldPos)) {
//             // the particle has been trapped, so it cannot enter a new cell and must remain where it is.
//             searchStatus = 2;
//           }
//           // then repeat the whole thing with the new position

//         } else { // the cell is not full and can be entered by the particle.
//           searchStatus = 1; 
//         }
//       }
//       // check whether we declared this new cell can be entered, or if there is no new cell since the particle is trapped.
//       if (searchStatus == 1) {
//         // the new cell can be entered. set the particles coordinates to the new cell.
//         cPart->x = newX;
//         cPart->y = newY;
//         cPart->z = newZ;

//         // remove 1 to the previous cells' reported particle count
//         water[oldX][oldY][oldZ] -= 1;
//         // add 1 to the new cells' reported particle count
//         water[newX][newY][newZ] += 1;
//       }
//     }
//   }
// }

void setGravity(Vec3 vector) {
  gravity = normalize(vector);
}

VoxelData voxelToColour(int8_t voxel) {
  // emptyish cells are black - stops flickering of like 1 particle moving across the surface
  if (voxel <= 1) {
    return VoxelData(CRGB(0, 0, 0), true);
  }

  // Calculate how full the cell is as a percentage.
  float fill = (float)voxel / CELL_CAPACITY;

  // Low fill levels look white-ish to represent foam/bubbles.
  if (fill <= 0.2) {
    // 0% -> white, 20% -> blue.
    float foam = fill / 0.2;

    uint8_t red = 255 * (1.0 - foam) + 40 * foam;
    uint8_t green = 255 * (1.0 - foam) + 40 * foam;
    uint8_t blue = 255;

    return VoxelData(CRGB(red, green, blue), false);
  }

  // Above 20% full, scale the blue brightness with the amount of water.
  float waterFill = (fill - 0.2) / 0.8;

  uint8_t red = 10 + (30 * waterFill);
  uint8_t green = 10 + (70 * waterFill);
  uint8_t blue = 50 + (205 * waterFill);

  return VoxelData(CRGB(red, green, blue), false);
}

void drawWater() {
  fadeAll(85);

  // FACE 6
  // x = 0
  for (int y = 1; y < SIZE - 1; y++) {
    for (int z = 1; z < SIZE - 1; z++) {
      VoxelData data = voxelToColour(water[0][y][z]);

      if (!data.empty) {
        face6[(5 - z) * 5 + (y - 1)] = data.colour;
      }
    }
  }

  // FACE 5
  // y = 0
  for (int x = 1; x < SIZE - 1; x++) {
    for (int z = 1; z < SIZE - 1; z++) {
      VoxelData data = voxelToColour(water[x][0][z]);

      if (!data.empty) {
        face5[(x - 1) * 5 + (5 - z)] = data.colour;
      }
    }
  }

  // FACE 4
  // y = SIZE - 1
  for (int x = 1; x < SIZE - 1; x++) {
    for (int z = 1; z < SIZE - 1; z++) {
      VoxelData data = voxelToColour(water[x][SIZE - 1][z]);

      if (!data.empty) {
        face4[(x - 1) * 5 + (z - 1)] = data.colour;
      }
    }
  }

  // FACE 3
  // z = SIZE - 1
  for (int x = 1; x < SIZE - 1; x++) {
    for (int y = 1; y < SIZE - 1; y++) {
      VoxelData data = voxelToColour(water[x][y][SIZE - 1]);

      if (!data.empty) {
        face3[(x - 1) * 5 + (5 - y)] = data.colour;
      }
    }
  }

  // FACE 2
  // z = 0
  for (int x = 1; x < SIZE - 1; x++) {
    for (int y = 1; y < SIZE - 1; y++) {
      VoxelData data = voxelToColour(water[x][y][0]);

      if (!data.empty) {
        face2[(x - 1) * 5 + (y - 1)] = data.colour;
      }
    }
  }

  // FACE 1
  // x = SIZE - 1
  for (int y = 1; y < SIZE - 1; y++) {
    for (int z = 1; z < SIZE - 1; z++) {
      VoxelData data = voxelToColour(water[SIZE - 1][y][z]);

      if (!data.empty) {
        face1[(5 - z) * 5 + (5 - y)] = data.colour;
      }
    }
  }
}

void effectWater() {
  // unsigned long currentMillis = millis();

  // if (currentMillis - previousMillis >= interval) {
    // Save the last time you executed the action
    // previousMillis = currentMillis;

    setGravity(Vec3(-az, ay, ax));
    updateParticles();
    updateCells();

    // for (int x = 0; x < SIZE; x++) {
    //   for (int y = 0; y < SIZE; y++) {
    //     for (int z = 0; z < SIZE; z++) {
    //       water[x][y][z] = 0;
    //     }
    //   }
    // }
    // water[4][4][4] = 4;
    drawWater();
  // }
}


/* ===================== IMU ======================*/
void writeRegister(uint8_t reg, uint8_t value)
{
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

uint8_t readRegister(uint8_t reg)
{
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);

  Wire.requestFrom(MPU_ADDR, 1);

  if (Wire.available())
    return Wire.read();

  return 0xFF;
}

int16_t readInt16(uint8_t reg)
{
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);

  Wire.requestFrom(MPU_ADDR, 2);

  if (Wire.available() < 2)
    return 0;

  uint8_t high = Wire.read();
  uint8_t low = Wire.read();

  return (int16_t)((high << 8) | low);
}

void readIMU()
{
  // Raw accelerometer values
  ax = readInt16(ACCEL_XOUT_H);
  ay = readInt16(ACCEL_XOUT_H + 2);
  az = readInt16(ACCEL_XOUT_H + 4);

  // Raw gyroscope values
  gx = readInt16(GYRO_XOUT_H);
  gy = readInt16(GYRO_XOUT_H + 2);
  gz = readInt16(GYRO_XOUT_H + 4);
}


/* ===================== OLED ===================== */

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

static const unsigned char PROGMEM sun_icon[] = {0x80, 0x20, 0x4e, 0x40, 0x1f, 0x00, 0xdf, 0x60, 0x1f, 0x00, 0x4e, 0x40, 0x80, 0x20};

static const unsigned char PROGMEM plug_icon[] = {0x03,0xc0,0x07,0xf8,0x07,0xc0,0xff,0xc0,0x07,0xc0,0x07,0xf8,0x03,0xc0};

/* ===================== MENU ===================== */

const char *mainMenu[] = {
    "effects",
    "brightness",
    "check battery"
};

const char *brightnessMenu[] = {
    "back",
    "brightness up",
    "brightness down"
};

const char *effectNames[] = {
    "back",
    "solid blue",
    "rainbow",
    "pulse",
    "confetti",
    "theatre chase",
    "smiley face",
    "face numbers",
    "conways game",
    "test geometry",
    "water sim",
    "mic effect"
};

const char *splashText[] = {
  "ello",
  "reports suggest the CUBE cures cancer",
  "also try cheetoDeck",
  "sponsored by palantir",
  "sponsored by the IDF",
  "funded by peter thiel",
  "would you like to try solid blue? or are you here for water sim? i knew it.",
  "developed by fartrius industries",
  "BREAKING NEWS: fart shot dead in florida",
  "BREAKING NEWS: spoingus shot dead in miami",
  "sites.google.com/view/fartd",
  "great work team",
  "barbeque sauce?",
  "now he's exploding lab mice again",
  "reports suggest fartrius demetrius esparagus the III junior has found a new apartment.",
  "made possible by Benjamin Netenyahu",
  "please charge the cubes battery. it might explode if it gets too low.",
  "Fartrius Industries (R) is not responsible for any bodily harm caused by the CUBE.",
  "BREAKING NEWS: man arrested for incorrectly capitalising the CUBE.",
  "the CUBE will enter public domain in approximately 4.5634x10^69420 years. get ready.",
  "BREAKING NEWS: the CUBE shown to mitigate effects of eating without table",
  "Also play Terraria!",
  "Music by C418!",
  "Special Appearance: Linus Torvalds",
  "Featuring: C++",
  "fartrius industries has added a surveillance microphone to the CUBE for no added cost.",
  "blazing fast 0.5 fps refresh rate - some arch linux user",
  "2500 hellish lines of code",
  "dont tell anyone - conways game of life is like entirely vibe coded"
};

const uint8_t MAIN_MENU_LENGTH = sizeof(mainMenu) / sizeof(mainMenu[0]);
const uint8_t EFFECTS_MENU_LENGTH = sizeof(effectNames) / sizeof(effectNames[0]);
const uint8_t BRIGHTNESS_MENU_LENGTH = sizeof(brightnessMenu) / sizeof(brightnessMenu[0]);
const uint8_t SPLASH_TEXT_LENGTH = sizeof(splashText) / sizeof(splashText[0]);

enum MenuState
{
  MENU_MAIN,
  MENU_EFFECTS,
  MENU_BRIGHTNESS
};

MenuState menuState = MENU_MAIN;
uint8_t menuIndex = 0;      // highlighted index within the active menu
uint8_t selectedEffect = 1; // currently running effect (0..n-1)

String currentEffectName = "rainbow";

/* ===================== STATE ===================== */

unsigned long lastButtonTime = 0;
const unsigned long debounceMs = 150;

uint8_t menuScroll = 0;

int brightness = 100;

int splashTextScroll = 0;
int currentSplashText = 0;

const int sampleWindow = 50;    // Sample window width in milliseconds (50 ms = 20Hz)
unsigned int sample;
double volts;

/* ===================== SETUP ===================== */

void setup()
{
  // Buttons
  pinMode(LEFT_BUTTON, INPUT);
  pinMode(RIGHT_BUTTON, INPUT);
  pinMode(SELECT_BUTTON, INPUT);

  pinMode(BATTERY_PIN, INPUT);
  pinMode(MIC_PIN, INPUT);

  // Dock detect
  pinMode(FACE6_DOCK_PIN, INPUT_PULLUP);

  // LEDs
  FastLED.addLeds<LED_TYPE, FACE1_PIN, COLOR_ORDER>(face1, LEDS_PER_FACE);
  FastLED.addLeds<LED_TYPE, FACE2_PIN, COLOR_ORDER>(face2, LEDS_PER_FACE);
  FastLED.addLeds<LED_TYPE, FACE3_PIN, COLOR_ORDER>(face3, LEDS_PER_FACE);
  FastLED.addLeds<LED_TYPE, FACE4_PIN, COLOR_ORDER>(face4, LEDS_PER_FACE);
  FastLED.addLeds<LED_TYPE, FACE5_PIN, COLOR_ORDER>(face5, LEDS_PER_FACE);
  FastLED.addLeds<LED_TYPE, FACE6_PIN, COLOR_ORDER>(face6, LEDS_PER_FACE);
  FastLED.setBrightness(brightness);
  FastLED.clear(true);

  analogSetAttenuation(ADC_6db);
  readBattery();

  // OLED (init only once)
  Wire.begin();

  // seed rng
  randomSeed(esp_random());

  Serial.begin(115200);

  // -- init mpu

  delay(100);

  // Wake up the MPU6050
  writeRegister(PWR_MGMT_1, 0x00);

  delay(100);

  uint8_t whoAmI = readRegister(WHO_AM_I);

  Serial.println("Starting sensor...");
  Serial.print("WHO_AM_I = 0x");
  Serial.println(whoAmI, HEX);

  Serial.print("Total water sim particles is ");
  Serial.println(TOTAL_PARTICLES);
  initialiseWater();

  handleOLED();
  handleButtons();
}

/* ===================== LOOP ===================== */

void loop()
{
  readIMU();

  static int consecutiveOpenDocks = 0;

  previousDockState = dockState;
  dockState = digitalRead(FACE6_DOCK_PIN) == HIGH;

  if (dockOpen)
  {
    handleOLED();
    handleButtons();
  }
  else
  {
    display.clearDisplay();
    display.display();
  }

  if (consecutiveOpenDocks >= dockDebounce) {
    dockOpen = false;
  } else {
    dockOpen = true;
  }

  if (!previousDockState && !dockState) {
    consecutiveOpenDocks = constrain(consecutiveOpenDocks++, 0, dockDebounce);
  } else {
    consecutiveOpenDocks = constrain(consecutiveOpenDocks--, 0, dockDebounce);
  }

  runEffect(selectedEffect);
  FastLED.show();
}

/* ===================== BUTTON HANDLING ===================== */

void handleButtons()
{
  if (millis() - lastButtonTime < debounceMs)
    return;

  uint8_t menuLength = 0;

  if (menuState == MENU_MAIN)
    menuLength = MAIN_MENU_LENGTH;
  else if (menuState == MENU_EFFECTS)
    menuLength = EFFECTS_MENU_LENGTH;
  else
    menuLength = BRIGHTNESS_MENU_LENGTH;

  if (digitalRead(LEFT_BUTTON))
  {
    menuIndex = (menuIndex == 0) ? menuLength - 1 : menuIndex - 1;
    lastButtonTime = millis();
  }

  if (digitalRead(RIGHT_BUTTON))
  {
    menuIndex = (menuIndex + 1) % menuLength;
    lastButtonTime = millis();
  }

  if (!digitalRead(SELECT_BUTTON))
  {
    if (menuState == MENU_MAIN)
    {
      if (strcmp(mainMenu[menuIndex], "effects") == 0)
      {
        menuState = MENU_EFFECTS;
        menuIndex = 0;
        menuScroll = 0;
      }
      else if (strcmp(mainMenu[menuIndex], "brightness") == 0)
      {
        menuState = MENU_BRIGHTNESS;
        menuIndex = 0;
        menuScroll = 0;
      }
      else if (strcmp(mainMenu[menuIndex], "check battery") == 0)
      {
        readBattery();
      }
    }
    else if (menuState == MENU_EFFECTS)
    {
      if (menuIndex == 0)
      {
        // "back"
        menuState = MENU_MAIN;
        menuIndex = 0;
        menuScroll = 0;
      }
      else
      {
        // select effect (effectNames[1] -> selectedEffect 0)
        selectedEffect = menuIndex - 1;
        currentEffectName = effectNames[selectedEffect + 1];

        if (currentEffectName == "water sim") {
          initialiseWater();
        }

        if (currentEffectName.length() > 9) {
          String oldName = currentEffectName;
          currentEffectName = oldName.substring(0, 5);
          currentEffectName = currentEffectName + "...";
        }
      }
    }
    else if (menuState == MENU_BRIGHTNESS)
    {
      if (menuIndex == 0)
      {
        // "back"
        menuState = MENU_MAIN;
        menuIndex = 0;
        menuScroll = 0;
      }
      else if (menuIndex == 1)
      {
        brightness += 10;
      }
      else
      {
        brightness -= 10;
      }
      brightness = constrain(brightness, 0, 255);
      FastLED.setBrightness(brightness);
    }

    lastButtonTime = millis();
  }
}

/* ===================== OLED MENU ===================== */

void handleOLED()
{
  static bool oledInit = false;

  if (!oledInit)
  {
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR))
      return;
    oledInit = true;
  }

  const uint8_t ROW_HEIGHT = 10;
  const uint8_t HEADER_HEIGHT = 16;
  const uint8_t VISIBLE_ROWS = (SCREEN_HEIGHT - HEADER_HEIGHT) / ROW_HEIGHT;

  // Clamp scroll window using the active menu index/length
  uint8_t menuLength = 0;

  if (menuState == MENU_MAIN)
    menuLength = MAIN_MENU_LENGTH;
  else if (menuState == MENU_EFFECTS)
    menuLength = EFFECTS_MENU_LENGTH;
  else
    menuLength = BRIGHTNESS_MENU_LENGTH;

  if (menuIndex < menuScroll)
  {
    menuScroll = menuIndex;
  }
  else if (menuIndex >= menuScroll + VISIBLE_ROWS)
  {
    menuScroll = menuIndex - VISIBLE_ROWS + 1;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Header
  display.setCursor(0, 0);

  display.println(currentEffectName);

  bool onBattery = true;
  int width = 1;

  if (batteryVoltage >= 4.05) { // battery 85%+
    width = 10;
  } else if (batteryVoltage >= 3.95) { // battery 75%+
    width = 7;
  } else if (batteryVoltage >= 3.80) { // battery 50%+
    width = 5;
  } else if (batteryVoltage >= 3.50) { // battery 25%+
    width = 3;
  } else if (batteryVoltage <= 1.50) { // using dock / battery not connected
    onBattery = false;
  }

  if (onBattery) {
    display.drawRect(115, 0, 12, 7, 1);  // battery box
    display.drawLine(127, 2, 127, 4, 1); // battery anode
    display.fillRect(116, 1, width, 5, 1);   // battery fill - width 10 = full, width 0 = empty

    // voltage display
    display.setCursor(90, 0);
    display.print(batteryVoltage);
  } else {
    display.drawBitmap(115, 0, plug_icon, 13, 7, 1); // plug icon thingy
  }

  // brightness display
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);
  display.setCursor(70, 0);
  uint8_t percent = (brightness * 100) / 255;
  display.print(String(percent) + "%");

  display.drawBitmap(57, 0, sun_icon, 11, 7, 1);

  // Draw visible menu entries for the active menu
  for (uint8_t i = 0; i < VISIBLE_ROWS; i++)
  {
    uint8_t idx = menuScroll + i;
    if (idx >= menuLength)
      break;

    display.setCursor(0, HEADER_HEIGHT + (i * ROW_HEIGHT));

    if (idx == menuIndex)
    {
      display.print("> ");
    }
    else
    {
      display.print("  ");
    }

    if (menuState == MENU_MAIN)
    {
      display.println(mainMenu[idx]);
    }
    else if (menuState == MENU_EFFECTS)
    {
      display.println(effectNames[idx]);
    }
    else
    {
      display.println(brightnessMenu[idx]);
      if (brightness > 191 && idx == menuLength - 1)
      {
        display.println("overdrive mode!");
        display.println("flickering may occur");
      }
    }
  }

  // Scroll indicator
  if (menuScroll > 0)
  {
    display.fillTriangle(120, 18, 124, 18, 122, 14, SSD1306_WHITE);
  }
  if (menuScroll + VISIBLE_ROWS < menuLength)
  {
    display.fillTriangle(120, 60, 124, 60, 122, 64, SSD1306_WHITE);
  }

  if (menuState == MENU_MAIN) {
    int minX = -6 * strlen(splashText[currentSplashText]);
    display.setCursor(splashTextScroll, 56);
    display.print(splashText[currentSplashText]);
    
    if (splashTextScroll-- < minX) {
      splashTextScroll = display.width();
      int previousText = currentSplashText;
      while (previousText == currentSplashText) {
        currentSplashText = random(0, SPLASH_TEXT_LENGTH);
      }
    }
  }

  display.display();
}

void readBattery() {
  clearCube();
  FastLED.show();
  delay(10);
  
  const int SAMPLES = 2;
  float totalVoltage = 0;

  for (int i = 0; i < SAMPLES; i++) {
    int v = analogRead(BATTERY_PIN);

    // Serial.print("read raw ADC ");
    // Serial.println(v);

    float adcVoltage = (v / 4095.0f) * 1.95f;
    totalVoltage += adcVoltage;
  }

  float measuredVoltage = totalVoltage / SAMPLES;
  batteryVoltage = (measuredVoltage * (R1 + R2)) / R2;

  // Serial.print("battery voltage: ");
  // Serial.println(batteryVoltage);
}

void readMic() {
  unsigned long startMillis = millis();  // Start of sample window
  unsigned int peakToPeak = 0;           // Total amplitude level

  unsigned int signalMax = 0;
  unsigned int signalMin = 4095;         // Max value for 12-bit ADC

  // Collect data for the duration of the sample window
  while (millis() - startMillis < sampleWindow) {
    sample = analogRead(MIC_PIN);
    
    // Extrapolate the absolute min and max positions during the window
    if (sample < 4095) {  // Reject occasional erroneous spike readings
      if (sample > signalMax) {
        signalMax = sample;  // Save the highest absolute peak
      }
      else if (sample < signalMin) {
        signalMin = sample;  // Save the lowest absolute trough
      }
    }
  }
  
  peakToPeak = signalMax - signalMin;  // Max peak minus min trough equals amplitude
  
  // Convert the raw ADC differential value into a physical voltage
  // 3.3V power rails divided across 4095 distinct resolution steps
  volts = (peakToPeak * 3.3) / 4095.0;  

  // Output to the Serial Plotter to visualize audio levels
  Serial.print("Min:");   Serial.print(signalMin);   Serial.print("\t");
  Serial.print("Max:");   Serial.print(signalMax);   Serial.print("\t");
  Serial.print("Volts:"); Serial.println(volts);
  //Serial.print("sample = "); Serial.println(sample);
}

/* ===================== EFFECTS ===================== */

void runEffect(uint8_t effect)
{
  switch (effect)
  {
  case 0:
    solidColor();
    break;
  case 1:
    rainbow();
    break;
  case 2:
    pulse();
    break;
  case 3:
    confetti();
    break;
  case 4:
    theatreChaseRGB();
    break;
  case 5:
    smileyFace();
    break;
  case 6:
    testMode();
    break;
  case 7:
    gameOfLifeTick();
    break;
  case 8:
    testCubeGeometry();
    break;
  case 9:
    effectWater();
    break;
  case 10:
    micEffect();
    break;
  }
}

void solidColor()
{
  fillAll(CRGB::Blue);
}

void micEffect()
{
  readMic();
  int cleanSample = constrain(int(volts) - 0.6, 0, 2.7);
  cleanSample = map(cleanSample, 0, 2.7, 0, 255);
  Serial.println(sample);
  Serial.println(cleanSample);

  if (sample == 0) {
    cleanSample = 255;
  }

  fadeAll(40);

  for (int face = 0; face < 6; face++) {
    for (int led = 0; led < 25; led++) {
      faces[face][led].red = constrain(faces[face][led].red + cleanSample, 0, 255);
    }
  }
}

void rainbow()
{
  static uint8_t hue = 0;
  forEachFace([&](CRGB *face)
              { fill_rainbow(face, LEDS_PER_FACE, hue, 5); });
  hue++;
}

void pulse()
{
  static uint8_t bri = 0;
  static int8_t dir = 1;
  bri += dir * 4;
  if (bri == 0 || bri == 200)
  {
    dir = -dir;
    bri = constrain(bri, 0, 200);
  };

  forEachFace([&](CRGB *face)
              { fill_solid(face, LEDS_PER_FACE, CHSV(0, 0, bri)); });
}

void confetti()
{
  fadeAll(20);
  forEachFace([&](CRGB *face)
              {
    int pos = random16(LEDS_PER_FACE);
    face[pos] += CHSV(random8(), 200, 255); });
}

void theatreChaseRGB()
{
  static uint8_t offset = 0;
  static uint8_t hue = 0;
  static unsigned long lastUpdate = 0;

  const uint16_t intervalMs = 80;

  if (millis() - lastUpdate < intervalMs)
    return;
  lastUpdate = millis();

  forEachFace([&](CRGB *face)
              {
    for (uint8_t i = 0; i < LEDS_PER_FACE; i++) {
      if ((i + offset) % 3 == 0) {
        face[i] = CHSV(hue + (i * 5), 255, 255);
      } else {
        face[i].fadeToBlackBy(200);
      }
    } });

  offset = (offset + 1) % 3;
  hue += 4;
}

void smileyFace()
{
  imageEffect(smiley);
}

void testMode()
{
  pushImage(num_one, face1);
  pushImage(num_two, face2);
  pushImage(num_three, face3);
  pushImage(num_four, face4);
  pushImage(num_five, face5);
  pushImage(num_six, face6);
}

void testCubeGeometry()
{
  for (int f = 0; f < NUM_FACES; f++)
  {
    for (int row = 0; row < FACE_SIZE; row++)
    {
      for (int col = 0; col < FACE_SIZE; col++)
      {

        // Top row = red
        // Bottom row = blue
        // Left column = green
        // Right column = white

        CRGB c = CRGB::Black;

        if (row == 0)
          c = CRGB::Red;
        else if (row == FACE_SIZE - 1)
          c = CRGB::Blue;
        else if (col == 0)
          c = CRGB::Green;
        else if (col == FACE_SIZE - 1)
          c = CRGB::White;

        faces[f][row * FACE_SIZE + col] = c;
      }
    }
  }
}

/* ===================== HELPERS ===================== */

template <typename F>
void forEachFace(F func)
{
  for (uint8_t i = 0; i < NUM_FACES; i++)
  {
    func(faces[i]);
  }
}

void clearCube()
{
  for (int f = 0; f < NUM_FACES; f++)
    for (int i = 0; i < LEDS_PER_FACE; i++)
      faces[f][i] = CRGB::Black;
}

void fillAll(const CRGB &color)
{
  forEachFace([&](CRGB *face)
              { fill_solid(face, LEDS_PER_FACE, color); });
}

void fadeAll(uint8_t amount)
{
  forEachFace([&](CRGB *face)
              { fadeToBlackBy(face, LEDS_PER_FACE, amount); });
}

uint8_t xyToIndex(uint8_t x, uint8_t y)
{
  return y * 5 + x; // 0–24
}

void imageEffect(CRGB image[5][5])
{
  forEachFace([&](CRGB *face)
              {
    for (uint8_t y = 0; y < 5; y++) {
      for (uint8_t x = 0; x < 5; x++) {
        uint8_t idx = xyToIndex(x, y);
        face[idx] = image[y][x];
      }
    } });
}

void pushImage(CRGB image[5][5], CRGB face[25])
{
  for (uint8_t y = 0; y < 5; y++)
  {
    for (uint8_t x = 0; x < 5; x++)
    {
      uint8_t idx = xyToIndex(x, y);
      face[idx] = image[y][x];
    }
  }
}