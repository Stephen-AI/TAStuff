#ifndef __P3_H__
#define __P3_H__
#include <string>

using namespace std;
const int MAXSQUARES = 60;
const int MAXPLAYERS = 4;
const int MAXCARDS = 45;
////////////////////////////////////////////////////////
/* Colors */
////////////////////////////////////////////////////////
enum Color
{
    BLUE,
    YELLOW,
    GREEN,
    RED,
    NONE
};
// Remember: by default, enumerations count from 0..N-1
const int MAXCOLORS = NONE + 1;
const string colorNames[MAXCOLORS] = {"BLUE", "YELLOW", "GREEN",

                                      "RED", "NONE"};

////////////////////////////////////////////////////////
/* The Player */
////////////////////////////////////////////////////////
// The state of a single pawn
enum PawnState
{
    STARTABLE,
    ON_BOARD,
    HOME
};
const int MAXSTATES = HOME + 1;
const string stateNames[MAXSTATES] = {"Startable", "On board square",

                                      "Home"};
struct PawnLocation
{
    PawnState state;
    int square; // Only valid if state == ON_BOARD
};
const int NUMPAWNS = 4;
// A single player
struct Player
{
    int startSquare;              // Square player STARTS on
    int homeSquare;               // This player's HOME square
    PawnLocation pawns[NUMPAWNS]; // Where the pawns are
};
//////////////////////////////////////////////////////////
/* The Deck */
//////////////////////////////////////////////////////////
// The different types of cards.
enum CardType
{
    START,
    FORWARD,
    BACKWARD,
    SWAP,
    SORRY
};
const int MAXTYPES = SORRY + 1;
const string cardNames[MAXTYPES] = {"START", "FORWARD", "BACKWARD",

                                    "SWAP", "SORRY"};

// A single card
struct Card
{
    CardType type;
    int value;
};
// A deck of cards

struct Deck
{
    int curCard; // The index of the card on "top" of the

    // deck.

    int numCards; // The total number of cards
    Card cards[MAXCARDS];
};

//////////////////////////////////////////////////////////
/* The Board */
//////////////////////////////////////////////////////////
// The different kinds of squares.
enum SquareKind
{
    REGULAR,
    BEGIN,   // Begins a slide
    END,     // Ends a slide
    STARTSQ, // Start square for a player
    HOMESQ   // Home square for a player
};
const int MAXKINDS = HOMESQ + 1;
const string kindNames[MAXKINDS] = {"REGULAR", "BEGIN", "END",

                                    "STARTSQ", "HOMESQ"};

struct Role
{ // The role played by a square
    SquareKind kind;
    Color color; // NONE if kind is REGULAR
};
struct Pawn
{
    Color color; // Color of this pawn, NONE if "no pawn here"
    int num;     // Number of this pawn, if Color != NONE
};
// Note: Each square is guaranteed to play at most one slide role,
// and at most one start/stop role.
struct Square
{
    Role slide;    // Slide start, end, or REGULAR
    Role ends;     // Player start, home, or REGULAR
    Pawn occupant; // Pawn on this square (if any)
};
// The board
struct Board
{
    int numSquares; // How many squares total
    Square squares[MAXSQUARES];
};
//////////////////////////////////////////////////////////
/* The Game */
//////////////////////////////////////////////////////////
struct Game
{
    Board board;
    Deck deck;
    int numPlayers; // Total number of players
    Player players[MAXPLAYERS];
    int rounds; // Rounds remaining to play
    bool shuffle;
};
struct Outcome
{
    bool canMove;  // true if a move exists, false if forfiet
    int pawnNum;   // Which of the player's pawns to move (if any)
    Pawn opponent; // Which opponent pawn is victim of swap/sorry

    // (if any)
};
/////////////////////////////////////////////////////////////////
/// You must write these
/////////////////////////////////////////////////////////////////
bool initGame(Game &game, int argc, char *argv[]);
// MODIFIES: game
// EFFECTS: reads the board and deck, as
// specified on the command line, and initializes the Game
// structure. Returns true on success, false on failure.
// Should produce no output.
Outcome eval(const Game &game, Color player);
// REQUIRES: game is well-formed
// EFFECTS: evaluates the desired Outcome of "player" drawing the card
// that is currently at the top of the deck.
// Must follow the specification's movement rules, and MUST NOT
// modify game. Should produce no output.
void apply(Game &game, Color player, Outcome move);
// REQUIRES: game is well-formed
// given the game state and the card that is
// currently at the top of the deck.
// MODIFIES: game
// EFFECTS: applies the changes to game that result from player making move.
// Should produce the appropriate output for the *move*, but not
// any output relating to drawing cards. Consumes the card
// at the top of the deck, but does *not* shuffle/reset if
// this is the last card.
int myMain(int argc, char *argv[]);
// REQUIRES: argc and argv are the arguments to the program.
// EFFECTS: performs the sorry simulation as specified.
/////////////////////////////////////////////////////////////////
/// We supply this
/////////////////////////////////////////////////////////////////
void shuffle(Deck &d);
// REQUIRES: d is a valid Deck
// MODIFIES: d
// EFFECTS: simple (deterministic) shuffle of d. Returns the Deck
// ready to deal all shuffled cards.

const string getSquareType(Square &sq)
{
    return sq.ends.color != NONE ? kindNames[sq.ends.kind] : kindNames[sq.slide.kind];
}

const string getSquareColor(Square &sq)
{
    return sq.ends.color != NONE ? colorNames[sq.ends.color] : colorNames[sq.slide.color];
}
#endif /* __P3_H__ */