// compiler header files
#include <stdbool.h> // bool, true, false
#include <stdlib.h> // rand
#include <stdio.h> // printf

// program header file
#include "bomberman.h"

// global declarations
extern const char BOMBERMAN; // ascii used for bomberman
extern const char WALL; // ascii used for the walls
extern const char BREAKABLE_WALL; // ascii used for the breakable walls
extern const char PATH; // ascii used for the pathes
extern const char EXIT; // ascii used for the EXIT
extern const char BOMB; // ascii used for a bomb
extern const char BOMB_BONUS; // ascii used for the bombs
extern const char FLAME_BONUS; // ascii used for the flames
extern const char FLAME_ENEMY; // ascii used for the flame ennemy
extern const char GHOST_ENEMY; // ascii used for the ghost ennemy

extern const int BOMB_DELAY; // time before a bomb explode
extern const int BREAKABLE_WALL_SCORE; // reward for bombing a breakable wall
extern const int FLAME_ENEMY_SCORE; // reward for bombing a flame enemy
extern const int GHOST_ENEMY_SCORE; // reward for bombing a ghost enemy 
extern const int BOMB_BONUS_SCORE; // reward for a bomb bonus, which increases by 1 the number of bombs that can be dropped at a time
extern const int FLAME_BONUS_SCORE; // reward for a flame bonus, which increses by 1 the explosion range of bombs
extern const int VIEW_DISTANCE; // bomberman view distance 

extern bool DEBUG; // indicates whether the game runs in DEBUG mode

char * binome="Thibaut Jolive"; // student name goes here


// Position : describes a position on the map.
struct positions {int x; int y;};
typedef struct positions position;

// Monster : Caracteristics of a monster
struct monster {position pos; // Position on the map
               int type; // Type of the monster : FLAME_ENEMY or GHOST_ENEMY, or something else.
               int dist; // Distance to the player 
               int parity;}; // Parity of x+y : 0 = even and 1 = odd;
typedef struct monster monster;

// Queue
typedef struct { int front; int tail; int count; position * table; unsigned int LMAX;} queue;

// prototypes of the local functions/procedures
void printAction(action);
void printBoolean(bool);
void printTree(tree);
void buildMap(tree,char ** , int, int,position *);
void printTable(int , int , char ** );
char ** initCharMap(int, int);
void initQueue(queue *, int, int);
bool enqueue(queue * , position);
position dequeue (queue *);
void createDistanceMap(position, char **, int **, int, int);
void createGhostDistanceMap(position, char **, int **, int, int);
void find(int, int, char **, int *, monster **, bool *, position*, int *, position**, int *, position**, int *, position**, action, position, int *, bool *, int **);
void danger(int, int, char **, monster *, int, position *, int, int, int ** , int *, int, position, position, bool);
int nearestMonster(int, monster *);
void neighbors(position *, position,int);
void pathfind(int, int, char **, position, position, int **, bool *, action *);
bool isSafe(int ** , int , int );
bool isBrickSpot(char **, int, int, int, int, int, int, monster*, int**);
void nearestBrickSpot(int,int, char **, position, int **,position *, bool *, int , monster *, int, int**, int **);
void nearestSafeSpot(int ,int , char ** , position , int ** ,position *, int **, int **, bool, position);
void nearestSuperSafeSpot(int ,int , char ** , position , int ** ,position *, int **, int **, bool, position);
bool wall(char **, int, int, position);
bool deadEnd(int, int, char **, int, int, int **);
bool crossRoad(char **, int, int);
action safeMode(int, int, char **, position, int ** , int, int, monster*,int, int **, int **, bool, position exitPos);
void nearestMonsterSpot(int,int, char **, position, int **,position *, bool * , int, monster * , int, int **, int **);
bool isMonsterSpot(char **, int, int, int, int, int, int **);
action destroyBricks(int , int , char **, position,int **, int, int, monster*,int, int **, int **, bool, position);
action isolating(int , int , char **, position,int **, int, int, monster*,int, int **, int **, bool, position);
action hunt(int, int, char **, position,int **, int , int, monster *, int, int **, int **, int, int, int, bool, position);
bool isValid(int mapxsize, int mapysize, position pos);
int absolute(int);
void printIntTable(int , int , int ** );
bool oscillate(char **,position, int, action, action);

/*
  bomberman function:
  This function randomly select a valid move for BOMBERMAN based on its current position on the game map.
 */
