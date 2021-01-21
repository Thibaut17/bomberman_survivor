/*
  Enumeration for the different actions that the player may choose
 */
enum actions {BOMBING, NORTH, EAST, SOUTH, WEST};
typedef enum actions action; // define action as a shorthand for enum actions

/*
  Tree of char modeling a subset of the game map
 */
struct node_s {char c; struct node_s * n; struct node_s * e; struct node_s * s; struct node_s * w;};
typedef struct node_s node;
typedef node * tree;

/* bomberman function to code in player.c */
action bomberman(tree, action, int, int);

