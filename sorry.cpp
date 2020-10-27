#include "sorry.h"
#include <fstream>
#include <iostream>
#include <algorithm>

#define BOARD_ARG 1
#define DECK_ARG BOARD_ARG + 1
#define PLAYERS_ARG DECK_ARG + 1
#define ROUNDS_ARG PLAYERS_ARG + 1
#define SHUFFLE_ARG ROUNDS_ARG + 1

#define START_MSG "%s "

template<typename T>
T str2Enum(string &str, const string names[], int count, T dummy)
{
    int val = -1;
    for(int i = 0; i < count; ++i) {
        const string curName = names[i];
        if (str == curName) {
            val = i;
            break;
        }
    }

    return static_cast<T>(val);
}

bool hasEnded(Game &g)
{
    for(Player &p : g.players)
    {
        bool allPawnsHome = true;
        for (PawnLocation &loc : p.pawns) {
            allPawnsHome = allPawnsHome && loc.state == HOME;
        }

        if (allPawnsHome) {
            return true;
        }
    }

    return false;
}

bool readDeck(Deck &deck, string &filename)
{
    ifstream deckFile;
    deckFile.open(filename);
    if (deckFile.is_open())
    {
        int numCards = -1, value;
        string type;
        deckFile >> numCards;
        deck.numCards = numCards;

        for (int i = 0; i < numCards; ++i)
        {
            deckFile >> type >> value;
            deck.cards[i].type = str2Enum(type, cardNames, MAXTYPES, SORRY);
            deck.cards[i].value = value;
        }

        return true;
    }

    cout << "could not open file: " << filename << endl;
    return false;
}

bool readBoard(Game &game, string &fileName)
{
    Board &board = game.board;
    ifstream boardFile;
    boardFile.open(fileName);
    if (boardFile.is_open()) {
        int numSquares = -1, num;
        string squaresWord, type, color;
        boardFile >> squaresWord >> numSquares;
        board.numSquares = numSquares;

        for(int i = 0; i < numSquares; ++i) {
            Square &curSquare = board.squares[i];
            curSquare.slide = {REGULAR, NONE};
            curSquare.ends = {REGULAR, NONE};
            curSquare.occupant.color = NONE;
        }

        while(!boardFile.eof()) {
            boardFile >> num >> type >> color;
            Square &curSquare = board.squares[num];
            SquareKind kind = str2Enum(type, kindNames, MAXKINDS, REGULAR);
            Color colorEnum = str2Enum(color, colorNames, MAXCOLORS, BLUE);
            if (kind == BEGIN || kind == END) {
                curSquare.slide = {kind, colorEnum};
            }
            else {
                curSquare.ends = {kind, colorEnum};
                if (kind == HOMESQ) {
                    game.players[colorEnum].homeSquare = num;
                }
                else {
                    game.players[colorEnum].startSquare = num;
                }
            }
        }

        return true;
    }

    return false;
}

bool initGame(Game &game, int argc, char *argv[])
{
    string boardFile = argv[BOARD_ARG];
    if (!readBoard(game, boardFile)) {
        printf("failure while reading board\n");
        return false;
    }

    string deckFile = argv[DECK_ARG];
    game.deck.curCard = 0;
    if (!readDeck(game.deck, deckFile)) {
        printf("failure while reading deck\n");
        return false;
    }

    game.numPlayers = atoi(argv[PLAYERS_ARG]);
    for (Player &p : game.players) {
        for (PawnLocation &loc : p.pawns) {
            loc.state = STARTABLE;
        }
    }

    game.rounds = atoi(argv[ROUNDS_ARG]);

    string shuffle = argv[SHUFFLE_ARG];
    game.shuffle = shuffle == "yes";

    return true;

}

void doHomeSquare(Game &game, Color playerId, Outcome &move)
{
    Player &player = game.players[playerId];
    PawnLocation &loc = player.pawns[move.pawnNum];
    Board &board = game.board;
    bool isHome = board.squares[loc.square].ends.kind == HOMESQ && board.squares[loc.square].ends.color == playerId;

    if(!isHome) {
        return;
    }

    loc.state = HOME;
    printf("%s pawn %d has reached home\n", colorNames[playerId].c_str(), move.pawnNum);
}



bool landsOnOwnPawn(const Board &board, Color player, int squareId)
{
    return player == board.squares[squareId].occupant.color;
}

int distance(const Board &board, int from, int to)
{
    if (to >= from) {
        return to - from;
    }

    return board.numSquares - 1 - from + to;
}