action bomberman(
		 tree treemap, // 4-ary tree of char modeling a subset of the game map
		 action last_action, // last action made, -1 in the be beginning 
		 int remaining_bombs, // number of bombs
		 int explosion_range // explosion range for the bombs 
		 ) {
  action a; // action to choose and return
  
  int mapysize=VIEW_DISTANCE*2+2;
  int mapxsize=VIEW_DISTANCE*2+2;
  position xy;
  xy.x=VIEW_DISTANCE-1+1;
  xy.y=VIEW_DISTANCE-1+1;
  char ** map=initCharMap(mapxsize,mapysize);
  
  buildMap(treemap,map,mapxsize,mapysize,&xy);
  
  if (DEBUG){
    printf("\n");
    printf("\n");
    printf("\n");
    printTable(mapxsize,mapysize,map);
    printf("\n");
    printf("\n");
    printf("\n");
  }
  
  position playerPos = { .x = xy.x, .y = xy.y}; // Using a position type to describe the player's position

  /*
    Finding monsters, exit, bonus, bombs, bircks
  */
  int i; int j;
  int nbMonsters = 0;
  bool exitFound = false;
  bool monsterReachable = false;
  int nbBombBonus = 0;
  int nbFlameBonus = 0;
  int nbBombs = 0;
  int nbDanger = 0;
  int nbBricks = 0;
  monster * monsterList = NULL;
  position * bombList = NULL;
  position exitPos;
  //exitPos.x=1;
  //exitPos.y=1;
  position * bombBonusList = NULL;
  position * flameBonusList = NULL;

  /*
    Creating the playerDistanceMap, 
    which marks the squares that are accesible to bomberman with the distance between them
  */
  int** playerDistanceMap = malloc(mapysize * sizeof(int*));
  for(i=0; i<mapysize; i++) {
    playerDistanceMap[i] = malloc(mapxsize * sizeof(int));
    for(j=0; j<mapxsize; j++) playerDistanceMap[i][j] = 99;
  }
  createDistanceMap(playerPos, map, playerDistanceMap, mapxsize, mapysize);
  
  /*
    Finding the important locations on the map
  */
  find(mapxsize, mapysize, map,&nbMonsters, &monsterList, &exitFound, &exitPos, &nbBombBonus, &bombBonusList, &nbFlameBonus, &flameBonusList, &nbBombs, &bombList,
    last_action, playerPos, &nbBricks, &monsterReachable, playerDistanceMap );
  if (DEBUG){
    printf("\n");
    printf("Find process end with success \n");
    printf("\n");
  }
  /*
    Find dangerous positions
    and store them in the dangerMap
  */
  int** dangerMap= malloc(mapxsize * sizeof(int*));
  for(j=0; j<mapxsize; j++) {
    dangerMap[j] = malloc(mapysize * sizeof(int));
    for(i=0; i<mapysize; i++) dangerMap[j][i] = 0;
  }
  danger(mapxsize, mapysize , map, monsterList, nbMonsters, bombList, nbBombs,nbFlameBonus, dangerMap, &nbDanger, explosion_range, playerPos, exitPos, exitFound);
  
  if (DEBUG){
    printf("\n");
    printf("Danger process end with success \n");
    printf("\n");
  }
  /*
    Monster distance map : table filled with 
    the distance to the nearest monster of the square.
  */
  int k;
  int** monsterDistanceMap = malloc(mapysize * sizeof(int*));
  for(i=0; i<mapysize; i++) {
    monsterDistanceMap[i] = malloc(mapxsize * sizeof(int));
    for(j=0; j<mapxsize; j++) monsterDistanceMap[i][j] = 99;
  }
  for (k=0; k<nbMonsters; k++) {
    if (monsterList[k].type == GHOST_ENEMY) createGhostDistanceMap(monsterList[k].pos, map, monsterDistanceMap, mapxsize, mapysize);
    else createDistanceMap(monsterList[k].pos, map, monsterDistanceMap, mapxsize, mapysize);
  }
  if (DEBUG){
      printf("\n");
      printf("Monster distance map created with success \n");
      printf("\n");
    }
  /*
    Checking if the player has the same parity as the nearest monster (if there is one)
  */  
  bool isSameParity = false;
  if (nbMonsters>0)  {
    if (monsterList[nearestMonster(nbMonsters,monsterList)].parity == ((playerPos.x + playerPos.y)%2))
    isSameParity = true;
    else isSameParity = false;
  }
  if (DEBUG){
    printf("\n");
    printf("Parity calculated with success \n");
    printf("\n");
  }

  /*
    Decision making
  */
  if (DEBUG) {
    printf("\n\n");
    printf("NEW TURN : Bomberman is on %d %d\n", playerPos.x, playerPos.y);
    printf(" | Explosion range : %d\n",explosion_range);

  }
  /*
    Player has different parity as the nearest monster,
    he can not be killed by it as long as he does not bomb.
    (See the diagram in the report)
  */
  if((!isSameParity) || (!monsterReachable)) {
    if (DEBUG) printf(" | Parity : different\n");

    // Bomberman is in danger
    if(!isSafe(dangerMap,playerPos.x,playerPos.y)) {
      if (DEBUG) printf("Bomberman is danger, going to the nearest safe spot\n");
      a = safeMode(mapxsize, mapysize, map, playerPos, dangerMap, nbDanger, nbMonsters, monsterList, explosion_range, playerDistanceMap, monsterDistanceMap, exitFound, exitPos);
    }

    // Bonus on the map
    else if(nbFlameBonus > 0) {
      if (DEBUG) printf("Flame bonus detected, pathfinding to it\n");
      bool * pWayFound = malloc(sizeof(bool));
      action * pReturnedAction = malloc(sizeof(action));
      pathfind(mapxsize,mapysize, map, flameBonusList[0], playerPos, dangerMap, pWayFound, pReturnedAction);
      if (*pWayFound) a = (*pReturnedAction);
      else a = safeMode(mapxsize, mapysize, map, playerPos, dangerMap, nbDanger, nbMonsters, monsterList, explosion_range, playerDistanceMap, monsterDistanceMap, exitFound, exitPos);
    }
    else if(nbBombBonus > 0) {
      if (DEBUG) printf("Bomb bonus detected, pathfinding to it\n");
      bool * pWayFound = malloc(sizeof(bool));
      action * pReturnedAction = malloc(sizeof(action));
      pathfind(mapxsize,mapysize, map, bombBonusList[0], playerPos, dangerMap, pWayFound, pReturnedAction);
      if (*pWayFound) a = (*pReturnedAction);
      else a = safeMode(mapxsize, mapysize, map, playerPos, dangerMap, nbDanger, nbMonsters, monsterList, explosion_range, playerDistanceMap, monsterDistanceMap, exitFound, exitPos);
    }

    // Exit
    else if (exitFound){
      if(DEBUG){
        printf("\n");
        printf("ExitFound : %d \n",exitFound);
        printf("\n");
      }
      if (DEBUG) printf("Exit Found, Bomberman is trying to reach it 1\n");
      bool * pWayFound = malloc(sizeof(bool));
      action * pReturnedAction = malloc(sizeof(action));
      pathfind(mapxsize,mapysize, map, exitPos, playerPos, dangerMap, pWayFound, pReturnedAction);
      if (DEBUG){
        printf("\n");
        printf("pathfind process ends with success \n");
        printf("\n");
      }
      if (*pWayFound) a =  (*pReturnedAction);
      
      else a = safeMode(mapxsize, mapysize, map, playerPos, dangerMap, nbDanger, nbMonsters, monsterList, explosion_range, playerDistanceMap, monsterDistanceMap, exitFound, exitPos);
      }
      

    // No bricks left, but there are still enemies on the map -> switching parity
    else if (nbBricks == 0) {
      if (DEBUG) printf("No bricks left, switching to same parity when possible\n");
      if (nbBombs >0) a = safeMode(mapxsize, mapysize, map, playerPos, dangerMap, nbDanger, nbMonsters, monsterList, explosion_range, playerDistanceMap, monsterDistanceMap, exitFound, exitPos);
      else if ((monsterDistanceMap[playerPos.y][playerPos.x] > 9) && nbBricks>0) {a = BOMBING;}
      else {
        a = isolating(mapxsize,mapysize,map,playerPos,dangerMap,nbDanger,nbMonsters, monsterList, explosion_range, playerDistanceMap,monsterDistanceMap, exitFound, exitPos);
      }
    }

    // Find the nearest brick and bomb it.
    else if ((remaining_bombs > 0) && (nbBricks >0) && (nbBombs == 0)) {
        if (isBrickSpot(map,playerPos.x,playerPos.y,explosion_range, mapxsize,mapysize,nbMonsters,monsterList, monsterDistanceMap)) {
          if (DEBUG) printf("On a brick spot -> BOMBING :\n");
          a = BOMBING;
        }
        else {
          if (DEBUG) printf("Destroying bricks\n");
          a = destroyBricks(mapxsize,mapysize,map,playerPos,dangerMap,nbDanger,nbMonsters, monsterList, explosion_range, playerDistanceMap,monsterDistanceMap, exitFound, exitPos);
        }
      }

    // There are bricks on the map, but bomberman have 0 bombs or already placed one
    else a = safeMode(mapxsize, mapysize, map, playerPos, dangerMap, nbDanger, nbMonsters, monsterList, explosion_range, playerDistanceMap, monsterDistanceMap, exitFound, exitPos);
    
  }

  /*
    Bomberman has the same parity as the nearest monster, he can be killed by it
    but he will be invulnerable as soon as he bombs.
    (See the diagram in the report)
  */
  else {
    if (DEBUG) printf(" | Parity : Same\n");
    
    // Bomberman is near a monster (1 square between them),
    // he can bomb to avoid the danger caused by the monster.
    if((remaining_bombs > 0) && (nbBombs == 0) && isMonsterSpot(map,playerPos.x,playerPos.y, explosion_range, mapxsize, mapysize, monsterDistanceMap)) {
      a = BOMBING;
      if (DEBUG) printf("Attacking the nearest enemy !\n");
    }

    // Bomberman is in danger
    else if(!isSafe(dangerMap,playerPos.x,playerPos.y)) {
      if (DEBUG) printf("Bomberman is danger, going to the nearest safe spot\n");
      a = safeMode(mapxsize, mapysize, map, playerPos, dangerMap, nbDanger, nbMonsters, monsterList, explosion_range, playerDistanceMap, monsterDistanceMap, exitFound, exitPos);
    }

    // If bomberman have a bomb and he did not already placed one -> go to the nearest enemy.
    else if ((remaining_bombs > 0) && (nbMonsters > 0) && (nbBombs == 0)) a = hunt(mapxsize, mapysize, map, playerPos, dangerMap, nbDanger, nbMonsters, monsterList, explosion_range, playerDistanceMap, monsterDistanceMap, remaining_bombs, nbBricks, nbBombs, exitFound, exitPos);
    
    // If he does not have a bomb or already placed one -> wait
    else if ((remaining_bombs == 0) && (nbMonsters > 0)) a = safeMode(mapxsize, mapysize, map, playerPos, dangerMap, nbDanger, nbMonsters, monsterList, explosion_range, playerDistanceMap, monsterDistanceMap, exitFound, exitPos);
    
    // Dead code, but just to be sure :> (if there is no enemies left, the other priority order is used)
    else a = safeMode(mapxsize, mapysize, map, playerPos, dangerMap, nbDanger, nbMonsters, monsterList, explosion_range, playerDistanceMap, monsterDistanceMap, exitFound, exitPos);
  }

  if(oscillate(map, playerPos, nbMonsters, last_action, a)){
    if(a==NORTH && last_action==SOUTH && map[playerPos.y+1][playerPos.x]==PATH){
      a=last_action;
    }
    if(a==SOUTH && last_action==NORTH && map[playerPos.y-1][playerPos.x]==PATH){
      a=last_action;
    }
  }

  free(map);

  return a; // answer to the game engine
}

