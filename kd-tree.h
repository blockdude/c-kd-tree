/*
 * this is only for k-dimensions
 * and has no self balancing
 * properties so performace
 * may degrade during use.
 */

#ifndef KD_TREE
#define KD_TREE

typedef struct kd_node kd_node;
typedef struct kd_tree kd_tree;

struct kd_node
{
    int *point;

    kd_node *r;
    kd_node *l;

    void *item;
};

struct kd_tree
{
    kd_node *root;
    int size;
    int k;
    void ( *free_item )( void * );
};

kd_tree *new_kd_tree    ( int k, void ( *free_item )( void *item ) );
kd_node *new_kd_node    ( int k, int point[], void *item );

int kd_insert           ( kd_tree *tree, int point[], void *item );
int kd_remove           ( kd_tree *tree, int point[] );

void **kd_query         ( kd_tree *tree, int point[], int range );
void *kd_search         ( kd_tree *tree, int point[] );
void *kd_nearest        ( kd_tree *tree, int point[] );

void kd_free            ( kd_tree *tree );

#endif