Outcome evalForward(const Board &board, const Player &player, Color playerId, int value)
{
    Outcome result = {false};
    int furthest = -1, dist, destination;
    for (int i = 0; i < NUMPAWNS; ++i) {
        const PawnLocation &p = player.pawns[i];
        // forward only applies to pawns on the board
        if (p.state == ON_BOARD) {
            dist = distance(board, p.square, player.homeSquare);
            if (dist > furthest) {
                // Can't move forward past home square
                if(value > dist) {
                    continue;
                }
                destination = (p.square + value) % board.numSquares;
                if (landsOnOwnPawn(board, playerId, destination)) {
                    continue;
                }
                result.canMove = true;
                result.pawnNum = i;
                break;
            }
        }
    }
    
    return result;
}

Outcome evalStart(const Board &board, const Player &player, Color playerId)
{
    
    Outcome result = {false};

    // if start square is occupied by pawn of the same color, you cant move
    if(landsOnOwnPawn(board, playerId, player.startSquare)) {
        return result;
    }

    const Square &startSquare =  board.squares[player.startSquare];

    for(int i = 0; i < NUMPAWNS; ++i) {
        const PawnLocation &p = player.pawns[i];
        // is an eligble pawn
        if (p.state == STARTABLE) {
            result.pawnNum = i;
            result.canMove = true;
            break;
        }
    }

    return result;
}

Outcome evalSwap(const Game &game, Color playerId)
{
    Outcome result = {false};
    // find player pawn furthest from home square
    int furthest = -1, chosen = 0, dist;
    const Player &player = game.players[playerId];
    for (int i = 0; i < NUMPAWNS; ++i) {
        const PawnLocation &loc = player.pawns[i];
        if (loc.state == ON_BOARD) {
            dist = distance(game.board, loc.square, player.homeSquare);
            if (dist > furthest) {
                chosen = i;
            }
        }
    }

    // player has no movable piece on the board
    if (furthest == -1) {
        return result;
    }

    result.pawnNum = furthest;

    // pick victim closest to home square
    int closest = game.numPlayers + 1;
    for (int i = 0; i < game.numPlayers; ++i) {
        // exclude the player themself
        if (i != playerId) {
            const Player &p = game.players[i];
            for (int j = 0; j < NUMPAWNS; ++j) {
                const PawnLocation &loc = p.pawns[j];
                // victim must be on board also
                if (loc.state == ON_BOARD) {
                    result.canMove = true;
                    dist = distance(game.board, loc.square, player.homeSquare);
                    if (dist < closest) {
                        result.opponent = {static_cast<Color>(i), j};
                    }
                }
            }
        }
    }

    return result;
}

Outcome evalSorry(const Game &game, Color playerId)
{
    Outcome result = {false};
    int chosen = -1;

    const Player &player = game.players[playerId];
    for(int i = 0; i < NUMPAWNS; ++i) {
        const PawnLocation &loc = player.pawns[i];
        if (loc.state == STARTABLE) {
            chosen = i;
            break;
        }
    }

    // all players are on the board (already started)
    if(chosen < 0) {
        return result;
    }
    result.pawnNum = chosen;

    int closest = game.numPlayers + 1, dist;
    for (int i = 0; i < game.numPlayers; ++i) {
        // exclude the player
        if (i != playerId) {
            const Player &p = game.players[i];
            for (int j = 0; j < NUMPAWNS; ++j) {
                const PawnLocation &loc = p.pawns[j];
                // victim must be on board also
                if (loc.state == ON_BOARD) {
                    result.canMove = true;
                    dist = distance(game.board, loc.square, player.homeSquare);
                    if (dist < closest) {
                        result.opponent = {static_cast<Color>(i), j};
                    }
                }
            }
        }
    }

    return result;
}

Outcome dispatchEval(const Game &game, const Card& card, Color playerId)
{
    const Board &board = game.board;
    const Player &player = game.players[playerId];
    const char *playerName = colorNames[playerId].c_str(), *cardName = cardNames[card.type].c_str();
    switch (card.type)
    {
    case START:
        printf("Player %s draws %s\n", playerName, cardName); 
        return evalStart(board, player, playerId);
    case FORWARD:
    case BACKWARD:
        printf("Player %s draws %s %d\n", playerName, cardName, card.value);
        return evalForward(board, player, playerId, card.type == FORWARD ? card.value : -card.value);
    case SWAP:
        printf("Player %s draws %s\n", playerName, cardName);
        return evalSwap(game, playerId);
    case SORRY:
        printf("Player %s draws %s\n", playerName, cardName);
        return evalSorry(game, playerId);
    default:
        return {false};
    }
}

void shuffle(Deck &d) {
    random_shuffle(d.cards, d.cards + d.numCards);
}

Outcome eval(const Game &game, Color player) 
{
    const Deck &deck = game.deck;
    const Card &card = deck.cards[deck.curCard];
    return dispatchEval(game, card, player);
}