/*
  Oscillate function:
  return a boolean wich is true if bomberbam will oscillate
*/

bool oscillate(char ** map, position player_position, int nbmonsters, action last_action, action action_wanted){
  bool oscillation=false;
  char north=map[player_position.y-1][player_position.x];
  char northNorth=map[player_position.y-2][player_position.x];
  char south=map[player_position.y+1][player_position.x];
  char east=map[player_position.y][player_position.x+1];
  char west=map[player_position.y][player_position.x-1];
  char southWest=map[player_position.y+1][player_position.x-1];
  char southEast=map[player_position.y+1][player_position.x+1];
  char northEast=map[player_position.y-1][player_position.x+1];
  char northWest=map[player_position.y-1][player_position.x-1];
  
  if(north==WALL && east==WALL && southEast==WALL && southWest==WALL && south==PATH && west==WALL && northWest==WALL && nbmonsters==0){
    oscillation=true;
  }
  if(north==PATH && northNorth==WALL && west==WALL && east==WALL && northEast==WALL && south==PATH && northWest==PATH && nbmonsters==0){
    oscillation=true;
  }
  if(north==PATH && south==PATH && west==PATH && east==PATH && southEast==WALL && northEast==WALL && southWest==WALL && northWest==WALL && nbmonsters==0){
    oscillation=true;
  }
  if((last_action==SOUTH && action_wanted==NORTH) || (last_action==NORTH && action_wanted==SOUTH) || (last_action==WEST && action_wanted==EAST) || (last_action==EAST && action_wanted==WEST) ){
    oscillation=true;
  }
  if(DEBUG && oscillation){
    printf("oscillation detected \n");
  }
  return oscillation;
}

/*
  printAction procedure:
  This procedure prints the input action name on screen.
*/
void printAction(action a) {
  switch(a) {
  case BOMBING:
    printf("BOMBING");
    break;
  case NORTH:
    printf("NORTH");
    break;
  case EAST:
    printf("EAST");
    break;
  case SOUTH:
    printf("SOUTH");
    break;
  case WEST:
    printf("WEST");
    break;
  }
  
}

/*
  printBoolean procedure:
  This procedure prints the input boolan value on screen.
 */
void printBoolean(bool b) {
  if(b==true) {
    printf("true");
  }
  else {
    printf("false");
  }
}

/*
  printBTree procedure:
  This procedure prints the input tree on screen.
 */
void printTree(tree a){
  if(a==NULL) {
    printf("0");
  } else {
    printf("<");
    printf("%c",a->c);
    printf(",");
    printTree(a->n);
    printf(",");
    printTree(a->e);
    printf(",");
    printTree(a->s);
    printf(",");
    printTree(a->w);
    printf(">");
  }
}

/*
  buildMap procedure:
  This process create in a 2D map what Bomberman can see.
*/
void buildMap(tree map,char ** flatMap,int mapxsize,int mapysize, position * xy){
  position pos;
  if(map!=NULL){
    flatMap[(*xy).y][(*xy).x]=map->c;
    pos.x = (*xy).x;
    pos.y = (*xy).y-1;
    buildMap(map->n,flatMap,mapxsize,mapysize,&(pos));
    pos.x = (*xy).x+1;
    pos.y = (*xy).y;
    buildMap(map->e,flatMap,mapxsize,mapysize,&(pos));
    pos.x = (*xy).x;
    pos.y = (*xy).y+1;
    buildMap(map->s,flatMap,mapxsize,mapysize,&(pos));
    pos.x = (*xy).x-1;
    pos.y = (*xy).y;
    buildMap(map->w,flatMap,mapxsize,mapysize,&(pos));
  }
}

/*
  printTable procedure:
  This process print an int table of the size of the map on the screen.
*/
void printTable(int mapxsize, int mapysize, char ** table){
  int i; int j;
  for(j=0; j<mapysize-1; j++) {
    for(i=0; i<mapxsize-1; i++) {
      if(absolute(table[j][i])==0)printf("0 ");
      else printf("%c",absolute(table[j][i]));
      
    }
    printf("\n");
  }
}

/*
  printTable procedure:
  This process print an int table of the size of the map on the screen.
*/
void printIntTable(int mapxsize, int mapysize, int ** table){
  int i; int j;
  for(j=0; j<mapysize; j++) {
          for(i=0; i<mapxsize; i++) printf("%*d ",2,table[j][i]);
          printf("\n");
  }
}

/*
  initCharMap function:
  This function return a map of char initialized by WALL and the size is mapysize*mapxsize .
*/
char ** initCharMap(int mapxsize, int mapysize){
  int i;
  int j;
  char** map = malloc(mapysize * sizeof(char*));
  for(i=0; i<mapysize; i++) {
    map[i] = malloc(mapxsize * sizeof(char));
    for(j=0; j<mapxsize; j++) map[i][j] = '*';
  }
  return map;
}

/*
  Implementation of queues using arrays
  As long as there is enough space, the next element is added at the end,
  or removed from the begining.
  If the tail reach the end of the array, it goes back to the begining.
*/
void initQueue(queue *q,int mapxsize,int mapysize) {
  q->front = 0;
  q->tail = 0;
  q->count = 0;
  q->LMAX = (mapxsize+1)*(mapysize+1);
  q->table = malloc((q->LMAX)*sizeof(position));
}

bool enqueue(queue * q, position data) {
  bool success = true;
  if (q->count == q->LMAX) success = false;
  else {
    q->table[q->tail] = data;
    q->count = q->count +1;
    if ((q->tail) == (q->LMAX -1)) q->tail = 0;
    else q->tail = q-> tail +1;
  }
  return success;
}

position dequeue (queue *q) {
  q->count = q->count -1;
  position res = q->table[q->front];
  if ((q->front) == (q->LMAX -1)) q->front = 0;
  else q->front = q-> front +1;
  return res;
}

/*
  createDistanceMap procedure:
  This procedure draw up the distanceMap int table,
  with the lengh of the shortest path that leads to the origin.
  On a position, the process adds the neighbors to the queue if they are walkable,
  then proceeds to replace the number contained in the distanceMap if it is larger than the position's +1
*/
void createDistanceMap(position origin, char ** map, int ** distanceMap, int mapxsize, int mapysize) {
  // Initializing the queue
  queue q;
  initQueue(&q, mapxsize, mapysize);
  enqueue(&q,origin);
  distanceMap[origin.y][origin.x] = 0;
  position pos = origin;

  position t[5]; // Neighbors table : Position on the NORTH of pos is t[(int)NORTH]
  int i;
  do {
    pos = dequeue(&q);
    neighbors(t,pos,1);
    for(i = 1; i<5; i++) { // We go through the neighbors list to find walkable spots with a larger path lenght.
      if(((map[t[i].y][t[i].x] == PATH) || (map[t[i].y][t[i].x] == FLAME_ENEMY) || (map[t[i].y][t[i].x] == BOMB_BONUS) || (map[t[i].y][t[i].x] == FLAME_BONUS) || ((map[t[i].y][t[i].x] == EXIT) && (map[origin.y][origin.x] == FLAME_ENEMY)) || (map[t[i].y][t[i].x] == BOMBERMAN))
      && (distanceMap[t[i].y][t[i].x] > (distanceMap[t[0].y][t[0].x] + 1))) {
        if( !enqueue(&q,t[i])) { 
          if (DEBUG) printf("WARNING : Queue is full, skiping possible shorter paths");
        }
        distanceMap[t[i].y][t[i].x] = (distanceMap[pos.y][pos.x] + 1);
      }
    }
  } while(q.count != 0);
  
}

/*
  absolute function:
  Returns the absolute value of an int
*/
int absolute(int x) {
  if (x<0) x=-x;
  return x;
}

/*
  createGhostDistanceMap procedure:
  Same as createDistanceMap, but for ghost enemies that can go though walls
*/
void createGhostDistanceMap(position origin, char ** map, int ** distanceMap, int mapxsize, int mapysize) {
  int i; int j;
  //distanceMap[origin.y][origin.x] = 0;
  for (i = 0; i<mapxsize; i++) {
    for(j=0; j<mapysize; j++) {
      if(((map[j][i] == PATH) 
        || (map[j][i] == FLAME_ENEMY) 
        || (map[j][i] == BOMB_BONUS)
        || (map[j][i] == FLAME_BONUS)
        || (map[j][i] == BOMB)
        || (map[j][i] == BOMBERMAN))
        && (distanceMap[j][i] > (absolute(j-origin.y) + absolute(i-origin.x)))) {
          distanceMap[j][i] = absolute(j-origin.y) + absolute(i-origin.x);
      }
    }
  }
}