void doBump(Game &game, Color playerId, Outcome &move, int square)
{
    Board &board = game.board;
    Pawn &occupant = board.squares[square].occupant;
    if (occupant.color == NONE) {
        return;
    }

    Player &victim = game.players[occupant.color];
    PawnLocation &victimPawn = victim.pawns[occupant.num];
    victimPawn.state = STARTABLE;

    board.squares[victimPawn.square].occupant.color = NONE;
    printf("%s pawn %d bumps %s pawn %d\n", colorNames[playerId].c_str(), 
                                            move.pawnNum, 
                                            colorNames[occupant.color].c_str(),
                                            occupant.num);
}

void doSlide(Game &game, Color playerId, Outcome move, int square)
{
    Board &board = game.board;
    bool willSlide = board.squares[square].slide.kind == BEGIN && board.squares[square].slide.color != playerId;
    if (!willSlide) {
        return;
    }

    //handle slide
    int dest = square;
    while(board.squares[dest].slide.kind != END) {
        Square &curSq = board.squares[dest];
        move.opponent = curSq.occupant;
        doBump(game, playerId, move, dest);
        dest = (dest + 1) % board.numSquares;
    }

    printf("%s pawn %d slides from square %d to square %d\n", colorNames[playerId].c_str(), move.pawnNum, square, dest);
}

void applyStart(Game &game, Color playerId, Outcome &move)
{
    Player &player = game.players[playerId];
    player.pawns[move.pawnNum].state = ON_BOARD;
    player.pawns[move.pawnNum].square = player.startSquare;
    printf("%s pawn %d starts at square %d\n", colorNames[playerId].c_str(), move.pawnNum, player.startSquare);
    doBump(game, playerId, move, player.startSquare);
    game.board.squares[player.startSquare].occupant = {playerId, move.pawnNum};
}

void applyForward(Game &game, Color playerId, Outcome &move, int value)
{
    Player &player = game.players[playerId];
    Board &board = game.board;
    PawnLocation &loc = player.pawns[move.pawnNum];
    int destination = (value + loc.square) % board.numSquares;

    printf("%s pawn %d moves from square %d to square %d\n", colorNames[playerId].c_str(), playerId, loc.square, destination);
    loc.square = destination;
    doBump(game, playerId, move, loc.square);
    doSlide(game, playerId, move, loc.square);
    doHomeSquare(game, playerId, move);
    game.board.squares[player.startSquare].occupant = {playerId, move.pawnNum};
}

void applySwap(Game &game, Color playerId, Outcome &move) 
{
    Player &player = game.players[playerId];
    Player &victim = game.players[move.opponent.color];
    Board &board = game.board;
    PawnLocation &loc = player.pawns[move.pawnNum];
    PawnLocation &victimLoc = victim.pawns[move.opponent.num];

    int temp = loc.square;
    loc.square = victimLoc.square;
    victimLoc.square = temp;

    Pawn tempOccupant = board.squares[loc.square].occupant;
    board.squares[loc.square].occupant = board.squares[victimLoc.square].occupant;
    board.squares[victimLoc.square].occupant = tempOccupant;
    Outcome victimMove = {true, move.opponent.num, {NONE}};

    printf("%s pawn %d swaps with %s pawn %d\n", colorNames[playerId].c_str(),
                                                 move.pawnNum,
                                                 colorNames[move.opponent.color].c_str(),
                                                 victimMove.pawnNum);
    // calling player's actions
    doSlide(game, playerId, move, loc.square);
    doHomeSquare(game, playerId, move);

    // victim's actions
    doSlide(game, move.opponent.color, victimMove, victimLoc.square);
    doHomeSquare(game, move.opponent.color, victimMove);
}

void apply(Game &game, Color player, Outcome move)
{
    if (!move.canMove) {
        return;
    }
    Card &card = game.deck.cards[game.deck.curCard];
    switch(card.type) {
        case START:
            applyStart(game, player, move);
            break;
        case FORWARD:
        case BACKWARD:
            applyForward(game, player, move, card.type == FORWARD ? card.value : -card.value);
            break;
        case SWAP:
            applySwap(game, player, move);
            break;
        case SORRY:
            break;

    }

}

int myMain(int argc, char *argv[])
{
    if (argc < SHUFFLE_ARG + 1) {
        printf("not enough arguments there should be %d args\n", SHUFFLE_ARG + 1);
        return -1;
    }
    Game game;
    if (!initGame(game, argc, argv)) {
        printf("failed to initialize game\n");
        return -1;
    }

    for(int i = 0; i < game.rounds; ++i) {
        cout << "Round " << i+1 << endl;  
        for (int player = 0; player < game.numPlayers; ++player) {
            Color playerId = static_cast<Color>(player);
            Outcome outcome = eval(game, playerId);

            apply(game, playerId, outcome);
            game.deck.curCard = (game.deck.curCard + 1) % game.deck.numCards;
            if (!game.deck.curCard) {
                shuffle(game.deck);
            }
        }
    }

    return 0;

}

int main(int argc, char *argv[])
{
    return myMain(argc, argv);
}