/*
  find procedure:
  Lists all the positions of bombs, monsters, bonus and the Exit
*/
void find(int mapxsize, int mapysize, char ** map, int * pNbMonsters, monster ** pMonsterList, bool * pExitFound, position * pExitPos, int * pNbBombBonus, position ** pBombBonusList, int * pNbFlameBonus, position ** pFlameBonusList, int * pNbBombs, position ** pBombList,action last_action, position player_position, int * pNbBricks, bool * monsterReachable, int ** playerDistanceMap){


  *pNbMonsters=0;
  int i; int j; // i is x and j is y
  // We go through every spot of the map, and add the relevent information to the arrays.
  for (i=0; i<mapxsize;i++) {
    for (j=0;j<mapysize;j++) {
      // Brick found
      if (map[j][i]==BREAKABLE_WALL){
        (*pNbBricks)++;
      }

      // Monster found
      else if (map[j][i]==FLAME_ENEMY || map[j][i]==GHOST_ENEMY){
        *pNbMonsters = *pNbMonsters+1;
        *pMonsterList = realloc(*pMonsterList, (*pNbMonsters) *sizeof(monster));
        ((*pMonsterList)[*pNbMonsters-1]).pos.x=i;
        ((*pMonsterList)[*pNbMonsters-1]).pos.y=j;
        ((*pMonsterList)[*pNbMonsters-1]).type = map[j][i];
        if (playerDistanceMap[j][i] != 99) (*monsterReachable) = true;
        ((*pMonsterList)[*pNbMonsters-1]).dist = playerDistanceMap[j][i];
        if (map[j][i] == GHOST_ENEMY) {
          position t[5];
          position pos = {.x = i, .y = j};
          (*pMonsterList)[*pNbMonsters-1].dist = 99;
          neighbors(t,pos,1);
          int k;
          for (k=0; k<5; k++) {
            if (playerDistanceMap[t[k].y][t[k].x] != 99) (*monsterReachable) = true;
            if (playerDistanceMap[t[k].y][t[k].x] < ((*pMonsterList)[*pNbMonsters-1].dist)) (*pMonsterList)[*pNbMonsters-1].dist = playerDistanceMap[t[k].y][t[k].x];
          }
        }
        ((*pMonsterList)[*pNbMonsters-1]).parity = (i+j)%2;
      }

      // Exit found
      else if (map[j][i]==EXIT) {
        if (DEBUG){
          printf("\n");
          printf("i: %d , j: %d \n",i,j);
          printf("\n");
        }
        *pExitFound = true;
        (*pExitPos).x = i;
        (*pExitPos).y = j;
      }

      // Bomb bonus found
      else if (map[j][i]==BOMB_BONUS){
        *pNbBombBonus = (*pNbBombBonus)+1;
        *pBombBonusList = realloc(*pBombBonusList, (*pNbBombBonus) *sizeof(position));
        ((*pBombBonusList)[*pNbBombBonus-1]).x=i;
        ((*pBombBonusList)[*pNbBombBonus-1]).y=j;
      }
      // Flame bonus found
      else if (map[j][i]==FLAME_BONUS){
        *pNbFlameBonus = (*pNbFlameBonus)+1;
        *pFlameBonusList = realloc(*pFlameBonusList, (*pNbFlameBonus) *sizeof(position));
        ((*pFlameBonusList)[*pNbFlameBonus-1]).x=i;
        ((*pFlameBonusList)[*pNbFlameBonus-1]).y=j;
      }
      // Bomb found
      else if (map[j][i]==BOMB 
      || ((last_action == BOMBING)&&(map[j][i]==BOMBERMAN))){
        *pNbBombs = *pNbBombs+1;
        *pBombList = realloc(*pBombList, (*pNbBombs) *sizeof(position));
        ((*pBombList)[*pNbBombs-1]).x=i;
        ((*pBombList)[*pNbBombs-1]).y=j;
      }
    }
  }
}

/*
  neighbors procedure:
  This process fills the neighbors table, which is a table 
  where t[action] is the range'th spot from the origin in the given direction
*/
void neighbors(position * table, position origin,int range){
  table[0].x = origin.x        ; table[0].y = origin.y        ;// Base position
  table[1].x = origin.x        ; table[1].y = origin.y -range ;// NORTH position
  table[2].x = origin.x +range ; table[2].y = origin.y        ;// EAST position
  table[3].x = origin.x        ; table[3].y = origin.y +range ;// SOUTH position
  table[4].x = origin.x -range ; table[4].y = origin.y        ;// WEST position
}

/*
  danger procedure:
  This process draw up a map of the potential danger at the time with the classification below:
  Certain death : WALLS         : 10
  Bomb about to explode         : 5
  Bomb potentialy under a ghost : 4
  Ghots enemy when map unknown  : 3
  Near ennemies in same parity  : 2 (prevents pathfinding)
  Safe                          : 0 
*/
void danger(int mapxsize, int mapysize , char ** map, monster * monsterList, int nbMonsters, position *bombList, int nbBombs, int nbFlameBonus, int ** dangerMap, int * pNbDanger, int explosion_range, position playerPos, position exitPos, bool exitFound){
  int i; int j; 
  
  // Danger : WALLs
  for(i=0;i<mapxsize;i++){
    for(j=0;j<mapysize;j++){
      //position pos = {.x = i, .y = j};
      if ((map[j][i]==WALL) || (map[j][i]==BREAKABLE_WALL)){
        (*pNbDanger)++;
        dangerMap[j][i]=10;
      }
      else if  (map[j][i]==GHOST_ENEMY) {
        dangerMap[j][i]=3; // We are NOT sure it is a wall
      }
      else {
        dangerMap[j][i]=0;
      }
    }
  }
    
  // Danger : Monsters if same parity as the player
  if (nbMonsters >0) {
    position t[5];
    for(i=0;i<nbMonsters;i++){ // i is the id of the monster
      if (monsterList[i].parity == ((playerPos.x + playerPos.y) %2)) {
        // Neighbors table of the monster position
        neighbors(t,((monsterList)[i].pos),1);
        for(j=1;j<5;j++){ // j is the id of the neighbors
          if (((map[t[j].y][t[j].x]==PATH) || (map[t[j].y][t[j].x]==BOMBERMAN) || (map[t[j].y][t[j].x]==BOMB_BONUS) || (map[t[j].y][t[j].x]==FLAME_BONUS) ) 
          && ((t[j].x) <= mapxsize) && ((t[j].y)<=mapysize)){
            (*pNbDanger)++;
            dangerMap[t[j].y][t[j].x]=2;
          }
        }
      }
      // If the monster is a ghost, he might be over a bomb
      else
      if ((monsterList[i].type == GHOST_ENEMY) && (nbBombs == 0)) {
        int range; int side;
        dangerMap[(monsterList[i]).pos.y][(monsterList[i]).pos.x]=4;
        for (range = 1; range <= explosion_range+nbFlameBonus; range ++) { // For each spot below and equal to the explosion range
        neighbors(t,((monsterList)[i].pos),range); // generating neigors table
        for (side = 1; side < 5; side ++) { // For each side
          // Adding the position on the map
          if(isValid(mapxsize,mapysize,t[side])) {
            (*pNbDanger)++;
            dangerMap[t[side].y][t[side].x]=4;
          }
        }
      }
      }
    }
  }

  // Danger : Bomb
  int range;
  position t[5];
  int side;
  for(i=0;i<nbBombs;i++){ // For each bomb
    dangerMap[(bombList[i]).y][(bombList[i]).x]=5;
    for (range = 1; range <= explosion_range+nbFlameBonus; range ++) { // For each spot below or equal to the explosion range
      neighbors(t,((bombList)[i]),range); // generating neigors table
      for (side = 1; side < 5; side ++) { // For each side
        // Adding the position on the map
        if(isValid(mapxsize,mapysize,t[side])) {
          (*pNbDanger)++;
          dangerMap[t[side].y][t[side].x]=5;
        }
      }
    }
  }
  
}

/*
  nearestMonster function:
  This function returns the id of the nearest monster in the monsterList
  This monster is monsterList[i]
  Returns -1 if there is none
*/
int nearestMonster(int nbMonster, monster * monsterList) {
  if (DEBUG && (nbMonster ==0)) printf("WARNING : no eney left, can not find the nearest");
  int iNearest = -1;
  int minDist = 99;
  int i;
  for (i=0; i< nbMonster; i++) { // for each monster
    if (monsterList[i].dist < minDist) { // if it is closer
      iNearest = i;                   // update it
      minDist = monsterList[i].dist;
    }
  }
  return iNearest;
}

/* 
  pathfind procedure:
  This process return the next action to do in order to go to the goal, if there is a way.
  Referenced as an action in debug
  Go though the neighbors and select the safest, 
  and if the danger is the same, select the closest to the goal.
*/
void pathfind(int mapxsize, int mapysize, char ** map, position goal, position player_position, int ** dangerMap, bool * pWayFound, action * pReturnedAction) {
  // Create the distanceMap to the goal
  if (DEBUG) printf("Pathfinding to %d %d\n",goal.x,goal.y);
  int** distanceMap = malloc(mapysize * sizeof(int*));
  int i; int j;
  for(i=0; i<mapysize; i++) {
    distanceMap[i] = malloc(mapxsize * sizeof(int));
    for(j=0; j<mapxsize; j++) distanceMap[i][j] = 99;
  }
  if (DEBUG){
    printf("\n");
    printf("pathfind distance map begins with success \n");
    printf("\n");
  }
  createDistanceMap(goal, map, distanceMap, mapxsize, mapysize);
  if (DEBUG){
    printf("\n");
    printf("pathfind distance map created with success \n");
    printf("\n");
  }

  position t[5];  // Neighbors table
  neighbors(t, player_position,1);
  (*pWayFound ) = false;
  if (DEBUG) printf("Searching a way\n");
  // Find if there is a way : look around the player on the distanceMap
  i = 0;
  do {
    if (distanceMap[t[i].y][t[i].x] != 99)  (*pWayFound) = true;
    i++;
  } while ((!(*pWayFound)) && i<5);

  // If there is a way
  if ((*pWayFound)) {
    if (DEBUG) printf(" | Way found\n");
    action nextAction;
    int minDanger = 10;
    int minDist = 99;
    for(i=1; i<5; i++) { // Go through all Bomberman's neighbors
      // If it is the goal, going to it
      if ((t[i].y == goal.y) && (t[i].x == goal.x) && deadEnd(mapxsize, mapysize, map, t[i].x, t[i].y, dangerMap)) {
        nextAction = i;
        minDanger = dangerMap[t[i].y][t[i].x];
        minDist = distanceMap[t[i].y][t[i].x];
      }
      // Minimising danger
      else if ((dangerMap[t[i].y][t[i].x] < minDanger) && (!deadEnd(mapxsize, mapysize, map, t[i].x, t[i].y, dangerMap))) {
        nextAction = i;
        minDanger = dangerMap[t[i].y][t[i].x];
        minDist = distanceMap[t[i].y][t[i].x];;
      }
      // If the danger is the same, going to the nearest to the goal
      else if ((dangerMap[t[i].y][t[i].x] == minDanger) && (!deadEnd(mapxsize, mapysize, map, t[i].x, t[i].y, dangerMap)) && (distanceMap[t[i].y][t[i].x] < minDist)){
        nextAction = i;
        minDanger = dangerMap[t[i].y][t[i].x];
        minDist = distanceMap[t[i].y][t[i].x];;
      }
    }
    (*pReturnedAction) = nextAction;
    if (DEBUG) {
      printf(" | The best way is : ");
      printAction(nextAction);
      printf("\n");
    }
  } else if (DEBUG) printf(" | Way NOT found !\n");
}

/*
  isSafe function:
  This function return a boolean wich is true if the point (x,y) is Safe 
  according to the map of dangers
*/
bool isSafe(int ** dangerMap, int x, int y){
  return dangerMap[y][x]==0;
}

/*
  nearestSafeSpot procedure:
  This process finds the nearst safe spot to the player.
  Go through all the squares of the map 
  and find the closest safe square from Bomberman
*/
void nearestSafeSpot(int mapxsize,int mapysize, char ** map, position player_position, int ** dangerMap,
            position * pNearest, int ** playerDistanceMap, int ** monsterDistanceMap, bool exitFound, position exitPos) {
  if (exitFound) (*pNearest) = exitPos;
  else (*pNearest) = player_position;
  int i; int j;
  int minDist = 99;
  int maxMonsterDist = 0;
  for(i = 0; i< mapxsize;i++) { // Go though all the squares of the map
    for (j = 0; j<mapysize; j++){
      // Finding the nearest Safe spot
      if ((isSafe(dangerMap,i,j) 
      && !deadEnd(mapxsize, mapysize, map,i,j, dangerMap)) 
      && (playerDistanceMap[j][i] < minDist)) {
        (*pNearest).x = i;
        (*pNearest).y = j;
        minDist = playerDistanceMap[j][i];
        maxMonsterDist = monsterDistanceMap[j][i];
      } 
      // If it is at equal distance, find the furthest from enemies
      else if (isSafe(dangerMap,i,j)
      && (playerDistanceMap[j][i] == minDist) 
      && !deadEnd(mapxsize, mapysize, map,i,j, dangerMap)
      && (monsterDistanceMap[j][i] > maxMonsterDist)) {
        (*pNearest).x = i;
        (*pNearest).y = j;
        minDist = playerDistanceMap[j][i];
        maxMonsterDist = monsterDistanceMap[j][i];
      }
    }
  }
}

/*
  nearestSuperSafeSpot procedure:
  This process finds the furthest square from enemies, used to switch modes.
  Go through all the squares of the map and select the furthest from enemies
*/
void nearestSuperSafeSpot(int mapxsize,int mapysize, char ** map, position player_position, int ** dangerMap,
            position * pNearest, int ** playerDistanceMap, int ** monsterDistanceMap, bool exitFound, position exitPos) {
  bool found = false;
  (*pNearest).x = player_position.x;
  (*pNearest).y = player_position.y;
  int i; int j;
  int maxDist = 0;
  for(i = 0; i< mapxsize;i++) { //Go through all the squares of the map
    for (j = 0; j<mapysize; j++){
      // Select the furthest from enemies
      if ( !deadEnd(mapxsize, mapysize, map,i,j, dangerMap) && isSafe(dangerMap,i,j)
      && (monsterDistanceMap[j][i] > maxDist) && (monsterDistanceMap[j][i] != 99)
      && (playerDistanceMap[j][i] != 99)) {
        (*pNearest).x = i;
        (*pNearest).y = j;
        maxDist = monsterDistanceMap[j][i];
        found = true;
      }
    }
  }
  if (!found) {
    if (DEBUG) printf(" | Can not isolate, going to a safe spot");
    nearestSafeSpot(mapxsize,mapysize, map, player_position,dangerMap, pNearest, playerDistanceMap, monsterDistanceMap, exitFound, exitPos);
  }
}

/*
  nearestBrickSpot procedure:
  This process finds the nearest square where bomberman can break a wall.
  Go through all the squares of the map and find the closest spot in range of a
  breakable wall and far enough from enemies.
*/
void nearestBrickSpot(int mapxsize,int mapysize, char ** map, position player_position, int ** dangerMap,
            position * pNearest, bool * pNearestFound, int nbMonster, monster * monsters_position, int explosion_range, int ** playerDistanceMap, int ** monsterDistanceMap) {
  
  int i; int j;
  (*pNearestFound) = false;
  int minDist = 99;
  for(i = 1; i< (mapxsize-1);i++) { //Go through all the squares of the map
    for (j = 1; j<(mapysize-1); j++){
      if (isBrickSpot(map,i,j, explosion_range, mapxsize, mapysize, nbMonster, monsters_position, monsterDistanceMap) && (playerDistanceMap[j][i] < minDist) ) {
          (*pNearestFound) = true;
          (*pNearest).x = i;
          (*pNearest).y = j;
          minDist = playerDistanceMap[j][i];
      }
    }
  }
}

/*
  isBrickSpot function:
  This function tell whether or not a position is a spot where bomberman can destroy a brick
  far enough from enemies
*/
bool isBrickSpot(char ** map, int x, int y, int explosion_range, int mapxsize, int mapysize, int nbMonsters, monster* monsterList, int**monsterDistanceMap) {
  
  position origin = {.x = x, .y=y};
  bool bricksAreAccesibles = false;
  int range = 1;
  bool isWall;
  int side = 1;
  position t[5];
  
  if ((monsterDistanceMap[y][x] > 9) && ((map[y][x] == PATH) || (map[y][x] == BOMBERMAN))) {
    for(side = 1; side<5; side++) {
      isWall = false;
      range = 1;
      neighbors(t,origin,range);
      while ((!isWall) && (range<=explosion_range) && isValid(mapxsize, mapysize, t[side])) {
        if(map[t[side].y][t[side].x] == WALL) isWall = true;
        else {
          if (map[t[side].y][t[side].x] == BREAKABLE_WALL) bricksAreAccesibles = true;
          range ++;
          neighbors(t,origin,range);
        }
      }
    }
  }
  return bricksAreAccesibles;
}

/*
  wall function:
  This function tells if a square is a wall or not
*/
bool wall(char ** map, int xi, int yi, position pos){
      return (map[pos.x+xi][pos.y+yi]==WALL) || (map[pos.x+xi][pos.y+yi]==BREAKABLE_WALL);
}

/*
  deadEnd function:
  This function tells if a square is surouded by dangerous squares.
  Go through all the neighbors and return true if there is 3 walls and a dangerous square.
*/
bool deadEnd(int mapxsize, int mapysize, char ** map, int x, int y, int ** dangerMap) {
  int wallCount = 0;
  int dangerCount = 0;
  int i;
  position t[5];
  position pos = {.x = x, .y = y};
  neighbors(t,pos,1);
  for (i=1;i <5; i++) {
    if(isValid(mapxsize, mapysize, t[i])) {
      if ((map[t[i].y][t[i].x] == WALL) || (map[t[i].y][t[i].x] == BREAKABLE_WALL) )
      wallCount ++;
      else if (!isSafe(dangerMap,t[i].x, t[i].y)) 
      dangerCount++;
    }
  }
  return ((wallCount >2) && (dangerCount >0));
}

/*
  isMonsterSpot funtion:
  This function tell whether or not a position is near a monster spot
  A monster spot is a location on the map with 1 square between it and the monster.
  If a player is on this spot, a monster can kill him if they both go on the square between them
*/
bool isMonsterSpot(char ** map, int x, int y, int explosion_range, int mapxsize, int mapysize, int ** monsterDistanceMap) {
 return (monsterDistanceMap[y][x] == 2);
}

/*
  nearestMonsterSpot procedure:
  This process finds the nearest monster spot
  similar to nearestSafeSpot
*/
void nearestMonsterSpot(int mapxsize,int mapysize, char ** map, position player_position, int ** dangerMap,
            position * pNearest, bool * pNearestFound, int nbMonster, monster * monsters_position, int explosion_range, int ** playerDistanceMap, int ** monsterDistanceMap) {
  int i; int j;
  (*pNearestFound) = false; 
  // Finding the nearest monster spot
  int minDist = 99;
  for(i = 1; i< (mapxsize-1);i++) 
  for (j = 1; j<(mapysize-1); j++){
    if (isMonsterSpot(map,i,j, explosion_range, mapxsize, mapysize, monsterDistanceMap) && (playerDistanceMap[j][i] < minDist)) {
        (*pNearestFound) = true;
        (*pNearest).x = i;
        (*pNearest).y = j;
        minDist = playerDistanceMap[j][i];
    }
  }
}

/* 
  isValid function:
  This function tells if a position is in the map's boundaries or not
*/
bool isValid(int mapxsize, int mapysize, position pos) {
  return ((pos.y>=0) && (pos.y<mapysize) && (pos.x>= 0) && (pos.x < mapxsize));
}

/*
  safeMode function:
  This function returns the next action to make in order to reach the nearest safe spot.
  Considered as an action in debug.
*/
action safeMode(int mapxsize, int mapysize, char ** map, position player_position, int ** dangerMap, int nb_danger, int nbMonsters, monster * monsters_position, int explosion_range, int ** playerDistanceMap, int ** monsterDistanceMap, bool exitFound, position exitPos) {
  if (DEBUG) printf("Searching for the nearest safe spot\n");
  action a;
  position * pNearest = malloc(sizeof(position));
  nearestSafeSpot(mapxsize,mapysize, map, player_position,dangerMap, pNearest, playerDistanceMap, monsterDistanceMap, exitFound, exitPos);
  if (DEBUG) printf(" | Nearest safe spot : %d %d\n",(*pNearest).x,(*pNearest).y);
  
  // Pathfind to it
  bool * pWayFound = malloc(sizeof(bool));
  action * pReturnedAction = malloc(sizeof(action));
  pathfind(mapxsize,mapysize, map, (*pNearest), player_position, dangerMap, pWayFound, pReturnedAction);
  if (*pWayFound) a = (*pReturnedAction);
  else {
    if (DEBUG) printf(" | Can not find way to the nearest safe spot, waiting ...\n");
    pathfind(mapxsize,mapysize, map, player_position, player_position, dangerMap, pWayFound, pReturnedAction);
    a = (*pReturnedAction);
  }
  return a;
}

/*
  The following functions find the appropriate position on the map
  and return the action give by pathfind
*/

/*
  destroyBricks function:
  This function returns the next action to do in order to each the nearest brick spot.
  Considered as an action in debug.
  */
action destroyBricks(int mapxsize, int mapysize, char ** map, position player_position,int ** dangerMap, int nb_danger, int nb_monster, monster * monsters_position, int explosion_range, int ** playerDistanceMap, int ** monsterDistanceMap,bool exitFound, position exitPos){
  action a;

  // Finding the nearest safe brick spot, if there is none, going to a safe spot.
  position * pNearest = malloc(sizeof(position));
  bool * pNearestFound = malloc(sizeof(bool));
  if (DEBUG) printf("Finding the nearest brick spot\n");
  nearestBrickSpot(mapxsize,mapysize, map, player_position, dangerMap, pNearest, pNearestFound, nb_monster, monsters_position,explosion_range, playerDistanceMap, monsterDistanceMap);
  if (!(*pNearestFound)) {
    if (DEBUG) printf(" | No safe brick spot found, going away to switch parity\n");
    if (monsterDistanceMap[player_position.y][player_position.x] > 9) return BOMBING;
    else nearestSuperSafeSpot(mapxsize,mapysize, map, player_position, dangerMap, pNearest, playerDistanceMap, monsterDistanceMap, exitFound, exitPos);
    
    }
  if (DEBUG) printf(" | Nearest safe brick spot : %d %d\n",(*pNearest).x,(*pNearest).y);
  
  // Pathfind to it
  bool * pWayFound = malloc(sizeof(bool));
  action * pReturnedAction = malloc(sizeof(action));
  pathfind(mapxsize,mapysize, map, (*pNearest), player_position, dangerMap, pWayFound, pReturnedAction);
  if (*pWayFound) a=(*pReturnedAction);
  else  a= safeMode(mapxsize, mapysize, map, player_position, dangerMap, nb_danger, nb_monster, monsters_position, explosion_range, playerDistanceMap, monsterDistanceMap, exitFound, exitPos); // Never happends, because there is alaways a way to a point given by the nearestSafeSpot function

  return a;
}

/*
  hunt function:
  This function returns the next action to do in order to reach a monster.
  Considered as an action in debug
*/
action hunt(int mapxsize, int mapysize, char ** map, position player_position,int ** dangerMap, int nb_danger, int nb_monster, monster * monsters_position, int explosion_range, int ** playerDistanceMap, int ** monsterDistanceMap, int remaining_bombs, int nbBricks, int nbBombs,bool exitFound,position exitPos){
  action a=-1;
  position t[5];
  neighbors(t,player_position,1);
  position * pNearest = malloc(sizeof(position));
  bool * pNearestFound = malloc(sizeof(bool));
  if (DEBUG) printf("Finding the nearest monster spot\n");
  nearestMonsterSpot(mapxsize,mapysize, map, player_position, dangerMap, pNearest, pNearestFound, nb_monster, monsters_position, explosion_range, playerDistanceMap, monsterDistanceMap);
  if (!(*pNearestFound)) {
    if (DEBUG) printf(" | Monster unreachable, breaking brinks in %d %d\n",(*pNearest).x,(*pNearest).y);
    if ((remaining_bombs > 0) && (nbBricks >0) && (nbBombs == 0)) {
      if (isBrickSpot(map,player_position.x,player_position.y,explosion_range, mapxsize,mapysize,nb_monster,monsters_position, monsterDistanceMap)) {
        a = BOMBING;
        if (DEBUG) printf("On a brick spot -> BOMBING :\n");
      }
      else a = destroyBricks(mapxsize,mapysize,map,player_position,dangerMap,nb_danger,nb_monster, monsters_position, explosion_range, playerDistanceMap,monsterDistanceMap, exitFound, exitPos);
    }
    else a = safeMode(mapxsize, mapysize, map, player_position, dangerMap, nb_danger, nb_monster, monsters_position, explosion_range, playerDistanceMap, monsterDistanceMap, exitFound, exitPos);
  } 
  else {
    if (DEBUG) printf(" | Nearest monster spot : %d %d\n",(*pNearest).x,(*pNearest).y);
    // Pathfind to the monster
    bool * pWayFound = malloc(sizeof(bool));
    action * pReturnedAction = malloc(sizeof(action));
    pathfind(mapxsize,mapysize, map, (*pNearest), player_position, dangerMap, pWayFound, pReturnedAction);
    if (*pWayFound) a=(*pReturnedAction);
    else  a=BOMBING; // Never happends, because there is alaways a way to a point given by the nearest functions
  }
  
  return a;
}

/*
  isolating function:
  This function is used to switch parity mode when there are no bricks left
*/
action isolating(int mapxsize, int mapysize, char ** map, position player_position,int ** dangerMap, int nb_danger, int nb_monster, monster * monsters_position, int explosion_range, int ** playerDistanceMap, int ** monsterDistanceMap, bool exitFound, position exitPos){
  action a;
  position * pNearest = malloc(sizeof(position));
   if (DEBUG) printf(" Finding the furthest square from monster to isolate\n");
  nearestSuperSafeSpot(mapxsize,mapysize, map, player_position, dangerMap, pNearest, playerDistanceMap, monsterDistanceMap, exitFound, exitPos);
  if (DEBUG) printf(" | Isolating in : %d %d\n",(*pNearest).x,(*pNearest).y);
  
  // Pathfind to it
  bool * pWayFound = malloc(sizeof(bool));
  action * pReturnedAction = malloc(sizeof(action));
  pathfind(mapxsize,mapysize, map, (*pNearest), player_position, dangerMap, pWayFound, pReturnedAction);
  if (*pWayFound) a=(*pReturnedAction);
  else  a=BOMBING; // Never happends, because there is alaways a way to a point given by the nearestSafeSpot function

  return a;
}
